package devices

import "github.com/gofiber/fiber/v2"

// RegisterRoutes wires up /api/devices and /api/device/:id endpoints,
// matching the paths from the project roadmap.
func RegisterRoutes(router fiber.Router, h *Handler) {
	router.Get("/devices", h.List)
	router.Post("/devices", h.Create)

	router.Get("/device/:id", h.Get)
	router.Put("/device/:id", h.Update)
	router.Delete("/device/:id", h.Delete)
}
