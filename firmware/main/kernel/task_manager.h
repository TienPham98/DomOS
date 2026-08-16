#pragma once

#include <cstddef>

class TaskManager {
public:
    bool Start(const char *name, void (*entry)(void *), void *context, size_t stack_size, unsigned priority);
};
