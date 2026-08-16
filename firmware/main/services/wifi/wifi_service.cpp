#include "wifi_service.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/inet.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "services/filesystem/upload_server.h"

namespace {
constexpr char TAG[] = "wifi";
constexpr EventBits_t CONNECTED_BIT = BIT0;
EventGroupHandle_t s_events = nullptr;
esp_netif_t *s_netif = nullptr;
char s_ssid[sizeof(((wifi_config_t *)nullptr)->sta.ssid)] = {};
char s_password[sizeof(((wifi_config_t *)nullptr)->sta.password)] = {};
char s_ip[16] = "--";
int8_t s_rssi = 0;
int s_retries = 0;
bool s_wifi_inited = false;

bool ParseIpv4(const char *text, esp_ip4_addr_t *out)
{
    if (text == nullptr || text[0] == '\0' || out == nullptr) return false;
    ip4_addr_t parsed{};
    if (ip4addr_aton(text, &parsed) == 0) return false;
    out->addr = parsed.addr;
    return true;
}

void StartSntp()
{
    static bool started = false;
    if (started) return;
    setenv("TZ", "ICT-7", 1);
    tzset();
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    started = true;
}
static WallpaperUploadServer *s_upload = nullptr;

void OnWifiEvent(void *, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_events, CONNECTED_BIT);
        std::snprintf(s_ip, sizeof(s_ip), "--");
        if (s_retries++ < CONFIG_DOMOS_WIFI_MAX_RETRY) esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const auto *event = static_cast<ip_event_got_ip_t *>(event_data);
        std::snprintf(s_ip, sizeof(s_ip), IPSTR, IP2STR(&event->ip_info.ip));
        wifi_ap_record_t ap_info{};
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) s_rssi = ap_info.rssi;
        s_retries = 0;
        xEventGroupSetBits(s_events, CONNECTED_BIT);
        StartSntp();
        ESP_LOGI(TAG, "connected: SSID=%s IP=%s RSSI=%d", s_ssid, s_ip, s_rssi);
        AddSystemLog("INFO", "wifi", "Connected to '%s' (IP: %s, RSSI: %d dBm)", s_ssid, s_ip, s_rssi);
        if (s_upload != nullptr) s_upload->Start(nullptr, nullptr);
    }
}


bool LoadCredentialsFromNvs()
{
    nvs_handle_t nvs;
    if (nvs_open("wifi_cfg", NVS_READONLY, &nvs) != ESP_OK) return false;
    size_t ssid_len = sizeof(s_ssid);
    size_t pass_len = sizeof(s_password);
    esp_err_t err_s = nvs_get_str(nvs, "ssid", s_ssid, &ssid_len);
    esp_err_t err_p = nvs_get_str(nvs, "pass", s_password, &pass_len);
    nvs_close(nvs);
    return (err_s == ESP_OK && err_p == ESP_OK && s_ssid[0] != '\0');
}

void SaveCredentialsToNvs(const char *ssid, const char *password)
{
    nvs_handle_t nvs;
    if (nvs_open("wifi_cfg", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_str(nvs, "ssid", ssid);
        nvs_set_str(nvs, "pass", password);
        nvs_commit(nvs);
        nvs_close(nvs);
    }
}
} // namespace

bool WifiService::Start()
{
    s_events = xEventGroupCreate();
    if (s_events == nullptr) return false;
    if (esp_netif_init() != ESP_OK || esp_event_loop_create_default() != ESP_OK) return false;
    s_netif = esp_netif_create_default_wifi_sta();
    if (s_netif == nullptr) return false;

    // DomOS uses a dedicated fixed-address LAN. The private address is injected
    // from the root .env during firmware configuration.
    const esp_err_t dhcp_result = esp_netif_dhcpc_stop(s_netif);
    if (dhcp_result != ESP_OK && dhcp_result != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
        ESP_LOGE(TAG, "failed to stop DHCP client: %s", esp_err_to_name(dhcp_result));
        return false;
    }
    esp_netif_ip_info_t fixed_ip{};
    if (!ParseIpv4(CONFIG_DOMOS_DEVICE_IP, &fixed_ip.ip)) {
        ESP_LOGE(TAG, "DOMOS_DEVICE_IP is missing or invalid");
        return false;
    }
    if (!ParseIpv4(CONFIG_DOMOS_GATEWAY_IP, &fixed_ip.gw) ||
        !ParseIpv4(CONFIG_DOMOS_NETMASK, &fixed_ip.netmask)) {
        ESP_LOGE(TAG, "DOMOS_GATEWAY_IP or DOMOS_NETMASK is missing or invalid");
        return false;
    }
    if (esp_netif_set_ip_info(s_netif, &fixed_ip) != ESP_OK) {
        ESP_LOGE(TAG, "failed to configure the fixed device IP");
        return false;
    }
    esp_netif_dns_info_t dns{};
    dns.ip.u_addr.ip4 = fixed_ip.gw;
    dns.ip.type = ESP_IPADDR_TYPE_V4;
    esp_netif_set_dns_info(s_netif, ESP_NETIF_DNS_MAIN, &dns);

    const wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&init_config) != ESP_OK) return false;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &OnWifiEvent, nullptr, nullptr);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &OnWifiEvent, nullptr, nullptr);
    s_wifi_inited = true;

    if (LoadCredentialsFromNvs()) {
        // Preserve the stored password, but keep the dedicated DomOS SSID fixed.
        std::strncpy(s_ssid, "Dom_12", sizeof(s_ssid) - 1);
    } else {
        if (CONFIG_DOMOS_WIFI_SSID[0] != '\0') {
            std::strncpy(s_ssid, CONFIG_DOMOS_WIFI_SSID, sizeof(s_ssid) - 1);
            std::strncpy(s_password, CONFIG_DOMOS_WIFI_PASSWORD, sizeof(s_password) - 1);
        }
    }

    if (s_ssid[0] == '\0') {
        ESP_LOGW(TAG, "Wi-Fi password not configured for Dom_12; use menuconfig or the touch UI");
        return true;
    }

    wifi_config_t station_config{};
    std::strncpy(reinterpret_cast<char *>(station_config.sta.ssid), s_ssid, sizeof(station_config.sta.ssid));
    std::strncpy(reinterpret_cast<char *>(station_config.sta.password), s_password, sizeof(station_config.sta.password));
    station_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    station_config.sta.pmf_cfg.capable = true;
    station_config.sta.pmf_cfg.required = false;

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
        esp_wifi_set_config(WIFI_IF_STA, &station_config) != ESP_OK ||
        esp_wifi_start() != ESP_OK) {
        return false;
    }

    // Continuous 60 ms PCM streaming is latency-sensitive. Modem sleep can
    // stall a WebSocket write long enough for the client to tear down an
    // otherwise healthy connection.
    return esp_wifi_set_ps(WIFI_PS_NONE) == ESP_OK;
}

bool WifiService::ConnectTo(const char *ssid, const char *password)
{
    if (ssid == nullptr || password == nullptr || !s_wifi_inited) return false;
    if (std::strcmp(ssid, "Dom_12") != 0) {
        ESP_LOGW(TAG, "Rejected SSID '%s': DomOS network is fixed to Dom_12", ssid);
        return false;
    }
    std::strncpy(s_ssid, "Dom_12", sizeof(s_ssid) - 1);
    std::strncpy(s_password, password, sizeof(s_password) - 1);
    SaveCredentialsToNvs(ssid, password);

    s_retries = 0;
    xEventGroupClearBits(s_events, CONNECTED_BIT);
    esp_wifi_stop();

    wifi_config_t station_config{};
    std::strncpy(reinterpret_cast<char *>(station_config.sta.ssid), s_ssid, sizeof(station_config.sta.ssid));
    std::strncpy(reinterpret_cast<char *>(station_config.sta.password), s_password, sizeof(station_config.sta.password));
    station_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    station_config.sta.pmf_cfg.capable = true;
    station_config.sta.pmf_cfg.required = false;

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &station_config);
    if (esp_wifi_start() != ESP_OK) return false;
    return esp_wifi_set_ps(WIFI_PS_NONE) == ESP_OK;
}

bool WifiService::Connected() const { return s_events != nullptr && (xEventGroupGetBits(s_events) & CONNECTED_BIT); }
const char *WifiService::Ssid() const { return s_ssid[0] ? s_ssid : "Not configured"; }
const char *WifiService::IpAddress() const { return s_ip; }
int8_t WifiService::Rssi() const { return s_rssi; }
void WifiService::SetUploadServer(WallpaperUploadServer *upload) { s_upload = upload; }
