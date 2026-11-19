#include "block-task/block-task.h"
#include "platform/platform.h"

#include <stddef.h>

static block_task_callback_t pending_callback;
static uint32_t pending_count;

bool block_task_start(uint32_t count, block_task_callback_t on_block)
{
    if (on_block == NULL || pending_callback != NULL)
        return false;
    if (!platform_block_start(count))
        return false;

    pending_count = count;
    pending_callback = on_block;
    return true;
}

void block_task_handle(void)
{
    if (pending_callback == NULL || !platform_block_done())
        return;

    // Убираем ожидание до вызова: callback может сразу запустить новый блок.
    block_task_callback_t callback = pending_callback;
    uint32_t count = pending_count;
    pending_callback = NULL;
    pending_count = 0;
    callback(platform_block_samples(), count, platform_block_time_us());
}
