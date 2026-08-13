#pragma once

#include <cstddef>

class StorageManager {
public:
    bool EnsureLayout();
    const char *WallpapersPath() const { return "/littlefs/wallpapers"; }
    const char *IconsPath() const { return "/littlefs/icons"; }
    const char *ThemesPath() const { return "/littlefs/themes"; }
    const char *VoiceCachePath() const { return "/littlefs/voice"; }
    bool GetUsage(size_t *total, size_t *used) const;
};
