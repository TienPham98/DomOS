import { HardDrive, Wifi } from "lucide-react";
import { render, screen } from "@testing-library/react";

import { MetricCard, PageHeader, StatusBadge, StorageBar } from "./dashboard-primitives";

describe("dashboard primitives", () => {
  it("renders page metadata and action", () => {
    render(<PageHeader title="Devices" subtitle="DomOS" badge="Live" action={<button>Refresh</button>} />);
    expect(screen.getByRole("heading", { name: "Devices" })).toBeInTheDocument();
    expect(screen.getByText("Live")).toBeInTheDocument();
    expect(screen.getByRole("button", { name: "Refresh" })).toBeInTheDocument();
  });

  it("renders online and offline states", () => {
    const { rerender } = render(<StatusBadge online />);
    expect(screen.getByText("Online")).toBeInTheDocument();
    rerender(<StatusBadge online={false} />);
    expect(screen.getByText("Offline")).toBeInTheDocument();
  });

  it("calculates storage percentage and metric content", () => {
    render(
      <>
        <StorageBar used={2 * 1024 * 1024} total={4 * 1024 * 1024} />
        <MetricCard icon={Wifi} label="Signal" value="-45 dBm" detail="Stable" trend="up" />
        <MetricCard icon={HardDrive} label="Heap" value="120 KB" />
      </>,
    );
    expect(screen.getByText("50%")).toBeInTheDocument();
    expect(screen.getByText("2.0 MB / 4.0 MB")).toBeInTheDocument();
    expect(screen.getByText("-45 dBm")).toBeInTheDocument();
  });
});
