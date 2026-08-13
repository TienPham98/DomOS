#include "wifi_manager.h"
#include "esp_log.h"
#include "nvs.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include <cstring>

static const char *TAG = "WifiManager";
#define NVS_NAMESPACE "domos_wifi"

WifiManager::WifiManager() : connected_(false), server_handle_(NULL) {}
WifiManager::~WifiManager() { StopCaptivePortal(); }

esp_err_t WifiManager::Init() {
    ESP_LOGI(TAG, "Initializing WiFi Manager...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::WifiEventHandler, this, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::WifiEventHandler, this, NULL));

    return ESP_OK;
}

void WifiManager::WifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    WifiManager* self = static_cast<WifiManager*>(arg);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        self->connected_ = false;
        ESP_LOGW(TAG, "WiFi disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        char ip_str[16];
        esp_ip4addr_ntoa(&event->ip_info.ip, ip_str, sizeof(ip_str));
        self->ip_address_ = ip_str;
        self->connected_ = true;
        ESP_LOGI(TAG, "Connected to WiFi! IP: %s", ip_str);
    }
}

bool WifiManager::ConnectStored() {
    char ssid[32] = {0};
    char pass[64] = {0};

    if (LoadCredentials(ssid, sizeof(ssid), pass, sizeof(pass)) == ESP_OK) {
        ESP_LOGI(TAG, "Attempting connection to stored SSID: %s", ssid);
        return ConnectAP(ssid, pass);
    }
    ESP_LOGW(TAG, "No stored WiFi credentials found.");
    return false;
}

bool WifiManager::ConnectAP(const char* ssid, const char* password) {
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    SaveCredentials(ssid, password);
    return true;
}

std::vector<wifi_scan_result_t> WifiManager::ScanNetworks() {
    std::vector<wifi_scan_result_t> results;
    wifi_scan_config_t scan_config = {};
    scan_config.show_hidden = false;

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_scan_start(&scan_config, true);

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count == 0) return results;

    std::vector<wifi_ap_record_t> ap_records(ap_count);
    esp_wifi_scan_get_ap_records(&ap_count, ap_records.data());

    for (const auto& ap : ap_records) {
        wifi_scan_result_t res;
        res.ssid = (char*)ap.ssid;
        res.rssi = ap.rssi;
        res.authmode = ap.authmode;
        results.push_back(res);
    }
    return results;
}

esp_err_t WifiManager::SaveCredentials(const char* ssid, const char* password) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;

    nvs_set_str(nvs, "ssid", ssid);
    nvs_set_str(nvs, "pass", password);
    err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

esp_err_t WifiManager::LoadCredentials(char* ssid_out, size_t ssid_len, char* pass_out, size_t pass_len) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) return err;

    nvs_get_str(nvs, "ssid", ssid_out, &ssid_len);
    nvs_get_str(nvs, "pass", pass_out, &pass_len);
    nvs_close(nvs);
    return (strlen(ssid_out) > 0) ? ESP_OK : ESP_FAIL;
}

void WifiManager::StartCaptivePortal() {
    ESP_LOGI(TAG, "Starting SoftAP Captive Portal: DomOS-Setup...");
    wifi_config_t ap_config = {};
    strcpy((char*)ap_config.ap.ssid, "DomOS-Setup");
    ap_config.ap.ssid_len = strlen("DomOS-Setup");
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode = WIFI_AUTH_OPEN;

    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();

    // Start HTTP Server for Captive Portal Web Page
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 5;

    if (httpd_start(&server_handle_, &config) == ESP_OK) {
        httpd_uri_t portal_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = [](httpd_req_t *req) -> esp_err_t {
                const char* html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>DomOS WiFi Setup</title><style>body{font-family:sans-serif;background:#090d16;color:#fff;text-align:center;padding:20px;}h2{color:#a855f7;}input,button{width:90%;padding:12px;margin:8px;border-radius:8px;border:none;}button{background:#06b6d4;color:#fff;font-weight:bold;cursor:pointer;}</style></head><body><h2>DomOS WiFi Setup</h2><form action='/connect' method='POST'><input name='ssid' placeholder='WiFi Name (SSID)' required><br><input name='pass' type='password' placeholder='Password'><br><button type='submit'>Connect DomOS</button></form></body></html>";
                httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
                return ESP_OK;
            },
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server_handle_, &portal_uri);
    }
}

void WifiManager::StopCaptivePortal() {
    if (server_handle_) {
        httpd_stop(server_handle_);
        server_handle_ = NULL;
    }
}
