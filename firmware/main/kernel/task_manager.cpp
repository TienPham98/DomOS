#include "task_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

bool TaskManager::Start(const char *name, void (*entry)(void *), void *context, size_t stack_size, unsigned priority)
{
    return entry != nullptr && xTaskCreate(entry, name, stack_size, context, priority, nullptr) == pdPASS;
}
