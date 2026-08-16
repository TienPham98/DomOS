// DomOS API client — connects to Go backend
const API_BASE = process.env.NEXT_PUBLIC_API_URL || "http://localhost:8081";

// ──── Types ────────────────────────────────────────────────

export interface Device {
  id: string;
  name: string;
  mac: string;
  ip: string;
  firmware: string;
  theme_id?: string;
  online: boolean;
  last_seen: string;
  rssi?: number;
  storage_used?: number;
  storage_total?: number;
}

export interface Theme {
  id: string;
  name: string;
  primary_color: string;
  bg_color: string;
  font: string;
  clock_style: string;
}

export interface Wallpaper {
  id: string;
  filename: string;
  url: string;
  thumbnail_url?: string;
  width: number;
  height: number;
  size: number;
  created_at: string;
}


export interface FirmwareVersion {
  id: string;
  version: string;
  notes: string;
  url: string;
  size: number;
  created_at: string;
}

export interface DeviceStatus {
  wifi_ssid: string;
  wifi_rssi: number;
  ip: string;
  firmware: string;
  uptime: number;
  free_heap: number;
  littlefs_used: number;
  littlefs_total: number;
  brightness: number;
  current_app: string;
}

export interface ApiResponse<T> {
  success: boolean;
  data: T;
  error?: string;
}

// ──── REST Client ──────────────────────────────────────────

let authToken: string | null = null;

export function setAuthToken(token: string) {
  authToken = token;
  if (typeof window !== "undefined") {
    localStorage.setItem("domos_token", token);
  }
}

export function getAuthToken(): string | null {
  if (authToken) return authToken;
  if (typeof window !== "undefined") {
    authToken = localStorage.getItem("domos_token");
  }
  return authToken;
}

async function request<T>(path: string, options: RequestInit = {}): Promise<T> {
  const token = getAuthToken();
  const headers: Record<string, string> = {
    ...(options.headers as Record<string, string>),
  };
  if (token) headers["Authorization"] = `Bearer ${token}`;
  if (!(options.body instanceof FormData)) {
    headers["Content-Type"] = "application/json";
  }

  const res = await fetch(`${API_BASE}${path}`, { ...options, headers });

  if (!res.ok) {
    const err = await res.json().catch(() => ({ error: res.statusText }));
    throw new Error(err.error || `API error ${res.status}`);
  }

  const payload: unknown = await res.json();
  if (
    payload !== null &&
    typeof payload === "object" &&
    "success" in payload &&
    "data" in payload
  ) {
    return (payload as ApiResponse<T>).data;
  }
  return payload as T;
}

// ──── Auth ─────────────────────────────────────────────────

export async function login(email: string, password: string) {
  const data = await request<{ token: string }>("/api/auth/login", {
    method: "POST",
    body: JSON.stringify({ email, password }),
  });
  setAuthToken(data.token);
  return data;
}

export async function register(email: string, password: string) {
  const data = await request<{ token: string }>("/api/auth/register", {
    method: "POST",
    body: JSON.stringify({ email, password }),
  });
  setAuthToken(data.token);
  return data;
}

// ──── Devices ──────────────────────────────────────────────

export const fetchDevices = () => request<Device[]>("/api/devices");
export const fetchDevice = (id: string) => request<Device>(`/api/device/${id}`);
export const updateDevice = (id: string, data: Partial<Device>) =>
  request<Device>(`/api/device/${id}`, {
    method: "PUT",
    body: JSON.stringify(data),
  });

// ──── Themes ───────────────────────────────────────────────

export const fetchThemes = () => request<Theme[]>("/api/themes");
export const createTheme = (theme: Partial<Theme>) =>
  request<Theme>("/api/themes", {
    method: "POST",
    body: JSON.stringify(theme),
  });

// ──── Wallpapers ───────────────────────────────────────────

export const fetchWallpapers = () => request<Wallpaper[]>("/api/wallpapers");
export const uploadWallpaper = (file: File) => {
  const form = new FormData();
  form.append("file", file);
  return request<Wallpaper>("/api/wallpaper", { method: "POST", body: form });
};
export const deleteWallpaper = (id: string) =>
  request("/api/wallpaper/" + id, { method: "DELETE" });

// ──── OTA ──────────────────────────────────────────────────

export const fetchFirmwareVersions = () => request<FirmwareVersion[]>("/api/ota");
export const fetchLatestFirmware = () => request<FirmwareVersion>("/api/ota/latest");
export const uploadFirmware = (file: File, version: string, notes: string) => {
  const form = new FormData();
  form.append("file", file);
  form.append("version", version);
  form.append("notes", notes);
  return request<FirmwareVersion>("/api/ota", { method: "POST", body: form });
};

// ──── Device Direct API ────────────────────────────────────
// These hit the ESP32 HTTP server directly

export const fetchDeviceStatus = (ip: string) =>
  fetch(`http://${ip}/api/status`).then((r) => r.json() as Promise<DeviceStatus>);

export const uploadWallpaperToDevice = (ip: string, file: File) =>
  file.arrayBuffer().then((buf) =>
    fetch(`http://${ip}/upload`, {
      method: "POST",
      headers: { "Content-Type": "image/jpeg" },
      body: buf,
    })
  );

export const setDeviceBrightness = (ip: string, brightness: number) =>
  fetch(`http://${ip}/api/brightness`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ brightness }),
  });

// ──── WebSocket ────────────────────────────────────────────

export type WsMessage =
  | { type: "device_state"; device_id: string; payload: Partial<Device> }
  | { type: "ota_available"; version: string; url: string }
  | { type: "notification"; message: string };

export function connectWebSocket(
  onMessage: (msg: WsMessage) => void,
  onClose?: () => void
): WebSocket | null {
  if (typeof window === "undefined") return null;

  const wsUrl = API_BASE.replace(/^http/, "ws") + "/ws";
  const ws = new WebSocket(wsUrl);

  ws.onmessage = (ev) => {
    try {
      const msg = JSON.parse(ev.data) as WsMessage;
      onMessage(msg);
    } catch {
      /* ignore malformed */
    }
  };

  ws.onclose = () => onClose?.();
  ws.onerror = () => ws.close();

  return ws;
}
