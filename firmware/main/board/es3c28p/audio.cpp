#include "board_es3c28p.h"

#include "board_config.h"
#include "driver/gpio.h"
#include "esp_log.h"

bool ES3C28PBoard::InitAudio()
{
    // The ES8311 data path is mapped here. Codec register programming and
    // stream policy are intentionally deferred until the Assistant phase.
    const gpio_config_t pa_config = {
        .pin_bit_mask = 1ULL << AUDIO_PA,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    if (gpio_config(&pa_config) != ESP_OK) return false;
    gpio_set_level(AUDIO_PA, 1); // active-low PA: keep speaker muted at boot
    ESP_LOGI("audio", "ES8311 I2S mapped MCLK=%d BCLK=%d WS=%d DOUT=%d DIN=%d", AUDIO_MCLK,
             AUDIO_BCLK, AUDIO_WS, AUDIO_DOUT, AUDIO_DIN);
    return true;
}
