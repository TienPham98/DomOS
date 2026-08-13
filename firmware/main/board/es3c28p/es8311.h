#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"

#define ES8311_I2C_ADDR 0x18

struct es8311_config_t {
    i2c_master_dev_handle_t i2c_handle;
    i2s_chan_handle_t tx_handle;
    i2s_chan_handle_t rx_handle;
    int sample_rate;
    int volume; // 0..100
    int mic_gain; // 0..30 dB
};

class ES8311Codec {
public:
    ES8311Codec();
    ~ES8311Codec();

    esp_err_t Init(const es8311_config_t &config);
    esp_err_t SetVolume(int volume_pct);
    esp_err_t SetMicGain(int gain_db);
    esp_err_t SetMute(bool mute);
    esp_err_t EnableMic(bool enable);
    esp_err_t PowerDown();

    // Stream operations
    esp_err_t PlayPcm(const uint8_t *data, size_t length, size_t *bytes_written, uint32_t timeout_ms);
    esp_err_t ReadMicPcm(uint8_t *buffer, size_t length, size_t *bytes_read, uint32_t timeout_ms);

private:
    es8311_config_t config_;
    bool initialized_;

    esp_err_t WriteRegister(uint8_t reg, uint8_t val);
    esp_err_t ReadRegister(uint8_t reg, uint8_t *val);
};
