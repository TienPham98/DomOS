#include "board_es3c28p.h"
#include "board_config.h"
#include "es8311.h"

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "audio";

// Shared handles — accessible by assistant_service via board methods
static i2s_chan_handle_t s_i2s_tx = nullptr;  // DAC → speaker
static i2s_chan_handle_t s_i2s_rx = nullptr;  // ADC ← mic
static ES8311Codec       s_codec;

// ─── Internal helpers ────────────────────────────────────────────────────────

static bool InitI2S()
{
    // ── Channel config (full duplex on I2S0) ──────────────────────────────
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;

    if (i2s_new_channel(&chan_cfg, &s_i2s_tx, &s_i2s_rx) != ESP_OK) {
        ESP_LOGE(TAG, "I2S channel create failed");
        return false;
    }

    // ES8311 transports samples in standard two-slot I2S. esp_codec_dev filters
    // the capture side to the mono PCM stream used by Voice Protocol v3.
    i2s_std_config_t std_cfg = {};
    std_cfg.clk_cfg               = I2S_STD_CLK_DEFAULT_CONFIG(16000);
    std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    std_cfg.slot_cfg              = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    std_cfg.slot_cfg.slot_mask    = I2S_STD_SLOT_BOTH;

    std_cfg.gpio_cfg.mclk = AUDIO_MCLK;  // GPIO4
    std_cfg.gpio_cfg.bclk = AUDIO_BCLK;  // GPIO5
    std_cfg.gpio_cfg.ws   = AUDIO_WS;    // GPIO7
    std_cfg.gpio_cfg.dout = AUDIO_DOUT;  // GPIO8 → speaker
    std_cfg.gpio_cfg.din  = AUDIO_DIN;   // GPIO6 ← mic
    std_cfg.gpio_cfg.invert_flags.mclk_inv = false;
    std_cfg.gpio_cfg.invert_flags.bclk_inv = false;
    std_cfg.gpio_cfg.invert_flags.ws_inv   = false;

    if (i2s_channel_init_std_mode(s_i2s_tx, &std_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "I2S TX init failed");
        return false;
    }
    if (i2s_channel_init_std_mode(s_i2s_rx, &std_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "I2S RX init failed");
        return false;
    }

    if (i2s_channel_enable(s_i2s_tx) != ESP_OK) {
        ESP_LOGE(TAG, "I2S TX enable failed");
        return false;
    }
    if (i2s_channel_enable(s_i2s_rx) != ESP_OK) {
        ESP_LOGE(TAG, "I2S RX enable failed");
        return false;
    }

    ESP_LOGI(TAG, "I2S0 ready 16kHz/16-bit stereo slots, mono codec stream MCLK=%d BCLK=%d WS=%d DOUT=%d DIN=%d",
             AUDIO_MCLK, AUDIO_BCLK, AUDIO_WS, AUDIO_DOUT, AUDIO_DIN);
    return true;
}

// ─── Board public API ────────────────────────────────────────────────────────

bool ES3C28PBoard::InitAudio()
{
    // Power Amplifier GPIO (active-low): keep muted at boot
    const gpio_config_t pa_config = {
        .pin_bit_mask      = 1ULL << AUDIO_PA,
        .mode              = GPIO_MODE_OUTPUT,
        .pull_up_en        = GPIO_PULLUP_DISABLE,
        .pull_down_en      = GPIO_PULLDOWN_DISABLE,
        .intr_type         = GPIO_INTR_DISABLE,
    };
    if (gpio_config(&pa_config) != ESP_OK) {
        ESP_LOGE(TAG, "PA GPIO config failed");
        return false;
    }
    gpio_set_level(AUDIO_PA, 1); // PA muted (active-low)

    if (!InitI2S()) {
        ESP_LOGW(TAG, "I2S init failed");
        return false;
    }

    // Codec registers via shared I2C_NUM_0 bus (initialized in InitTouch)
    es8311_config_t codec_cfg = {};
    codec_cfg.tx_handle   = s_i2s_tx;
    codec_cfg.rx_handle   = s_i2s_rx;
    codec_cfg.i2c_bus     = i2c_bus_;
    codec_cfg.sample_rate = 16000;
    codec_cfg.volume      = 80;
    // This ES3C28P unit produces sub-600 peaks at 30 dB. Use the codec's
    // supported maximum analog gain so cloud STT receives the consonants in
    // short wake phrases before any server-side normalization.
    codec_cfg.mic_gain    = 42;

    if (s_codec.Init(codec_cfg) != ESP_OK) {
        ESP_LOGW(TAG, "ES8311 codec init failed (I2C check)");
        return false;
    }

    audio_ready_ = true;
    ESP_LOGI(TAG, "Audio subsystem ready");
    return true;
}

void ES3C28PBoard::SetPAEnabled(bool enabled)
{
    // PA active-low: LOW = speaker on, HIGH = muted
    gpio_set_level(AUDIO_PA, enabled ? 0 : 1);
    ESP_LOGI(TAG, "PA %s", enabled ? "ON" : "OFF (muted)");
}

esp_err_t ES3C28PBoard::I2S_ReadPCM(int16_t *buf, size_t samples)
{
    size_t bytes_read = 0;
    esp_err_t ret = s_codec.ReadMicPcm(
        reinterpret_cast<uint8_t *>(buf),
        samples * sizeof(int16_t),
        &bytes_read,
        100  // 100ms timeout
    );
    return ret;
}

esp_err_t ES3C28PBoard::I2S_WritePCM(const int16_t *buf, size_t samples)
{
    size_t bytes_written = 0;
    esp_err_t ret = s_codec.PlayPcm(
        reinterpret_cast<const uint8_t *>(buf),
        samples * sizeof(int16_t),
        &bytes_written,
        100  // 100ms timeout
    );
    return ret;
}

ES8311Codec* ES3C28PBoard::GetCodec()
{
    return &s_codec;
}
