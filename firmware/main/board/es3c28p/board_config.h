#pragma once

#include "driver/gpio.h"

#define BOARD_NAME "ES3C28P"

// The LCD is physically 240x320. DomOS renders it in landscape.
#define TFT_WIDTH 320
#define TFT_HEIGHT 240

#define TFT_MOSI GPIO_NUM_11
#define TFT_MISO GPIO_NUM_13
#define TFT_SCLK GPIO_NUM_12
#define TFT_CS GPIO_NUM_10
#define TFT_DC GPIO_NUM_46
#define TFT_BL GPIO_NUM_45

#define TOUCH_SDA GPIO_NUM_16
#define TOUCH_SCL GPIO_NUM_15
#define TOUCH_RST GPIO_NUM_18
#define TOUCH_INT GPIO_NUM_17
#define FT6336_I2C_ADDRESS 0x38

#define AUDIO_MCLK GPIO_NUM_4
#define AUDIO_BCLK GPIO_NUM_5
#define AUDIO_DIN GPIO_NUM_6
#define AUDIO_WS GPIO_NUM_7
#define AUDIO_DOUT GPIO_NUM_8
#define AUDIO_PA GPIO_NUM_1

#define RGB_LED_PIN GPIO_NUM_42
#define BOOT_BUTTON GPIO_NUM_0
