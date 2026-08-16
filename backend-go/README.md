# DomOS Go Core Backend

Trong tài liệu, thay `<HOST_IP>` bằng địa chỉ của host triển khai; không commit địa chỉ thật vào README public.

Core REST/MQTT/WebSocket backend quản lý user, thiết bị, theme, wallpaper và OTA metadata. Service dùng Fiber v2, GORM và Paho MQTT.

## Địa chỉ triển khai

- API: `http://<HOST_IP>:8081`
- Health: `http://<HOST_IP>:8081/healthz`
- Dashboard WebSocket: `ws://<HOST_IP>:8081/ws`
- MQTT broker: `tcp://<HOST_IP>:1883`
- Upload static: `http://<HOST_IP>:8081/uploads/...`

## Cấu trúc

```text
backend-go/
├── cmd/server/main.go             entrypoint, middleware và route groups
├── internal/
│   ├── auth/                      JWT register/login/me
│   ├── devices/                   device CRUD và MQTT command
│   ├── themes/                    theme CRUD
│   ├── wallpapers/                upload, thumbnail, slideshow
│   ├── ota/                       firmware metadata/upload/broadcast
│   ├── mqtt/                      Paho client và topic
│   └── websocket/                 realtime dashboard hub
├── pkg/
│   ├── config/                    load `.env`
│   ├── database/                  PostgreSQL + SQLite fallback
│   ├── logger/
│   └── response/
├── migrations/                    SQL tham khảo
├── uploads/                       file wallpaper/firmware local
├── mosquitto/mosquitto.conf
├── docker-compose.yml
└── server.exe                     binary Windows nếu đã build
```

## Cấu hình

Copy file mẫu:

```powershell
cd "D:\Work space\DomOS\backend-go"
Copy-Item .env.example .env
```

Sửa `.env` theo mạng DomOS:

```dotenv
APP_ENV=development
APP_PORT=8081

DATABASE_URL=host=localhost user=domos password=domos dbname=domos port=5432 sslmode=disable TimeZone=UTC

JWT_SECRET=thay_bang_chuoi_ngau_nhien_dai
JWT_EXPIRY_HOURS=72

MQTT_BROKER=tcp://<HOST_IP>:1883
MQTT_CLIENT_ID=domos-backend
MQTT_USERNAME=
MQTT_PASSWORD=

UPLOAD_DIR=./uploads
```

| Biến | Vai trò |
|---|---|
| `APP_PORT` | Phải là `8081` trong deployment DomOS |
| `DATABASE_URL` | PostgreSQL DSN; nếu thất bại sẽ fallback SQLite |
| `JWT_SECRET` | Ký token API; phải thay trước production |
| `JWT_EXPIRY_HOURS` | Thời gian sống JWT |
| `MQTT_BROKER` | Broker triển khai `<HOST_IP>:1883` |
| `UPLOAD_DIR` | Thư mục static wallpaper và firmware |

## Database

Khi boot, `database.Connect` thử PostgreSQL trước. Nếu PostgreSQL không sẵn sàng, backend log warning và mở `domos.db` bằng SQLite. Sau đó `AutoMigrate` tạo/cập nhật bảng:

- users;
- devices;
- themes;
- wallpapers;
- firmware versions.

SQLite fallback phù hợp development một máy. PostgreSQL nên dùng khi triển khai nhiều instance hoặc cần backup/HA.

## Khởi động

MQTT broker cần khởi động trước để lệnh realtime tới board ngay lập tức. Nếu MQTT chưa có, API vẫn chạy và Paho sẽ reconnect.

### Chạy source

```powershell
cd "D:\Work space\DomOS\backend-go"
go mod download
go run .\cmd\server
```

### Build và chạy binary

```powershell
go build -o server.exe .\cmd\server
.\server.exe
```

### Docker Compose

```powershell
docker compose up --build
```

Compose chạy PostgreSQL 16, Mosquitto và backend `:8081`. Trong container, backend dùng hostname `postgres`/`mosquitto`; bên ngoài LAN truy cập qua `<HOST_IP>`.

## Authentication

Public:

- `POST /api/auth/register`
- `POST /api/auth/login`
- `GET /api/firmware/latest`
- wallpaper routes hiện được đăng ký trước middleware protected và do đó đang public
- `/`, `/healthz`, `/ws`, `/uploads/*`

Protected routes yêu cầu:

```http
Authorization: Bearer <jwt>
```

`GET /api/auth/me` tự áp middleware JWT riêng. Device, theme và OTA management nằm trong protected group.

## REST API

| Method | Path | Auth | Mô tả |
|---|---|---:|---|
| `GET` | `/` | Không | Thông tin service/version |
| `GET` | `/healthz` | Không | `{"status":"ok"}` |
| `POST` | `/api/auth/register` | Không | Tạo user và token |
| `POST` | `/api/auth/login` | Không | Đăng nhập và token |
| `GET` | `/api/auth/me` | Có | Profile hiện tại |
| `GET` | `/api/devices` | Có | Danh sách thiết bị |
| `POST` | `/api/devices` | Có | Tạo thiết bị |
| `GET` | `/api/device/:id` | Có | Chi tiết thiết bị |
| `PUT` | `/api/device/:id` | Có | Cập nhật tên/theme; có thể publish MQTT |
| `DELETE` | `/api/device/:id` | Có | Xóa thiết bị |
| `GET` | `/api/themes` | Có | Danh sách theme |
| `POST` | `/api/themes` | Có | Tạo theme |
| `GET` | `/api/theme/:id` | Có | Chi tiết theme |
| `PUT` | `/api/theme/:id` | Có | Cập nhật theme |
| `DELETE` | `/api/theme/:id` | Có | Xóa theme |
| `GET` | `/api/wallpapers` | Không | Danh sách wallpaper |
| `GET` | `/api/wallpapers/slideshow` | Không | URL slideshow cho firmware/gateway |
| `POST` | `/api/wallpaper` | Không | Upload multipart field `file` |
| `GET` | `/api/wallpaper/:id` | Không | Metadata wallpaper |
| `DELETE` | `/api/wallpaper/:id` | Không | Xóa record/file |
| `GET` | `/api/ota` | Có | Danh sách firmware |
| `POST` | `/api/ota` | Có | Upload `file`, `version`, `notes` |
| `GET` | `/api/ota/latest` | Có | Firmware mới nhất qua API quản trị |
| `GET` | `/api/firmware/latest` | Không | Discovery metadata cho thiết bị |
| `GET` | `/ws` | Không | WebSocket realtime dashboard |

Fiber giới hạn body 64 MB. File được serve từ `/uploads/wallpapers/...` và `/uploads/firmware/...`.

## MQTT

| Topic | Hướng | Vai trò |
|---|---|---|
| `domos/device/{id}/state` | device → server | online, firmware, IP, telemetry |
| `domos/device/{id}/theme` | server → device | cập nhật theme |
| `domos/device/{id}/wallpaper` | server → device | wallpaper command/reserved |
| `domos/device/{id}/audio` | hai chiều | reserved; voice hiện dùng WebSocket riêng |
| `domos/device/{id}/ota` | server → device | OTA per-device/reserved |
| `domos/server/broadcast` | server → all | `ota_available` và broadcast khác |

Backend subscribe `domos/device/+/state`, cập nhật `online/last_seen` trong database và phát sự kiện `device_state` tới dashboard WebSocket.

## Kiểm tra

```powershell
go test .\...
go vet .\...
go build .\...
Invoke-RestMethod http://<HOST_IP>:8081/healthz
```

Log `postgres connection failed ... falling back to local SQLite` là hành vi hợp lệ trong development, không phải lỗi dừng service.

## Production checklist

- Đổi `JWT_SECRET` và password PostgreSQL.
- Bật authentication/TLS cho MQTT.
- Giới hạn CORS thay vì `*`.
- Bảo vệ wallpaper upload/delete nếu không muốn public.
- Xác minh chữ ký firmware trước OTA.
- Dùng object storage nếu chạy nhiều backend instance.
