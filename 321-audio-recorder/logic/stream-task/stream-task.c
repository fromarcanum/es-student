#include "stream-task/stream-task.h"
#include "platform/platform.h"

#include <stddef.h>

static stream_task_callback_t frame_callback;
static stream_state_t state;
static uint8_t stream_bits;
static uint16_t frame_number;
static uint32_t frame_count;
static uint32_t sample_count;

void stream_task_init(stream_task_callback_t on_frame)
{
    frame_callback = on_frame;
    state = STREAM_STATE_IDLE;
    stream_bits = 16;
    frame_number = 0;
    frame_count = 0;
    sample_count = 0;
}

bool stream_task_start(uint8_t bits)
{
    if (state != STREAM_STATE_IDLE || frame_callback == NULL)
        return false;
    if (!platform_stream_start(bits))
        return false;

    stream_bits = bits;
    frame_number = 0;
    frame_count = 0;
    sample_count = 0;
    state = STREAM_STATE_RUN;
    return true;
}

void stream_task_stop(void)
{
    if (state != STREAM_STATE_RUN)
        return;
    platform_stream_stop();
    state = STREAM_STATE_IDLE;
}

void stream_task_handle(void)
{
    if (state != STREAM_STATE_RUN)
        return;

    uint32_t samples;
    const uint8_t *buffer = platform_stream_next(&samples);
    if (buffer == NULL)
        return;

    stream_header_t header = {
        .sync_word = STREAM_SYNC_WORD,
        .number = frame_number,
        .count = (uint16_t)samples,
    };
    uint32_t bytes = samples * (stream_bits == 16 ? 2u : 1u);
    frame_callback(&header, buffer, bytes);
    platform_stream_release();

    ++frame_number;
    ++frame_count;
    sample_count += samples;
}

stream_state_t stream_task_get_state(void) { return state; }
uint8_t stream_task_get_bits(void) { return stream_bits; }
uint32_t stream_task_get_frames(void) { return frame_count; }
uint32_t stream_task_get_samples(void) { return sample_count; }
uint32_t stream_task_get_overruns(void) { return platform_stream_overruns(); }
