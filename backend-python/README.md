# DomOS Python Voice Gateway

Trong tài liệu, thay `<HOST_IP>` và `<DEVICE_IP>` bằng địa chỉ của môi trường triển khai; không commit địa chỉ thật vào README public.

FastAPI gateway kết nối ESP32-S3 với các dịch vụ AI cloud. Service nhận PCM qua WebSocket, phát hiện wake word/VAD, nhận dạng tiếng Việt, gọi OpenRouter, thực thi MCP trên board, tổng hợp giọng nói và lưu lịch sử hội thoại.

## Trạng thái triển khai

- HTTP: `http://<HOST_IP>:8000`
- Voice WebSocket: `ws://<HOST_IP>:8000/api/v1/voice/stream`
- AI: OpenRouter, mặc định `openrouter/free`
- Local AI: tắt hoàn toàn
- STT mặc định: Google Web Speech
- TTS mặc định: Google TTS; có Edge TTS
- Memory: SQLite `data/conversations.db`
- Protocol: Dom Voice Protocol v3, PCM 16 kHz, 16-bit, mono, frame 60 ms/1920 byte

## Cấu trúc

```text
backend-python/
├── main.py                              FastAPI app và route
├── config.py                            Pydantic Settings
├── run_broker.py                        MQTT broker development :1883
├── requirements.txt
├── services/
│   ├── openrouter_voice_service.py      session, VAD, STT, LLM, MCP, TTS
│   └── conversation_store.py            SQLite conversation/tool trace
├── tests/test_voice_protocol.py
└── data/conversations.db                 tạo tự động
```

Các service AI cục bộ cũ (`llm_service.py`, `stt_service.py`, `tts_service.py`, `session_manager.py`) không còn thuộc pipeline hiện tại.

## Setup

```powershell
cd "D:\Work space\DomOS\backend-python"
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install --upgrade pip
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe -m pip install amqtt
```

`amqtt` hiện được dùng riêng bởi `run_broker.py` nhưng chưa nằm trong `requirements.txt`, vì vậy cần cài thêm khi tạo môi trường mới. Có thể bỏ bước này nếu dùng Mosquitto bên ngoài.

`config.py` đọc biến từ root `.env` trước và `backend-python/.env` sau. Tạo một trong hai file với key thật, không commit secret:

```dotenv
APP_NAME=DomOS OpenRouter Voice Gateway
HOST=0.0.0.0
PORT=8000
DOMOS_DEBUG=false

OPENROUTER_API_KEY=thay_bang_openrouter_api_key
OPENROUTER_BASE_URL=https://openrouter.ai/api/v1
OPENROUTER_MODEL=openrouter/free
OPENROUTER_AUDIO_MODEL=nvidia/nemotron-3-nano-omni-30b-a3b-reasoning:free
OPENROUTER_TIMEOUT_SEC=60

STT_PROVIDER=google-web
STT_LANGUAGE=vi-VN
TTS_PROVIDER=google
TTS_VOICE=vi-VN-HoaiMyNeural
TTS_TIMEOUT_SEC=15

CONVERSATION_DB_PATH=data/conversations.db
MQTT_BROKER_HOST=<HOST_IP>
MQTT_BROKER_PORT=1883
MQTT_CLIENT_ID=domos-ai-gateway
MQTT_USERNAME=
MQTT_PASSWORD=

VOICE_SESSION_TIMEOUT_SEC=30
VOICE_AUTH_TOKEN=
```

### Ý nghĩa cấu hình

| Biến | Mặc định | Vai trò |
|---|---|---|
| `OPENROUTER_API_KEY` | rỗng | Bắt buộc để gọi LLM OpenRouter |
| `OPENROUTER_MODEL` | `openrouter/free` | Router/model chat và tool calling |
| `OPENROUTER_AUDIO_MODEL` | Nemotron free | Chỉ dùng nếu đổi STT khỏi `google-web` |
| `STT_PROVIDER` | `google-web` | STT đang hoạt động trên board hiện tại |
| `STT_LANGUAGE` | `vi-VN` | Ngôn ngữ câu lệnh; wake còn chạy thêm `en-US` |
| `TTS_PROVIDER` | `google` | `google` hoặc nhánh Edge TTS |
| `TTS_VOICE` | `vi-VN-HoaiMyNeural` | Voice dùng bởi Edge TTS |
| `VOICE_SESSION_TIMEOUT_SEC` | `30` | Thời gian chờ lệnh sau khi wake |
| `VOICE_AUTH_TOKEN` | rỗng | Nếu có, firmware phải gửi Bearer token giống hệt |
| `CONVERSATION_DB_PATH` | `data/conversations.db` | File lưu hội thoại và trace tool |

## Khởi động

Khởi động MQTT development broker trước:

```powershell
.\.venv\Scripts\python.exe run_broker.py
```

Khởi động gateway:

```powershell
.\.venv\Scripts\python.exe -m uvicorn main:app --host 0.0.0.0 --port 8000
```

Chế độ reload khi phát triển:

```powershell
.\.venv\Scripts\python.exe -m uvicorn main:app --host 0.0.0.0 --port 8000 --reload
```

## HTTP API

| Method | Path | Mô tả |
|---|---|---|
| `GET` | `/health` | Provider, model, STT/TTS, API key status, số Voice session |
| `WS` | `/api/v1/voice/stream` | Dom Voice Protocol v3 |
| `GET` | `/api/v1/conversations?device_id=&limit=50` | Lịch sử hội thoại và tool trace |
| `GET` | `/api/wallpapers/slideshow` | Proxy danh sách slideshow từ Go :8081 |
| `GET` | `/uploads/wallpapers/{filename}` | Proxy file wallpaper từ Go :8081 |

Kiểm tra:

```powershell
Invoke-RestMethod http://<HOST_IP>:8000/health
```

Khi Assistant đang mở, `active_sessions` phải là `1`.

## Voice Protocol v3

### Handshake

Board gửi:

```json
{
  "type": "hello",
  "version": 3,
  "features": {"mcp": true, "aec": false, "vad": true},
  "audio_params": {
    "codec": "pcm",
    "sample_rate": 16000,
    "channels": 1,
    "frame_duration": 60
  }
}
```

Gateway trả `hello` với `session_id`, provider và feature acknowledgement. Binary frame hai chiều là raw signed PCM little-endian, không phải Opus/MP3.

### Message JSON

| Type | Hướng | Nội dung |
|---|---|---|
| `listen` | hai chiều | `wake`, `start`, `processing`, `stop` |
| `stt` | gateway → board | Transcript câu lệnh |
| `llm` | gateway → board | Text/emotion; firmware chỉ hiện trạng thái |
| `tts` | gateway → board | `start`, `sentence_start`, `stop` |
| `abort` | board → gateway | Hủy pipeline/TTS khi nhấn nút X |
| `mcp` | hai chiều | JSON-RPC 2.0 `initialize`, `tools/list`, `tools/call` |

## Wake word và VAD

Gateway có hai chế độ capture:

```text
WAKE_WORD -> LISTENING -> PROCESSING -> SPEAKING -> WAKE_WORD
```

- Wake phrase: `Hey Dom` và các transcript gần âm do Google tạo trên board hiện tại.
- Wake STT chạy song song `en-US` và `vi-VN`; một số lỗi lặp lại chỉ được chấp nhận khi cả hai kết quả tạo đúng cặp chữ ký, giúp giảm false positive.
- Đoạn wake ngắn được normalize và thêm 250 ms silence hai đầu.
- `VAD_ENERGY_THRESHOLD=180`.
- `VAD_MIN_SPEECH_FRAMES=3`.
- `VAD_SILENCE_FRAMES=9`, xét trên cửa sổ 12 frame để chịu được xung nhiễu.
- Wake hard limit: 3 giây; câu lệnh hard limit: 20 giây.
- Mỗi frame: 60 ms, 1920 byte.
- Sau wake, nếu không có lệnh trong 30 giây, gateway trở lại `WAKE_WORD`.

Không chấp nhận transcript rỗng làm wake. Khi test lặp, sau một wake thành công phải đợi `Listening` và nói lệnh; cụm tiếp theo không còn là một wake event.

## Pipeline câu lệnh

```text
PCM -> VAD end -> listen.processing -> STT vi-VN
    -> direct deterministic command hoặc OpenRouter chat/tool call
    -> llm text/emotion
    -> TTS sentence_start + binary PCM
    -> tts.stop -> WAKE_WORD
```

Lệnh volume/brightness/app có parser trực tiếp để vẫn ổn định khi model free được OpenRouter chọn có tool calling yếu. Yêu cầu khác đi qua OpenRouter.

## MCP tools

| Tool | Arguments | Kết quả |
|---|---|---|
| `device.get_status` | `{}` | State assistant và audio pipeline |
| `speaker.set_volume` | `{"volume": 0..100}` | Đặt volume tuyệt đối |
| `speaker.adjust_volume` | `{"delta": -100..100}` | Tăng/giảm volume |
| `display.adjust_brightness` | `{"delta": -100..100}` | Tăng/giảm backlight |
| `app.launch` | `{"app":"wallpaper"}` hoặc `clock` | Yêu cầu AppManager mở app |

Gateway chỉ xác nhận thành công sau khi firmware trả MCP result.

## Conversation memory

`ConversationStore` tạo SQLite schema tự động. Mỗi turn lưu:

- `device_id`, câu người dùng, câu AI;
- provider/model;
- trạng thái và timestamp;
- tool name, arguments, result, duration và success/error.

Context gần nhất được đưa vào lần gọi OpenRouter tiếp theo. Dashboard đọc cùng dữ liệu qua `/api/v1/conversations`.

## Test và debug

```powershell
.\.venv\Scripts\python.exe -m py_compile main.py config.py services\openrouter_voice_service.py
.\.venv\Scripts\python.exe -m unittest discover -s tests -v
```

Log cần quan sát:

- `OpenRouter voice connected`: handshake thành công.
- `Wake audio`: số frame, speech frame, peak/average RMS.
- `Wake phrase accepted/rejected`: transcript song ngữ.
- `Command speech ended`: VAD đã tự chốt câu.
- `STT device=...`: transcript lệnh.
- HTTP `200 OK` từ OpenRouter.

Nếu board online nhưng `active_sessions=0`, mở app Assistant hoặc `POST http://<DEVICE_IP>/api/launch` với `{"app":"assistant"}`.
