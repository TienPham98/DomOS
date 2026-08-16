package wallpapers

import (
	"encoding/json"
	"image"
	"image/color"
	"net/http/httptest"
	"testing"
	"time"

	"github.com/glebarez/sqlite"
	"github.com/gofiber/fiber/v2"
	"gorm.io/gorm"
)

func wallpaperTestDB(t *testing.T) *gorm.DB {
	t.Helper()
	db, err := gorm.Open(sqlite.Open("file:"+t.Name()+"?mode=memory&cache=shared"), &gorm.Config{})
	if err != nil {
		t.Fatal(err)
	}
	if err := db.AutoMigrate(&Wallpaper{}); err != nil {
		t.Fatal(err)
	}
	return db
}

func TestResizeBilinearProducesLCDAndThumbnailDimensions(t *testing.T) {
	source := image.NewRGBA(image.Rect(0, 0, 2, 2))
	source.Set(0, 0, color.RGBA{R: 255, A: 255})
	if got := resizeBilinear(source, 320, 240).Bounds(); got.Dx() != 320 || got.Dy() != 240 {
		t.Fatalf("unexpected LCD dimensions: %v", got)
	}
	if got := resizeBilinear(source, 107, 80).Bounds(); got.Dx() != 107 || got.Dy() != 80 {
		t.Fatalf("unexpected thumbnail dimensions: %v", got)
	}
}

func TestSlideshowNormalizesURLsToConfiguredHost(t *testing.T) {
	db := wallpaperTestDB(t)
	db.Create(&Wallpaper{ID: "relative", URL: "/uploads/wallpapers/a.jpg", CreatedAt: time.Now()})
	db.Create(&Wallpaper{ID: "absolute", URL: "https://cdn.example/a.jpg", CreatedAt: time.Now().Add(time.Second)})
	app := fiber.New()
	app.Get("/api/wallpapers/slideshow", NewHandler(db, t.TempDir(), "https://api.example", nil).GetSlideshow)
	response, _ := app.Test(httptest.NewRequest("GET", "/api/wallpapers/slideshow", nil))
	var envelope struct {
		Data struct {
			Count      int      `json:"count"`
			Wallpapers []string `json:"wallpapers"`
		} `json:"data"`
	}
	if err := json.NewDecoder(response.Body).Decode(&envelope); err != nil {
		t.Fatal(err)
	}
	if envelope.Data.Count != 2 {
		t.Fatalf("expected 2 wallpapers, got %d", envelope.Data.Count)
	}
	if envelope.Data.Wallpapers[1] != "https://api.example/uploads/wallpapers/a.jpg" {
		t.Fatalf("relative URL was not normalized: %+v", envelope.Data.Wallpapers)
	}
}
