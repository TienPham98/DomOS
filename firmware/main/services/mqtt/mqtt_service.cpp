#include "mqtt_service.h"

#include <cstring>

#include "esp_event.h"
#include "esp_log.h"
#include "kernel/event_bus.h"
#include "mqtt_client.h"

namespace {
constexpr char TAG[] = "mqtt";
}

bool MqttService::Start(EventBus *events)
{
    events_ = events;
    if (CONFIG_DOMOS_MQTT_URI[0] == '\0') {
        ESP_LOGW(TAG, "MQTT URI not configured");
        return true;
    }
    esp_mqtt_client_config_t config{};
    config.broker.address.uri = CONFIG_DOMOS_MQTT_URI;
    config.credentials.client_id = "domos-es3c28p";
    client_ = esp_mqtt_client_init(&config);
    if (client_ == nullptr) return false;
    esp_mqtt_client_register_event(static_cast<esp_mqtt_client_handle_t>(client_), MQTT_EVENT_ANY,
                                   OnEvent, this);
    return esp_mqtt_client_start(static_cast<esp_mqtt_client_handle_t>(client_)) == ESP_OK;
}

bool MqttService::Publish(const char *topic, const char *payload)
{
    if (!connected_ || client_ == nullptr || topic == nullptr || payload == nullptr) return false;
    return esp_mqtt_client_publish(static_cast<esp_mqtt_client_handle_t>(client_), topic, payload, 0, 1, 0) >= 0;
}

void MqttService::OnEvent(void *handler_args, esp_event_base_t, int32_t event_id, void *event_data)
{
    auto *service = static_cast<MqttService *>(handler_args);
    auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);
    switch (event_id) {
    case MQTT_EVENT_CONNECTED:
        service->connected_ = true;
        esp_mqtt_client_subscribe(event->client, "dom/#", 1);
        if (service->events_) service->events_->Publish(EventType::MqttConnected, TAG, "connected");
        break;
    case MQTT_EVENT_DISCONNECTED:
        service->connected_ = false;
        break;
    case MQTT_EVENT_DATA: {
        char message[96] = {};
        const int count = event->data_len < static_cast<int>(sizeof(message) - 1) ? event->data_len : sizeof(message) - 1;
        std::memcpy(message, event->data, count);
        if (service->events_) service->events_->Publish(EventType::MqttMessage, TAG, message);
        break;
    }
    default:
        break;
    }
}
