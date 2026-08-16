# Wallpaper — Kiến trúc và troubleshooting

## Luồng hiện tại

```text
Dashboard :3000
  -> POST Go /api/wallpaper :8081
  -> file + thumbnail trong backend-go/uploads/wallpapers
  -> POST ESP32 /api/wallpaper với URL/name

ESP32
  -> GET Gateway /api/wallpapers/slideshow :8000
  -> Gateway proxy Go /api/wallpapers/slideshow :8081
  -> download file qua Gateway /uploads/wallpapers/{name}
  -> cache /littlefs/wallpapers/wp_0.jpg ... wp_9.jpg
  -> TJpgDec vào PSRAM RGB565 320×240
  -> lv_img_set_src()
```

## Vì sao dùng PSRAM + LVGL image

Một frame RGB565 320×240 cần:

```text
320 × 240 × 2 = 153600 byte
```

Board có 8 MB PSRAM nên firmware cấp `s_wallpaper_buf` bằng `MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT`, decode JPEG vào buffer và giao render cho LVGL.

Không vẽ JPEG trực tiếp lên SPI panel trong khi LVGL vẫn chạy. Nếu direct draw và LVGL cùng sở hữu panel:

- LVGL có thể repaint background lên ảnh vừa vẽ;
- hai task tranh chấp SPI;
- ảnh nhấp nháy/đen hoặc UI overlay sai.

## Empty state

Khi chưa có file/URL hợp lệ, Wallpaper app phải hiện hướng dẫn truy cập:

```text
http://<HOST_IP>:3000/wallpaper
```

Không coi màn hình đen là empty state.

## Endpoint liên quan

### Go Backend

- `GET /api/wallpapers`
- `GET /api/wallpapers/slideshow`
- `POST /api/wallpaper`
- `GET /api/wallpaper/:id`
- `DELETE /api/wallpaper/:id`
- `/uploads/wallpapers/*`

### Python Gateway

- `GET /api/wallpapers/slideshow`
- `GET /uploads/wallpapers/{filename}`

Gateway proxy giúp URL trả cho board nằm trên port 8000 và forward nội bộ sang Go port 8081.

### ESP32

- `POST /api/wallpaper`: chọn URL/name hoặc `{"action":"sync"}`.
- `POST /api/clock`: có thể kèm wallpaper URL cho Clock.
- `/upload`: hiện chỉ acknowledgement, không lưu raw JPEG.

## Cấu hình Dashboard

```dotenv
NEXT_PUBLIC_API_URL=http://<HOST_IP>:8081
NEXT_PUBLIC_AI_GATEWAY_URL=http://<HOST_IP>:8000
NEXT_PUBLIC_DEVICE_IP=<DEVICE_IP>
```

ESP32 không truy cập được `localhost`; mọi URL gửi xuống board phải dùng `<HOST_IP>`.

## Chẩn đoán theo triệu chứng

### Dashboard không thấy danh sách ảnh

```powershell
Invoke-RestMethod http://<HOST_IP>:8081/api/wallpapers
```

Kiểm tra Go backend, database và `UPLOAD_DIR`.

### Board không sync slideshow

```powershell
Invoke-RestMethod http://<HOST_IP>:8000/api/wallpapers/slideshow
```

Nếu trả `502`, Gateway không gọi được Go tại `127.0.0.1:8081`. Cả hai service phải chạy trên cùng host theo cấu hình hiện tại.

### URL có nhưng file 404

Kiểm tra:

```powershell
Invoke-WebRequest -UseBasicParsing `
  http://<HOST_IP>:8081/uploads/wallpapers/<filename>
```

Đảm bảo file tồn tại trong `backend-go/uploads/wallpapers` và metadata URL đúng.

### Board tải được nhưng màn hình đen

1. Xem `GET http://<DEVICE_IP>/api/logs`.
2. Tìm `jd_prepare`, `jd_decomp`, `File not found` hoặc allocation failure.
3. Xác minh JPEG hợp lệ và kích thước phù hợp.
4. Kiểm tra `free_psram` trong `/api/status`.
5. Không thay `lv_img_set_src` bằng direct SPI draw.

### Ảnh sai màu

- LCD dùng RGB565 với byte swap bật.
- Panel order BGR và inversion bật.
- Không đổi riêng `CONFIG_LV_COLOR_16_SWAP` nếu chưa test toàn bộ UI.

### Touch làm ảnh biến mất

Đây thường là dấu hiệu code mới đang direct draw ngoài LVGL hoặc object image bị xóa/ẩn khi controls thay đổi. Wallpaper image phải là LVGL object sống cùng screen; controls chỉ overlay trên image.

## Sync thủ công

```powershell
Invoke-RestMethod -Method Post `
  -Uri http://<DEVICE_IP>/api/wallpaper `
  -ContentType application/json `
  -Body '{"action":"sync"}'
```

Chọn URL cụ thể:

```powershell
Invoke-RestMethod -Method Post `
  -Uri http://<DEVICE_IP>/api/wallpaper `
  -ContentType application/json `
  -Body '{"url":"http://<HOST_IP>:8000/uploads/wallpapers/example.jpg","name":"example.jpg"}'
```

## Quy tắc phòng tái phát

1. LVGL là chủ sở hữu duy nhất của framebuffer/panel khi UI task đang chạy.
2. Buffer ảnh lớn nằm trong PSRAM; DMA buffer display nằm trong internal DMA RAM.
3. Luôn có empty/error state.
4. Giữ URL host cố định `<HOST_IP>`; không dùng `localhost` cho board.
5. Validate HTTP status, content length và JPEG decode result trước khi đổi active image.
6. Không ghi file lớn hoặc response buffer lớn lên HTTP task stack.
7. Giữ cache slot giới hạn để tránh lấp LittleFS.

## File nguồn

- `firmware/main/app/launcher/app_manager.cpp`
- `firmware/main/services/filesystem/upload_server.cpp`
- `backend-go/internal/wallpapers/handlers.go`
- `backend-python/main.py`
- `dashboard-next/app/wallpaper/page.tsx`
