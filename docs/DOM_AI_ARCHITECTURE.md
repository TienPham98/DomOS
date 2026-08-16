# Kiến trúc Dom AI Assistant

Tài liệu mô tả kiến trúc voice assistant đang được triển khai trong DomOS. Hệ thống dùng AI cloud qua OpenRouter; ESP32 không chạy LLM/STT/TTS cục bộ và gateway không gọi API Xiaozhi.

## Sơ đồ thành phần

```text
ES3C28P / ESP32-S3
  ├── ES8311 mic ── PCM 16 kHz/16-bit/mono ─┐
  ├── Assistant UI/state                     │ WebSocket v3
  ├── ES8311 speaker <── PCM TTS ────────────┤
  └── MCP device tools <── JSON-RPC ─────────┘
                                                │
                                                ▼
Python Voice Gateway :8000
  ├── wake matcher + VAD
  ├── Google Web Speech STT
  ├── OpenRouter LLM/tool selection
  ├── Google/Edge TTS + PyAV resample
  └── SQLite conversation memory
        │                    │
        ▼                    ▼
Dashboard :3000       OpenRouter cloud
```

## Ranh giới trách nhiệm

### Firmware

- Capture/phát PCM và giữ realtime task không bị block.
- Quản lý state `Idle/Connecting/Armed/Listening/Processing/Speaking`.
- Chỉ hiển thị trạng thái trên LCD.
- Thực thi MCP tool trên phần cứng thật.
- Hủy response ngay khi người dùng nhấn nút X.

### Python Gateway

- Xác thực handshake/audio format.
- Wake STT song ngữ, normalize audio và VAD.
- STT câu lệnh tiếng Việt.
- Gọi OpenRouter hoặc parser lệnh deterministic.
- Stream TTS theo từng câu.
- Lưu hội thoại/tool trace.

### OpenRouter

- Model mặc định `openrouter/free`.
- Trả lời tiếng Việt ngắn, phù hợp đọc thành tiếng.
- Chọn tool khi yêu cầu điều khiển thiết bị.
- Model thực tế có thể thay đổi theo free router/quota.

### Dashboard

- Xem lịch sử hội thoại và tool call.
- Không nằm trên đường realtime PCM.
- Quản lý board, theme, wallpaper, OTA và log qua các backend/API trực tiếp.

## Dom Voice Protocol v3

Endpoint cố định:

```text
ws://<HOST_IP>:8000/api/v1/voice/stream
```

Audio negotiation:

| Thuộc tính | Giá trị |
|---|---:|
| Codec | raw PCM signed little-endian |
| Sample rate | 16000 Hz |
| Sample width | 16 bit |
| Channels | 1 |
| Frame duration | 60 ms |
| Frame size | 960 sample / 1920 byte |

JSON message:

```text
ESP32 --hello--> Gateway
ESP32 <-hello-- Gateway
ESP32 --binary PCM--> Gateway
ESP32 <-listen/stt/llm/tts-- Gateway
ESP32 <-binary PCM-- Gateway
ESP32 --abort--> Gateway
ESP32 <->mcp JSON-RPC 2.0<-> Gateway
```

`tts.sentence_start` luôn đi trước binary audio của câu tương ứng. Khi TTS hoàn tất, gateway gửi `tts.stop`, `listen.wake` và emotion idle.

## State machine

```text
Idle
  -> Connecting
  -> Armed
  -> Listening
  -> Processing
  -> Speaking
  -> Armed
```

Luồng thay thế:

- Touch ở `Armed` → `Listening`.
- Touch ở `Listening` → manual end utterance → `Processing`.
- Cancel ở `Processing/Speaking` → `abort` → `Armed`.
- WebSocket mất kết nối → `Idle`, flush loa và mute PA.

## Wake word

Wake phrase là “Hey Dom”. Gateway chạy cùng audio qua Google STT `en-US` và `vi-VN`. Matcher ưu tiên transcript trực tiếp; với các lỗi lặp lại của mic/giọng trên board hiện tại, chỉ chấp nhận khi hai recognizer tạo đúng cặp chữ ký đã quan sát. Transcript rỗng luôn bị từ chối.

Thông số:

- threshold RMS 180;
- ít nhất 3 speech frame;
- 9 silent vote trong cửa sổ 12 frame;
- pre-roll 3 frame;
- wake tối đa 3 giây;
- lệnh tối đa 20 giây;
- activation timeout 30 giây.

ES8311 được reset digital block trước init và mic gain đang dùng 42 dB trên unit hiện tại.

## STT, LLM và TTS

### STT

Mặc định `STT_PROVIDER=google-web`, câu lệnh dùng `vi-VN`. Nếu đổi provider, OpenRouter audio path tồn tại trong code nhưng model free có thể yêu cầu credit hoặc không hỗ trợ audio.

### LLM

Gateway gửi system prompt tiếng Việt, recent conversation context và danh sách tool. Một số lệnh thiết bị được parse deterministic trước để không phụ thuộc chất lượng tool calling của model free.

### TTS

- `TTS_PROVIDER=google`: gTTS tiếng Việt.
- Nhánh Edge dùng `TTS_VOICE=vi-VN-HoaiMyNeural`; lỗi/timeout fallback Google.
- MP3 được PyAV decode/resample thành PCM 16 kHz mono trước khi stream.

## MCP bridge

Firmware triển khai JSON-RPC 2.0 protocol version `2024-11-05`:

- `initialize`;
- `tools/list`;
- `tools/call`.

Tools:

| Tool | Thực thi |
|---|---|
| `device.get_status` | Trả assistant/audio state |
| `speaker.set_volume` | Gọi ES8311 output volume |
| `speaker.adjust_volume` | Clamp volume 0–100 |
| `display.adjust_brightness` | Điều khiển LEDC backlight |
| `app.launch` | AppManager mở wallpaper/clock |

Gateway ghi duration, arguments, result và status của mỗi tool call vào SQLite.

## Persistence

Voice history nằm tại `backend-python/data/conversations.db`, tách khỏi database Go. Context gần nhất được tái sử dụng cho lần nói sau của cùng `device_id`. API đọc:

```text
GET http://<HOST_IP>:8000/api/v1/conversations
```

## Security và độ tin cậy

- API key chỉ nằm trong `.env` bị Git ignore.
- Có thể bật `VOICE_AUTH_TOKEN`; board và gateway phải giống nhau.
- Không expose MQTT anonymous hoặc HTTP CORS `*` ra Internet.
- Không xác nhận tool thành công trước khi board trả result.
- Không upload mic khi TTS đang phát vì firmware chưa có AEC.
- Tất cả shared state callback WebSocket phải dùng mutex/atomic.

## File nguồn chính

- `backend-python/main.py`
- `backend-python/config.py`
- `backend-python/services/openrouter_voice_service.py`
- `backend-python/services/conversation_store.py`
- `firmware/main/services/assistant/assistant_service.*`
- `firmware/main/services/assistant/audio_pipeline.*`
- `firmware/main/services/assistant/ws_client.*`
- `dashboard-next/app/assistant/page.tsx`

Xem setup đầy đủ tại [`../README.md`](../README.md) và [`../backend-python/README.md`](../backend-python/README.md).
