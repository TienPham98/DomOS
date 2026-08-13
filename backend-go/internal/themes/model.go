package themes

import "time"

// Theme is a named visual style (colors + wallpaper) that can be applied
// to one or more devices.
type Theme struct {
	ID           string    `gorm:"primaryKey" json:"id"`
	Name         string    `json:"name"`
	WallpaperURL string    `json:"wallpaper_url"`
	PrimaryColor string    `json:"primary_color"`
	CreatedAt    time.Time `json:"created_at"`
	UpdatedAt    time.Time `json:"updated_at"`
}
