package wallpapers

import "time"

// Wallpaper is an uploaded background image devices download over HTTP
// and store in their onboard flash (no SD card on the target hardware).
type Wallpaper struct {
	ID           string    `gorm:"primaryKey" json:"id"`
	Name         string    `json:"name"`
	URL          string    `json:"url"`
	ThumbnailURL string    `json:"thumbnail_url"`
	Width        int       `json:"width"`
	Height       int       `json:"height"`
	SizeBytes    int64     `json:"size_bytes"`
	CreatedAt    time.Time `json:"created_at"`
}

