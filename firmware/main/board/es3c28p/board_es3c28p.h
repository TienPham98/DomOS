#pragma once

#include <cstdint>

#include "esp_lcd_panel_ops.h"

struct TouchPoint {
    bool pressed;
    uint16_t x;
    uint16_t y;
};

class ES3C28PBoard {
public:
    bool Init();
    bool InitDisplay();
    bool InitTouch();
    bool InitAudio();
    bool InitStorage();
    bool InitLvgl();
    void StartLvglTask();

    void SetBrightness(uint8_t percent);
    bool ReadTouch(TouchPoint *point);
    esp_lcd_panel_handle_t Panel() const { return panel_; }

private:
    esp_lcd_panel_handle_t panel_ = nullptr;
    bool touch_ready_ = false;
};
