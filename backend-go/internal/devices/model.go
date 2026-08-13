package devices

import "time"

// Device represents a single physical DomOS terminal (ESP32-S3 board).
type Device struct {
	ID        string    `gorm:"primaryKey" json:"id"`
	OwnerID   string    `gorm:"index" json:"owner_id"`
	Name      string    `json:"name"`
	Firmware  string    `json:"firmware"`
	Online    bool      `gorm:"default:false" json:"online"`
	LastSeen  time.Time `json:"last_seen"`
	ThemeID   string    `json:"theme_id"`
	IPAddress string    `json:"ip_address"`
	CreatedAt time.Time `json:"created_at"`
	UpdatedAt time.Time `json:"updated_at"`
}

// StatePayload is the shape published/received on the
// domos/device/{id}/state MQTT topic.
type StatePayload struct {
	Online    bool      `json:"online"`
	Firmware  string    `json:"firmware,omitempty"`
	IPAddress string    `json:"ip,omitempty"`
	Timestamp time.Time `json:"timestamp"`
}
