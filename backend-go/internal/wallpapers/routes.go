package wallpapers

import "github.com/gofiber/fiber/v2"

// RegisterRoutes wires up /api/wallpaper(s) endpoints.
func RegisterRoutes(router fiber.Router, h *Handler) {
	router.Get("/wallpapers", h.List)
	router.Get("/wallpapers/slideshow", h.GetSlideshow)
	router.Post("/wallpaper", h.Upload)
	router.Get("/wallpaper/:id", h.Get)
	router.Delete("/wallpaper/:id", h.Delete)
}

