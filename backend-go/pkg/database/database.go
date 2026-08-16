package database

import (
	"github.com/glebarez/sqlite"
	"gorm.io/driver/postgres"
	"gorm.io/gorm"
	"gorm.io/gorm/logger"

	"github.com/domos/backend-go/internal/auth"
	"github.com/domos/backend-go/internal/devices"
	"github.com/domos/backend-go/internal/ota"
	"github.com/domos/backend-go/internal/themes"
	"github.com/domos/backend-go/internal/wallpapers"
	applogger "github.com/domos/backend-go/pkg/logger"
)

// Connect opens a GORM connection using PostgreSQL, or falls back to SQLite (domos.db) if PostgreSQL is unavailable.
func Connect(dsn string) (*gorm.DB, error) {
	db, err := gorm.Open(postgres.Open(dsn), &gorm.Config{
		Logger: logger.Default.LogMode(logger.Warn),
	})
	if err == nil {
		return db, nil
	}

	applogger.Warn.Printf("database: postgres connection failed (%v), falling back to local SQLite (domos.db)", err)
	sqliteDB, errSqlite := gorm.Open(sqlite.Open("domos.db"), &gorm.Config{
		Logger: logger.Default.LogMode(logger.Warn),
	})
	if errSqlite != nil {
		return nil, errSqlite
	}
	return sqliteDB, nil
}

// AutoMigrate creates/updates all tables for the registered domain models.
// Raw SQL equivalents are kept under /migrations for reference and for
// deployments that prefer explicit migration tooling.
func AutoMigrate(db *gorm.DB) error {
	return db.AutoMigrate(
		&auth.User{},
		&devices.Device{},
		&themes.Theme{},
		&wallpapers.Wallpaper{},
		&ota.FirmwareVersion{},
	)
}
