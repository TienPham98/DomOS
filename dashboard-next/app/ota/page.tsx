"use client";

import { useState, useRef } from "react";
import { Upload, Shield, Download, RotateCcw, CheckCircle2, Clock } from "lucide-react";
import { PageHeader, Section } from "@/components/dashboard-primitives";
import { Button } from "@/components/ui/button";
import { demoFirmware } from "@/lib/demo-data";

const defaultFirmware = [
  { id: "fw-0.2.1", version: "0.2.1", notes: "DomOS Stable release with ILI9341 display & FT6336 touch support", url: "#", size: 1454112, created_at: new Date().toISOString() },
];

export default function OtaPage() {
  const [firmwareList] = useState(demoFirmware.length > 0 ? demoFirmware : defaultFirmware);
  const [uploading, setUploading] = useState(false);
  const [uploadProgress, setUploadProgress] = useState(0);
  const [statusMessage, setStatusMessage] = useState<string | null>(null);
  const fileInputRef = useRef<HTMLInputElement>(null);


  const latest = firmwareList[0];

  const handleFileUpload = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;

    setUploading(true);
    setUploadProgress(10);
    setStatusMessage(`Uploading firmware binary ${file.name} to board...`);

    try {
      const formData = new FormData();
      formData.append("file", file);

      const defaultDevIp = process.env.NEXT_PUBLIC_DEVICE_IP || "device.local";
      const deviceIp = typeof window !== "undefined" ? localStorage.getItem("domos_device_ip") || defaultDevIp : defaultDevIp;

      const res = await fetch(`http://${deviceIp}/api/upload`, {
        method: "POST",
        body: formData,
      });


      setUploadProgress(100);
      if (!res.ok) throw new Error(`HTTP ${res.status}: ${res.statusText}`);
      setStatusMessage(`Firmware binary ${file.name} uploaded successfully to board LittleFS!`);
      setTimeout(() => setStatusMessage(null), 4000);
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : "Unknown upload error";
      setStatusMessage(`Error: ${message}`);
      setTimeout(() => setStatusMessage(null), 5000);
    } finally {
      setUploading(false);
      if (fileInputRef.current) fileInputRef.current.value = "";
    }
  };

  return (
    <>
      <input
        ref={fileInputRef}
        type="file"
        accept=".bin"
        className="hidden"
        onChange={handleFileUpload}
      />
      <PageHeader
        title="OTA Update"
        subtitle="System"
        badge="Firmware"
        action={
          <Button className="gap-2" onClick={() => fileInputRef.current?.click()} disabled={uploading}>
            <Upload className="w-4 h-4" />
            {uploading ? `Uploading ${uploadProgress}%` : "Upload Firmware"}
          </Button>
        }
      />

      {statusMessage && (
        <div className={`p-4 rounded-xl mb-6 font-medium text-sm border animate-fade-in ${
          statusMessage.startsWith("Error")
            ? "bg-red-500/10 text-red-400 border-red-500/20"
            : "bg-cyan-500/10 text-cyan-400 border-cyan-500/20"
        }`}>
          {statusMessage}
        </div>
      )}


      {/* Upload Progress */}
      {uploading && (
        <div className="dom-card-static p-6 mb-8 animate-fade-in">
          <div className="flex items-center gap-4 mb-4">
            <div className="flex items-center justify-center w-10 h-10 rounded-xl bg-cyan-500/10">
              <Upload className="w-5 h-5 text-cyan-400 animate-pulse" />
            </div>
            <div className="flex-1">
              <p className="text-sm font-semibold text-white">
                {uploadProgress < 100 ? "Uploading firmware…" : "Upload complete!"}
              </p>
              <p className="text-xs text-slate-500">
                {uploadProgress < 100
                  ? "Broadcasting OTA to fleet via MQTT"
                  : "All devices notified"}
              </p>
            </div>
            <span className="text-lg font-bold text-cyan-400 dom-stat">{uploadProgress}%</span>
          </div>
          <div className="dom-progress">
            <div
              className="dom-progress-fill"
              style={{
                width: `${uploadProgress}%`,
                background:
                  uploadProgress >= 100
                    ? "linear-gradient(90deg, #10b981, #22d3ee)"
                    : undefined,
              }}
            />
          </div>
        </div>
      )}

      {/* Current Version */}
      <div className="grid gap-6 xl:grid-cols-3 mb-8">
        <div className="dom-card p-6 animate-slide-up">
          <div className="flex items-center gap-3 mb-4">
            <div className="flex items-center justify-center w-10 h-10 rounded-xl bg-emerald-500/10">
              <CheckCircle2 className="w-5 h-5 text-emerald-400" />
            </div>
            <div>
              <p className="text-xs text-slate-500">Latest Version</p>
              <p className="text-xl font-bold text-white">v{latest.version}</p>
            </div>
          </div>
          <p className="text-sm text-slate-400">{latest.notes}</p>
          <p className="text-xs text-slate-600 mt-3">
            Released {new Date(latest.created_at).toLocaleDateString()}
          </p>
        </div>

        <div className="dom-card p-6 animate-slide-up" style={{ animationDelay: "80ms" }}>
          <div className="flex items-center gap-3 mb-4">
            <div className="flex items-center justify-center w-10 h-10 rounded-xl bg-blue-500/10">
              <Shield className="w-5 h-5 text-blue-400" />
            </div>
            <div>
              <p className="text-xs text-slate-500">OTA Channel</p>
              <p className="text-xl font-bold text-white">HTTPS</p>
            </div>
          </div>
          <p className="text-sm text-slate-400">
            Dual partition (ota_0 + ota_1) with certificate bundle verification
          </p>
        </div>

        <div className="dom-card p-6 animate-slide-up" style={{ animationDelay: "160ms" }}>
          <div className="flex items-center gap-3 mb-4">
            <div className="flex items-center justify-center w-10 h-10 rounded-xl bg-orange-500/10">
              <RotateCcw className="w-5 h-5 text-orange-400" />
            </div>
            <div>
              <p className="text-xs text-slate-500">Rollback</p>
              <p className="text-xl font-bold text-white">Available</p>
            </div>
          </div>
          <p className="text-sm text-slate-400">
            Previous firmware preserved in alternate OTA slot for instant rollback
          </p>
          <Button variant="outline" className="mt-3 text-xs h-8">
            <RotateCcw className="w-3 h-3 mr-1.5" /> Rollback to v{firmwareList[1]?.version}
          </Button>
        </div>
      </div>

      {/* Firmware History */}
      <Section title="Firmware History">
        <div className="space-y-3">
          {firmwareList.map((fw, i) => (
            <div
              key={fw.id}
              className={`flex items-center justify-between p-4 rounded-xl border transition animate-slide-up ${
                i === 0
                  ? "border-emerald-500/30 bg-emerald-500/5"
                  : "border-slate-800/50 bg-slate-900/30 hover:border-slate-700"
              }`}
              style={{ animationDelay: `${i * 60}ms` }}
            >
              <div className="flex items-center gap-4">
                <div className="flex items-center justify-center w-10 h-10 rounded-xl bg-slate-800">
                  {i === 0 ? (
                    <CheckCircle2 className="w-5 h-5 text-emerald-400" />
                  ) : (
                    <Clock className="w-5 h-5 text-slate-500" />
                  )}
                </div>
                <div>
                  <div className="flex items-center gap-2">
                    <p className="text-sm font-semibold text-white">v{fw.version}</p>
                    {i === 0 && (
                      <span className="dom-badge dom-badge-green">Current</span>
                    )}
                  </div>
                  <p className="text-xs text-slate-500 mt-0.5 max-w-lg">{fw.notes}</p>
                </div>
              </div>
              <div className="flex items-center gap-4 text-right">
                <div>
                  <p className="text-xs text-slate-400">
                    {(fw.size / (1024 * 1024)).toFixed(1)} MB
                  </p>
                  <p className="text-xs text-slate-600">
                    {new Date(fw.created_at).toLocaleDateString()}
                  </p>
                </div>
                <Button variant="outline" className="text-xs h-8 px-3">
                  <Download className="w-3 h-3 mr-1.5" /> Download
                </Button>
              </div>
            </div>
          ))}
        </div>
      </Section>
    </>
  );
}
