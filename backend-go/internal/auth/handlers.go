package auth

import (
	"strings"
	"time"

	"github.com/gofiber/fiber/v2"
	"github.com/google/uuid"
	"golang.org/x/crypto/bcrypt"
	"gorm.io/gorm"

	"github.com/domos/backend-go/pkg/response"
)

type Handler struct {
	db          *gorm.DB
	jwtSecret   string
	jwtExpiryHr int
}

func NewHandler(db *gorm.DB, jwtSecret string, jwtExpiryHr int) *Handler {
	return &Handler{db: db, jwtSecret: jwtSecret, jwtExpiryHr: jwtExpiryHr}
}

type registerRequest struct {
	Email    string `json:"email"`
	Password string `json:"password"`
	Name     string `json:"name"`
}

type loginRequest struct {
	Email    string `json:"email"`
	Password string `json:"password"`
}

type authResponse struct {
	Token string `json:"token"`
	User  User   `json:"user"`
}

// Register creates a new user account and returns a signed JWT.
func (h *Handler) Register(c *fiber.Ctx) error {
	var req registerRequest
	if err := c.BodyParser(&req); err != nil {
		return response.Error(c, fiber.StatusBadRequest, "invalid request body")
	}

	req.Email = strings.ToLower(strings.TrimSpace(req.Email))
	if req.Email == "" || len(req.Password) < 8 {
		return response.Error(c, fiber.StatusBadRequest, "email is required and password must be at least 8 characters")
	}

	var existing User
	if err := h.db.Where("email = ?", req.Email).First(&existing).Error; err == nil {
		return response.Error(c, fiber.StatusConflict, "an account with this email already exists")
	}

	hash, err := bcrypt.GenerateFromPassword([]byte(req.Password), bcrypt.DefaultCost)
	if err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to hash password")
	}

	user := User{
		ID:           uuid.NewString(),
		Email:        req.Email,
		PasswordHash: string(hash),
		Name:         req.Name,
		Role:         RoleUser,
		CreatedAt:    time.Now(),
		UpdatedAt:    time.Now(),
	}

	if err := h.db.Create(&user).Error; err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to create user")
	}

	token, err := GenerateToken(&user, h.jwtSecret, h.jwtExpiryHr)
	if err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to generate token")
	}

	return response.Success(c, fiber.StatusCreated, authResponse{Token: token, User: user})
}

// Login validates credentials and returns a signed JWT.
func (h *Handler) Login(c *fiber.Ctx) error {
	var req loginRequest
	if err := c.BodyParser(&req); err != nil {
		return response.Error(c, fiber.StatusBadRequest, "invalid request body")
	}

	req.Email = strings.ToLower(strings.TrimSpace(req.Email))

	var user User
	if err := h.db.Where("email = ?", req.Email).First(&user).Error; err != nil {
		return response.Error(c, fiber.StatusUnauthorized, "invalid email or password")
	}

	if err := bcrypt.CompareHashAndPassword([]byte(user.PasswordHash), []byte(req.Password)); err != nil {
		return response.Error(c, fiber.StatusUnauthorized, "invalid email or password")
	}

	token, err := GenerateToken(&user, h.jwtSecret, h.jwtExpiryHr)
	if err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to generate token")
	}

	return response.Success(c, fiber.StatusOK, authResponse{Token: token, User: user})
}

// Me returns the currently authenticated user's profile.
func (h *Handler) Me(c *fiber.Ctx) error {
	userID := UserIDFromCtx(c)

	var user User
	if err := h.db.First(&user, "id = ?", userID).Error; err != nil {
		return response.Error(c, fiber.StatusNotFound, "user not found")
	}

	return response.Success(c, fiber.StatusOK, user)
}
