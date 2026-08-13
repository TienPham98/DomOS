#include "storage_manager.h"

#include <cerrno>
#include <sys/stat.h>

#include "esp_err.h"
#include "esp_littlefs.h"

namespace {
bool MakeDirectory(const char *path)
{
    return mkdir(path, 0775) == 0 || errno == EEXIST;
}
} // namespace

bool StorageManager::EnsureLayout()
{
    return MakeDirectory("/littlefs/wallpapers") && MakeDirectory("/littlefs/icons") &&
           MakeDirectory("/littlefs/themes") && MakeDirectory("/littlefs/voice");
}

bool StorageManager::GetUsage(size_t *total, size_t *used) const
{
    return total != nullptr && used != nullptr && esp_littlefs_info("littlefs", total, used) == ESP_OK;
}
