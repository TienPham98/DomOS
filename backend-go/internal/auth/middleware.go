package auth

import (
	"strings"

	"github.com/gofiber/fiber/v2"

	"github.com/domos/backend-go/pkg/response"
)

const (
	localsUserID = "user_id"
	localsEmail  = "email"
	localsRole   = "role"
)

// Protected returns Fiber middleware that requires a valid Bearer JWT.
// On success it stores user_id / email / role in the request locals so
// downstream handlers can read them without re-parsing the token.
func Protected(secret string) fiber.Handler {
	return func(c *fiber.Ctx) error {
		header := c.Get("Authorization")
		if header == "" || !strings.HasPrefix(header, "Bearer ") {
			return response.Error(c, fiber.StatusUnauthorized, "missing or malformed bearer token")
		}

		tokenStr := strings.TrimPrefix(header, "Bearer ")
		claims, err := ParseToken(tokenStr, secret)
		if err != nil {
			return response.Error(c, fiber.StatusUnauthorized, "invalid or expired token")
		}

		c.Locals(localsUserID, claims.UserID)
		c.Locals(localsEmail, claims.Email)
		c.Locals(localsRole, claims.Role)

		return c.Next()
	}
}

// RequireRole returns middleware that additionally checks the caller has
// one of the given roles. Must be used after Protected().
func RequireRole(roles ...Role) fiber.Handler {
	allowed := make(map[Role]bool, len(roles))
	for _, r := range roles {
		allowed[r] = true
	}

	return func(c *fiber.Ctx) error {
		role, _ := c.Locals(localsRole).(Role)
		if !allowed[role] {
			return response.Error(c, fiber.StatusForbidden, "insufficient permissions")
		}
		return c.Next()
	}
}

// UserIDFromCtx is a small helper for handlers to read the authenticated user.
func UserIDFromCtx(c *fiber.Ctx) string {
	id, _ := c.Locals(localsUserID).(string)
	return id
}
