package themes

import "github.com/gofiber/fiber/v2"

// RegisterRoutes wires up /api/theme(s) endpoints.
func RegisterRoutes(router fiber.Router, h *Handler) {
	router.Get("/themes", h.List)
	router.Post("/themes", h.Create)

	router.Get("/theme/:id", h.Get)
	router.Put("/theme/:id", h.Update)
	router.Delete("/theme/:id", h.Delete)
}
