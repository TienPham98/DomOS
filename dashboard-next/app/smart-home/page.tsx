"use client";

import { useState } from "react";
import {
  Home,
  Lightbulb,
  Thermometer,
  DoorOpen,
  Fan,
  Power,
  Plus,
  Layers,
} from "lucide-react";
import { PageHeader, Section } from "@/components/dashboard-primitives";
import { Button } from "@/components/ui/button";

interface SmartDevice {
  id: string;
  name: string;
  room: string;
  type: "light" | "sensor" | "switch" | "fan";
  state: boolean;
  value?: string;
  mqtt_topic: string;
}

const demoSmartDevices: SmartDevice[] = [
  { id: "s1", name: "Ceiling Light", room: "Living Room", type: "light", state: true, mqtt_topic: "dom/living/ceiling" },
  { id: "s2", name: "Desk Lamp", room: "Living Room", type: "light", state: false, mqtt_topic: "dom/living/desk" },
  { id: "s3", name: "Temperature", room: "Living Room", type: "sensor", state: true, value: "26°C", mqtt_topic: "dom/living/temp" },
  { id: "s4", name: "Bedroom Light", room: "Bedroom", type: "light", state: true, mqtt_topic: "dom/bedroom/light" },
  { id: "s5", name: "Night Lamp", room: "Bedroom", type: "light", state: false, mqtt_topic: "dom/bedroom/night" },
  { id: "s6", name: "AC Fan", room: "Bedroom", type: "fan", state: true, mqtt_topic: "dom/bedroom/fan" },
  { id: "s7", name: "Humidity", room: "Bedroom", type: "sensor", state: true, value: "65%", mqtt_topic: "dom/bedroom/humidity" },
  { id: "s8", name: "Main Door", room: "Entry", type: "switch", state: false, mqtt_topic: "dom/entry/door" },
  { id: "s9", name: "Office Light", room: "Office", type: "light", state: true, mqtt_topic: "dom/office/light" },
  { id: "s10", name: "Monitor Lamp", room: "Office", type: "light", state: true, mqtt_topic: "dom/office/monitor" },
];

const scenes = [
  { name: "Good Morning", icon: "☀️", devices: ["s1", "s9", "s10"], color: "#f59e0b" },
  { name: "Movie Night", icon: "🎬", devices: ["s2"], color: "#a855f7" },
  { name: "Good Night", icon: "🌙", devices: [], color: "#3b82f6" },
  { name: "Work Mode", icon: "💻", devices: ["s9", "s10"], color: "#10b981" },
  { name: "Away", icon: "🏃", devices: [], color: "#ef4444" },
];

const deviceIcon = (type: string) => {
  switch (type) {
    case "light": return Lightbulb;
    case "sensor": return Thermometer;
    case "switch": return DoorOpen;
    case "fan": return Fan;
    default: return Power;
  }
};

export default function SmartHomePage() {
  const [devices, setDevices] = useState(demoSmartDevices);
  const [activeRoom, setActiveRoom] = useState<string | null>(null);

  const rooms = Array.from(new Set(devices.map((d) => d.room)));
  const filteredDevices = activeRoom
    ? devices.filter((d) => d.room === activeRoom)
    : devices;

  const toggleDevice = (id: string) => {
    setDevices((prev) =>
      prev.map((d) => (d.id === id ? { ...d, state: !d.state } : d))
    );
  };

  const onlineCount = devices.filter((d) => d.state).length;

  return (
    <>
      <PageHeader
        title="Smart Home"
        subtitle="Intelligence"
        badge={`${onlineCount} active`}
        action={
          <Button variant="outline" className="gap-2">
            <Plus className="w-4 h-4" /> Add Device
          </Button>
        }
      />

      {/* Scenes */}
      <div className="grid gap-3 grid-cols-2 sm:grid-cols-3 lg:grid-cols-5 mb-8">
        {scenes.map((scene, i) => (
          <button
            key={scene.name}
            className="dom-card p-4 text-center group animate-slide-up"
            style={{ animationDelay: `${i * 40}ms` }}
          >
            <div
              className="flex items-center justify-center w-12 h-12 rounded-2xl mx-auto mb-3 transition-transform group-hover:scale-110"
              style={{ background: `${scene.color}15` }}
            >
              <span className="text-2xl">{scene.icon}</span>
            </div>
            <p className="text-sm font-semibold text-white">{scene.name}</p>
            <p className="text-xs text-slate-500 mt-0.5">
              {scene.devices.length > 0
                ? `${scene.devices.length} devices`
                : "All off"}
            </p>
          </button>
        ))}
      </div>

      {/* Room Filter */}
      <div className="flex gap-2 mb-6 overflow-x-auto pb-2">
        <button
          className={`px-4 py-2 rounded-xl text-sm font-medium transition flex-shrink-0 ${
            !activeRoom
              ? "bg-cyan-500/10 text-cyan-400 border border-cyan-500/30"
              : "text-slate-400 border border-slate-800 hover:border-slate-700"
          }`}
          onClick={() => setActiveRoom(null)}
        >
          <Layers className="w-4 h-4 inline mr-1.5" />
          All Rooms
        </button>
        {rooms.map((room) => (
          <button
            key={room}
            className={`px-4 py-2 rounded-xl text-sm font-medium transition flex-shrink-0 ${
              activeRoom === room
                ? "bg-cyan-500/10 text-cyan-400 border border-cyan-500/30"
                : "text-slate-400 border border-slate-800 hover:border-slate-700"
            }`}
            onClick={() => setActiveRoom(activeRoom === room ? null : room)}
          >
            {room}
          </button>
        ))}
      </div>

      {/* Device Grid */}
      <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4">
        {filteredDevices.map((device, i) => {
          const Icon = deviceIcon(device.type);
          const isSensor = device.type === "sensor";

          return (
            <div
              key={device.id}
              className={`dom-card p-5 transition animate-slide-up ${
                device.state && !isSensor ? "border-cyan-500/30" : ""
              }`}
              style={{ animationDelay: `${i * 40}ms` }}
            >
              <div className="flex items-center justify-between mb-4">
                <div
                  className="flex items-center justify-center w-10 h-10 rounded-xl"
                  style={{
                    background: device.state
                      ? isSensor
                        ? "rgba(59,130,246,0.1)"
                        : "rgba(6,182,212,0.1)"
                      : "rgba(100,116,139,0.1)",
                  }}
                >
                  <Icon
                    className="w-5 h-5"
                    style={{
                      color: device.state
                        ? isSensor
                          ? "#3b82f6"
                          : "#06b6d4"
                        : "#64748b",
                    }}
                  />
                </div>
                {!isSensor && (
                  <div
                    className={`w-12 h-7 rounded-full transition-colors flex items-center px-1 cursor-pointer ${
                      device.state ? "bg-cyan-500" : "bg-slate-700"
                    }`}
                    onClick={() => toggleDevice(device.id)}
                  >
                    <div
                      className={`w-5 h-5 rounded-full bg-white transition-transform shadow-md ${
                        device.state ? "translate-x-5" : "translate-x-0"
                      }`}
                    />
                  </div>
                )}
              </div>
              <p className="text-sm font-semibold text-white">{device.name}</p>
              <p className="text-xs text-slate-500 mt-0.5">{device.room}</p>
              {isSensor && device.value && (
                <p className="text-2xl font-bold text-blue-400 mt-2 dom-stat">
                  {device.value}
                </p>
              )}
              <p className="text-[10px] text-slate-700 mt-3 font-mono truncate">
                {device.mqtt_topic}
              </p>
            </div>
          );
        })}
      </div>
    </>
  );
}
