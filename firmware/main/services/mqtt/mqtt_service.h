#pragma once

#include <cstdint>

#include "esp_event.h"

class EventBus;

class MqttService {
public:
    bool Start(EventBus *events);
    bool Publish(const char *topic, const char *payload);
    bool Connected() const { return connected_; }

private:
    static void OnEvent(void *handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data);

    void *client_ = nullptr;
    EventBus *events_ = nullptr;
    bool connected_ = false;
};
