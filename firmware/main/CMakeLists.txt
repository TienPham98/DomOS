idf_component_register(
    SRCS
        "main.cpp"
        "board/es3c28p/board_es3c28p.cpp"
        "board/es3c28p/display.cpp"
        "board/es3c28p/touch.cpp"
        "board/es3c28p/audio.cpp"
        "board/es3c28p/storage.cpp"
        "kernel/event_bus.cpp"
        "kernel/task_manager.cpp"
        "kernel/storage_manager.cpp"
        "services/wifi/wifi_service.cpp"
        "services/mqtt/mqtt_service.cpp"
        "services/ota/ota_service.cpp"
        "services/media/media_service.cpp"
        "services/filesystem/upload_server.cpp"
        "app/launcher/app_manager.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        driver esp_event esp_http_client esp_http_server esp_https_ota esp_lcd esp_netif esp_timer esp_wifi
        esp_lcd_ili9341 esp_driver_ledc littlefs lvgl mbedtls mqtt nvs_flash
)

