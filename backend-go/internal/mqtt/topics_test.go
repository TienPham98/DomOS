package mqtt

import "testing"

func TestDeviceTopics(t *testing.T) {
	tests := map[string]string{
		TopicDeviceState("board"):     "domos/device/board/state",
		TopicDeviceTheme("board"):     "domos/device/board/theme",
		TopicDeviceWallpaper("board"): "domos/device/board/wallpaper",
		TopicDeviceAudio("board"):     "domos/device/board/audio",
		TopicDeviceOTA("board"):       "domos/device/board/ota",
	}
	for got, want := range tests {
		if got != want {
			t.Errorf("got %q, want %q", got, want)
		}
	}
	if TopicDeviceStateWildcard != "domos/device/+/state" {
		t.Fatalf("unexpected state wildcard: %s", TopicDeviceStateWildcard)
	}
}
