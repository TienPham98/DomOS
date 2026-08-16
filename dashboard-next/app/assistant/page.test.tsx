import { render, screen } from "@testing-library/react";

import AssistantPage from "./page";

describe("assistant history page", () => {
  beforeEach(() => vi.restoreAllMocks());

  it("renders persisted turns and MCP traces from the gateway", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({
        ok: true,
        json: async () => ({
          count: 1,
          items: [{
            id: "turn-1",
            device_id: "board",
            user_text: "increase volume",
            assistant_text: "Volume is now 80",
            provider: "openrouter",
            model: "openrouter/free",
            created_at: "2026-08-16T12:00:00.000Z",
            completed_at: "2026-08-16T12:00:01.000Z",
            tool_calls: [{
              id: "tool-1",
              name: "speaker.set_volume",
              arguments: { volume: 80 },
              result: { isError: false },
              duration_ms: 56,
              status: "success",
              created_at: "2026-08-16T12:00:00.500Z",
            }],
          }],
        }),
      }),
    );

    render(await AssistantPage());
    expect(screen.getByText("increase volume")).toBeInTheDocument();
    expect(screen.getByText("Volume is now 80")).toBeInTheDocument();
    expect(screen.getByText(/speaker\.set_volume/)).toBeInTheDocument();
    expect(fetch).toHaveBeenCalledWith(
      "http://localhost:8000/api/v1/conversations?limit=50",
      { cache: "no-store" },
    );
  });

  it("shows a recoverable error when the gateway is unavailable", async () => {
    vi.stubGlobal("fetch", vi.fn().mockRejectedValue(new Error("offline")));
    render(await AssistantPage());
    expect(screen.getByText(/offline/)).toBeInTheDocument();
  });
});
