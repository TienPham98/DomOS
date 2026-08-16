// Board data store — empty by default until real ESP32-S3 boards connect
import type { Device, Wallpaper, FirmwareVersion, Theme } from "./api";

export const demoDevices: Device[] = [];

export const demoWallpapers: Wallpaper[] = [];

export const demoFirmware: FirmwareVersion[] = [];

export const demoThemes: Theme[] = [];

export function generateTimeSeriesData() {
  return [];
}

export const demoLogs: { ts: string; level: string; source: string; msg: string }[] = [];
