#include <assert.h>
#include "trigger.chip.c"
static int flash_level, output, writes;
static unsigned scheduled;
static void (*timer_cb)(void *);
static void *timer_data;
pin_t pin_init(const char *name, uint32_t mode) { (void)mode; return name[0]=='F' ? 1 : 2; }
uint32_t pin_read(pin_t p) { (void)p; return flash_level; }
void pin_write(pin_t p, uint32_t v) { (void)p; output=v; writes++; }
bool pin_watch(pin_t p, const pin_watch_config_t *c) { (void)p; (void)c; return true; }
timer_t timer_init(const timer_config_t *c) { timer_cb=c->callback; timer_data=c->user_data; return 1; }
void timer_start(timer_t t,uint32_t us,bool repeat) { (void)t; (void)repeat; scheduled=us; }
int main(void) {
    chip_init(); flash_level=1; on_flash(&state,1,HIGH); assert(scheduled==100 && state.phase==DELAY);
    flash_level=0; timer_cb(timer_data); assert(state.phase==READY && writes==0);
    flash_level=1; on_flash(&state,1,HIGH); timer_cb(timer_data); assert(output==HIGH && scheduled==200000);
    on_flash(&state,1,HIGH); assert(state.phase==HIGH_PULSE && writes==1);
    timer_cb(timer_data); assert(output==LOW && state.phase==REFRACTORY && scheduled==200000);
    on_flash(&state,1,HIGH); assert(state.phase==REFRACTORY && writes==2);
    timer_cb(timer_data); assert(state.phase==READY); return 0;
}
