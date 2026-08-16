-- 0001_init.up.sql
-- Matches the GORM AutoMigrate schema in pkg/database. Kept for teams that
-- prefer explicit SQL migrations (e.g. via golang-migrate) over AutoMigrate
-- in production.

CREATE TABLE IF NOT EXISTS users (
    id            TEXT PRIMARY KEY,
    email         TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    name          TEXT,
    role          VARCHAR(20) DEFAULT 'user',
    created_at    TIMESTAMPTZ DEFAULT now(),
    updated_at    TIMESTAMPTZ DEFAULT now()
);

CREATE TABLE IF NOT EXISTS devices (
    id         TEXT PRIMARY KEY,
    owner_id   TEXT,
    name       TEXT,
    firmware   TEXT,
    online     BOOLEAN DEFAULT false,
    last_seen  TIMESTAMPTZ,
    theme_id   TEXT,
    ip_address TEXT,
    created_at TIMESTAMPTZ DEFAULT now(),
    updated_at TIMESTAMPTZ DEFAULT now()
);
CREATE INDEX IF NOT EXISTS idx_devices_owner_id ON devices (owner_id);

CREATE TABLE IF NOT EXISTS themes (
    id             TEXT PRIMARY KEY,
    name           TEXT,
    wallpaper_url  TEXT,
    primary_color  TEXT,
    created_at     TIMESTAMPTZ DEFAULT now(),
    updated_at     TIMESTAMPTZ DEFAULT now()
);

CREATE TABLE IF NOT EXISTS wallpapers (
    id         TEXT PRIMARY KEY,
    name       TEXT,
    url        TEXT,
    size_bytes BIGINT,
    created_at TIMESTAMPTZ DEFAULT now()
);

CREATE TABLE IF NOT EXISTS firmware_versions (
    id         TEXT PRIMARY KEY,
    version    TEXT UNIQUE,
    url        TEXT,
    size_bytes BIGINT,
    notes      TEXT,
    created_at TIMESTAMPTZ DEFAULT now()
);
