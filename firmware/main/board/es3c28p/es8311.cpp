#include "es8311.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ES8311";

// ES8311 Register Definitions
#define ES8311_REG_RESET        0x00
#define ES8311_REG_CLK_MANAGER1 0x01
#define ES8311_REG_CLK_MANAGER2 0x02
#define ES8311_REG_ADC_CAC     0x09
#define ES8311_REG_DAC_CAC     0x0A
#define ES8311_REG_SYS_PERIPH1 0x0D
#define ES8311_REG_SYS_PERIPH2 0x0E
#define ES8311_REG_SYSTEM      0x12
#define ES8311_REG_ADC_PWR     0x13
#define ES8311_REG_ADC_MUTE    0x14
#define ES8311_REG_ADC_VOLUME  0x17
#define ES8311_REG_ADC_ALC1    0x16
#define ES8311_REG_DAC_PWR     0x31
#define ES8311_REG_DAC_MUTE    0x32
#define ES8311_REG_DAC_VOLUME  0x33

ES8311Codec::ES8311Codec() : initialized_(false) {}

ES8311Codec::~ES8311Codec() {
    PowerDown();
}

esp_err_t ES8311Codec::WriteRegister(uint8_t reg, uint8_t val) {
    if (!config_.i2c_handle) return ESP_ERR_INVALID_STATE;
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(config_.i2c_handle, buf, sizeof(buf), 100);
}

esp_err_t ES8311Codec::ReadRegister(uint8_t reg, uint8_t *val) {
    if (!config_.i2c_handle) return ESP_ERR_INVALID_STATE;
    return i2c_master_transmit_receive(config_.i2c_handle, &reg, 1, val, 1, 100);
}

esp_err_t ES8311Codec::Init(const es8311_config_t &config) {
    config_ = config;
    ESP_LOGI(TAG, "Initializing ES8311 Codec over I2C (sample rate: %d Hz)...", config_.sample_rate);

    // Reset codec
    WriteRegister(ES8311_REG_RESET, 0x1F);
    vTaskDelay(pdMS_TO_TICKS(20));
    WriteRegister(ES8311_REG_RESET, 0x00);

    // System clock setup (slave mode, MCLK supplied by ESP32-S3 GPIO4)
    WriteRegister(ES8311_REG_CLK_MANAGER1, 0x3F);
    WriteRegister(ES8311_REG_CLK_MANAGER2, 0x00);

    // System power management
    WriteRegister(ES8311_REG_SYS_PERIPH1, 0x00);
    WriteRegister(ES8311_REG_SYS_PERIPH2, 0x10);

    // ADC setup (Microphone input)
    WriteRegister(ES8311_REG_ADC_PWR, 0x00);
    WriteRegister(ES8311_REG_ADC_MUTE, 0x00);
    SetMicGain(config_.mic_gain > 0 ? config_.mic_gain : 24);

    // DAC setup (Speaker output)
    WriteRegister(ES8311_REG_DAC_PWR, 0x00);
    WriteRegister(ES8311_REG_DAC_MUTE, 0x00);
    SetVolume(config_.volume > 0 ? config_.volume : 80);

    initialized_ = true;
    ESP_LOGI(TAG, "ES8311 initialized successfully.");
    return ESP_OK;
}

esp_err_t ES8311Codec::SetVolume(int volume_pct) {
    if (volume_pct < 0) volume_pct = 0;
    if (volume_pct > 100) volume_pct = 100;
    uint8_t reg_val = (uint8_t)((volume_pct * 255) / 100);
    return WriteRegister(ES8311_REG_DAC_VOLUME, reg_val);
}

esp_err_t ES8311Codec::SetMicGain(int gain_db) {
    uint8_t gain_val = (uint8_t)(gain_db & 0x0F);
    return WriteRegister(ES8311_REG_ADC_VOLUME, gain_val);
}

esp_err_t ES8311Codec::SetMute(bool mute) {
    return WriteRegister(ES8311_REG_DAC_MUTE, mute ? 0x01 : 0x00);
}

esp_err_t ES8311Codec::EnableMic(bool enable) {
    return WriteRegister(ES8311_REG_ADC_MUTE, enable ? 0x00 : 0x01);
}

esp_err_t ES8311Codec::PowerDown() {
    WriteRegister(ES8311_REG_SYS_PERIPH1, 0xFF);
    WriteRegister(ES8311_REG_SYS_PERIPH2, 0xFF);
    initialized_ = false;
    return ESP_OK;
}

esp_err_t ES8311Codec::PlayPcm(const uint8_t *data, size_t length, size_t *bytes_written, uint32_t timeout_ms) {
    if (!config_.tx_handle) return ESP_ERR_INVALID_STATE;
    return i2s_channel_write(config_.tx_handle, data, length, bytes_written, pdMS_TO_TICKS(timeout_ms));
}

esp_err_t ES8311Codec::ReadMicPcm(uint8_t *buffer, size_t length, size_t *bytes_read, uint32_t timeout_ms) {
    if (!config_.rx_handle) return ESP_ERR_INVALID_STATE;
    return i2s_channel_read(config_.rx_handle, buffer, length, bytes_read, pdMS_TO_TICKS(timeout_ms));
}
