package main

import (
	"strings"

	"github.com/gofiber/fiber/v2"
	"github.com/gofiber/fiber/v2/middleware/cors"
	fiberlogger "github.com/gofiber/fiber/v2/middleware/logger"
	"github.com/gofiber/fiber/v2/middleware/recover"
	fiberws "github.com/gofiber/websocket/v2"

	"github.com/domos/backend-go/internal/auth"
	"github.com/domos/backend-go/internal/devices"
	"github.com/domos/backend-go/internal/mqtt"
	"github.com/domos/backend-go/internal/ota"
	"github.com/domos/backend-go/internal/themes"
	"github.com/domos/backend-go/internal/wallpapers"
	"github.com/domos/backend-go/internal/websocket"
	"github.com/domos/backend-go/pkg/config"
	"github.com/domos/backend-go/pkg/database"
	"github.com/domos/backend-go/pkg/logger"
)

func main() {
	cfg := config.Load()

	// --- Database -----------------------------------------------------
	db, err := database.Connect(cfg.DatabaseURL)
	if err != nil {
		logger.Error.Fatalf("failed to connect to database: %v", err)
	}
	if err := database.AutoMigrate(db); err != nil {
		logger.Error.Fatalf("failed to run migrations: %v", err)
	}
	logger.Info.Println("database connected and migrated")

	// --- WebSocket hub (needed by MQTT client for broadcasting) -------
	hub := websocket.NewHub()

	// --- Device repository (needed by both HTTP handlers and MQTT) ----
	deviceRepo := devices.NewRepository(db)

	// --- MQTT client ----------------------------------------------------
	mqttClient, err := mqtt.New(mqtt.Config{
		Broker:   cfg.MQTTBroker,
		ClientID: cfg.MQTTClientID,
		Username: cfg.MQTTUsername,
		Password: cfg.MQTTPassword,
	}, deviceRepo, hub)
	if err != nil {
		// Non-fatal: the API/dashboard should still work even if the
		// broker is temporarily unreachable; commands just won't reach
		// devices until it reconnects.
		logger.Warn.Printf("mqtt: initial connect failed, continuing without it: %v", err)
	} else {
		defer mqttClient.Close()
	}

	// --- Fiber app ------------------------------------------------------
	app := fiber.New(fiber.Config{
		AppName:      "DomOS Backend",
		ErrorHandler: defaultErrorHandler,
		BodyLimit:    64 * 1024 * 1024, // 64MB, comfortably covers firmware.bin/wallpaper uploads
	})

	app.Use(recover.New())
	app.Use(fiberlogger.New())
	app.Use(cors.New(cors.Config{
		AllowOrigins: "*",
		AllowHeaders: "Origin, Content-Type, Accept, Authorization",
	}))

	// Serve uploaded wallpapers/firmware as static files.
	app.Static("/uploads", cfg.UploadDir)

	app.Get("/", func(c *fiber.Ctx) error {
		return c.JSON(fiber.Map{"status": "ok", "app": "DomOS Backend API", "version": "1.0.0"})
	})

	app.Get("/healthz", func(c *fiber.Ctx) error {
		return c.JSON(fiber.Map{"status": "ok"})
	})

	// --- WebSocket upgrade guard + route ---------------------------------
	app.Use("/ws", func(c *fiber.Ctx) error {
		if fiberws.IsWebSocketUpgrade(c) {
			c.Locals("allowed", true)
			return c.Next()
		}
		return fiber.ErrUpgradeRequired
	})
	websocket.RegisterRoutes(app, hub)

	// --- REST API ---------------------------------------------------------
	api := app.Group("/api")
	publicURLBase := strings.TrimRight(cfg.PublicBaseURL, "/") + "/uploads"

	authHandler := auth.NewHandler(db, cfg.JWTSecret, cfg.JWTExpiryHr)
	auth.RegisterRoutes(api, authHandler, cfg.JWTSecret)
	// Devices need a small public, read-only firmware discovery endpoint at
	// boot. Firmware binaries must still be signed before production OTA.
	otaHandler := ota.NewHandler(db, cfg.UploadDir, publicURLBase+"/firmware", mqttClient)
	api.Get("/firmware/latest", otaHandler.Latest)

	wallpaperHandler := wallpapers.NewHandler(db, cfg.UploadDir, publicURLBase+"/wallpapers", mqttClient)
	wallpapers.RegisterRoutes(api, wallpaperHandler)

	// Everything below requires a valid JWT.
	protected := api.Group("", auth.Protected(cfg.JWTSecret))

	deviceHandler := devices.NewHandler(deviceRepo, mqttClient)
	devices.RegisterRoutes(protected, deviceHandler)

	themeHandler := themes.NewHandler(db)
	themes.RegisterRoutes(protected, themeHandler)

	ota.RegisterRoutes(protected, otaHandler)

	logger.Info.Printf("DomOS backend listening on :%s (env=%s)", cfg.AppPort, cfg.AppEnv)
	if err := app.Listen(":" + cfg.AppPort); err != nil {
		logger.Error.Fatalf("server stopped: %v", err)
	}
}

func defaultErrorHandler(c *fiber.Ctx, err error) error {
	code := fiber.StatusInternalServerError
	if e, ok := err.(*fiber.Error); ok {
		code = e.Code
	}
	return c.Status(code).JSON(fiber.Map{
		"success": false,
		"error":   err.Error(),
	})
}
