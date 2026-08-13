"use client";

import { useState, useEffect, useRef } from "react";
import { ScrollText, Filter, Download, Trash2, Search, ArrowDown } from "lucide-react";
import { PageHeader } from "@/components/dashboard-primitives";
import { Button } from "@/components/ui/button";
import { useBoard } from "@/hooks/use-board";

type LogLevel = "ALL" | "INFO" | "WARN" | "ERROR";

export default function LogsPage() {
  const { logs, clearLogs } = useBoard();

  const [filter, setFilter] = useState<LogLevel>("ALL");
  const [searchQuery, setSearchQuery] = useState("");
  const [autoScroll, setAutoScroll] = useState(true);
  const scrollRef = useRef<HTMLDivElement>(null);

  const filtered = logs.filter((log) => {
    const matchLevel = filter === "ALL" || log.level === filter;
    const matchSearch =
      !searchQuery ||
      log.msg.toLowerCase().includes(searchQuery.toLowerCase()) ||
      log.source.toLowerCase().includes(searchQuery.toLowerCase());
    return matchLevel && matchSearch;
  });

  useEffect(() => {
    if (autoScroll && scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  }, [filtered.length, autoScroll]);

  const handleExport = () => {
    if (logs.length === 0) return;
    const jsonStr = JSON.stringify(logs, null, 2);
    const blob = new Blob([jsonStr], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = `domos_system_logs_${new Date().toISOString().slice(0, 10)}.json`;
    a.click();
    URL.revokeObjectURL(url);
  };


  const levelColor = (level: string) => {
    switch (level) {
      case "ERROR": return "text-red-400 bg-red-500/10";
      case "WARN": return "text-orange-400 bg-orange-500/10";
      default: return "text-slate-500 bg-slate-800/50";
    }
  };

  const sourceColor = (source: string) => {
    const map: Record<string, string> = {
      wifi: "text-cyan-400",
      mqtt: "text-emerald-400",
      ota: "text-purple-400",
      apps: "text-blue-400",
      touch: "text-amber-400",
      display: "text-pink-400",
      http: "text-teal-400",
      audio: "text-orange-400",
      voice: "text-fuchsia-400",
      storage: "text-yellow-400",
      system: "text-slate-400",
    };
    return map[source] || "text-slate-500";
  };

  return (
    <>
      <PageHeader
        title="System Logs"
        subtitle="System"
        badge={`${filtered.length} entries`}
        action={
          <div className="flex gap-2">
            <Button variant="outline" className="gap-2 text-xs h-8" onClick={handleExport} disabled={logs.length === 0}>
              <Download className="w-3 h-3" /> Export
            </Button>
            <Button variant="outline" className="gap-2 text-xs h-8" onClick={clearLogs} disabled={logs.length === 0}>
              <Trash2 className="w-3 h-3" /> Clear
            </Button>
          </div>
        }
      />

      {/* Toolbar */}
      <div className="dom-card-static p-4 mb-6 animate-fade-in">
        <div className="flex flex-col sm:flex-row gap-3">
          {/* Search */}
          <div className="relative flex-1">
            <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-slate-500" />
            <input
              type="text"
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              placeholder="Filter logs by keyword or source…"
              className="w-full pl-10 pr-4 py-2.5 rounded-xl bg-slate-900 border border-slate-700 text-sm text-white placeholder-slate-600 focus:border-cyan-500 focus:outline-none"
            />
          </div>

          {/* Level Filter */}
          <div className="flex gap-1.5">
            {(["ALL", "INFO", "WARN", "ERROR"] as LogLevel[]).map((level) => (
              <button
                key={level}
                className={`px-3 py-2 rounded-lg text-xs font-semibold transition ${
                  filter === level
                    ? level === "ERROR"
                      ? "bg-red-500/15 text-red-400 border border-red-500/30"
                      : level === "WARN"
                        ? "bg-orange-500/15 text-orange-400 border border-orange-500/30"
                        : "bg-cyan-500/10 text-cyan-400 border border-cyan-500/30"
                    : "text-slate-500 border border-slate-800 hover:border-slate-700"
                }`}
                onClick={() => setFilter(level)}
              >
                {level}
                {level !== "ALL" && (
                  <span className="ml-1.5 opacity-60">
                    {logs.filter((l) => l.level === level).length}
                  </span>
                )}
              </button>
            ))}
          </div>

          {/* Auto-scroll */}
          <button
            className={`flex items-center gap-1.5 px-3 py-2 rounded-lg text-xs font-medium transition border ${
              autoScroll
                ? "bg-cyan-500/10 text-cyan-400 border-cyan-500/30"
                : "text-slate-500 border-slate-800"
            }`}
            onClick={() => setAutoScroll(!autoScroll)}
          >
            <ArrowDown className="w-3 h-3" />
            Auto-scroll
          </button>
        </div>
      </div>

      {/* Log Table */}
      <div className="dom-card-static overflow-hidden animate-slide-up">
        <div ref={scrollRef} className="overflow-y-auto" style={{ maxHeight: "calc(100vh - 340px)" }}>
          <table className="w-full">

            <thead>
              <tr className="border-b border-slate-800/50">
                <th className="text-left text-[10px] font-semibold text-slate-600 uppercase tracking-wider py-3 px-4 w-20">
                  Time
                </th>
                <th className="text-left text-[10px] font-semibold text-slate-600 uppercase tracking-wider py-3 px-4 w-20">
                  Level
                </th>
                <th className="text-left text-[10px] font-semibold text-slate-600 uppercase tracking-wider py-3 px-4 w-24">
                  Source
                </th>
                <th className="text-left text-[10px] font-semibold text-slate-600 uppercase tracking-wider py-3 px-4">
                  Message
                </th>
              </tr>
            </thead>
            <tbody className="font-mono text-xs">
              {filtered.length === 0 ? (
                <tr>
                  <td colSpan={4} className="py-12 text-center text-slate-500 font-sans text-sm">
                    {logs.length === 0
                      ? "No system logs recorded yet from board (http://192.168.0.106)"
                      : "No log entries match the current filter"}
                  </td>
                </tr>
              ) : (
                filtered.map((log, i) => (
                  <tr
                    key={i}
                    className="border-b border-slate-800/30 hover:bg-slate-800/20 transition-colors"
                  >
                    <td className="py-2.5 px-4 text-slate-600">{log.ts}</td>
                    <td className="py-2.5 px-4">
                      <span className={`inline-flex px-2 py-0.5 rounded-md text-[10px] font-bold ${levelColor(log.level)}`}>
                        {log.level}
                      </span>
                    </td>
                    <td className={`py-2.5 px-4 font-semibold ${sourceColor(log.source)}`}>
                      {log.source}
                    </td>
                    <td className="py-2.5 px-4 text-slate-300">{log.msg}</td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </div>
    </>
  );
}

