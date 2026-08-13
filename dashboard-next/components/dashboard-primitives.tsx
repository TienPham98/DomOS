"use client";

import type { ReactNode } from "react";
import type { LucideIcon } from "lucide-react";

// ──── Page Header ─────────────────────────────────────────

export function PageHeader({
  title,
  subtitle,
  badge,
  action,
}: {
  title: string;
  subtitle?: string;
  badge?: string;
  action?: ReactNode;
}) {
  return (
    <header className="flex flex-col sm:flex-row sm:items-end justify-between gap-4 mb-8 animate-fade-in">
      <div>
        {subtitle && (
          <p className="text-sm font-medium text-cyan-400 tracking-wide mb-1">{subtitle}</p>
        )}
        <div className="flex items-center gap-3">
          <h2 className="text-3xl font-bold tracking-tight text-white">{title}</h2>
          {badge && <span className="dom-badge dom-badge-cyan">{badge}</span>}
        </div>
      </div>
      {action && <div>{action}</div>}
    </header>
  );
}

// ──── Metric Card ─────────────────────────────────────────

export function MetricCard({
  icon: Icon,
  label,
  value,
  detail,
  trend,
  color = "cyan",
  delay = 0,
}: {
  icon: LucideIcon;
  label: string;
  value: string | number;
  detail?: string;
  trend?: "up" | "down" | "neutral";
  color?: "cyan" | "green" | "orange" | "purple" | "blue" | "red";
  delay?: number;
}) {
  const colorMap = {
    cyan: { bg: "rgba(6,182,212,0.08)", icon: "#06b6d4", shadow: "#06b6d430" },
    green: { bg: "rgba(16,185,129,0.08)", icon: "#10b981", shadow: "#10b98130" },
    orange: { bg: "rgba(249,115,22,0.08)", icon: "#f97316", shadow: "#f9731630" },
    purple: { bg: "rgba(168,85,247,0.08)", icon: "#a855f7", shadow: "#a855f730" },
    blue: { bg: "rgba(59,130,246,0.08)", icon: "#3b82f6", shadow: "#3b82f630" },
    red: { bg: "rgba(239,68,68,0.08)", icon: "#ef4444", shadow: "#ef444430" },
  };

  const c = colorMap[color];

  return (
    <article
      className="dom-card p-5 animate-slide-up"
      style={{ animationDelay: `${delay}ms` }}
    >
      <div className="flex items-center justify-between mb-4">
        <div
          className="flex items-center justify-center w-10 h-10 rounded-xl"
          style={{ background: c.bg }}
        >
          <Icon className="w-5 h-5" style={{ color: c.icon }} />
        </div>
        {trend && (
          <span
            className={`text-xs font-medium ${
              trend === "up" ? "text-emerald-400" : trend === "down" ? "text-red-400" : "text-slate-500"
            }`}
          >
            {trend === "up" ? "▲" : trend === "down" ? "▼" : "—"}
          </span>
        )}
      </div>
      <p className="text-sm text-slate-400 mb-1">{label}</p>
      <p className="text-2xl font-bold dom-stat text-white">{value}</p>
      {detail && <p className="text-xs text-slate-600 mt-2">{detail}</p>}
    </article>
  );
}

// ──── Status Badge ────────────────────────────────────────

export function StatusBadge({ online }: { online: boolean }) {
  return (
    <span className={`dom-badge ${online ? "dom-badge-green" : "dom-badge-red"}`}>
      <span className={`dom-dot ${online ? "dom-dot-online" : "dom-dot-offline"} mr-2`} />
      {online ? "Online" : "Offline"}
    </span>
  );
}

// ──── Storage Bar ─────────────────────────────────────────

export function StorageBar({ used, total }: { used: number; total: number }) {
  const pct = total > 0 ? Math.round((used / total) * 100) : 0;
  const usedMB = (used / (1024 * 1024)).toFixed(1);
  const totalMB = (total / (1024 * 1024)).toFixed(1);

  return (
    <div>
      <div className="flex justify-between text-xs mb-1">
        <span className="text-slate-400">{usedMB} MB / {totalMB} MB</span>
        <span className="text-cyan-400 font-medium">{pct}%</span>
      </div>
      <div className="dom-progress">
        <div className="dom-progress-fill" style={{ width: `${pct}%` }} />
      </div>
    </div>
  );
}

// ──── Empty State ─────────────────────────────────────────

export function EmptyState({
  icon: Icon,
  title,
  description,
  action,
}: {
  icon: LucideIcon;
  title: string;
  description: string;
  action?: ReactNode;
}) {
  return (
    <div className="flex flex-col items-center justify-center py-20 text-center">
      <div className="flex items-center justify-center w-16 h-16 rounded-2xl bg-slate-800/50 mb-5">
        <Icon className="w-8 h-8 text-slate-500" />
      </div>
      <h3 className="text-lg font-semibold text-slate-300 mb-2">{title}</h3>
      <p className="text-sm text-slate-500 max-w-md mb-6">{description}</p>
      {action}
    </div>
  );
}

// ──── Section ─────────────────────────────────────────────

export function Section({
  title,
  action,
  className,
  children,
}: {
  title: string;
  action?: ReactNode;
  className?: string;
  children: ReactNode;
}) {
  return (
    <section className={`dom-card-static p-6 animate-slide-up ${className || ""}`}>
      <div className="flex items-center justify-between mb-5">
        <h3 className="text-lg font-semibold text-white">{title}</h3>
        {action}
      </div>
      {children}
    </section>
  );
}

