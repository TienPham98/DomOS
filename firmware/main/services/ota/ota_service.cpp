#include "ota_service.h"

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_system.h"
#include "kernel/event_bus.h"

namespace {
constexpr char TAG[] = "ota";
}

bool OtaService::Start(EventBus *events)
{
    events_ = events;
    return true;
}

bool OtaService::UpdateFromUrl(const char *firmware_url)
{
    if (busy_ || firmware_url == nullptr || firmware_url[0] == '\0') return false;
    busy_ = true;
    if (events_) events_->Publish(EventType::OtaProgress, TAG, "starting");

    esp_http_client_config_t http_config{};
    http_config.url = firmware_url;
    http_config.timeout_ms = 15000;
    http_config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_https_ota_config_t ota_config{};
    ota_config.http_config = &http_config;

    const esp_err_t result = esp_https_ota(&ota_config);
    busy_ = false;
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "update failed: %s", esp_err_to_name(result));
        if (events_) events_->Publish(EventType::OtaProgress, TAG, "failed");
        return false;
    }
    if (events_) events_->Publish(EventType::OtaProgress, TAG, "complete");
    ESP_LOGI(TAG, "update complete; restarting");
    esp_restart();
    return true;
}
