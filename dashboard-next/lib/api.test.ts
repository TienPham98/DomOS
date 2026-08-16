import {
  connectWebSocket,
  fetchDevices,
  getAuthToken,
  login,
  setAuthToken,
  uploadWallpaper,
} from "./api";

class FakeWebSocket {
  static instances: FakeWebSocket[] = [];
  onmessage: ((event: MessageEvent) => void) | null = null;
  onclose: (() => void) | null = null;
  onerror: (() => void) | null = null;
  closed = false;

  constructor(public url: string) {
    FakeWebSocket.instances.push(this);
  }

  close() {
    this.closed = true;
  }
}

describe("DomOS API client", () => {
  beforeEach(() => {
    vi.restoreAllMocks();
    FakeWebSocket.instances = [];
  });

  it("unwraps the Go backend response envelope", async () => {
    const devices = [{ id: "board", name: "Dom", online: true }];
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({
        ok: true,
        json: async () => ({ success: true, data: devices }),
      }),
    );

    await expect(fetchDevices()).resolves.toEqual(devices);
    expect(fetch).toHaveBeenCalledWith(
      "http://localhost:8081/api/devices",
      expect.objectContaining({ headers: expect.any(Object) }),
    );
  });

  it("stores a login token and sends it on later requests", async () => {
    const fetchMock = vi.fn().mockResolvedValue({
      ok: true,
      json: async () => ({ success: true, data: { token: "jwt-token" } }),
    });
    vi.stubGlobal("fetch", fetchMock);

    await login("dom@example.com", "password123");
    expect(getAuthToken()).toBe("jwt-token");
    await fetchDevices();
    expect(fetchMock.mock.calls[1][1].headers.Authorization).toBe("Bearer jwt-token");
  });

  it("does not set JSON content type for multipart uploads", async () => {
    setAuthToken("upload-token");
    const fetchMock = vi.fn().mockResolvedValue({
      ok: true,
      json: async () => ({ success: true, data: { id: "wallpaper" } }),
    });
    vi.stubGlobal("fetch", fetchMock);

    await uploadWallpaper(new File(["image"], "wallpaper.jpg", { type: "image/jpeg" }));
    const options = fetchMock.mock.calls[0][1];
    expect(options.body).toBeInstanceOf(FormData);
    expect(options.headers["Content-Type"]).toBeUndefined();
  });

  it("parses websocket messages and ignores malformed payloads", () => {
    vi.stubGlobal("WebSocket", FakeWebSocket);
    const onMessage = vi.fn();
    const socket = connectWebSocket(onMessage) as unknown as FakeWebSocket;

    expect(socket.url).toBe("ws://localhost:8081/ws");
    socket.onmessage?.({ data: JSON.stringify({ type: "notification", message: "ok" }) } as MessageEvent);
    socket.onmessage?.({ data: "not-json" } as MessageEvent);
    expect(onMessage).toHaveBeenCalledTimes(1);

    socket.onerror?.();
    expect(socket.closed).toBe(true);
  });
});
