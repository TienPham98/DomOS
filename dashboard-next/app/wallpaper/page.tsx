"use client";

import { useState, useRef, useCallback, useEffect } from "react";
import { Image, Trash2, Send, CheckCircle2, CloudUpload, Play, Square, Timer } from "lucide-react";
import { PageHeader, EmptyState } from "@/components/dashboard-primitives";
import { Button } from "@/components/ui/button";
import { demoWallpapers } from "@/lib/demo-data";
import type { Wallpaper } from "@/lib/api";

const getBackendUrl = () => {
  if (typeof window !== "undefined" && window.location.hostname) {
    return `http://${window.location.hostname}:8080`;
  }
  return "http://localhost:8080";
};

export default function WallpaperPage() {
  const [wallpapers, setWallpapers] = useState<Wallpaper[]>(demoWallpapers);
  const [selected, setSelected] = useState<string | null>(null);
  const [dragging, setDragging] = useState(false);
  const [pushing, setPushing] = useState(false);
  const [slideshowRunning, setSlideshowRunning] = useState(false);
  const [intervalSec, setIntervalSec] = useState<number>(30);
  const [pushStatus, setPushStatus] = useState<string | null>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  const [deviceIp, setDeviceIp] = useState(() => {
    if (typeof window !== "undefined") {
      return localStorage.getItem("domos_device_ip") || "192.168.0.106";
    }
    return "192.168.0.106";
  });


  const [serverIp, setServerIp] = useState(() => {
    if (typeof window !== "undefined") {
      return localStorage.getItem("domos_server_ip") || (window.location.hostname !== "localhost" && window.location.hostname !== "127.0.0.1" ? window.location.hostname : "");
    }
    return "";
  });


  const handleIpChange = (newIp: string) => {
    setDeviceIp(newIp);
    if (typeof window !== "undefined") {
      localStorage.setItem("domos_device_ip", newIp);
    }
  };

  const handleServerIpChange = (newIp: string) => {
    setServerIp(newIp);
    if (typeof window !== "undefined") {
      localStorage.setItem("domos_server_ip", newIp);
    }
  };


  const fetchWallpapersFromBackend = async () => {
    const backendUrl = getBackendUrl();
    try {
      const res = await fetch(`${backendUrl}/api/wallpapers`);
      if (res.ok) {
        const json = await res.json();
        if (json.data && Array.isArray(json.data)) {
          const list: Wallpaper[] = json.data.map((item: any) => ({
            id: item.id,
            filename: item.name,
            url: item.url.startsWith("http") ? item.url : `${backendUrl}${item.url}`,
            thumbnail_url: item.thumbnail_url ? (item.thumbnail_url.startsWith("http") ? item.thumbnail_url : `${backendUrl}${item.thumbnail_url}`) : undefined,
            width: item.width || 320,
            height: item.height || 240,
            size: item.size_bytes || 0,
            created_at: item.created_at,
          }));
          setWallpapers(list);
        }
      }
    } catch (e) {
      console.warn("Backend offline, using fallback list:", e);
    }
  };

  useEffect(() => {
    fetchWallpapersFromBackend();
  }, []);

  const handlePushToDevice = async (wp?: Wallpaper) => {
    const target = wp || wallpapers.find((w) => w.id === selected);
    if (!target) return;

    setPushing(true);
    const targetHost = deviceIp.includes(":") ? deviceIp : `${deviceIp}:80`;
    
    // Resolve PC Server IP so ESP32 can download wallpaper over local Wi-Fi network
    let hostForDevice = serverIp;
    if (!hostForDevice && typeof window !== "undefined" && window.location.hostname !== "localhost" && window.location.hostname !== "127.0.0.1") {
      hostForDevice = window.location.hostname;
    }
    if (!hostForDevice) {
      // Fallback to subnet of deviceIp if possible (e.g. if deviceIp is 192.168.0.106)
      hostForDevice = "192.168.0.105";
    }

    const playableUrl = target.url.replace(/localhost|127\.0\.0\.1/g, hostForDevice);

    setPushStatus(`Sending 320x240 JPEG stream URL (${playableUrl}) to ${targetHost}...`);
    try {
      const res = await fetch(`http://${targetHost}/api/wallpaper`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ style: "wallpaper", wallpaper_url: playableUrl, name: target.filename }),
      });

      if (!res.ok) {
        await fetch(`http://${targetHost}/api/clock`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ style: "wallpaper", wallpaper_url: playableUrl, name: target.filename }),
        });
      }

      setPushStatus(`Wallpaper '${target.filename}' active on ${targetHost} (Zero RGB RAM, JPEG stream)!`);
      setTimeout(() => setPushStatus(null), 4000);
    } catch (err: any) {
      const msg = err.message === "Failed to fetch"
        ? "ESP32 IP unreachable. Please check board Wi-Fi connection and update 'Device IP' input above."
        : err.message;
      setPushStatus(`Device Sync Error (${targetHost}): ${msg}`);
      setTimeout(() => setPushStatus(null), 7000);
    } finally {
      setPushing(false);
    }
  };

  const handleToggleSlideshow = async () => {
    const targetHost = deviceIp.includes(":") ? deviceIp : `${deviceIp}:80`;
    const nextState = !slideshowRunning;
    setSlideshowRunning(nextState);

    setPushStatus(nextState ? `Starting ESP32 3-slot preloader slideshow (${intervalSec}s interval)...` : "Stopping slideshow...");
    try {
      await fetch(`http://${targetHost}/api/slideshow`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ enabled: nextState, interval: intervalSec }),
      });
      setPushStatus(nextState ? `Slideshow active on ${targetHost} (${intervalSec}s preloader cycle)` : `Slideshow stopped on ${targetHost}`);
      setTimeout(() => setPushStatus(null), 4000);
    } catch (err: any) {
      const msg = err.message === "Failed to fetch"
        ? "ESP32 IP unreachable. Please check board Wi-Fi connection and update 'Device IP' input above."
        : err.message;
      setPushStatus(`Slideshow Command Error (${targetHost}): ${msg}`);
      setTimeout(() => setPushStatus(null), 7000);
    }
  };

  const handleFiles = useCallback(async (files: FileList | null) => {
    if (!files) return;
    const fileList = Array.from(files).filter((f) => f.type.startsWith("image/"));
    for (const file of fileList) {
      try {
        const formData = new FormData();
        formData.append("file", file);
        const res = await fetch(`${getBackendUrl()}/api/wallpaper`, {
          method: "POST",
          body: formData,
        });
        if (res.ok) {
          setPushStatus(`Backend auto-resized '${file.name}' to 320x240 JPEG Q85 + thumbnail!`);
          setTimeout(() => setPushStatus(null), 3500);
          fetchWallpapersFromBackend();
        } else {
          const newWp: Wallpaper = {
            id: `wp-${Date.now()}-${Math.random().toString(36).slice(2, 6)}`,
            filename: file.name,
            url: URL.createObjectURL(file),
            width: 320,
            height: 240,
            size: file.size,
            created_at: new Date().toISOString(),
          };
          setWallpapers((prev) => [newWp, ...prev]);
        }
      } catch (err) {
        const newWp: Wallpaper = {
          id: `wp-${Date.now()}-${Math.random().toString(36).slice(2, 6)}`,
          filename: file.name,
          url: URL.createObjectURL(file),
          width: 320,
          height: 240,
          size: file.size,
          created_at: new Date().toISOString(),
        };
        setWallpapers((prev) => [newWp, ...prev]);
      }
    }
  }, []);

  const handleDelete = async (id: string) => {
    try {
      await fetch(`${getBackendUrl()}/api/wallpaper/${id}`, { method: "DELETE" });
    } catch (e) {
      console.warn("Delete backend failed:", e);
    }
    setWallpapers((prev) => prev.filter((w) => w.id !== id));
    if (selected === id) setSelected(null);
  };

  return (
    <>
      <PageHeader
        title="Wallpaper & Slideshow"
        subtitle="Go Backend resizes to 320x240 JPEG Q85 • ESP32 3-Slot LittleFS Flash Preloader"
        badge={`${wallpapers.length} images`}
        action={
          <div className="flex flex-wrap items-center gap-2">
            <div className="flex items-center gap-1 bg-slate-900 border border-slate-700 px-2.5 py-1.5 rounded-lg text-xs">
              <span className="text-slate-400 font-medium">Device:</span>
              <input
                type="text"
                value={deviceIp}
                onChange={(e) => handleIpChange(e.target.value)}
                placeholder="Device IP"
                className="bg-transparent text-white font-mono focus:outline-none w-28 text-xs"
                title="Target ESP32 Device IP"
              />
            </div>
            <div className="flex items-center gap-1 bg-slate-900 border border-slate-700 px-2.5 py-1.5 rounded-lg text-xs">
              <span className="text-slate-400 font-medium">Server IP:</span>
              <input
                type="text"
                value={serverIp}
                onChange={(e) => handleServerIpChange(e.target.value)}
                placeholder="PC LAN IP (192.168.x.x)"
                className="bg-transparent text-white font-mono focus:outline-none w-36 text-xs"
                title="Your PC LAN IP so ESP32 can download images over Wi-Fi"
              />
            </div>
            {selected && (
              <Button className="gap-2" onClick={() => handlePushToDevice()} disabled={pushing}>
                <Send className="w-4 h-4" /> {pushing ? "Applying..." : "Set on Device"}
              </Button>
            )}
          </div>

        }
      />




      {pushStatus && (
        <div className={`p-4 rounded-xl mb-6 font-medium text-sm border animate-fade-in ${
          pushStatus.startsWith("Error") || pushStatus.startsWith("Upload Error") || pushStatus.startsWith("Device Sync Error") || pushStatus.startsWith("Slideshow Command Error")
            ? "bg-red-500/10 text-red-400 border-red-500/20"
            : "bg-cyan-500/10 text-cyan-400 border-cyan-500/20"
        }`}>
          {pushStatus}
        </div>
      )}

      {/* Slideshow Control Panel */}
      <div className="mb-8 p-4 rounded-2xl bg-slate-900/60 border border-slate-800 flex flex-wrap items-center justify-between gap-4 animate-fade-in">
        <div className="flex items-center gap-3">
          <div className="w-10 h-10 rounded-xl bg-cyan-500/10 border border-cyan-500/20 text-cyan-400 flex items-center justify-center">
            <Timer className="w-5 h-5" />
          </div>
          <div>
            <h4 className="text-sm font-semibold text-white">ESP32 Preloaded Slideshow</h4>
            <p className="text-xs text-slate-400">
              Uses 3 Flash slots (current, next, next2). Background prefetching eliminates HTTP delay.
            </p>
          </div>
        </div>

        <div className="flex items-center gap-3">
          <div className="flex items-center gap-1.5 bg-slate-950 px-3 py-1.5 rounded-lg border border-slate-800">
            <span className="text-xs text-slate-400 font-medium">Interval:</span>
            <select
              value={intervalSec}
              onChange={(e) => setIntervalSec(Number(e.target.value))}
              className="bg-transparent text-xs text-white font-mono focus:outline-none cursor-pointer"
            >
              <option value={10}>10s</option>
              <option value={30}>30s</option>
              <option value={60}>60s</option>
              <option value={300}>5m</option>
            </select>
          </div>

          <Button
            variant={slideshowRunning ? "destructive" : "default"}
            className="gap-2"
            onClick={handleToggleSlideshow}
          >
            {slideshowRunning ? (
              <>
                <Square className="w-4 h-4" /> Stop Slideshow
              </>
            ) : (
              <>
                <Play className="w-4 h-4" /> Start Slideshow
              </>
            )}
          </Button>
        </div>
      </div>

      {/* Upload Dropzone */}
      <div
        className={`dom-dropzone mb-8 animate-fade-in ${dragging ? "dragging" : ""}`}
        onDragOver={(e) => { e.preventDefault(); setDragging(true); }}
        onDragLeave={() => setDragging(false)}
        onDrop={(e) => { e.preventDefault(); setDragging(false); handleFiles(e.dataTransfer.files); }}
        onClick={() => inputRef.current?.click()}
      >
        <input
          ref={inputRef}
          type="file"
          accept="image/*"
          multiple
          className="hidden"
          onChange={(e) => handleFiles(e.target.files)}
        />
        <div className="w-12 h-12 rounded-2xl bg-cyan-500/10 border border-cyan-500/20 text-cyan-400 flex items-center justify-center mb-3">
          <CloudUpload className="w-6 h-6" />
        </div>
        <p className="text-sm font-semibold text-white">Upload Wallpapers to Webserver Database</p>
        <p className="text-xs text-slate-500 mt-1">
          Backend automatically resizes uploaded 4K images to 320x240 JPEG Q85 + 107x80 thumbnail.
        </p>
        <p className="text-[11px] text-cyan-400/80 mt-2 font-medium">
          ESP32 512KB SRAM Optimized: Zero RGB RAM buffer, streamed directly to ILI9341 LCD.
        </p>
      </div>

      {/* Wallpapers Grid */}
      {wallpapers.length === 0 ? (
        <EmptyState
          icon={Image}
          title="No Wallpapers Available"
          description="Upload JPEG/PNG images above to save them in the webserver database."
        />
      ) : (
        <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-4 gap-4">
          {wallpapers.map((wp) => (
            <div
              key={wp.id}
              className={`group relative rounded-2xl border overflow-hidden transition-all duration-300 ${
                selected === wp.id
                  ? "border-cyan-500 bg-cyan-500/5 shadow-xl shadow-cyan-500/10 ring-2 ring-cyan-500/20"
                  : "border-slate-800/80 bg-slate-900/40 hover:border-slate-700"
              }`}
              onClick={() => setSelected(wp.id)}
            >
              {/* Thumbnail Container */}
              <div className="aspect-[4/3] bg-slate-950 relative overflow-hidden flex items-center justify-center">
                {/* eslint-disable-next-line @next/next/no-img-element */}
                <img
                  src={wp.thumbnail_url || wp.url}
                  alt={wp.filename}
                  className="w-full h-full object-cover group-hover:scale-105 transition-transform duration-500"
                />

                {/* Selected Overlay Checkmark */}
                {selected === wp.id && (
                  <div className="absolute inset-0 bg-cyan-950/40 backdrop-blur-[2px] flex items-center justify-center">
                    <CheckCircle2 className="w-10 h-10 text-cyan-400 drop-shadow-md animate-scale-in" />
                  </div>
                )}
              </div>

              {/* Card Footer */}
              <div className="p-3 flex items-center justify-between">
                <div className="min-w-0 flex-1">
                  <p className="text-xs font-semibold text-white truncate">{wp.filename}</p>
                  <p className="text-[10px] text-cyan-400 mt-0.5 font-medium">
                    320×240 px • JPEG Q85
                  </p>
                </div>
                <div className="flex items-center gap-1 opacity-0 group-hover:opacity-100 transition-opacity">
                  <button
                    className="p-1.5 rounded-lg text-slate-400 hover:text-red-400 hover:bg-red-500/10 transition"
                    onClick={(e) => {
                      e.stopPropagation();
                      handleDelete(wp.id);
                    }}
                    title="Delete wallpaper from database"
                  >
                    <Trash2 className="w-3.5 h-3.5" />
                  </button>
                  <button
                    className="p-1.5 rounded-lg text-cyan-400 hover:bg-cyan-500/10 transition"
                    onClick={(e) => {
                      e.stopPropagation();
                      handlePushToDevice(wp);
                    }}
                    title="Stream wallpaper to device"
                  >
                    <Send className="w-3.5 h-3.5" />
                  </button>
                </div>
              </div>
            </div>
          ))}
        </div>
      )}
    </>
  );
}
