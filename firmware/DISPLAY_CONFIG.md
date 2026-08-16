# Display và Touch — ES3C28P

Tài liệu cấu hình ILI9341, FT6336G và LVGL đang dùng trong firmware DomOS.

## Pinout

### LCD ILI9341

| Signal | GPIO | Cấu hình |
|---|---:|---|
| MOSI | 11 | SPI2 |
| MISO | 13 | SPI2 |
| SCLK | 12 | 40 MHz |
| CS | 10 | chip select |
| DC | 46 | data/command |
| BL | 45 | LEDC PWM, 5 kHz, 10-bit |
| Reset | EN | dùng reset hệ thống |

### Touch FT6336G

| Signal | GPIO | Cấu hình |
|---|---:|---|
| SDA | 16 | I2C0 400 kHz |
| SCL | 15 | I2C0 400 kHz |
| RST | 18 | hardware reset |
| INT | 17 | interrupt |
| Address | — | `0x38` |

Touch và ES8311 dùng chung I2C0; không khởi tạo bus thứ hai trên cùng pin.

## Panel và canvas

- Physical panel: 240×320.
- DomOS landscape canvas: 320×240.
- Pixel format: RGB565, 16-bit.
- Color order: BGR.
- Color inversion: bật.
- Swap X/Y: bật.
- Mirror: tắt cả hai trục.

Cấu hình tương ứng:

```cpp
esp_lcd_panel_invert_color(panel_, true);
esp_lcd_panel_swap_xy(panel_, true);
esp_lcd_panel_mirror(panel_, false, false);
```

## Mapping touch landscape

```cpp
point->x = raw_y < TFT_WIDTH ? raw_y : TFT_WIDTH - 1;
point->y = raw_x < TFT_HEIGHT ? TFT_HEIGHT - 1 - raw_x : 0;
```

Nếu đổi rotate/mirror của LCD, mapping touch phải được kiểm tra lại cùng lúc.

## LVGL và memory

Các option chính trong `sdkconfig.defaults`:

```text
CONFIG_LV_COLOR_DEPTH_16=y
CONFIG_LV_COLOR_16_SWAP=y
CONFIG_LV_MEM_SIZE_KILOBYTES=64
CONFIG_LV_USE_SJPG=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=65536
```

Font đang bật: Montserrat 14, 16, 20, 24 và 48.

LVGL task chạy core 0, priority 6, stack 8192 byte. Draw buffer SPI phải dùng `MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL`; PSRAM không thay thế trực tiếp cho DMA buffer nếu driver yêu cầu DMA-capable internal memory.

Wallpaper dùng PSRAM buffer RGB565 320×240 và JPEG decoder. LittleFS giữ cache wallpaper.

## Backlight

Backlight GPIO45 dùng LEDC. Brightness nằm trong khoảng `0..100` và có thể chỉnh qua MCP `display.adjust_brightness`.

Firmware hiện chưa đăng ký HTTP `/api/brightness`; helper Dashboard gọi route này chưa có tác dụng cho tới khi route được implement.

## Quy tắc ổn định

### HTTP task stack

HTTP server dùng `stack_size = 10240`, nhưng handler vẫn không nên đặt response/file buffer lớn trên stack. Dùng heap, chunked response hoặc stream. `/api/logs` đang gửi JSON theo chunk để tránh stack overflow.

### LVGL thread safety

- Không update object LVGL trực tiếp từ WebSocket/MQTT callback.
- Callback chỉ cập nhật state có mutex/atomic và báo UI/EventBus.
- Tạo/hủy app và object trong LVGL context.

### DMA

- Kiểm tra mọi kết quả `heap_caps_malloc`.
- Giữ đủ internal RAM dự phòng (`CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=65536`).
- Không truyền PSRAM buffer vào API DMA nếu capability không phù hợp.

### Touch

- Reset FT6336G trước khi poll.
- Clamp coordinate về `0..319` và `0..239`.
- Tránh xử lý dài trong touch callback.

## Kiểm tra

1. Boot không có lỗi ILI9341/FT6336.
2. Launcher hiển thị đủ 320×240, đúng màu và không đảo byte.
3. Chạm bốn góc khớp vị trí hiển thị.
4. Brightness thay đổi mượt, không tắt panel ngoài ý muốn.
5. Wallpaper decode không làm giảm mạnh internal heap.
6. Mở/đóng Assistant không crash LVGL task.

API hỗ trợ quan sát:

```powershell
Invoke-RestMethod http://<DEVICE_IP>/api/status
Invoke-RestMethod http://<DEVICE_IP>/api/logs
```

## File liên quan

- `main/board/es3c28p/board_config.h`
- `main/board/es3c28p/display.cpp`
- `main/board/es3c28p/touch.cpp`
- `main/app/launcher/app_manager.cpp`
- `sdkconfig.defaults`
- [Firmware README](README.md)
