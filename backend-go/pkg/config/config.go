package config

import (
	"os"
	"strconv"

	"github.com/joho/godotenv"
)

// Config holds all runtime configuration for the backend, loaded from
// environment variables (with a .env file loaded in local development).
type Config struct {
	AppEnv      string
	AppPort     string
	DatabaseURL string
	JWTSecret   string
	JWTExpiryHr int

	MQTTBroker   string
	MQTTClientID string
	MQTTUsername string
	MQTTPassword string

	UploadDir     string
	PublicBaseURL string
}

// Load reads a .env file if present and returns a populated Config,
// falling back to sane defaults for local development.
func Load() *Config {
	_ = godotenv.Load("../.env", ".env")

	return &Config{
		AppEnv:      getEnv("APP_ENV", "development"),
		AppPort:     getEnv("APP_PORT", "8080"),
		DatabaseURL: getEnv("DATABASE_URL", "host=localhost user=domos password=domos dbname=domos port=5432 sslmode=disable TimeZone=UTC"),
		JWTSecret:   getEnv("JWT_SECRET", "change-me-in-production"),
		JWTExpiryHr: getEnvAsInt("JWT_EXPIRY_HOURS", 72),

		MQTTBroker:   getEnv("MQTT_BROKER", "tcp://localhost:1883"),
		MQTTClientID: getEnv("MQTT_CLIENT_ID", "domos-backend"),
		MQTTUsername: getEnv("MQTT_USERNAME", ""),
		MQTTPassword: getEnv("MQTT_PASSWORD", ""),

		UploadDir:     getEnv("UPLOAD_DIR", "./uploads"),
		PublicBaseURL: getEnv("PUBLIC_BASE_URL", "http://localhost:8081"),
	}
}

func getEnv(key, fallback string) string {
	if v, ok := os.LookupEnv(key); ok && v != "" {
		return v
	}
	return fallback
}

func getEnvAsInt(key string, fallback int) int {
	if v, ok := os.LookupEnv(key); ok {
		if i, err := strconv.Atoi(v); err == nil {
			return i
		}
	}
	return fallback
}
