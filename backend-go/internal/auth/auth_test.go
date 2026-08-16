package auth

import (
	"bytes"
	"encoding/json"
	"net/http/httptest"
	"testing"

	"github.com/glebarez/sqlite"
	"github.com/gofiber/fiber/v2"
	"gorm.io/gorm"
)

func authTestDB(t *testing.T) *gorm.DB {
	t.Helper()
	db, err := gorm.Open(sqlite.Open("file:"+t.Name()+"?mode=memory&cache=shared"), &gorm.Config{})
	if err != nil {
		t.Fatal(err)
	}
	if err := db.AutoMigrate(&User{}); err != nil {
		t.Fatal(err)
	}
	return db
}

func TestRegisterLoginAndProtectedProfile(t *testing.T) {
	const secret = "test-secret"
	app := fiber.New()
	handler := NewHandler(authTestDB(t), secret, 1)
	RegisterRoutes(app.Group("/api"), handler, secret)

	body := []byte(`{"email":" User@Example.com ","password":"password123","name":"Dom User"}`)
	registerRequest := httptest.NewRequest("POST", "/api/auth/register", bytes.NewReader(body))
	registerRequest.Header.Set("Content-Type", "application/json")
	response, err := app.Test(registerRequest)
	if err != nil || response.StatusCode != fiber.StatusCreated {
		t.Fatalf("register failed: status=%v err=%v", response.StatusCode, err)
	}
	var envelope struct {
		Success bool `json:"success"`
		Data    struct {
			Token string `json:"token"`
			User  User   `json:"user"`
		} `json:"data"`
	}
	if err := json.NewDecoder(response.Body).Decode(&envelope); err != nil {
		t.Fatal(err)
	}
	if !envelope.Success || envelope.Data.Token == "" || envelope.Data.User.Email != "user@example.com" {
		t.Fatalf("unexpected register response: %+v", envelope)
	}

	req := httptest.NewRequest("GET", "/api/auth/me", nil)
	req.Header.Set("Authorization", "Bearer "+envelope.Data.Token)
	response, err = app.Test(req)
	if err != nil || response.StatusCode != fiber.StatusOK {
		t.Fatalf("protected profile failed: status=%v err=%v", response.StatusCode, err)
	}

	response, _ = app.Test(httptest.NewRequest("GET", "/api/auth/me", nil))
	if response.StatusCode != fiber.StatusUnauthorized {
		t.Fatalf("expected unauthorized without token, got %d", response.StatusCode)
	}
}

func TestTokenCannotBeParsedWithDifferentSecret(t *testing.T) {
	user := &User{ID: "u1", Email: "dom@example.com", Role: RoleUser}
	token, err := GenerateToken(user, "correct", 1)
	if err != nil {
		t.Fatal(err)
	}
	if _, err := ParseToken(token, "wrong"); err == nil {
		t.Fatal("token signed with a different secret must be rejected")
	}
}
