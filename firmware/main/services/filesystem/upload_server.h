#pragma once

class WifiService;
class AppManager;

void AddSystemLog(const char *level, const char *source, const char *fmt, ...);

class WallpaperUploadServer {
public:
    bool Start(WifiService *wifi, AppManager *apps = nullptr);
};

