#pragma once

class EventBus;

class MediaService {
public:
    bool Start(EventBus *events);
    void SetListening(bool enabled);
    bool Listening() const { return listening_; }

private:
    EventBus *events_ = nullptr;
    bool listening_ = false;
};
