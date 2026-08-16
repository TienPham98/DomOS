package auth

import "github.com/gofiber/fiber/v2"

// RegisterRoutes wires up /api/auth/* endpoints.
func RegisterRoutes(router fiber.Router, h *Handler, jwtSecret string) {
	g := router.Group("/auth")
	g.Post("/register", h.Register)
	g.Post("/login", h.Login)
	g.Get("/me", Protected(jwtSecret), h.Me)
}
