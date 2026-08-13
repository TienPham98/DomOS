#include "board_es3c28p.h"

#include "board_config.h"
#include "esp_log.h"

namespace {
constexpr char TAG[] = "es3c28p";
}

bool ES3C28PBoard::Init()
{
    if (!InitStorage() || !InitDisplay() || !InitTouch() || !InitAudio() || !InitLvgl()) {
        return false;
    }
    ESP_LOGI(TAG, "%s board initialised", BOARD_NAME);
    return true;
}
