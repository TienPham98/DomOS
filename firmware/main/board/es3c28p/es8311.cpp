#include "es8311.h"

#include <algorithm>
#include <climits>

#include "driver/gpio.h"
#include "esp_codec_dev_defaults.h"
#include "esp_log.h"
#include "es8311_codec.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr char kTag[] = "ES8311";

esp_err_t CodecResult(int result)
{
    return result == ESP_CODEC_DEV_OK ? ESP_OK : static_cast<esp_err_t>(result);
}
}  // namespace

ES8311Codec::ES8311Codec() = default;

ES8311Codec::~ES8311Codec()
{
    Cleanup();
}

esp_err_t ES8311Codec::Init(const es8311_config_t &config)
{
    if (initialized_) {
        return ESP_ERR_INVALID_STATE;
    }
    config_ = config;

    audio_codec_i2s_cfg_t i2s_config = {};
    i2s_config.port = I2S_NUM_0;
    i2s_config.rx_handle = config_.rx_handle;
    i2s_config.tx_handle = config_.tx_handle;
    data_if_ = audio_codec_new_i2s_data(&i2s_config);

    // Touch and codec share the board's I2C_NUM_0 master bus.
    audio_codec_i2c_cfg_t i2c_config = {};
    i2c_config.port = I2C_NUM_0;
    i2c_config.addr = ES8311_CODEC_DEFAULT_ADDR;
    i2c_config.bus_handle = config_.i2c_bus;
    ctrl_if_ = audio_codec_new_i2c_ctrl(&i2c_config);
    gpio_if_ = audio_codec_new_gpio();

    if (!data_if_ || !ctrl_if_ || !gpio_if_) {
        ESP_LOGE(kTag, "Failed to create codec interfaces");
        Cleanup();
        return ESP_ERR_NO_MEM;
    }

    // Reset the ES8311 digital blocks before creating the codec interface.
    // This matches Xiaozhi's initialization sequence and prevents stale codec
    // state after warm resets from corrupting short microphone captures.
    uint8_t reset_value = 0x1F;
    esp_err_t reset_result = CodecResult(
        ctrl_if_->write_reg(ctrl_if_, 0x00, 1, &reset_value, 1));
    if (reset_result != ESP_OK) {
        ESP_LOGE(kTag, "Codec software reset failed: %s", esp_err_to_name(reset_result));
        Cleanup();
        return reset_result;
    }
    vTaskDelay(pdMS_TO_TICKS(5));

    es8311_codec_cfg_t codec_config = {};
    codec_config.ctrl_if = ctrl_if_;
    codec_config.gpio_if = gpio_if_;
    codec_config.codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH;
    // AssistantService exclusively controls the active-low PA on GPIO1.
    codec_config.pa_pin = GPIO_NUM_NC;
    codec_config.use_mclk = true;
    codec_if_ = es8311_codec_new(&codec_config);
    if (!codec_if_) {
        ESP_LOGE(kTag, "Failed to create ES8311 interface");
        Cleanup();
        return ESP_FAIL;
    }

    esp_codec_dev_cfg_t device_config = {};
    device_config.dev_type = ESP_CODEC_DEV_TYPE_IN_OUT;
    device_config.codec_if = codec_if_;
    device_config.data_if = data_if_;
    device_ = esp_codec_dev_new(&device_config);
    if (!device_) {
        ESP_LOGE(kTag, "Failed to create codec device");
        Cleanup();
        return ESP_ERR_NO_MEM;
    }

    esp_codec_dev_sample_info_t sample_info = {};
    sample_info.bits_per_sample = 16;
    sample_info.channel = 1;
    sample_info.channel_mask = 0;
    sample_info.sample_rate = config_.sample_rate;
    sample_info.mclk_multiple = 0;

    esp_err_t result = CodecResult(esp_codec_dev_open(device_, &sample_info));
    if (result != ESP_OK) {
        ESP_LOGE(kTag, "Codec open failed: %s", esp_err_to_name(result));
        Cleanup();
        return result;
    }

    initialized_ = true;
    if ((result = SetMicGain(config_.mic_gain)) != ESP_OK ||
        (result = SetVolume(config_.volume)) != ESP_OK) {
        ESP_LOGE(kTag, "Codec gain setup failed: %s", esp_err_to_name(result));
        Cleanup();
        return result;
    }

    ESP_LOGI(kTag, "Ready: %d Hz, 16-bit mono, mic gain %d dB, volume %d%%",
             config_.sample_rate, config_.mic_gain, config_.volume);
    return ESP_OK;
}

esp_err_t ES8311Codec::SetVolume(int volume_pct)
{
    if (!device_) {
        return ESP_ERR_INVALID_STATE;
    }
    config_.volume = std::clamp(volume_pct, 0, 100);
    return CodecResult(esp_codec_dev_set_out_vol(device_, config_.volume));
}

esp_err_t ES8311Codec::SetMicGain(int gain_db)
{
    if (!device_) {
        return ESP_ERR_INVALID_STATE;
    }
    config_.mic_gain = std::clamp(gain_db, 0, 42);
    return CodecResult(esp_codec_dev_set_in_gain(device_, config_.mic_gain));
}

esp_err_t ES8311Codec::SetMute(bool mute)
{
    return device_ ? CodecResult(esp_codec_dev_set_out_mute(device_, mute))
                   : ESP_ERR_INVALID_STATE;
}

esp_err_t ES8311Codec::EnableMic(bool enable)
{
    return device_ ? CodecResult(esp_codec_dev_set_in_mute(device_, !enable))
                   : ESP_ERR_INVALID_STATE;
}

esp_err_t ES8311Codec::PowerDown()
{
    if (!device_) {
        return ESP_OK;
    }
    const esp_err_t result = CodecResult(esp_codec_dev_close(device_));
    initialized_ = false;
    return result;
}

esp_err_t ES8311Codec::PlayPcm(const uint8_t *data, size_t length,
                               size_t *bytes_written, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (!initialized_ || !device_ || !data || length > INT_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    const esp_err_t result = CodecResult(
        esp_codec_dev_write(device_, const_cast<uint8_t *>(data), static_cast<int>(length)));
    if (bytes_written) {
        *bytes_written = result == ESP_OK ? length : 0;
    }
    return result;
}

esp_err_t ES8311Codec::ReadMicPcm(uint8_t *buffer, size_t length,
                                  size_t *bytes_read, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (!initialized_ || !device_ || !buffer || length > INT_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    const esp_err_t result = CodecResult(
        esp_codec_dev_read(device_, buffer, static_cast<int>(length)));
    if (bytes_read) {
        *bytes_read = result == ESP_OK ? length : 0;
    }
    return result;
}

void ES8311Codec::Cleanup()
{
    if (device_) {
        esp_codec_dev_delete(device_);
        device_ = nullptr;
    }
    if (codec_if_) {
        audio_codec_delete_codec_if(codec_if_);
        codec_if_ = nullptr;
    }
    if (gpio_if_) {
        audio_codec_delete_gpio_if(gpio_if_);
        gpio_if_ = nullptr;
    }
    if (ctrl_if_) {
        audio_codec_delete_ctrl_if(ctrl_if_);
        ctrl_if_ = nullptr;
    }
    if (data_if_) {
        audio_codec_delete_data_if(data_if_);
        data_if_ = nullptr;
    }
    initialized_ = false;
}
