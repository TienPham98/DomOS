#pragma once

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

#include <vector>
#include <string>

typedef struct {
    std::string ssid;
    int rssi;
    uint8_t authmode;
} wifi_scan_result_t;

class WifiManager {
public:
    static WifiManager& GetInstance() {
        static WifiManager instance;
        return instance;
    }

    esp_err_t Init();
    bool ConnectStored();
    bool ConnectAP(const char* ssid, const char* password);
    void StartCaptivePortal();
    void StopCaptivePortal();
    
    std::vector<wifi_scan_result_t> ScanNetworks();
    bool IsConnected() const { return connected_; }
    std::string GetIpAddress() const { return ip_address_; }

private:
    WifiManager();
    ~WifiManager();

    bool connected_;
    std::string ip_address_;
    httpd_handle_t server_handle_;

    static void WifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    esp_err_t SaveCredentials(const char* ssid, const char* password);
    esp_err_t LoadCredentials(char* ssid_out, size_t ssid_len, char* pass_out, size_t pass_len);
};
