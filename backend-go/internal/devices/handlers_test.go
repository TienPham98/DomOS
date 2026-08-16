package devices

import (
	"bytes"
	"encoding/json"
	"net/http/httptest"
	"testing"

	"github.com/glebarez/sqlite"
	"github.com/gofiber/fiber/v2"
	"gorm.io/gorm"
)

type publishedMessage struct {
	topic   string
	payload interface{}
}

type fakePublisher struct{ messages []publishedMessage }

func (p *fakePublisher) Publish(topic string, payload interface{}) error {
	p.messages = append(p.messages, publishedMessage{topic: topic, payload: payload})
	return nil
}

func deviceTestDB(t *testing.T) *gorm.DB {
	t.Helper()
	db, err := gorm.Open(sqlite.Open("file:"+t.Name()+"?mode=memory&cache=shared"), &gorm.Config{})
	if err != nil {
		t.Fatal(err)
	}
	if err := db.AutoMigrate(&Device{}); err != nil {
		t.Fatal(err)
	}
	return db
}

func TestDeviceCRUDAndThemeNotification(t *testing.T) {
	db := deviceTestDB(t)
	publisher := &fakePublisher{}
	app := fiber.New()
	RegisterRoutes(app.Group("/api"), NewHandler(NewRepository(db), publisher))

	create := httptest.NewRequest("POST", "/api/devices", bytes.NewBufferString(`{"name":"Desk Dom"}`))
	create.Header.Set("Content-Type", "application/json")
	response, _ := app.Test(create)
	if response.StatusCode != fiber.StatusCreated {
		t.Fatalf("create status=%d", response.StatusCode)
	}
	var created struct {
		Data Device `json:"data"`
	}
	if err := json.NewDecoder(response.Body).Decode(&created); err != nil {
		t.Fatal(err)
	}
	if created.Data.ID == "" || created.Data.Name != "Desk Dom" {
		t.Fatalf("bad device: %+v", created.Data)
	}

	update := httptest.NewRequest("PUT", "/api/device/"+created.Data.ID, bytes.NewBufferString(`{"theme_id":"night"}`))
	update.Header.Set("Content-Type", "application/json")
	response, _ = app.Test(update)
	if response.StatusCode != fiber.StatusOK {
		t.Fatalf("update status=%d", response.StatusCode)
	}
	if len(publisher.messages) != 1 || publisher.messages[0].topic != "domos/device/"+created.Data.ID+"/theme" {
		t.Fatalf("theme update was not published: %+v", publisher.messages)
	}

	response, _ = app.Test(httptest.NewRequest("GET", "/api/devices", nil))
	if response.StatusCode != fiber.StatusOK {
		t.Fatalf("list status=%d", response.StatusCode)
	}
	response, _ = app.Test(httptest.NewRequest("DELETE", "/api/device/"+created.Data.ID, nil))
	if response.StatusCode != fiber.StatusOK {
		t.Fatalf("delete status=%d", response.StatusCode)
	}
	response, _ = app.Test(httptest.NewRequest("GET", "/api/device/"+created.Data.ID, nil))
	if response.StatusCode != fiber.StatusNotFound {
		t.Fatalf("deleted device status=%d", response.StatusCode)
	}
}

func TestCreateDeviceValidatesName(t *testing.T) {
	app := fiber.New()
	RegisterRoutes(app.Group("/api"), NewHandler(NewRepository(deviceTestDB(t)), nil))
	req := httptest.NewRequest("POST", "/api/devices", bytes.NewBufferString(`{"name":""}`))
	req.Header.Set("Content-Type", "application/json")
	response, _ := app.Test(req)
	if response.StatusCode != fiber.StatusBadRequest {
		t.Fatalf("expected 400, got %d", response.StatusCode)
	}
}
