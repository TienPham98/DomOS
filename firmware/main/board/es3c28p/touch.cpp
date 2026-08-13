#include "board_es3c28p.h"

#include "board_config.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr char TAG[] = "ft6336";
constexpr uint8_t REG_TOUCHES = 0x02;

bool Check(esp_err_t err, const char *operation)
{
    if (err == ESP_OK) return true;
    ESP_LOGE(TAG, "%s failed: %s", operation, esp_err_to_name(err));
    return false;
}
} // namespace

bool ES3C28PBoard::InitTouch()
{
    gpio_config_t reset_config{};
    reset_config.pin_bit_mask = 1ULL << TOUCH_RST;
    reset_config.mode = GPIO_MODE_OUTPUT;
    reset_config.pull_up_en = GPIO_PULLUP_DISABLE;
    reset_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    reset_config.intr_type = GPIO_INTR_DISABLE;
    if (!Check(gpio_config(&reset_config), "touch reset GPIO")) return false;
    gpio_set_level(TOUCH_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TOUCH_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    i2c_config_t i2c_config{};
    i2c_config.mode = I2C_MODE_MASTER;
    i2c_config.sda_io_num = TOUCH_SDA;
    i2c_config.scl_io_num = TOUCH_SCL;
    i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_config.master.clk_speed = 400000;
    i2c_config.clk_flags = 0;
    if (!Check(i2c_param_config(I2C_NUM_0, &i2c_config), "touch I2C config") ||
        !Check(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0), "touch I2C driver")) return false;

    touch_ready_ = true;
    ESP_LOGI(TAG, "FT6336G ready at I2C address 0x%02X", FT6336_I2C_ADDRESS);
    return true;
}

bool ES3C28PBoard::ReadTouch(TouchPoint *point)
{
    if (point == nullptr || !touch_ready_) return false;
    point->pressed = false;
    const uint8_t reg = REG_TOUCHES;
    uint8_t raw[5] = {};
    if (i2c_master_write_read_device(I2C_NUM_0, FT6336_I2C_ADDRESS, &reg, sizeof(reg), raw, sizeof(raw),
                                     pdMS_TO_TICKS(20)) != ESP_OK || (raw[0] & 0x0F) == 0) return true;

    const uint16_t raw_x = static_cast<uint16_t>(((raw[1] & 0x0F) << 8) | raw[2]);
    const uint16_t raw_y = static_cast<uint16_t>(((raw[3] & 0x0F) << 8) | raw[4]);

    // Keep touch coordinates in the same landscape orientation as the LVGL display.
    point->x = raw_y < TFT_WIDTH ? raw_y : TFT_WIDTH - 1;
    point->y = raw_x < TFT_HEIGHT ? TFT_HEIGHT - 1 - raw_x : 0;
    point->pressed = true;
    ESP_LOGI(TAG, "touch x=%u y=%u", point->x, point->y);
    return true;
}
