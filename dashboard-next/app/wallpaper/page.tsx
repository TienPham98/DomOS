"use client";

import { useState, useRef, useCallback, useEffect } from "react";
import { Image, Trash2, Send, CheckCircle2, CloudUpload } from "lucide-react";
import { PageHeader, EmptyState } from "@/components/dashboard-primitives";
import { Button } from "@/components/ui/button";
import { demoWallpapers } from "@/lib/demo-data";
import type { Wallpaper } from "@/lib/api";

const getBackendUrl = () => {
  return process.env.NEXT_PUBLIC_API_URL || "http://localhost:8081";
};

interface BackendWallpaper {
  id: string;
  name: string;
  url: string;
  thumbnail_url?: string;
  width?: number;
  height?: number;
  size_bytes?: number;
  created_at: string;
}

export default function WallpaperPage() {
  const [wallpapers, setWallpapers] = useState<Wallpaper[]>(demoWallpapers);
  const [selected, setSelected] = useState<string | null>(null);
  const [dragging, setDragging] = useState(false);
  const [pushing, setPushing] = useState(false);
  const [pushStatus, setPushStatus] = useState<string | null>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  const deviceIp = process.env.NEXT_PUBLIC_DEVICE_IP || "device.local";
  const serverIp = process.env.NEXT_PUBLIC_HOST_IP || new URL(getBackendUrl()).hostname;
  const defaultServerIp = serverIp;


  const loadWallpapersFromBackend = useCallback(async (): Promise<Wallpaper[] | null> => {
    const backendUrl = getBackendUrl();
    try {
      const res = await fetch(`${backendUrl}/api/wallpapers`);
      if (res.ok) {
        const json = await res.json();
        if (json.data && Array.isArray(json.data)) {
          const list: Wallpaper[] = json.data.map((item: BackendWallpaper) => ({
            id: item.id,
            filename: item.name,
            url: item.url.startsWith("http") ? item.url : `${backendUrl}${item.url}`,
            thumbnail_url: item.thumbnail_url ? (item.thumbnail_url.startsWith("http") ? item.thumbnail_url : `${backendUrl}${item.thumbnail_url}`) : undefined,
            width: item.width || 320,
            height: item.height || 240,
            size: item.size_bytes || 0,
            created_at: item.created_at,
          }));
          return list;
        }
      }
    } catch (e) {
      console.warn("Backend offline, using fallback list:", e);
    }
    return null;
  }, []);

  useEffect(() => {
    let active = true;
    void loadWallpapersFromBackend().then((list) => {
      if (active && list) setWallpapers(list);
    });
    return () => {
      active = false;
    };
  }, [loadWallpapersFromBackend]);

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
      hostForDevice = defaultServerIp;
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
    } catch (err: unknown) {
      const errorMessage = err instanceof Error ? err.message : "Unknown device error";
      const msg = errorMessage === "Failed to fetch"
        ? "ESP32 IP unreachable. Please check board Wi-Fi connection and update 'Device IP' input above."
        : errorMessage;
      setPushStatus(`Device Sync Error (${targetHost}): ${msg}`);
      setTimeout(() => setPushStatus(null), 7000);
    } finally {
      setPushing(false);
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
          const refreshed = await loadWallpapersFromBackend();
          if (refreshed) setWallpapers(refreshed);

          // Immediately sync with ESP32 device
          const targetHost = deviceIp.includes(":") ? deviceIp : `${deviceIp}:80`;
          fetch(`http://${targetHost}/api/wallpaper`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ action: "sync" }),
          }).catch(() => null);
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
      } catch {
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
  }, [deviceIp, loadWallpapersFromBackend]);

  const handleDelete = async (id: string) => {
    try {
      await fetch(`${getBackendUrl()}/api/wallpaper/${id}`, { method: "DELETE" });
      const targetHost = deviceIp.includes(":") ? deviceIp : `${deviceIp}:80`;
      await fetch(`http://${targetHost}/api/wallpaper`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action: "sync", deleted_id: id }),
      }).catch(() => null);
    } catch (e) {
      console.warn("Delete backend failed:", e);
    }
    setWallpapers((prev) => prev.filter((w) => w.id !== id));
    if (selected === id) setSelected(null);
  };

  return (
    <>
      <PageHeader
        title="Wallpaper Gallery"
        subtitle="Go Backend resizes to 320x240 JPEG Q85 • Tap Screen Slideshow Button for 5s Loop"
        badge={`${wallpapers.length} images`}
        action={
          <div className="flex flex-wrap items-center gap-2">
            <div className="flex items-center gap-1 bg-slate-900 border border-slate-700 px-2.5 py-1.5 rounded-lg text-xs">
              <span className="text-slate-400 font-medium">Device:</span>
              <input
                type="text"
                value={deviceIp}
                readOnly
                className="bg-transparent text-white font-mono w-28 text-xs"
                title="Fixed ESP32 device IP"
              />
            </div>
            <div className="flex items-center gap-1 bg-slate-900 border border-slate-700 px-2.5 py-1.5 rounded-lg text-xs">
              <span className="text-slate-400 font-medium">Server IP:</span>
              <input
                type="text"
                value={serverIp}
                readOnly
                className="bg-transparent text-white font-mono w-36 text-xs"
                title="Fixed DomOS server IP"
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
              onClick={() => {
                setSelected(wp.id);
                handlePushToDevice(wp);
              }}
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
