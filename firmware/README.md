# DomOS Firmware — ES3C28P

Trong tài liệu, thay `<HOST_IP>` và `<DEVICE_IP>` bằng địa chỉ của môi trường triển khai. Giá trị firmware thực tế vẫn được cấu hình riêng khi build.

Firmware C++17 cho ESP32-S3, build bằng ESP-IDF 5.3.1. Firmware quản lý LCD/touch, audio ES8311, launcher/app, Wi-Fi, MQTT, HTTP API, LittleFS, OTA metadata và Dom Voice Protocol v3.

## Phần cứng cố định

| Thành phần | GPIO/cấu hình |
|---|---|
| LCD ILI9341 | MOSI 11, MISO 13, SCLK 12, CS 10, DC 46, BL 45; SPI2 40 MHz |
| Touch FT6336G | SDA 16, SCL 15, RST 18, INT 17; I2C0 400 kHz, addr `0x38` |
| ES8311 | MCLK 4, BCLK 5, WS 7, DIN 6, DOUT 8; I2S0 16 kHz |
| Speaker PA | GPIO1 active-low: LOW bật, HIGH mute |
| RGB LED | GPIO42 |
| Boot button | GPIO0 |
| Flash | 16 MB QIO/80 MHz |
| PSRAM | 8 MB Octal/80 MHz |
| Serial | COM5, flash baud 460800 |

Không thay đổi pin trong `main/board/es3c28p/board_config.h` khi chỉ sửa tính năng assistant/app.

## Cấu trúc

```text
firmware/
├── main/
│   ├── main.cpp                         app_main và boot order
│   ├── Kconfig.projbuild                menu DomOS
│   ├── board/es3c28p/
│   │   ├── board_config.h               pin và kích thước
│   │   ├── board_es3c28p.*              board facade
│   │   ├── display.cpp                  ILI9341 + LVGL
│   │   ├── touch.cpp                    FT6336G
│   │   ├── audio.cpp                    I2S và PA
│   │   └── es8311.*                     codec wrapper/reset/gain
│   ├── app/launcher/app_manager.*       app registry/UI
│   ├── kernel/                           EventBus, TaskManager, storage
│   └── services/
│       ├── assistant/                    WebSocket, state, audio pipeline, MCP
│       ├── wifi/                         STA/static IP
│       ├── mqtt/                         broker client
│       ├── filesystem/                   HTTP API, upload, logs
│       ├── media/
│       └── ota/
├── partitions.csv
├── sdkconfig.defaults
├── build.bat
├── flash.bat
└── monitor.bat
```

## Boot sequence

`app_main()` thực hiện:

1. Khởi tạo NVS; erase/re-init nếu schema NVS cũ.
2. Khởi tạo `ES3C28PBoard`: display, touch, audio, storage.
3. Khởi tạo `EventBus` và layout LittleFS.
4. Chạy task dispatch event.
5. Start Wi-Fi, OTA, media và HTTP server.
6. Start `AssistantService` với Voice Gateway cố định.
7. Start `AppManager` và đăng ký callback MCP/app.
8. Đợi Wi-Fi có IP rồi mới start MQTT.
9. Start LVGL task.

MQTT được trì hoãn tới `IP_EVENT_GOT_IP` để tránh lwIP gửi dữ liệu khi Wi-Fi còn association và để AppManager cài message handler trước khi broker deliver command.

## Cấu hình mạng

- Board: `<DEVICE_IP>`.
- SSID: `Dom_12`.
- Host backend: `<HOST_IP>`.
- MQTT: `mqtt://<HOST_IP>:1883`.
- Voice: `ws://<HOST_IP>:8000/api/v1/voice/stream`.
- HTTP board: port 80.

Các địa chỉ triển khai chỉ được khai báo trong `.env` ở thư mục gốc. Khi CMake cấu hình firmware, các giá trị này được ghi vào `build/sdkconfig` (đã bị Git bỏ qua). `sdkconfig` không còn là file mã nguồn.

Trong `idf.py menuconfig` → `DomOS`:

| Kconfig | Mặc định | Ý nghĩa |
|---|---|---|
| `CONFIG_DOMOS_WIFI_SSID` | `Dom_12` | SSID Wi-Fi |
| `CONFIG_DOMOS_WIFI_PASSWORD` | rỗng | WPA2 password, compile vào firmware development |
| `CONFIG_DOMOS_WIFI_MAX_RETRY` | `10` | Số lần reconnect |
| `CONFIG_DOMOS_MQTT_URI` | từ `.env` | MQTT broker |
| `CONFIG_DOMOS_OTA_FIRMWARE_URL` | rỗng | URL firmware; OTA disabled khi rỗng |
| `CONFIG_DOMOS_AI_WS_URI` | từ `.env` | Voice WebSocket |
| `CONFIG_DOMOS_AI_AUTH_TOKEN` | rỗng | Optional Bearer token |

## Build, flash và monitor

Môi trường đã khóa trong batch script:

```text
IDF_PATH=C:\Espressif\frameworks\esp-idf-v5.3.1
IDF_PYTHON_ENV_PATH=C:\Espressif\python_env\idf5.3_py3.11_env
```

Build sạch:

```powershell
cd "D:\Work space\DomOS\firmware"
.\build.bat
```

Flash và monitor:

```powershell
.\flash.bat
```

Lệnh thủ công trong ESP-IDF shell:

```powershell
idf.py menuconfig
idf.py build
idf.py -p COM5 -b 460800 flash monitor
```

Thoát monitor bằng `Ctrl+]`.

## Partition table

| Partition | Offset | Size | Vai trò |
|---|---:|---:|---|
| `nvs` | `0x9000` | 64 KB | config/NVS |
| `otadata` | `0x19000` | 8 KB | OTA selection |
| `phy_init` | `0x1B000` | 4 KB | PHY data |
| `ota_0` | `0x20000` | 4 MB | app slot A |
| `ota_1` | tự động | 4 MB | app slot B |
| `littlefs` | tự động | 7 MB | wallpaper/config/cache |
| `coredump` | tự động | 64 KB | crash dump |

## App Manager

App được đăng ký khi boot:

- `launcher`
- `clock`
- `wallpaper`
- `dashboard`
- `settings`
- `smart-home`
- `assistant`
- `ota`

Launcher mặc định mở trước. HTTP `/api/launch` có thể mở các app trên; MCP `app.launch` hiện chỉ cho phép `wallpaper` và `clock`.

## Assistant state machine

```text
Idle -> Connecting -> Armed -> Listening -> Processing -> Speaking -> Armed
```

| State | LCD | Hành vi |
|---|---|---|
| `Idle` | Offline | Chưa có Voice session |
| `Connecting` | Connecting | WebSocket handshake |
| `Armed` | Waiting for Hey Dom | Mic uplink cho wake word |
| `Listening` | Listening | Thu câu lệnh; VAD gateway tự chốt |
| `Processing` | Thinking | STT/LLM/MCP đang chạy |
| `Speaking` | Responding | Nhận binary PCM và phát loa |

Mọi transition đi qua `AssistantService::SetState()`. `EventBus` phát `AssistantState` để UI/subsystem khác đồng bộ.

### Touch/cancel

- Chạm màn hình ở `Armed`: bắt đầu `Listening`.
- Chạm ở `Listening`: manual submit câu nói; VAD vẫn là đường chính.
- Nút X góc trên ở `Processing`/`Speaking`: gửi `abort`, flush output, tắt PA và trở về `Armed`.
- LCD chỉ hiện trạng thái, không hiện transcript/answer. Nội dung đầy đủ nằm trên Dashboard.

## Audio

- PCM: 16 kHz, signed 16-bit, mono.
- Mic chunk: 960 sample = 60 ms = 1920 byte.
- I2S physical bus dùng two-slot Philips stereo như ES8311/Xiaozhi; `esp_codec_dev` đưa ra stream mono.
- ES8311 digital block được software reset (`reg 0x00 = 0x1F`, delay 5 ms) trước init.
- Mic analog gain của board hiện đặt 42 dB vì unit thực tế có mức tín hiệu thấp ở 30 dB.
- PA GPIO1 active-low và chỉ `AssistantService` được bật/tắt PA.
- Không có AEC; mic chỉ upload trong `Armed`/`Listening`, không upload khi loa đang nói.

### FreeRTOS task

| Task | Core | Priority | Vai trò |
|---|---:|---:|---|
| `mic_capture` | 1 | 7 | Đọc ES8311 mic |
| `audio_out` | 0 | 7 | Phát PCM loa |
| `audio_uplink` | theo scheduler | 6 | Gửi PCM qua WebSocket |
| `lvgl` | 0 | 6 | UI tick/render |

Mic/output task không được block bởi HTTP, STT hay network operation dài.

## Dom Voice Protocol v3

Endpoint: `/api/v1/voice/stream`.

- `hello`: version/features/audio negotiation.
- binary uplink: PCM mic.
- `listen`: start/stop/wake/processing.
- `stt`: transcript từ gateway.
- `llm`: text/emotion/state.
- `tts`: start/sentence_start/stop.
- binary downlink: PCM TTS.
- `abort`: user cancel.
- `mcp`: JSON-RPC 2.0 hai chiều.

WebSocket callback chạy trong task của ESP client. Shared state dùng atomic/mutex; không gọi thao tác block dài trong callback.

## MCP tools trên board

- `device.get_status`
- `speaker.set_volume`
- `speaker.adjust_volume`
- `display.adjust_brightness`
- `app.launch` (`wallpaper`, `clock`)

Firmware hỗ trợ MCP `initialize`, `tools/list`, `tools/call` và trả JSON-RPC result/error.

## HTTP API của board

Base URL: `http://<DEVICE_IP>`.

| Method | Path | Trạng thái hiện tại |
|---|---|---|
| `GET` | `/api/status` | MAC, firmware 0.3.5, Wi-Fi, heap, PSRAM, storage |
| `GET` | `/api/logs` | Ring buffer tối đa 50 log |
| `WS` | `/ws` | Realtime log/connection |
| `POST` | `/api/launch` | Mở app theo JSON body |
| `POST/OPTIONS` | `/api/clock` | Style/color/mode/wallpaper clock |
| `POST/OPTIONS` | `/api/wallpaper` | Chọn URL hoặc yêu cầu sync wallpaper |
| `POST/OPTIONS` | `/api/slideshow` | Cấu hình slideshow |
| `POST` | `/upload` | Compatibility acknowledgement |
| `POST` | `/api/upload` | Compatibility acknowledgement; chưa flash OTA binary |

Không có `/api/brightness` trong HTTP route table hiện tại. Brightness dùng MCP.

Ví dụ mở Assistant:

```powershell
Invoke-RestMethod -Method Post `
  -Uri "http://<DEVICE_IP>/api/launch" `
  -ContentType "application/json" `
  -Body '{"app":"assistant"}'
```

## Wallpaper

Firmware tải slideshow list từ:

```text
http://<HOST_IP>:8000/api/wallpapers/slideshow
```

Gateway proxy URL/file từ Go Backend. Firmware cache tối đa các slot `wp_0.jpg` đến `wp_9.jpg` trong `/littlefs/wallpapers/` và decode JPEG vào PSRAM RGB565 buffer 320×240.

## Quy tắc ổn định

- Chỉ `AssistantService` điều khiển PA.
- Không sửa `state_` trực tiếp.
- Không block mic/output task.
- Dùng mutex/atomic trong WebSocket callback.
- Dùng EventBus cho state notification.
- HTTP handler không đặt buffer lớn trên stack.
- LVGL chỉ được gọi trong context an toàn của UI/task.
- DMA buffer phải nằm trong internal DMA-capable RAM.
- Không đổi IP cố định hoặc pin board.

Xem thêm [DISPLAY_CONFIG.md](DISPLAY_CONFIG.md).

Chạy host-side contract test trước khi build/flash:

```powershell
python -m unittest discover -s tests -v
```

Test bảo vệ pin map ES3C28P, địa chỉ mạng cố định, cấu hình PCM/ES8311,
core/priority audio task, state transition, quyền điều khiển PA và HTTP route.

## Debug

```powershell
Invoke-RestMethod http://<DEVICE_IP>/api/status
Invoke-RestMethod http://<DEVICE_IP>/api/logs
```

Trong serial log cần thấy:

- ES8311 ready và mic gain;
- Wi-Fi IP `<DEVICE_IP>`;
- MQTT connected;
- WebSocket connected và hello;
- `Wake-word mode armed`;
- state transition `armed/listening/processing/speaking`.
