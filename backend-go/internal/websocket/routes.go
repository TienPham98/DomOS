package websocket

import (
	"github.com/gofiber/fiber/v2"
	fiberws "github.com/gofiber/websocket/v2"
)

// RegisterRoutes mounts GET /ws, upgrading to a websocket connection for
// real-time dashboard updates (device state, OTA progress, AI events).
func RegisterRoutes(router fiber.Router, hub *Hub) {
	router.Get("/ws", fiberws.New(func(c *fiberws.Conn) {
		hub.Handle(c)
	}))
}
