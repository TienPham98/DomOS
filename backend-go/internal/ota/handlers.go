package ota

import (
	"fmt"
	"path/filepath"
	"time"

	"github.com/gofiber/fiber/v2"
	"github.com/google/uuid"
	"gorm.io/gorm"

	"github.com/domos/backend-go/pkg/response"
)

// Publisher is the minimal MQTT capability the OTA module needs.
type Publisher interface {
	Publish(topic string, payload interface{}) error
}

type Handler struct {
	db        *gorm.DB
	uploadDir string
	publicURL string
	publisher Publisher
}

func NewHandler(db *gorm.DB, uploadDir, publicURL string, publisher Publisher) *Handler {
	return &Handler{db: db, uploadDir: uploadDir, publicURL: publicURL, publisher: publisher}
}

type uploadMeta struct {
	Version string `form:"version"`
	Notes   string `form:"notes"`
}

// Upload handles POST /api/ota (multipart/form-data: "file" + "version" + "notes").
// On success it broadcasts an OTA notification to every device over MQTT.
func (h *Handler) Upload(c *fiber.Ctx) error {
	version := c.FormValue("version")
	if version == "" {
		return response.Error(c, fiber.StatusBadRequest, "version is required")
	}

	fileHeader, err := c.FormFile("file")
	if err != nil {
		return response.Error(c, fiber.StatusBadRequest, "file field is required")
	}

	ext := filepath.Ext(fileHeader.Filename)
	if ext == "" {
		ext = ".bin"
	}
	id := uuid.NewString()
	storedName := id + ext
	destPath := filepath.Join(h.uploadDir, "firmware", storedName)

	if err := c.SaveFile(fileHeader, destPath); err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to save firmware file")
	}

	fw := FirmwareVersion{
		ID:        id,
		Version:   version,
		URL:       fmt.Sprintf("%s/%s", h.publicURL, storedName),
		SizeBytes: fileHeader.Size,
		Notes:     c.FormValue("notes"),
		CreatedAt: time.Now(),
	}

	if err := h.db.Create(&fw).Error; err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to save firmware metadata (version may already exist)")
	}

	if h.publisher != nil {
		_ = h.publisher.Publish("domos/server/broadcast", fiber.Map{
			"cmd":     "ota_available",
			"version": fw.Version,
			"url":     fw.URL,
		})
	}

	return response.Success(c, fiber.StatusCreated, fw)
}

// Latest returns the most recently uploaded firmware (GET /api/ota/latest),
// which is what devices poll on boot to decide whether to update.
func (h *Handler) Latest(c *fiber.Ctx) error {
	var fw FirmwareVersion
	if err := h.db.Order("created_at desc").First(&fw).Error; err != nil {
		return response.Error(c, fiber.StatusNotFound, "no firmware versions available")
	}
	return response.Success(c, fiber.StatusOK, fw)
}

// List returns every uploaded firmware version.
func (h *Handler) List(c *fiber.Ctx) error {
	var list []FirmwareVersion
	if err := h.db.Order("created_at desc").Find(&list).Error; err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to list firmware versions")
	}
	return response.Success(c, fiber.StatusOK, list)
}
