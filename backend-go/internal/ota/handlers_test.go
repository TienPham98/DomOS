package ota

import (
	"encoding/json"
	"net/http/httptest"
	"testing"
	"time"

	"github.com/glebarez/sqlite"
	"github.com/gofiber/fiber/v2"
	"gorm.io/gorm"
)

func TestLatestReturnsNewestFirmware(t *testing.T) {
	db, err := gorm.Open(sqlite.Open("file:"+t.Name()+"?mode=memory&cache=shared"), &gorm.Config{})
	if err != nil {
		t.Fatal(err)
	}
	if err := db.AutoMigrate(&FirmwareVersion{}); err != nil {
		t.Fatal(err)
	}
	db.Create(&FirmwareVersion{ID: "old", Version: "0.4.0", CreatedAt: time.Now()})
	db.Create(&FirmwareVersion{ID: "new", Version: "0.5.0", CreatedAt: time.Now().Add(time.Second)})
	app := fiber.New()
	app.Get("/api/ota/latest", NewHandler(db, t.TempDir(), "", nil).Latest)
	response, _ := app.Test(httptest.NewRequest("GET", "/api/ota/latest", nil))
	var envelope struct {
		Data FirmwareVersion `json:"data"`
	}
	if err := json.NewDecoder(response.Body).Decode(&envelope); err != nil {
		t.Fatal(err)
	}
	if envelope.Data.Version != "0.5.0" {
		t.Fatalf("got version %s", envelope.Data.Version)
	}
}
