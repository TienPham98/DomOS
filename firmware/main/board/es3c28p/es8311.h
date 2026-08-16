#pragma once

#include "esp_err.h"
#include "driver/i2s_std.h"
#include "driver/i2c_master.h"
#include "esp_codec_dev.h"
#include "audio_codec_if.h"
#include "audio_codec_ctrl_if.h"
#include "audio_codec_data_if.h"
#include "audio_codec_gpio_if.h"

struct es8311_config_t {
    i2s_chan_handle_t tx_handle = nullptr;
    i2s_chan_handle_t rx_handle = nullptr;
    i2c_master_bus_handle_t i2c_bus = nullptr;
    int sample_rate = 16000;
    int volume = 80;    // 0..100
    int mic_gain = 42;  // 0..42 dB; this ES3C28P microphone needs full analog gain
};

class ES8311Codec {
public:
    ES8311Codec();
    ~ES8311Codec();

    esp_err_t Init(const es8311_config_t &config);
    esp_err_t SetVolume(int volume_pct);
    int GetVolume() const { return config_.volume; }
    esp_err_t SetMicGain(int gain_db);
    esp_err_t SetMute(bool mute);
    esp_err_t EnableMic(bool enable);
    esp_err_t PowerDown();

    // Stream operations
    esp_err_t PlayPcm(const uint8_t *data, size_t length, size_t *bytes_written, uint32_t timeout_ms);
    esp_err_t ReadMicPcm(uint8_t *buffer, size_t length, size_t *bytes_read, uint32_t timeout_ms);

private:
    es8311_config_t config_;
    bool initialized_ = false;
    const audio_codec_data_if_t *data_if_ = nullptr;
    const audio_codec_ctrl_if_t *ctrl_if_ = nullptr;
    const audio_codec_gpio_if_t *gpio_if_ = nullptr;
    const audio_codec_if_t *codec_if_ = nullptr;
    esp_codec_dev_handle_t device_ = nullptr;

    void Cleanup();
};
