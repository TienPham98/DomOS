package wallpapers

import (
	"fmt"
	"image"
	"image/color"
	_ "image/gif"
	"image/jpeg"
	_ "image/png"
	"math"
	"net/url"
	"os"
	"path/filepath"
	"time"

	"github.com/gofiber/fiber/v2"
	"github.com/google/uuid"
	"gorm.io/gorm"

	"github.com/domos/backend-go/pkg/response"
)

type Publisher interface {
	Publish(topic string, payload interface{}) error
}

type Handler struct {
	db           *gorm.DB
	uploadDir    string
	publicURL    string // Absolute public upload URL supplied by runtime configuration.
	publicOrigin string
	publisher    Publisher
}

func NewHandler(db *gorm.DB, uploadDir, publicURL string, publisher Publisher) *Handler {
	publicOrigin := publicURL
	if parsed, err := url.Parse(publicURL); err == nil && parsed.Scheme != "" && parsed.Host != "" {
		publicOrigin = parsed.Scheme + "://" + parsed.Host
	}
	return &Handler{db: db, uploadDir: uploadDir, publicURL: publicURL, publicOrigin: publicOrigin, publisher: publisher}
}

func resizeBilinear(src image.Image, targetWidth, targetHeight int) *image.RGBA {
	dst := image.NewRGBA(image.Rect(0, 0, targetWidth, targetHeight))
	srcBounds := src.Bounds()
	srcW := srcBounds.Dx()
	srcH := srcBounds.Dy()

	if srcW == 0 || srcH == 0 {
		return dst
	}

	scaleX := float64(srcW) / float64(targetWidth)
	scaleY := float64(srcH) / float64(targetHeight)

	for y := 0; y < targetHeight; y++ {
		sy := (float64(y)+0.5)*scaleY - 0.5
		iy := int(math.Floor(sy))
		fy := sy - float64(iy)
		iy1 := iy
		iy2 := iy + 1
		if iy1 < 0 {
			iy1 = 0
		}
		if iy2 >= srcH {
			iy2 = srcH - 1
		}

		for x := 0; x < targetWidth; x++ {
			sx := (float64(x)+0.5)*scaleX - 0.5
			ix := int(math.Floor(sx))
			fx := sx - float64(ix)
			ix1 := ix
			ix2 := ix + 1
			if ix1 < 0 {
				ix1 = 0
			}
			if ix2 >= srcW {
				ix2 = srcW - 1
			}

			r11, g11, b11, a11 := src.At(srcBounds.Min.X+ix1, srcBounds.Min.Y+iy1).RGBA()
			r21, g21, b21, a21 := src.At(srcBounds.Min.X+ix2, srcBounds.Min.Y+iy1).RGBA()
			r12, g12, b12, a12 := src.At(srcBounds.Min.X+ix1, srcBounds.Min.Y+iy2).RGBA()
			r22, g22, b22, a22 := src.At(srcBounds.Min.X+ix2, srcBounds.Min.Y+iy2).RGBA()

			w11 := (1.0 - fx) * (1.0 - fy)
			w21 := fx * (1.0 - fy)
			w12 := (1.0 - fx) * fy
			w22 := fx * fy

			r := uint8((float64(r11>>8)*w11 + float64(r21>>8)*w21 + float64(r12>>8)*w12 + float64(r22>>8)*w22))
			g := uint8((float64(g11>>8)*w11 + float64(g21>>8)*w21 + float64(g12>>8)*w12 + float64(g22>>8)*w22))
			b := uint8((float64(b11>>8)*w11 + float64(b21>>8)*w21 + float64(b12>>8)*w12 + float64(b22>>8)*w22))
			a := uint8((float64(a11>>8)*w11 + float64(a21>>8)*w21 + float64(a12>>8)*w12 + float64(a22>>8)*w22))

			dst.SetRGBA(x, y, color.RGBA{R: r, G: g, B: b, A: a})
		}
	}
	return dst
}

// Upload handles POST /api/wallpaper (multipart/form-data, field "file").
// Backend pre-resizes images to 320x240 JPEG Quality 85 for ESP32 LCD
// and creates a 107x80 thumbnail for the Web Dashboard.
func (h *Handler) Upload(c *fiber.Ctx) error {
	fileHeader, err := c.FormFile("file")
	if err != nil {
		return response.Error(c, fiber.StatusBadRequest, "file field is required")
	}

	id := uuid.NewString()
	wallpaperDir := filepath.Join(h.uploadDir, "wallpapers")
	if err := os.MkdirAll(wallpaperDir, 0755); err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to create upload directory")
	}

	tempFile := filepath.Join(wallpaperDir, fmt.Sprintf("raw_%s%s", id, filepath.Ext(fileHeader.Filename)))
	if err := c.SaveFile(fileHeader, tempFile); err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to save uploaded file")
	}
	defer os.Remove(tempFile)

	f, err := os.Open(tempFile)
	if err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to open uploaded file")
	}
	defer f.Close()

	srcImg, _, err := image.Decode(f)
	if err != nil {
		// Fallback: if format not decodable, save original directly
		destName := id + filepath.Ext(fileHeader.Filename)
		destPath := filepath.Join(wallpaperDir, destName)
		if err := c.SaveFile(fileHeader, destPath); err != nil {
			return response.Error(c, fiber.StatusInternalServerError, "failed to save fallback file")
		}
		wp := Wallpaper{
			ID:           id,
			Name:         fileHeader.Filename,
			URL:          fmt.Sprintf("%s/%s", h.publicURL, destName),
			ThumbnailURL: fmt.Sprintf("%s/%s", h.publicURL, destName),
			Width:        320,
			Height:       240,
			SizeBytes:    fileHeader.Size,
			CreatedAt:    time.Now(),
		}
		if err := h.db.Create(&wp).Error; err != nil {
			return response.Error(c, fiber.StatusInternalServerError, "failed to save wallpaper metadata")
		}
		return response.Success(c, fiber.StatusCreated, wp)
	}

	// 1. Generate ESP32 optimized background: 320x240 JPEG Quality 85
	bgImg := resizeBilinear(srcImg, 320, 240)
	bgName := fmt.Sprintf("bg_%s.jpg", id)
	bgPath := filepath.Join(wallpaperDir, bgName)
	bgOut, err := os.Create(bgPath)
	if err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to create optimized image")
	}
	if err := jpeg.Encode(bgOut, bgImg, &jpeg.Options{Quality: 85}); err != nil {
		bgOut.Close()
		return response.Error(c, fiber.StatusInternalServerError, "failed to encode optimized JPEG")
	}
	bgOut.Close()

	fi, _ := os.Stat(bgPath)
	var bgSizeBytes int64 = 0
	if fi != nil {
		bgSizeBytes = fi.Size()
	}

	// 2. Generate Dashboard thumbnail: 107x80 JPEG Quality 80
	thumbImg := resizeBilinear(srcImg, 107, 80)
	thumbName := fmt.Sprintf("thumb_%s.jpg", id)
	thumbPath := filepath.Join(wallpaperDir, thumbName)
	thumbOut, err := os.Create(thumbPath)
	if err == nil {
		_ = jpeg.Encode(thumbOut, thumbImg, &jpeg.Options{Quality: 80})
		thumbOut.Close()
	}

	wp := Wallpaper{
		ID:           id,
		Name:         fileHeader.Filename,
		URL:          fmt.Sprintf("%s/%s", h.publicURL, bgName),
		ThumbnailURL: fmt.Sprintf("%s/%s", h.publicURL, thumbName),
		Width:        320,
		Height:       240,
		SizeBytes:    bgSizeBytes,
		CreatedAt:    time.Now(),
	}

	if err := h.db.Create(&wp).Error; err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to save wallpaper metadata")
	}

	if h.publisher != nil {
		_ = h.publisher.Publish("domos/device/es3c28p-01/command", fiber.Map{
			"action":  "sync_wallpapers",
			"new_id":  wp.ID,
			"new_url": wp.URL,
		})
	}

	return response.Success(c, fiber.StatusCreated, wp)
}

// Get returns wallpaper metadata by ID.
func (h *Handler) Get(c *fiber.Ctx) error {
	var wp Wallpaper
	if err := h.db.First(&wp, "id = ?", c.Params("id")).Error; err != nil {
		return response.Error(c, fiber.StatusNotFound, "wallpaper not found")
	}
	return response.Success(c, fiber.StatusOK, wp)
}

// List returns all wallpapers.
func (h *Handler) List(c *fiber.Ctx) error {
	var list []Wallpaper
	if err := h.db.Order("created_at desc").Find(&list).Error; err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to list wallpapers")
	}
	return response.Success(c, fiber.StatusOK, list)
}

// GetSlideshow returns optimized wallpaper URLs for ESP32 slideshow preloader.
func (h *Handler) GetSlideshow(c *fiber.Ctx) error {
	var list []Wallpaper
	if err := h.db.Order("created_at desc").Find(&list).Error; err != nil {
		return response.Error(c, fiber.StatusInternalServerError, "failed to fetch slideshow list")
	}
	urls := make([]string, len(list))
	for i, wp := range list {
		if len(wp.URL) > 4 && (wp.URL[:7] == "http://" || wp.URL[:8] == "https://") {
			urls[i] = wp.URL
		} else if len(wp.URL) > 0 && wp.URL[0] == '/' {
			urls[i] = fmt.Sprintf("%s%s", h.publicOrigin, wp.URL)
		} else {
			urls[i] = fmt.Sprintf("%s/%s", h.publicOrigin, wp.URL)
		}
	}
	return response.Success(c, fiber.StatusOK, fiber.Map{
		"count":      len(urls),
		"wallpapers": urls,
	})
}

// Delete removes a wallpaper record and its associated bg & thumb files.
func (h *Handler) Delete(c *fiber.Ctx) error {
	var wp Wallpaper
	if err := h.db.First(&wp, "id = ?", c.Params("id")).Error; err == nil {
		bgPath := filepath.Join(h.uploadDir, "wallpapers", fmt.Sprintf("bg_%s.jpg", wp.ID))
		thumbPath := filepath.Join(h.uploadDir, "wallpapers", fmt.Sprintf("thumb_%s.jpg", wp.ID))
		_ = os.Remove(bgPath)
		_ = os.Remove(thumbPath)
		_ = h.db.Delete(&wp)

		if h.publisher != nil {
			_ = h.publisher.Publish("domos/device/es3c28p-01/command", fiber.Map{
				"action":      "sync_wallpapers",
				"deleted_id":  wp.ID,
				"deleted_url": wp.URL,
			})
		}
	}
	return response.Success(c, fiber.StatusOK, fiber.Map{"deleted": true})
}
