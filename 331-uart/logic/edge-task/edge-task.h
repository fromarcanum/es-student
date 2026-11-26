// Неблокирующее измерение фронтов RC-линии.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "platform/platform.h"

// Функция вызывается по завершении обоих фронтов или при таймауте.
typedef void (*edge_task_callback_t)(platform_line_mode_t mode,
                                     bool ok,
                                     uint32_t rise_us,
                                     uint32_t fall_us);

// Возвращает false, если предыдущий замер не закончен.
bool edge_task_start(platform_line_mode_t mode, edge_task_callback_t on_done);

// Выполняет один неблокирующий шаг машины состояний.
void edge_task_handle(void);
