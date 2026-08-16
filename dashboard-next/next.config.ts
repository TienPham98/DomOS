import type { NextConfig } from "next";
import { readFileSync } from "node:fs";
import { resolve } from "node:path";

// Next.js normally searches only this package directory. Load the shared,
// ignored root .env so deployment addresses have a single source of truth.
try {
  const rootEnv = readFileSync(resolve(process.cwd(), "../.env"), "utf8");
  for (const line of rootEnv.split(/\r?\n/)) {
    const match = line.match(/^([A-Za-z_][A-Za-z0-9_]*)=(.*)$/);
    if (match && process.env[match[1]] === undefined) process.env[match[1]] = match[2];
  }
} catch {
  // Localhost fallbacks below keep tests and isolated development usable.
}

const deploymentHost = process.env.NEXT_PUBLIC_HOST_IP ?? process.env.DOMOS_HOST_IP;

const nextConfig: NextConfig = {
  allowedDevOrigins: [
    deploymentHost,
    deploymentHost ? `${deploymentHost}:3000` : undefined,
    "localhost:3000",
  ].filter((origin): origin is string => Boolean(origin)),

  experimental: {
    optimizePackageImports: [],
  },
};

export default nextConfig;
