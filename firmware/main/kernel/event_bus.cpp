#include "event_bus.h"

#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

bool EventBus::Init()
{
    queue_ = xQueueCreate(24, sizeof(DomosEvent));
    return queue_ != nullptr;
}

bool EventBus::Publish(EventType type, const char *source, const char *data)
{
    if (queue_ == nullptr) return false;
    DomosEvent event{};
    event.type = type;
    std::strncpy(event.source, source == nullptr ? "system" : source, sizeof(event.source) - 1);
    std::strncpy(event.data, data == nullptr ? "" : data, sizeof(event.data) - 1);
    return xQueueSend(static_cast<QueueHandle_t>(queue_), &event, 0) == pdPASS;
}

bool EventBus::Subscribe(EventType type, EventListener listener, void *context)
{
    if (listener == nullptr || listener_count_ == kMaxListeners) return false;
    listeners_[listener_count_++] = {.type = type, .callback = listener, .context = context};
    return true;
}

void EventBus::DispatchPending()
{
    if (queue_ == nullptr) return;
    DomosEvent event{};
    while (xQueueReceive(static_cast<QueueHandle_t>(queue_), &event, 0) == pdPASS) {
        for (size_t index = 0; index < listener_count_; ++index) {
            const Listener &listener = listeners_[index];
            if (listener.type == event.type) listener.callback(event, listener.context);
        }
    }
}
