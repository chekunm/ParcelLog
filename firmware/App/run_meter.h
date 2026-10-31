#ifndef RUN_METER_H
#define RUN_METER_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { uint32_t n, min, max; uint64_t sum; } RunMeter;
typedef struct {
    uint32_t period, deadline, due, previous, skipped, late;
    int32_t jitter_min, jitter_max;
    bool started;
} Release;
void meter_add(RunMeter *, uint32_t);
bool release_ready(Release *, uint32_t);
void release_finish(Release *, uint32_t);
#endif
