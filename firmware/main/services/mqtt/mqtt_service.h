#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>

#include "esp_event.h"

class EventBus;

class MqttService {
public:
    using MessageHandler = std::function<void(const char *topic, size_t topic_len,
                                               const char *payload, size_t payload_len)>;

    bool Start(EventBus *events);
    bool Publish(const char *topic, const char *payload);
    bool Connected() const { return connected_; }
    void SetMessageHandler(MessageHandler handler) { message_handler_ = std::move(handler); }

private:
    static void OnEvent(void *handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data);

    void *client_ = nullptr;
    EventBus *events_ = nullptr;
    bool connected_ = false;
    MessageHandler message_handler_;
    std::string incoming_topic_;
    std::string incoming_payload_;
};
