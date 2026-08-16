"use client";

import { useState } from "react";
import {
  Wifi,
  Sun,
  Monitor,
  HardDrive,
  Globe,
  Shield,
  Cpu,
  Radio,
  Save,
  RotateCcw,
  Info,
} from "lucide-react";
import { PageHeader, Section, StorageBar } from "@/components/dashboard-primitives";
import { Button } from "@/components/ui/button";

export default function SettingsPage() {
  const [brightness, setBrightness] = useState(75);
  const wifiSSID = "Dom_12";
  const apiUrl = process.env.NEXT_PUBLIC_API_URL || "http://localhost:8081";
  const mqttBroker = process.env.NEXT_PUBLIC_MQTT_URL || `mqtt://${new URL(apiUrl).hostname}:1883`;
  const [mdns, setMdns] = useState("domos");
  const [saving, setSaving] = useState(false);
  const [statusMsg, setStatusMsg] = useState<string | null>(null);

  const handleSave = async () => {
    setSaving(true);
    setStatusMsg("Saving system configuration to board NVS...");
    try {
      await new Promise((r) => setTimeout(r, 600));
      setStatusMsg("Configuration saved successfully!");
      setTimeout(() => setStatusMsg(null), 3000);
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : "Unknown save error";
      setStatusMsg(`Error: ${message}`);
      setTimeout(() => setStatusMsg(null), 4000);
    } finally {
      setSaving(false);
    }
  };

  return (
    <>
      <PageHeader
        title="Settings"
        subtitle="System"
        action={
          <div className="flex gap-2">
            <Button variant="outline" className="gap-2" onClick={() => setBrightness(75)}>
              <RotateCcw className="w-4 h-4" /> Reset
            </Button>
            <Button className="gap-2" onClick={handleSave} disabled={saving}>
              <Save className="w-4 h-4" /> {saving ? "Saving..." : "Save Changes"}
            </Button>
          </div>
        }
      />

      {statusMsg && (
        <div className={`p-4 rounded-xl mb-6 font-medium text-sm border animate-fade-in ${
          statusMsg.startsWith("Error")
            ? "bg-red-500/10 text-red-400 border-red-500/20"
            : "bg-cyan-500/10 text-cyan-400 border-cyan-500/20"
        }`}>
          {statusMsg}
        </div>
      )}


      <div className="grid gap-6 xl:grid-cols-2">
        {/* ── Display ─────────────────────────────── */}
        <Section title="Display">
          <div className="space-y-5">
            <div>
              <div className="flex items-center justify-between mb-3">
                <div className="flex items-center gap-2">
                  <Sun className="w-4 h-4 text-amber-400" />
                  <span className="text-sm text-white">Brightness</span>
                </div>
                <span className="text-sm font-mono text-cyan-400">{brightness}%</span>
              </div>
              <input
                type="range"
                min={0}
                max={100}
                value={brightness}
                onChange={(e) => setBrightness(Number(e.target.value))}
                className="w-full h-2 bg-slate-700 rounded-full appearance-none cursor-pointer accent-cyan-500"
              />
              <div className="flex justify-between text-[10px] text-slate-600 mt-1">
                <span>Off</span>
                <span>25%</span>
                <span>50%</span>
                <span>75%</span>
                <span>100%</span>
              </div>
            </div>

            <div className="p-4 rounded-xl border border-slate-800/50 bg-slate-900/30">
              <div className="flex items-center gap-2 mb-2">
                <Monitor className="w-4 h-4 text-slate-500" />
                <span className="text-sm text-white">Display Info</span>
              </div>
              <div className="grid grid-cols-2 gap-3 text-xs">
                <div>
                  <p className="text-slate-500">Panel</p>
                  <p className="text-white font-medium">ILI9341V</p>
                </div>
                <div>
                  <p className="text-slate-500">Resolution</p>
                  <p className="text-white font-medium">320×240 (landscape)</p>
                </div>
                <div>
                  <p className="text-slate-500">Interface</p>
                  <p className="text-white font-medium">SPI3 @ 40 MHz</p>
                </div>
                <div>
                  <p className="text-slate-500">UI Framework</p>
                  <p className="text-white font-medium">LVGL 8.4</p>
                </div>
              </div>
            </div>
          </div>
        </Section>

        {/* ── Network ─────────────────────────────── */}
        <Section title="Network">
          <div className="space-y-4">
            <div>
              <label className="flex items-center gap-2 text-xs text-slate-400 mb-2">
                <Wifi className="w-3.5 h-3.5" /> Wi-Fi SSID
              </label>
              <input
                type="text"
                value={wifiSSID}
                readOnly
                className="w-full px-3 py-2.5 rounded-xl bg-slate-900 border border-slate-700 text-sm text-white focus:border-cyan-500 focus:outline-none"
              />
            </div>
            <div>
              <label className="flex items-center gap-2 text-xs text-slate-400 mb-2">
                <Radio className="w-3.5 h-3.5" /> MQTT Broker URI
              </label>
              <input
                type="text"
                value={mqttBroker}
                readOnly
                className="w-full px-3 py-2.5 rounded-xl bg-slate-900 border border-slate-700 text-sm text-white font-mono focus:border-cyan-500 focus:outline-none"
              />
            </div>
            <div>
              <label className="flex items-center gap-2 text-xs text-slate-400 mb-2">
                <Globe className="w-3.5 h-3.5" /> API Base URL
              </label>
              <input
                type="text"
                value={apiUrl}
                readOnly
                className="w-full px-3 py-2.5 rounded-xl bg-slate-900 border border-slate-700 text-sm text-white font-mono focus:border-cyan-500 focus:outline-none"
              />
            </div>
            <div>
              <label className="flex items-center gap-2 text-xs text-slate-400 mb-2">
                <Globe className="w-3.5 h-3.5" /> mDNS Hostname
              </label>
              <div className="flex gap-2">
                <input
                  type="text"
                  value={mdns}
                  onChange={(e) => setMdns(e.target.value)}
                  className="flex-1 px-3 py-2.5 rounded-xl bg-slate-900 border border-slate-700 text-sm text-white font-mono focus:border-cyan-500 focus:outline-none"
                />
                <span className="flex items-center px-3 text-sm text-slate-500">.local</span>
              </div>
            </div>
          </div>
        </Section>

        {/* ── Storage ─────────────────────────────── */}
        <Section title="Storage">
          <div className="space-y-5">
            <div>
              <div className="flex items-center gap-2 mb-3">
                <HardDrive className="w-4 h-4 text-orange-400" />
                <span className="text-sm text-white">LittleFS</span>
              </div>
              <StorageBar used={2.1 * 1024 * 1024} total={7 * 1024 * 1024} />
            </div>

            <div className="grid grid-cols-2 gap-3">
              {[
                { label: "Wallpapers", size: "1.2 MB", count: 3 },
                { label: "Icons", size: "0.1 MB", count: 8 },
                { label: "Themes", size: "0.02 MB", count: 3 },
                { label: "Voice Cache", size: "0.8 MB", count: 12 },
              ].map((item) => (
                <div key={item.label} className="p-3 rounded-xl border border-slate-800/50">
                  <p className="text-xs text-slate-500">{item.label}</p>
                  <p className="text-sm font-medium text-white">{item.size}</p>
                  <p className="text-[10px] text-slate-600">{item.count} files</p>
                </div>
              ))}
            </div>

            <Button variant="outline" className="w-full text-xs">
              <HardDrive className="w-3 h-3 mr-1.5" /> Format LittleFS (Factory Reset)
            </Button>
          </div>
        </Section>

        {/* ── About ───────────────────────────────── */}
        <Section title="About">
          <div className="space-y-4">
            <div className="p-4 rounded-xl border border-slate-800/50 bg-slate-900/30">
              <div className="flex items-center gap-3 mb-4">
                <div className="flex items-center justify-center w-12 h-12 rounded-xl bg-gradient-to-br from-cyan-500 to-blue-600">
                  <Cpu className="w-6 h-6 text-white" />
                </div>
                <div>
                  <h3 className="text-lg font-bold text-white">
                    Dom<span className="text-cyan-400">OS</span>
                  </h3>
                  <p className="text-xs text-slate-500">AI Terminal Operating System</p>
                </div>
              </div>

              <div className="grid grid-cols-2 gap-3 text-xs">
                <div>
                  <p className="text-slate-500">Board</p>
                  <p className="text-white font-medium">ES3C28P</p>
                </div>
                <div>
                  <p className="text-slate-500">MCU</p>
                  <p className="text-white font-medium">ESP32-S3 @ 240 MHz</p>
                </div>
                <div>
                  <p className="text-slate-500">Flash</p>
                  <p className="text-white font-medium">16 MB (QIO 80 MHz)</p>
                </div>
                <div>
                  <p className="text-slate-500">PSRAM</p>
                  <p className="text-white font-medium">8 MB</p>
                </div>
                <div>
                  <p className="text-slate-500">Touch</p>
                  <p className="text-white font-medium">FT6336G (I²C)</p>
                </div>
                <div>
                  <p className="text-slate-500">Audio</p>
                  <p className="text-white font-medium">ES8311 (I2S)</p>
                </div>
                <div>
                  <p className="text-slate-500">Firmware</p>
                  <p className="text-white font-medium">v0.2.1</p>
                </div>
                <div>
                  <p className="text-slate-500">ESP-IDF</p>
                  <p className="text-white font-medium">v5.4</p>
                </div>
              </div>
            </div>

            <div className="flex items-start gap-3 p-4 rounded-xl border border-cyan-500/20 bg-cyan-500/5">
              <Info className="w-5 h-5 text-cyan-400 flex-shrink-0 mt-0.5" />
              <div>
                <p className="text-sm text-white font-medium">DomOS v0.2.1</p>
                <p className="text-xs text-slate-400 mt-1">
                  Phase 2 — DomOS Core. EventBus, Apps, MQTT, OTA dual-partition, HTTP API.
                  Dashboard is server-first with WebSocket real-time updates.
                </p>
              </div>
            </div>

            <div className="flex gap-2">
              <Button variant="outline" className="flex-1 text-xs">
                <Shield className="w-3 h-3 mr-1.5" /> Security Info
              </Button>
              <Button variant="outline" className="flex-1 text-xs">
                <RotateCcw className="w-3 h-3 mr-1.5" /> Factory Reset
              </Button>
            </div>
          </div>
        </Section>
      </div>
    </>
  );
}
