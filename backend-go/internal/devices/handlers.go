package devices

import (
	"time"

	"github.com/gofiber/fiber/v2"
	"github.com/google/uuid"

	"github.com/domos/backend-go/pkg/response"
)

// Publisher is the minimal MQTT capability the devices module needs.
// Implemented by internal/mqtt.Client; kept as an interface here to avoid
// a circular import between devices <-> mqtt.
type Publisher interface {
	Publish(topic string, payload interface{}) error
}

type Handler struct {
	repo      *Repository
	publisher Publisher
}

func NewHandler(repo *Repository, publisher Publisher) *Handler {
	return &Handler{repo: repo, publisher: publisher}
}

// List returns devices owned by the authenticated user (or all devices
// for admins hitting this without an owner filter).
func (h *Handler) List(c *fiber.Ctx) error {
	ownerID := c.Query("owner_id")
	list, err := h.repo.List(ownerID)
	if err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to list devices")
	}
	return response.Success(c, fiber.StatusOK, list)
}

// Get returns a single device by ID.
func (h *Handler) Get(c *fiber.Ctx) error {
	d, err := h.repo.Get(c.Params("id"))
	if err != nil {
		return response.Error(c, fiber.StatusNotFound, "device not found")
	}
	return response.Success(c, fiber.StatusOK, d)
}

type createDeviceRequest struct {
	Name string `json:"name"`
}

// Create registers a new device, typically called by the dashboard when
// a user pairs a new board.
func (h *Handler) Create(c *fiber.Ctx) error {
	var req createDeviceRequest
	if err := c.BodyParser(&req); err != nil {
		return response.Error(c, fiber.StatusBadRequest, "invalid request body")
	}
	if req.Name == "" {
		return response.Error(c, fiber.StatusBadRequest, "name is required")
	}

	d := &Device{
		ID:        uuid.NewString(),
		Name:      req.Name,
		CreatedAt: time.Now(),
		UpdatedAt: time.Now(),
	}

	if err := h.repo.Create(d); err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to create device")
	}

	return response.Success(c, fiber.StatusCreated, d)
}

type updateDeviceRequest struct {
	Name    *string `json:"name"`
	ThemeID *string `json:"theme_id"`
}

// Update patches a device's mutable fields. When ThemeID changes, an
// update_theme command is published to the device over MQTT.
func (h *Handler) Update(c *fiber.Ctx) error {
	d, err := h.repo.Get(c.Params("id"))
	if err != nil {
		return response.Error(c, fiber.StatusNotFound, "device not found")
	}

	var req updateDeviceRequest
	if err := c.BodyParser(&req); err != nil {
		return response.Error(c, fiber.StatusBadRequest, "invalid request body")
	}

	themeChanged := false
	if req.Name != nil {
		d.Name = *req.Name
	}
	if req.ThemeID != nil && *req.ThemeID != d.ThemeID {
		d.ThemeID = *req.ThemeID
		themeChanged = true
	}
	d.UpdatedAt = time.Now()

	if err := h.repo.Update(d); err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to update device")
	}

	if themeChanged && h.publisher != nil {
		_ = h.publisher.Publish("domos/device/"+d.ID+"/theme", fiber.Map{
			"cmd":   "update_theme",
			"theme": d.ThemeID,
		})
	}

	return response.Success(c, fiber.StatusOK, d)
}

// Delete removes a device.
func (h *Handler) Delete(c *fiber.Ctx) error {
	if err := h.repo.Delete(c.Params("id")); err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to delete device")
	}
	return response.Success(c, fiber.StatusOK, fiber.Map{"deleted": true})
}
