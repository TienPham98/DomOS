"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import {
  LayoutDashboard,
  Image,
  Clock,
  Cpu,
  Upload,
  ScrollText,
  Monitor,
  Home,
  Settings,
  Mic,
  Zap,
} from "lucide-react";

const sections = [
  {
    label: "Overview",
    items: [
      { href: "/", icon: LayoutDashboard, label: "Dashboard" },
      { href: "/devices", icon: Monitor, label: "Devices" },
    ],
  },
  {
    label: "Customize",
    items: [
      { href: "/wallpaper", icon: Image, label: "Wallpaper" },
      { href: "/themes", icon: Zap, label: "Theme & Clock" },
    ],
  },
  {
    label: "Intelligence",
    items: [
      { href: "/assistant", icon: Mic, label: "Assistant" },
      { href: "/smart-home", icon: Home, label: "Smart Home" },
    ],
  },
  {
    label: "System",
    items: [
      { href: "/ota", icon: Upload, label: "OTA Update" },
      { href: "/logs", icon: ScrollText, label: "Logs" },
      { href: "/settings", icon: Settings, label: "Settings" },
    ],
  },
];

export function Sidebar() {
  const pathname = usePathname();

  return (
    <aside className="dom-sidebar">
      {/* Logo */}
      <Link href="/" className="flex items-center gap-3 px-3 mb-8 no-underline">
        <div className="relative flex items-center justify-center w-9 h-9 rounded-xl bg-gradient-to-br from-cyan-500 to-blue-600 shadow-lg shadow-cyan-500/20">
          <Cpu className="w-5 h-5 text-white" />
          <div className="absolute inset-0 rounded-xl" style={{ animation: "glow-pulse 3s infinite" }} />
        </div>
        <div className="sidebar-title-text">
          <h1 className="text-lg font-bold tracking-tight text-white leading-none">
            Dom<span className="text-cyan-400">OS</span>
          </h1>
          <p className="text-[10px] text-slate-500 font-medium tracking-widest uppercase">
            ES3C28P
          </p>
        </div>
      </Link>

      {/* Navigation */}
      <nav className="flex flex-col gap-1 flex-1">
        {sections.map((section) => (
          <div key={section.label} className="mb-4">
            <p className="sidebar-section px-4 mb-2 text-[10px] font-semibold tracking-widest uppercase text-slate-600">
              {section.label}
            </p>
            {section.items.map((item) => {
              const active = pathname === item.href;
              return (
                <Link
                  key={item.href}
                  href={item.href}
                  className={`dom-sidebar-link ${active ? "active" : ""}`}
                >
                  <item.icon className="w-[18px] h-[18px] flex-shrink-0" />
                  <span className="sidebar-label">{item.label}</span>
                </Link>
              );
            })}
          </div>
        ))}
      </nav>

      {/* Bottom status */}
      <div className="dom-card-static p-3 mt-4">
        <div className="flex items-center gap-2 mb-2">
          <div className="dom-dot dom-dot-online" />
          <span className="sidebar-label text-xs text-slate-400">System Active</span>
        </div>
        <p className="sidebar-label text-[10px] text-slate-600">v0.2.1 • 3 devices</p>
      </div>
    </aside>
  );
}
