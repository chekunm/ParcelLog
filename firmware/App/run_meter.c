#include "run_meter.h"
void meter_add(RunMeter *m, uint32_t us) {
    if (!m->n || us < m->min) m->min = us;
    if (us > m->max) m->max = us;
    ++m->n; m->sum += us;
}
bool release_ready(Release *r, uint32_t now) {
    if ((int32_t)(now - r->due) < 0) return false;
    uint32_t missed = (now - r->due) / r->period;
    r->skipped += missed; r->due += missed * r->period;
    if (r->started) {
        int32_t jitter = (int32_t)(now - r->previous) - (int32_t)r->period;
        if (jitter < r->jitter_min) r->jitter_min = jitter;
        if (jitter > r->jitter_max) r->jitter_max = jitter;
    }
    r->previous = now; r->started = true;
    return true;
}
void release_finish(Release *r, uint32_t now) {
    if ((uint32_t)(now - r->due) > r->deadline) ++r->late;
    r->due += r->period;
}
