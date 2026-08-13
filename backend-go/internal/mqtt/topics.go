package mqtt

import "fmt"

// Topic layout, matching the project roadmap:
//
//	domos/device/{id}/state      device -> server, online/offline + heartbeat
//	domos/device/{id}/theme      server -> device, theme change commands
//	domos/device/{id}/wallpaper  server -> device, wallpaper change commands
//	domos/device/{id}/audio      bidirectional, voice assistant audio events
//	domos/device/{id}/ota        server -> device, per-device OTA commands
//	domos/server/broadcast       server -> all devices
const (
	topicDeviceStateFmt     = "domos/device/%s/state"
	topicDeviceThemeFmt     = "domos/device/%s/theme"
	topicDeviceWallpaperFmt = "domos/device/%s/wallpaper"
	topicDeviceAudioFmt     = "domos/device/%s/audio"
	topicDeviceOTAFmt       = "domos/device/%s/ota"

	TopicServerBroadcast = "domos/server/broadcast"

	// TopicDeviceStateWildcard subscribes to state updates from every device.
	TopicDeviceStateWildcard = "domos/device/+/state"
)

func TopicDeviceState(id string) string     { return fmt.Sprintf(topicDeviceStateFmt, id) }
func TopicDeviceTheme(id string) string     { return fmt.Sprintf(topicDeviceThemeFmt, id) }
func TopicDeviceWallpaper(id string) string { return fmt.Sprintf(topicDeviceWallpaperFmt, id) }
func TopicDeviceAudio(id string) string     { return fmt.Sprintf(topicDeviceAudioFmt, id) }
func TopicDeviceOTA(id string) string       { return fmt.Sprintf(topicDeviceOTAFmt, id) }
