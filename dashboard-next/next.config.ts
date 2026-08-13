import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  allowedDevOrigins: ["192.168.0.103", "192.168.0.102", "localhost:3000", "192.168.0.102:3000"],

  experimental: {
    optimizePackageImports: [],
  },
};

export default nextConfig;
