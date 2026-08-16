package ota

import "time"

// FirmwareVersion is one uploaded firmware.bin the ESP32 fleet can
// download and flash over the air.
type FirmwareVersion struct {
	ID        string    `gorm:"primaryKey" json:"id"`
	Version   string    `gorm:"uniqueIndex" json:"version"`
	URL       string    `json:"url"`
	SizeBytes int64     `json:"size_bytes"`
	Notes     string    `json:"notes"`
	CreatedAt time.Time `json:"created_at"`
}
