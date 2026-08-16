# Phase 2 — Services, apps và quản lý thiết bị

Phase 2 tổ chức hardware bring-up thành hệ thống event-driven có app lifecycle, backend, MQTT, HTTP API và Dashboard.

## Firmware services đã triển khai

| Service | Vai trò |
|---|---|
| `EventBus` | Queue và phát sự kiện giữa subsystem |
| `TaskManager` | Tạo/quản lý FreeRTOS task |
| `StorageManager` | LittleFS layout |
| `WifiService` | STA/static IP và reconnect |
| `MqttService` | Device state/command |
| `WallpaperUploadServer` | HTTP API, status, log, WebSocket |
| `OtaService` | OTA service boundary và URL config |
| `MediaService` | Media/assistant event boundary |
| `AppManager` | App registry, launch/close/navigation |

## App hiện có

- Launcher
- Clock
- Wallpaper
- Dashboard
- Settings
- Smart Home
- Assistant
- OTA

## Backend/Dashboard

- Go Fiber API `<HOST_IP>:8081`.
- PostgreSQL với SQLite fallback.
- MQTT broker `<HOST_IP>:1883`.
- Next.js Dashboard `<HOST_IP>:3000`.
- Device API `<DEVICE_IP>:80`.

## Boot flow hiện tại

```text
NVS -> Board -> EventBus/Storage
    -> Wi-Fi + HTTP + OTA + Media
    -> AssistantService + AppManager/Launcher
    -> đợi IP -> MQTT
    -> LVGL task
```

## Wallpaper flow

```text
Browser -> Go upload/storage :8081
       -> POST device /api/wallpaper
ESP32 -> Python slideshow proxy :8000
      -> Go slideshow/file :8081
      -> LittleFS cache -> JPEG decode PSRAM -> LVGL image
```

## Theme/clock flow

Dashboard gửi style/color/mode tới `POST http://<DEVICE_IP>/api/clock`. Firmware lưu clock settings vào NVS và cập nhật app.

## MQTT flow

- Board kết nối broker sau khi Wi-Fi có IP.
- Go subscribe state topic và cập nhật database.
- Theme/OTA broadcast dùng MQTT.
- Voice PCM không đi qua MQTT; dùng WebSocket riêng.

## Các ranh giới chưa hoàn thiện

- `POST /api/upload` chưa flash OTA binary.
- `/api/brightness` chưa tồn tại; dùng MCP.
- Smart Home UI/MQTT là nền tảng, chưa phải integration đầy đủ với Home Assistant.
- Telemetry temperature trên Dashboard là placeholder.

## Kiểm tra phase

```powershell
Invoke-RestMethod http://<HOST_IP>:8081/healthz
Invoke-WebRequest -UseBasicParsing http://<HOST_IP>:3000
Invoke-RestMethod http://<DEVICE_IP>/api/status
Get-NetTCPConnection -LocalPort 1883 -State Listen
```

Xem [`../backend-go/README.md`](../backend-go/README.md), [`../dashboard-next/README.md`](../dashboard-next/README.md) và [`WALLPAPER_TROUBLESHOOTING.md`](WALLPAPER_TROUBLESHOOTING.md).
