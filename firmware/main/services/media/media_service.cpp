#include "media_service.h"

#include "kernel/event_bus.h"

bool MediaService::Start(EventBus *events)
{
    events_ = events;
    return true;
}

void MediaService::SetListening(bool enabled)
{
    listening_ = enabled;
    if (events_) events_->Publish(EventType::AssistantState, "media", enabled ? "listening" : "idle");
}
