#pragma once

class EventBus;

class OtaService {
public:
    bool Start(EventBus *events);
    bool UpdateFromUrl(const char *firmware_url);
    bool Busy() const { return busy_; }

private:
    EventBus *events_ = nullptr;
    bool busy_ = false;
};
