package config

import "testing"

func TestLoadUsesEnvironmentAndFallbacks(t *testing.T) {
	t.Setenv("APP_PORT", "8081")
	t.Setenv("JWT_EXPIRY_HOURS", "24")
	t.Setenv("MQTT_BROKER", "tcp://broker.example:1883")

	cfg := Load()
	if cfg.AppPort != "8081" || cfg.JWTExpiryHr != 24 {
		t.Fatalf("unexpected configuration: %+v", cfg)
	}
	if cfg.MQTTBroker != "tcp://broker.example:1883" {
		t.Fatalf("fixed MQTT broker was not preserved: %s", cfg.MQTTBroker)
	}
}

func TestInvalidIntegerFallsBack(t *testing.T) {
	t.Setenv("JWT_EXPIRY_HOURS", "not-a-number")
	if got := Load().JWTExpiryHr; got != 72 {
		t.Fatalf("expected fallback 72, got %d", got)
	}
}
