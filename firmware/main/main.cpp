#include "app/launcher/app_manager.h"
#include "board/es3c28p/board_es3c28p.h"
#include "esp_log.h"
#include "kernel/event_bus.h"
#include "kernel/storage_manager.h"
#include "kernel/task_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "services/filesystem/upload_server.h"
#include "services/media/media_service.h"
#include "services/mqtt/mqtt_service.h"
#include "services/ota/ota_service.h"
#include "services/wifi/wifi_service.h"

namespace {
void EventDispatcher(void *context)
{
    auto *events = static_cast<EventBus *>(context);
    while (true) {
        events->DispatchPending();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
} // namespace

extern "C" void app_main(void)
{
    static ES3C28PBoard board;
    static EventBus events;
    static TaskManager tasks;
    static StorageManager storage;
    static WifiService wifi;
    static MqttService mqtt;
    static OtaService ota;
    static MediaService media;
    static WallpaperUploadServer upload;
    static AppManager apps;

    if (!board.Init()) {
        ESP_LOGE("domos", "board initialisation failed");
        return;
    }
    if (!events.Init() || !storage.EnsureLayout()) {
        ESP_LOGE("domos", "kernel initialisation failed");
        return;
    }
    tasks.Start("event_bus", EventDispatcher, &events, 4096, 4);
    if (!wifi.Start()) ESP_LOGW("domos", "Wi-Fi service did not start");
    if (!mqtt.Start(&events)) ESP_LOGW("domos", "MQTT service did not start");
    ota.Start(&events);
    media.Start(&events);
    wifi.SetUploadServer(&upload);
    if (!upload.Start(&wifi, &apps)) ESP_LOGW("domos", "wallpaper upload server did not start");



    apps.Start(&board, &wifi, &mqtt, &ota, &media, &storage);
    board.StartLvglTask();
}
