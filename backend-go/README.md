# DomOS Backend (Go)

REST + MQTT + WebSocket backend for the DomOS AI Terminal project (ESP32-S3
based smart desk terminal). Covers device management, theme sync, wallpaper
storage, and OTA firmware distribution — the backend pieces needed by
**Phase 1** (Wallpaper, OTA) and **Phase 2** (Dashboard, MQTT) of the
project roadmap.

## Stack

- [Fiber](https://gofiber.io) — HTTP framework
- [GORM](https://gorm.io) + PostgreSQL — persistence
- JWT — auth
- [Paho MQTT](https://github.com/eclipse/paho.mqtt.golang) — device comms
- Fiber WebSocket — real-time dashboard updates

## Project layout

```
backend-go/
├── cmd/server          entrypoint (main.go)
├── internal
│   ├── auth             users, JWT, register/login/me
│   ├── devices           device CRUD + MQTT theme commands
│   ├── themes             theme CRUD
│   ├── wallpapers         wallpaper upload/serve
│   ├── ota                firmware upload + OTA broadcast
│   ├── mqtt               Paho client, topic constants, state sync
│   └── websocket          real-time hub for the dashboard
├── pkg
│   ├── config             env loading
│   ├── database           GORM connect + AutoMigrate
│   ├── logger             stdout/stderr loggers
│   └── response           JSON envelope helpers
├── migrations             raw SQL, mirrors the GORM schema
├── mosquitto/mosquitto.conf
├── Dockerfile
└── docker-compose.yml     backend + Postgres + Mosquitto
```

## Running locally

### Option A — Docker Compose (recommended)

```bash
docker compose up --build
```

This starts Postgres, Mosquitto, and the backend on `:8080`.

### Option B — Go directly

Requires a local Postgres and MQTT broker (or point `MQTT_BROKER` /
`DATABASE_URL` at remote ones).

```bash
cp .env.example .env
# edit .env as needed
go run ./cmd/server
```

The server auto-runs `AutoMigrate` on boot — no manual migration step
needed for development. The SQL files under `migrations/` are provided for
teams that prefer explicit migrations (e.g. with `golang-migrate`) in
production.

## API

All routes are prefixed `/api` and (aside from `/auth/register` and
`/auth/login`) require `Authorization: Bearer <token>`.

| Method | Path                  | Description                     |
|--------|-----------------------|----------------------------------|
| POST   | `/api/auth/register`  | Create account, returns JWT      |
| POST   | `/api/auth/login`     | Login, returns JWT                |
| GET    | `/api/auth/me`        | Current user profile              |
| GET    | `/api/devices`        | List devices (optional `?owner_id=`) |
| POST   | `/api/devices`        | Register a new device             |
| GET    | `/api/device/:id`     | Get a device                      |
| PUT    | `/api/device/:id`     | Update device (name/theme_id) — theme change publishes MQTT `update_theme` |
| DELETE | `/api/device/:id`     | Delete a device                   |
| GET    | `/api/themes`         | List themes                       |
| POST   | `/api/themes`         | Create theme                      |
| GET    | `/api/theme/:id`      | Get theme                         |
| PUT    | `/api/theme/:id`      | Update theme                      |
| DELETE | `/api/theme/:id`      | Delete theme                      |
| GET    | `/api/wallpapers`     | List wallpapers                   |
| POST   | `/api/wallpaper`      | Upload wallpaper (multipart `file`) |
| GET    | `/api/wallpaper/:id`  | Get wallpaper metadata            |
| DELETE | `/api/wallpaper/:id`  | Delete wallpaper record           |
| GET    | `/api/ota`            | List firmware versions            |
| POST   | `/api/ota`             | Upload firmware (multipart `file`, `version`, `notes`) — broadcasts `ota_available` over MQTT |
| GET    | `/api/ota/latest`     | Latest firmware version            |
| GET    | `/api/firmware/latest`| Public device firmware discovery metadata |
| GET    | `/ws`                 | WebSocket upgrade — real-time device state, OTA events |
| GET    | `/healthz`            | Health check                      |

Uploaded files are served statically from `/uploads/wallpapers/...` and
`/uploads/firmware/...`.

## MQTT topics

```
domos/device/{id}/state       device -> server   {"online":true,"firmware":"1.2.0","ip":"..."}
domos/device/{id}/theme       server -> device    {"cmd":"update_theme","theme":"cyberpunk"}
domos/device/{id}/wallpaper   server -> device    (reserved for future use)
domos/device/{id}/audio       bidirectional        (reserved for AI voice service)
domos/device/{id}/ota         server -> device     (reserved for per-device OTA push)
domos/server/broadcast        server -> all         {"cmd":"ota_available","version":"1.2.0","url":"..."}
```

The backend subscribes to `domos/device/+/state` on boot; incoming state
messages update the device's `online`/`last_seen` fields in Postgres and are
re-broadcast to connected dashboard WebSocket clients as a `device_state`
event.

## Notes on dependencies

This environment's build was verified with `go build ./...` and `go vet
./...` — both pass cleanly, and a real binary (`domos-server`) was produced
and runs. Because this sandbox's network allowlist doesn't include
`proxy.golang.org` or `gopkg.in`, `go.mod` routes a handful of transitive
dependencies (`golang.org/x/*`, `gorm.io/*`, `gopkg.in/yaml.v3`,
`gopkg.in/check.v1`) through their GitHub source mirrors via `replace`
directives instead of the default module proxy. This is functionally
equivalent and, if anything, more portable (no dependency on
`proxy.golang.org` being reachable) — but feel free to run `go mod tidy`
with your normal `GOPROXY` and remove the `replace` block if you'd rather
pull straight from the canonical `golang.org/x/*` and `gorm.io/*` module
paths.

## Security notes before production

- Set a strong, random `JWT_SECRET`.
- Mosquitto config here allows anonymous connections for local dev —
  add username/password or TLS client certs before exposing it.
- Add rate limiting / request size limits per-route if wallpaper/firmware
  uploads are exposed publicly.
- Swap local-disk wallpaper/firmware storage for S3-compatible object
  storage for multi-instance deployments (the roadmap calls for this in the
  Wallpaper Service section).
