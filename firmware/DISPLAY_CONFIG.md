# Stable Display & Touch Configuration (ES3C28P Board)

> **Note:** Updated on 2026-08-07. This configuration is confirmed to be stable and working.

## 1. Hardware Pinout

### LCD Display (ILI9341 SPI)
| Pin Name | ESP32-S3 GPIO | Note |
| :--- | :--- | :--- |
| `TFT_MOSI` | `GPIO_NUM_11` | SPI2 Host |
| `TFT_MISO` | `GPIO_NUM_13` | SPI2 Host |
| `TFT_SCLK` | `GPIO_NUM_12` | SPI2 Host (40 MHz clock) |
| `TFT_CS`   | `GPIO_NUM_10` | Chip Select |
| `TFT_DC`   | `GPIO_NUM_46` | Data / Command Select |
| `TFT_BL`   | `GPIO_NUM_45` | Backlight (LEDC PWM, 5kHz, 10-bit duty) |
| `RESET`    | Hardware EN | Shared with ESP32-S3 EN pin |

### Touch Panel (FT6336G I2C)
| Pin Name | ESP32-S3 GPIO | Note |
| :--- | :--- | :--- |
| `TOUCH_SDA` | `GPIO_NUM_16` | I2C0 Master (400 kHz) |
| `TOUCH_SCL` | `GPIO_NUM_15` | I2C0 Master (400 kHz) |
| `TOUCH_RST` | `GPIO_NUM_18` | Hardware Reset |
| `TOUCH_INT` | `GPIO_NUM_17` | Interrupt Pin |
| `I2C ADDR`  | `0x38`        | FT6336G I2C Address |

---

## 2. Display Parameters & Panel Settings

- **Driver IC:** `ILI9341` (via `esp_lcd_ili9341`)
- **Physical Panel:** 240 x 320 pixels
- **Render Canvas (DomOS Landscape Mode):** 320 x 240 (`TFT_WIDTH = 320`, `TFT_HEIGHT = 240`)
- **Pixel Format:** RGB565 (16-bit)
- **Color Element Order:** `LCD_RGB_ELEMENT_ORDER_BGR`
- **Panel Transformations:**
  - `esp_lcd_panel_invert_color(panel_, true)`
  - `esp_lcd_panel_swap_xy(panel_, true)`
  - `esp_lcd_panel_mirror(panel_, false, false)`

---

## 3. Touch Coordinate Mapping (Landscape)

```cpp
point->x = raw_y < TFT_WIDTH ? raw_y : TFT_WIDTH - 1;
point->y = raw_x < TFT_HEIGHT ? TFT_HEIGHT - 1 - raw_x : 0;
```

---

## 4. LVGL & sdkconfig Settings

- **Color Depth:** `CONFIG_LV_COLOR_DEPTH_16=y`
- **Byte Swap:** `CONFIG_LV_COLOR_16_SWAP=y`
- **Draw Buffer:** Double DMA buffer in Internal RAM (`DRAW_LINES = 60`, `320 x 60 * 2` bytes per buffer)
- **LVGL Memory:** 64 KB (`CONFIG_LV_MEM_SIZE_KILOBYTES=64`)

---

## 5. Critical Stability Rules & Gotchas

1. **HTTP Handler Stack Management (Prevent Crash Loops):**
   - NEVER allocate large response buffers (e.g. `char buf[2048]`) on the stack inside HTTP URI handlers.
   - `httpd` worker tasks operate on constrained task stacks (4KB). Large stack arrays cause silent **Stack Overflow (`***ERROR*** A stack overflow in task httpd has been detected`)**, triggering continuous watchdog/RTC software reboots (`rst:0xc`).
   - ALWAYS allocate large HTTP response buffers on the heap (`malloc()` / `free()`).

2. **LVGL DMA Allocation:**
   - SPI DMA buffers (`s_draw_buffer_1`, `s_draw_buffer_2`) MUST be allocated with `MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL`.
   - Always check for `nullptr` and provide a fallback if internal DMA memory is constrained.

## 6. Network & Wi-Fi Configuration

- **Device IP (ESP32-S3 Board):** `192.168.0.106` (Wi-Fi STA Mode / Dashboard Target IP)
- **HTTP Server Port:** `80` (Endpoints: `/api/status`, `/api/logs`, `/api/wallpaper`, `/api/clock`, `/api/launch`, `/ws`)
- **Backend Go Server IP & Port:** `http://192.168.0.105:8080` (or `http://192.168.0.102:8080`)
- **Dashboard Next.js App:** `http://localhost:3000` (Network: `http://192.168.0.105:3000`)

---

## Source Files Reference
- Config Header: [`main/board/es3c28p/board_config.h`](file:///d:/Work%20space/DomOS/firmware/main/board/es3c28p/board_config.h)
- Display Driver: [`main/board/es3c28p/display.cpp`](file:///d:/Work%20space/DomOS/firmware/main/board/es3c28p/display.cpp)
- Touch Driver: [`main/board/es3c28p/touch.cpp`](file:///d:/Work%20space/DomOS/firmware/main/board/es3c28p/touch.cpp)
- SDK Config Defaults: [`sdkconfig.defaults`](file:///d:/Work%20space/DomOS/firmware/sdkconfig.defaults)

