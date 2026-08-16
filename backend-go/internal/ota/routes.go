package ota

import "github.com/gofiber/fiber/v2"

// RegisterRoutes wires up /api/ota endpoints.
func RegisterRoutes(router fiber.Router, h *Handler) {
	router.Get("/ota", h.List)
	router.Post("/ota", h.Upload)
	router.Get("/ota/latest", h.Latest)
}
