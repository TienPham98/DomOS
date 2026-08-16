#include "board_es3c28p.h"

#include "board_config.h"
#include "esp_log.h"

namespace {
constexpr char TAG[] = "es3c28p";
}

bool ES3C28PBoard::Init()
{
    if (!InitStorage()) {
        ESP_LOGE(TAG, "Storage init failed");
        return false;
    }
    if (!InitDisplay()) {
        ESP_LOGE(TAG, "Display init failed");
        return false;
    }
    if (!InitTouch()) {
        ESP_LOGE(TAG, "Touch init failed");
        return false;
    }
    if (!InitAudio()) {
        ESP_LOGW(TAG, "Audio init warning (non-fatal, continuing boot)");
    }
    if (!InitLvgl()) {
        ESP_LOGE(TAG, "LVGL init failed");
        return false;
    }
    ESP_LOGI(TAG, "%s board initialised successfully", BOARD_NAME);
    return true;
}
