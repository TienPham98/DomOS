"use client";

import { useEffect, useState } from "react";
import type { Device } from "@/lib/api";

export interface BoardStatus {
  id: string;
  name: string;
  board: string;
  mac: string;
  firmware: string;
  online: boolean;
  wifi: {
    ssid: string;
    ip: string;
    rssi: number;
  };
  free_heap: number;
  min_free_heap?: number;
  free_psram?: number;
  storage_used: number;
  storage_total: number;
}

export interface LogItem {
  ts: string;
  level: string;
  source: string;
  msg: string;
}

export function useBoard() {
  const [board, setBoard] = useState<BoardStatus | null>(null);
  const [logs, setLogs] = useState<LogItem[]>([]);
  const [loading, setLoading] = useState<boolean>(true);
  const [error, setError] = useState<string | null>(null);
  const [telemetry, setTelemetry] = useState<Array<{ time: string; rssi: number; heap: number; temp: number }>>([]);

  const targetIp = process.env.NEXT_PUBLIC_DEVICE_IP || "device.local";


  useEffect(() => {
    let isMounted = true;

    async function fetchStatus() {
      try {
        const [statusRes, logsRes] = await Promise.all([
          fetch(`http://${targetIp}/api/status`, { signal: AbortSignal.timeout(3000) }),
          fetch(`http://${targetIp}/api/logs`, { signal: AbortSignal.timeout(3000) }).catch(() => null),
        ]);


        if (!statusRes.ok) throw new Error(`HTTP error ${statusRes.status}`);
        const data: BoardStatus = await statusRes.json();
        const logsData: LogItem[] = logsRes && logsRes.ok ? await logsRes.json() : [];

        if (isMounted) {
          setBoard(data);
          if (logsData.length > 0) {
            setLogs((prev) => {
              const existingKeys = new Set(prev.map((l) => `${l.ts}-${l.source}-${l.msg}`));
              const newItems = logsData.filter((l) => !existingKeys.has(`${l.ts}-${l.source}-${l.msg}`));
              return [...prev, ...newItems].slice(-100);
            });
          }
          setError(null);
          setLoading(false);

          // Append telemetry point
          const nowTime = new Date().toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit" });
          setTelemetry((prev) => {
            const next = [...prev, { time: nowTime, rssi: data.wifi.rssi, heap: data.free_heap, temp: 42 }];
            return next.slice(-20); // Keep last 20 points
          });
        }
      } catch (err: unknown) {
        if (isMounted) {
          setError(err instanceof Error ? err.message : "Failed to connect to board");
          setBoard(null);
          setLoading(false);
        }
      }
    }

    fetchStatus();
    const interval = setInterval(fetchStatus, 3000);

    return () => {
      isMounted = false;
      clearInterval(interval);
    };
  }, [targetIp]);

  const deviceList: Device[] = board
    ? [
        {
          id: board.id || "es3c28p-01",
          name: board.name || "ES3C28P Desk Terminal",
          mac: board.mac || "B8:1F:3F:C3:97:54",
          ip: board.wifi.ip,
          firmware: board.firmware || "0.2.1",
          online: board.online,
          last_seen: "Just now",
          rssi: board.wifi.rssi,
          storage_used: board.storage_used,
          storage_total: board.storage_total,
        },
      ]
    : [];


  const clearLogs = () => setLogs([]);

  return { board, deviceList, logs, clearLogs, telemetry, loading, error };
}
