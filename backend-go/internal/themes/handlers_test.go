package themes

import (
	"bytes"
	"net/http/httptest"
	"testing"

	"github.com/glebarez/sqlite"
	"github.com/gofiber/fiber/v2"
	"gorm.io/gorm"
)

func TestThemeCRUD(t *testing.T) {
	db, err := gorm.Open(sqlite.Open("file:"+t.Name()+"?mode=memory&cache=shared"), &gorm.Config{})
	if err != nil {
		t.Fatal(err)
	}
	if err := db.AutoMigrate(&Theme{}); err != nil {
		t.Fatal(err)
	}
	app := fiber.New()
	RegisterRoutes(app.Group("/api"), NewHandler(db))

	req := httptest.NewRequest("POST", "/api/themes", bytes.NewBufferString(`{"name":"Night","primary_color":"#001122"}`))
	req.Header.Set("Content-Type", "application/json")
	response, _ := app.Test(req)
	if response.StatusCode != fiber.StatusCreated {
		t.Fatalf("create status=%d", response.StatusCode)
	}

	response, _ = app.Test(httptest.NewRequest("GET", "/api/themes", nil))
	if response.StatusCode != fiber.StatusOK {
		t.Fatalf("list status=%d", response.StatusCode)
	}
}
