package themes

import (
	"time"

	"github.com/gofiber/fiber/v2"
	"github.com/google/uuid"
	"gorm.io/gorm"

	"github.com/domos/backend-go/pkg/response"
)

type Handler struct {
	db *gorm.DB
}

func NewHandler(db *gorm.DB) *Handler {
	return &Handler{db: db}
}

// List returns every theme available to assign to devices.
func (h *Handler) List(c *fiber.Ctx) error {
	var list []Theme
	if err := h.db.Order("created_at desc").Find(&list).Error; err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to list themes")
	}
	return response.Success(c, fiber.StatusOK, list)
}

// Get returns a single theme by ID.
func (h *Handler) Get(c *fiber.Ctx) error {
	var t Theme
	if err := h.db.First(&t, "id = ?", c.Params("id")).Error; err != nil {
		return response.Error(c, fiber.StatusNotFound, "theme not found")
	}
	return response.Success(c, fiber.StatusOK, t)
}

type upsertThemeRequest struct {
	Name         string `json:"name"`
	WallpaperURL string `json:"wallpaper_url"`
	PrimaryColor string `json:"primary_color"`
}

// Create adds a new theme.
func (h *Handler) Create(c *fiber.Ctx) error {
	var req upsertThemeRequest
	if err := c.BodyParser(&req); err != nil {
		return response.Error(c, fiber.StatusBadRequest, "invalid request body")
	}
	if req.Name == "" {
		return response.Error(c, fiber.StatusBadRequest, "name is required")
	}

	t := Theme{
		ID:           uuid.NewString(),
		Name:         req.Name,
		WallpaperURL: req.WallpaperURL,
		PrimaryColor: req.PrimaryColor,
		CreatedAt:    time.Now(),
		UpdatedAt:    time.Now(),
	}

	if err := h.db.Create(&t).Error; err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to create theme")
	}

	return response.Success(c, fiber.StatusCreated, t)
}

// Update replaces a theme's fields (matches PUT /api/theme/:id in the roadmap).
func (h *Handler) Update(c *fiber.Ctx) error {
	var t Theme
	if err := h.db.First(&t, "id = ?", c.Params("id")).Error; err != nil {
		return response.Error(c, fiber.StatusNotFound, "theme not found")
	}

	var req upsertThemeRequest
	if err := c.BodyParser(&req); err != nil {
		return response.Error(c, fiber.StatusBadRequest, "invalid request body")
	}

	if req.Name != "" {
		t.Name = req.Name
	}
	if req.WallpaperURL != "" {
		t.WallpaperURL = req.WallpaperURL
	}
	if req.PrimaryColor != "" {
		t.PrimaryColor = req.PrimaryColor
	}
	t.UpdatedAt = time.Now()

	if err := h.db.Save(&t).Error; err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to update theme")
	}

	return response.Success(c, fiber.StatusOK, t)
}

// Delete removes a theme.
func (h *Handler) Delete(c *fiber.Ctx) error {
	if err := h.db.Delete(&Theme{}, "id = ?", c.Params("id")).Error; err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to delete theme")
	}
	return response.Success(c, fiber.StatusOK, fiber.Map{"deleted": true})
}
