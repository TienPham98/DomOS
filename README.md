# DomOS

DomOS là hệ sinh thái trợ lý giọng nói và giao diện nhà thông minh chạy trên board ES3C28P (ESP32-S3). Repository gồm firmware nhúng, Voice Gateway Python, Core Backend Go, Dashboard Next.js và MQTT broker dùng chung.

## Kiến trúc đang hoạt động

```text
Trình duyệt
   ├── HTTP/WS ──> Dashboard Next.js :3000
   │                    ├── REST/WS ──> Go Backend :8081
   │                    ├── REST ─────> Voice Gateway :8000
   │                    └── HTTP ─────> ESP32 :80
   │
ESP32-S3 <DEVICE_IP>
   ├── PCM/WebSocket v3 ──────────────> Python Voice Gateway :8000
   ├── MQTT ──────────────────────────> Broker :1883
   └── HTTP API/WebSocket log ────────> Dashboard

Python Voice Gateway
   ├── STT: Google Web Speech (vi-VN + en-US cho wake word)
   ├── LLM: OpenRouter, model openrouter/free
   ├── TTS: Google TTS; Edge TTS là lựa chọn/fallback
   ├── MCP JSON-RPC: điều khiển volume, brightness và app
   └── SQLite: backend-python/data/conversations.db
```

## Thành phần

| Thành phần | Thư mục | Công nghệ | Địa chỉ cố định |
|---|---|---|---|
| Firmware | `firmware/` | C++17, ESP-IDF 5.3.1, FreeRTOS, LVGL 8.4 | `<DEVICE_IP>:80` |
| Voice Gateway | `backend-python/` | Python, FastAPI, WebSocket, SQLite | `<HOST_IP>:8000` |
| Core Backend | `backend-go/` | Go, Fiber v2, GORM, MQTT | `<HOST_IP>:8081` |
| Dashboard | `dashboard-next/` | Next.js 16, React 19, Tailwind CSS 4 | `<HOST_IP>:3000` |
| MQTT broker | `backend-python/run_broker.py` hoặc Mosquitto | AMQTT/MQTT | `<HOST_IP>:1883` |

## Cấu hình mạng bắt buộc

Các địa chỉ sau là cấu hình cố định của hệ thống và phải được giữ nguyên:

| Node | IP/URL |
|---|---|
| Host chạy tất cả backend | `<HOST_IP>` |
| ESP32-S3 | `<DEVICE_IP>` |
| Wi-Fi | SSID `Dom_12` |
| Voice WebSocket | `ws://<HOST_IP>:8000/api/v1/voice/stream` |
| Go API | `http://<HOST_IP>:8081` |
| Dashboard | `http://<HOST_IP>:3000` |
| MQTT | `mqtt://<HOST_IP>:1883` |

Thay `<HOST_IP>` và `<DEVICE_IP>` bằng địa chỉ LAN thật khi triển khai. Nên đặt DHCP reservation hoặc static IPv4; hai thiết bị phải cùng subnet và Wi-Fi không được bật AP isolation. Không commit địa chỉ triển khai thật vào tài liệu public.

## Yêu cầu môi trường

- Windows PowerShell.
- Python và virtual environment tại `backend-python/.venv`.
- Go phù hợp với `backend-go/go.mod` (hiện khai báo Go 1.25).
- Node.js và npm.
- ESP-IDF 5.3.1 tại `C:\Espressif\frameworks\esp-idf-v5.3.1`.
- ESP-IDF Python env tại `C:\Espressif\python_env\idf5.3_py3.11_env`.
- Board nối tại `COM5`, baud flash `460800`.
- PyAV xử lý decode/resample TTS; pipeline hiện tại không yêu cầu tiến trình FFmpeg riêng.

## Setup lần đầu

### Voice Gateway và MQTT broker

```powershell
cd "D:\Work space\DomOS\backend-python"
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe -m pip install amqtt
```

Tạo `D:\Work space\DomOS\.env` hoặc `backend-python\.env`:

```dotenv
OPENROUTER_API_KEY=thay_bang_key_cua_ban
OPENROUTER_MODEL=openrouter/free
STT_PROVIDER=google-web
STT_LANGUAGE=vi-VN
TTS_PROVIDER=google
CONVERSATION_DB_PATH=data/conversations.db
MQTT_BROKER_HOST=<HOST_IP>
MQTT_BROKER_PORT=1883
VOICE_SESSION_TIMEOUT_SEC=30
```

### Go Backend

```powershell
cd "D:\Work space\DomOS\backend-go"
Copy-Item .env.example .env
```

Giá trị triển khai LAN cần dùng:

```dotenv
APP_ENV=development
APP_PORT=8081
MQTT_BROKER=tcp://<HOST_IP>:1883
MQTT_CLIENT_ID=domos-backend
UPLOAD_DIR=./uploads
JWT_SECRET=thay_bang_chuoi_ngau_nhien_dai
```

Go thử PostgreSQL trước; nếu không kết nối được, code tự chuyển sang SQLite `backend-go/domos.db` và vẫn chạy API.

### Dashboard

```powershell
cd "D:\Work space\DomOS\dashboard-next"
npm install
```

Tạo `.env.local`:

```dotenv
NEXT_PUBLIC_API_URL=http://<HOST_IP>:8081
NEXT_PUBLIC_AI_GATEWAY_URL=http://<HOST_IP>:8000
NEXT_PUBLIC_DEVICE_IP=<DEVICE_IP>
```

### Firmware

```powershell
cd "D:\Work space\DomOS\firmware"
.\build.bat
.\flash.bat
```

`flash.bat` flash COM5 và mở monitor. Địa chỉ mạng được nạp từ `.env` gốc vào cấu hình build riêng tư; Wi-Fi password và token Voice Gateway vẫn có thể đặt bằng `idf.py menuconfig` trong menu `DomOS`.

## Khởi động toàn hệ thống

Mở bốn terminal theo thứ tự sau.

### Terminal 1 — MQTT

```powershell
cd "D:\Work space\DomOS\backend-python"
.\.venv\Scripts\python.exe run_broker.py
```

### Terminal 2 — Go Backend

```powershell
cd "D:\Work space\DomOS\backend-go"
go run .\cmd\server
```

Có thể dùng binary đã build: `./server.exe`.

### Terminal 3 — Voice Gateway

```powershell
cd "D:\Work space\DomOS\backend-python"
.\.venv\Scripts\python.exe -m uvicorn main:app --host 0.0.0.0 --port 8000
```

### Terminal 4 — Dashboard

```powershell
cd "D:\Work space\DomOS\dashboard-next"
npm run dev
```

Sau khi board boot, mở Assistant trên màn hình hoặc gọi:

```powershell
Invoke-RestMethod -Method Post `
  -Uri "http://<DEVICE_IP>/api/launch" `
  -ContentType "application/json" `
  -Body '{"app":"assistant"}'
```

## Kiểm tra nhanh

```powershell
Invoke-RestMethod http://<HOST_IP>:8000/health
Invoke-RestMethod http://<HOST_IP>:8081/healthz
Invoke-WebRequest -UseBasicParsing http://<HOST_IP>:3000
Invoke-RestMethod http://<DEVICE_IP>/api/status
Get-NetTCPConnection -State Listen |
  Where-Object LocalPort -In 1883,3000,8000,8081
```

Chạy toàn bộ test, lint và production build của Gateway, Go Backend,
Dashboard và firmware:

```powershell
.\test-all.ps1
```

Khi chỉ cần vòng test nhanh, bỏ qua các bước build:

```powershell
.\test-all.ps1 -SkipBuild
```

Firmware contract test kiểm tra pin ES3C28P, IP cố định, task audio, state
machine, quyền điều khiển PA và HTTP route. Lệnh đầy đủ còn build firmware
bằng ESP-IDF 5.3.1 nếu toolchain tồn tại tại đường dẫn chuẩn của dự án.

Voice Gateway phải báo `active_sessions: 1` khi app Assistant đã mở và WebSocket kết nối.

## Luồng trợ lý giọng nói

1. Board ở trạng thái `Armed`, gửi PCM mic để kiểm tra wake word.
2. Người dùng nói “Hey Dom” hoặc chạm màn hình Assistant.
3. Wake STT chạy song song `en-US` và `vi-VN`; matcher hỗ trợ các transcript gần âm thanh thực tế của board.
4. Board chuyển `Listening`. VAD chốt câu sau 9 frame im lặng trong cửa sổ 12 frame.
5. Gateway gửi `listen.processing`; LCD hiện `Thinking`.
6. STT tiếng Việt → lệnh trực tiếp/MCP hoặc OpenRouter → TTS.
7. LCD hiện `Responding`; PCM 16 kHz được phát qua ES8311.
8. Kết thúc TTS, hệ thống trở về `Armed`.

Wake word thành công chỉ kích hoạt chế độ nghe. Câu nói kế tiếp là lệnh; khi stress test không nên lặp “Hey Dom” ngay sau khi LCD đã hiện `Listening`.

## Công cụ AI có thể điều khiển

- `device.get_status`
- `speaker.set_volume` (`0..100`)
- `speaker.adjust_volume` (`-100..100`)
- `display.adjust_brightness` (`-100..100`)
- `app.launch`: `wallpaper` hoặc `clock`

Gateway lưu câu người dùng, câu trả lời, model, thời gian và trace tool vào SQLite. Dashboard `/assistant` đọc lịch sử từ `/api/v1/conversations`.

## Tài liệu thành phần

- [Voice Gateway](backend-python/README.md)
- [Go Backend](backend-go/README.md)
- [Dashboard](dashboard-next/README.md)
- [Firmware](firmware/README.md)
- [Display và touch](firmware/DISPLAY_CONFIG.md)
- [Kiến trúc Dom AI](docs/DOM_AI_ARCHITECTURE.md)
- [Phase 1 — Hardware](docs/PHASE1.md)
- [Phase 2 — Services và apps](docs/PHASE2.md)
- [Phase 3 — Voice assistant](docs/PHASE3.md)
- [Wallpaper troubleshooting](docs/WALLPAPER_TROUBLESHOOTING.md)
