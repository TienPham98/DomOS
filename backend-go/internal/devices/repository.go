package devices

import (
	"time"

	"gorm.io/gorm"
)

type Repository struct {
	db *gorm.DB
}

func NewRepository(db *gorm.DB) *Repository {
	return &Repository{db: db}
}

func (r *Repository) List(ownerID string) ([]Device, error) {
	var devices []Device
	q := r.db.Order("created_at desc")
	if ownerID != "" {
		q = q.Where("owner_id = ?", ownerID)
	}
	err := q.Find(&devices).Error
	return devices, err
}

func (r *Repository) Get(id string) (*Device, error) {
	var d Device
	if err := r.db.First(&d, "id = ?", id).Error; err != nil {
		return nil, err
	}
	return &d, nil
}

func (r *Repository) Create(d *Device) error {
	return r.db.Create(d).Error
}

func (r *Repository) Update(d *Device) error {
	return r.db.Save(d).Error
}

func (r *Repository) Delete(id string) error {
	return r.db.Delete(&Device{}, "id = ?", id).Error
}

// SetOnlineState updates a device's online/last-seen status, typically
// driven by MQTT state messages rather than the HTTP API.
func (r *Repository) SetOnlineState(id string, online bool) error {
	return r.db.Model(&Device{}).Where("id = ?", id).Updates(map[string]interface{}{
		"online":    online,
		"last_seen": time.Now(),
	}).Error
}
