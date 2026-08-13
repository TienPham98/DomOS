#include "board_es3c28p.h"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>

#include "esp_littlefs.h"
#include "esp_log.h"
#include "nvs_flash.h"

namespace {
constexpr char TAG[] = "storage";
constexpr char TEST_FILE[] = "/littlefs/test.txt";
constexpr char TEST_CONTENT[] = "DomOS LittleFS ready\n";
} // namespace

bool ES3C28PBoard::InitStorage()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        if (nvs_flash_erase() != ESP_OK) return false;
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return false;

    esp_vfs_littlefs_conf_t fs_config{};
    fs_config.base_path = "/littlefs";
    fs_config.partition_label = "littlefs";
    fs_config.format_if_mount_failed = true;
    if ((err = esp_vfs_littlefs_register(&fs_config)) != ESP_OK) {
        ESP_LOGE(TAG, "LittleFS mount failed: %s", esp_err_to_name(err));
        return false;
    }
    if (mkdir("/littlefs/wallpapers", 0775) != 0 && errno != EEXIST) {
        ESP_LOGE(TAG, "unable to create wallpaper directory");
        return false;
    }

    FILE *file = std::fopen(TEST_FILE, "w");
    if (file == nullptr || std::fputs(TEST_CONTENT, file) < 0) {
        if (file != nullptr) std::fclose(file);
        ESP_LOGE(TAG, "unable to write %s", TEST_FILE);
        return false;
    }
    std::fclose(file);

    char readback[sizeof(TEST_CONTENT)] = {};
    file = std::fopen(TEST_FILE, "r");
    if (file == nullptr || std::fgets(readback, sizeof(readback), file) == nullptr) {
        if (file != nullptr) std::fclose(file);
        ESP_LOGE(TAG, "unable to read %s", TEST_FILE);
        return false;
    }
    std::fclose(file);
    if (std::strcmp(readback, TEST_CONTENT) != 0) {
        ESP_LOGE(TAG, "LittleFS readback verification failed");
        return false;
    }

    size_t total = 0, used = 0;
    esp_littlefs_info("littlefs", &total, &used);
    ESP_LOGI(TAG, "LittleFS verified: %u/%u bytes used", static_cast<unsigned>(used), static_cast<unsigned>(total));
    return true;
}
