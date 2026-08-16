import { renderHook, waitFor } from "@testing-library/react";

import { useBoard } from "./use-board";

const board = {
  id: "es3c28p-01",
  name: "Desk Dom",
  board: "ES3C28P",
  mac: "B8:1F:3F:C3:97:54",
  firmware: "0.5.0",
  online: true,
  wifi: { ssid: "Dom_12", ip: "device.local", rssi: -42 },
  free_heap: 120000,
  storage_used: 100,
  storage_total: 1000,
};

describe("useBoard", () => {
  beforeEach(() => vi.restoreAllMocks());

  it("fetches status and logs in parallel and maps a dashboard device", async () => {
    const fetchMock = vi
      .fn()
      .mockResolvedValueOnce({ ok: true, json: async () => board })
      .mockResolvedValueOnce({
        ok: true,
        json: async () => [{ ts: "now", level: "INFO", source: "wifi", msg: "connected" }],
      });
    vi.stubGlobal("fetch", fetchMock);

    const { result, unmount } = renderHook(() => useBoard());
    await waitFor(() => expect(result.current.loading).toBe(false));

    expect(fetchMock).toHaveBeenCalledTimes(2);
    expect(result.current.board?.wifi.ip).toBe("device.local");
    expect(result.current.deviceList[0].firmware).toBe("0.5.0");
    expect(result.current.logs).toHaveLength(1);
    expect(result.current.telemetry[0].rssi).toBe(-42);
    unmount();
  });

  it("exposes connection failures without stale board data", async () => {
    vi.stubGlobal("fetch", vi.fn().mockRejectedValue(new Error("board offline")));
    const { result, unmount } = renderHook(() => useBoard());
    await waitFor(() => expect(result.current.loading).toBe(false));
    expect(result.current.error).toBe("board offline");
    expect(result.current.board).toBeNull();
    unmount();
  });
});
