# DomOS Dashboard

Trong tài liệu, thay `<HOST_IP>` và `<DEVICE_IP>` bằng địa chỉ của môi trường triển khai; không commit địa chỉ thật vào README public.

Dashboard web quản lý board ES3C28P, lịch sử trợ lý, wallpaper, theme, OTA và log. Code hiện dùng Next.js 16, React 19, TypeScript và Tailwind CSS 4.

## Địa chỉ và kết nối

- Local: `http://localhost:3000`
- LAN: `http://<HOST_IP>:3000`
- Go API: `http://<HOST_IP>:8081`
- Voice Gateway: `http://<HOST_IP>:8000`
- ESP32 direct API: `http://<DEVICE_IP>`

## Setup

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

Các biến `NEXT_PUBLIC_*` được đưa vào bundle trình duyệt, vì vậy không đặt secret/API key trong file này.

## Khởi động

Development:

```powershell
npm run dev
```

Production build:

```powershell
npm run build
npm run start
```

Kiểm tra lint:

```powershell
npm run lint
```

Chạy 11 test UI/API/hook bằng Vitest và Testing Library:

```powershell
npm test
```

Chế độ phát triển test liên tục:

```powershell
npm run test:watch
```

`next.config.ts` phải cho phép dev origin `localhost:3000` và `<HOST_IP>:3000`.

## Trang hiện có

| Route | Vai trò |
|---|---|
| `/` | Tổng quan hệ thống |
| `/devices` | Trạng thái thiết bị |
| `/assistant` | Lịch sử hội thoại và tool trace |
| `/wallpaper` | Upload, chọn, sync wallpaper |
| `/themes` | Theme/clock style và gửi cấu hình tới board |
| `/ota` | Quản lý firmware/OTA |
| `/logs` | Log board |
| `/smart-home` | Giao diện điều khiển smart home |
| `/settings` | Hiển thị cấu hình kết nối |
| `/clock` | Trang clock |

## Nguồn dữ liệu

### Go Backend

`lib/api.ts` quản lý JWT trong `localStorage` key `domos_token` và gọi:

- `/api/auth/*`;
- `/api/devices`, `/api/device/:id`;
- `/api/themes`, `/api/theme/:id`;
- `/api/wallpapers`, `/api/wallpaper/:id`;
- `/api/ota`, `/api/ota/latest`;
- `/ws` cho realtime event.

### Voice Gateway

Trang Assistant gọi:

```text
GET http://<HOST_IP>:8000/api/v1/conversations?limit=50
```

Response chứa câu người dùng, câu AI, model, timestamp và danh sách tool call với arguments/result/duration/status.

### ESP32 direct API

Hook `useBoard` poll mỗi 3 giây:

- `GET /api/status`;
- `GET /api/logs`.

Các trang khác gọi trực tiếp:

- `POST /api/launch`;
- `POST /api/clock`;
- `POST /api/wallpaper`;
- `POST /api/upload`;
- `WS /ws` khi dùng realtime log.

Trình duyệt và board phải ở cùng LAN. Firmware hiện bật CORS `*` cho các REST API được đăng ký.

## Wallpaper flow

1. Browser upload file tới Go `POST /api/wallpaper`.
2. Go lưu file/thumbnail trong `uploads/wallpapers`.
3. Dashboard gửi URL wallpaper tới `POST http://<DEVICE_IP>/api/wallpaper`.
4. Firmware tải file hoặc sync danh sách qua proxy Gateway `/api/wallpapers/slideshow`.
5. Ảnh được cache trong LittleFS và decode JPEG vào PSRAM.

## Board telemetry

`useBoard` giữ tối đa 20 điểm telemetry gần nhất và 100 log item trong state trình duyệt. Nhiệt độ hiện là placeholder `42`, chưa phải cảm biến thật.

## Giới hạn cần biết

- `lib/api.ts` có helper `setDeviceBrightness()` gọi `/api/brightness`, nhưng firmware chưa đăng ký endpoint này. Điều chỉnh brightness đang hoạt động qua AI/MCP.
- `uploadWallpaperToDevice()` gọi `/upload`, trong khi handler firmware hiện chỉ trả hướng dẫn quản lý wallpaper; flow chính nên dùng trang `/wallpaper` và `/api/wallpaper`.
- Dashboard OTA gọi board `/api/upload`; firmware hiện acknowledgement request nhưng chưa ghi OTA image.
- Một số API Go cần JWT. Nếu request trả `401`, đăng nhập/đăng ký để `domos_token` được lưu.

## Debug

```powershell
Invoke-WebRequest -UseBasicParsing http://<HOST_IP>:3000
Invoke-RestMethod http://<DEVICE_IP>/api/status
Invoke-RestMethod http://<HOST_IP>:8000/health
Invoke-RestMethod http://<HOST_IP>:8081/healthz
```

Nếu Dashboard mở được nhưng không thấy board, kiểm tra firewall, IP host, AP isolation và browser có truy cập trực tiếp `http://<DEVICE_IP>/api/status` hay không.
