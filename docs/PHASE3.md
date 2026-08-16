# Phase 3 — Cloud voice assistant và device tools

Phase 3 hiện đã triển khai pipeline trợ lý giọng nói cloud end-to-end.

## Chức năng đã hoàn thành

- Dom Voice Protocol v3 qua WebSocket.
- PCM full-duplex 16 kHz/16-bit/mono.
- Wake word “Hey Dom” và touch activation.
- VAD tự chốt câu nói.
- Google Web Speech tiếng Việt.
- OpenRouter `openrouter/free`.
- Google/Edge TTS stream về board.
- MCP điều khiển volume, brightness, wallpaper và clock.
- Cancel response bằng nút X.
- LCD chỉ hiển thị trạng thái.
- SQLite conversation/tool history và Dashboard viewer.

## Trạng thái UI

```text
Waiting for Hey Dom -> Listening -> Thinking -> Responding
```

## Tiêu chí chấp nhận

1. Gateway `/health` online và `active_sessions=1`.
2. Nói “Hey Dom” chuyển LCD sang `Listening`.
3. Im lặng sau câu lệnh chuyển sang `Thinking` mà không cần chạm.
4. OpenRouter trả tiếng Việt và loa phát được.
5. Nút X dừng âm thanh/PA ngay.
6. “Tăng âm lượng/độ sáng” thực thi tool thật.
7. “Mở wallpaper/clock” mở đúng app.
8. Dashboard Assistant hiển thị turn và tool duration.
9. Kết thúc response trở về wake mode.

## Kiểm tra backend

```powershell
cd "D:\Work space\DomOS\backend-python"
.\.venv\Scripts\python.exe -m unittest discover -s tests -v
Invoke-RestMethod http://<HOST_IP>:8000/health
```

## Giới hạn

- Wake phụ thuộc STT cloud và chất lượng mic; transcript rỗng bị từ chối.
- Không có AEC nên firmware ngừng mic uplink khi TTS phát.
- Free model/quota OpenRouter thay đổi theo thời điểm.
- Không có offline fallback vì yêu cầu hiện tại là cloud API only.

Chi tiết tại [`DOM_AI_ARCHITECTURE.md`](DOM_AI_ARCHITECTURE.md) và [`../backend-python/README.md`](../backend-python/README.md).
