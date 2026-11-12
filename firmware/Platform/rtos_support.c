#include "main.h"
#include "instrument.h"
#include "FreeRTOS.h"
#include "task.h"
#include "parcel_print.h"
#define printf parcel_printf
#include <string.h>

extern UART_HandleTypeDef huart2;
volatile unsigned parcel_scheduler_running;
static uint32_t tick_n, tick_max, switches;
static uint64_t tick_sum;
typedef struct { uint32_t us; void *task; } Trace;
static Trace trace[16];
static unsigned trace_count;
static uint32_t switch_out_at;
static uint32_t switch_n, switch_max;
static uint64_t switch_sum;
void parcel_trace_arm(void) {
    taskENTER_CRITICAL(); trace_count=0; taskEXIT_CRITICAL();
}

/* Nominal cycles from the shared HAL clock: 1-ms/48000-cycle bins at 48 MHz.
 * Native VAL tick/switch spans and GPIO/VCD resolve sub-ms costs. No DWT. */
uint32_t parcel_cycles(void) {
    return instrument_now()*(SystemCoreClock/1000000u);
}
extern uint32_t parcel_critical_depth(void);
void parcel_trace_out(void *task) {
    (void)task;
    uint32_t depth=parcel_critical_depth();
    if (depth) parcel_assert("switch_with_critical", depth);
    switch_out_at = SysTick->VAL+1u;
}
void parcel_trace_in(void *task) {
    ++switches;
    uint32_t now = SysTick->VAL;
    if (switch_out_at) {
        /* Scheduler critical interval: native decrementing counter, at most
         * one tick wrap. Independent GPIO bounds this assumption. */
        uint32_t before=switch_out_at-1u;
        uint32_t d = before>=now ? before-now : before+SysTick->LOAD+1u-now;
        ++switch_n; switch_sum += d; if (d > switch_max) switch_max = d;
    }
    if (trace_count < 16) trace[trace_count++] = (Trace){instrument_now(), task};
}
/* Official ARM_CM0 port declares this hook weak. Keep HAL at 1 ms before
 * scheduler start, then enable the shared 1000/100 Hz tick coherently. */
void vPortSetupTimerInterrupt(void) {
    SysTick->CTRL=0;
    SysTick->LOAD=SystemCoreClock/configTICK_RATE_HZ-1u;
    SysTick->VAL=0;
    SCB->ICSR=SCB_ICSR_PENDSTCLR_Msk;
    tick_n=tick_max=switches=trace_count=switch_out_at=switch_n=switch_max=0;
    tick_sum=switch_sum=0;
    parcel_scheduler_running=1;
    SysTick->CTRL=SysTick_CTRL_CLKSOURCE_Msk|SysTick_CTRL_TICKINT_Msk|SysTick_CTRL_ENABLE_Msk;
}
void parcel_tick(void) {
#if defined(LAB4_EXPERIMENT) && LAB4_EXPERIMENT == 8
    instrument_probe(7,true);
#endif
    uint32_t start = SysTick->VAL;
    /* Shared SysTick, explicit HAL millisecond increment. No HAL timeout is
     * called from an ISR; ticks run before the scheduler and after it starts. */
    unsigned ms = parcel_scheduler_running ? 1000u / configTICK_RATE_HZ : 1u;
    while (ms--) HAL_IncTick();
    if (parcel_scheduler_running) {
        extern void xPortSysTickHandler(void);
        xPortSysTickHandler();
    }
    uint32_t end = SysTick->VAL;
    uint32_t d = start >= end ? start - end : start + SysTick->LOAD + 1u - end;
    ++tick_n; tick_sum += d; if (d > tick_max) tick_max = d;
#if defined(LAB4_EXPERIMENT) && LAB4_EXPERIMENT == 8
    instrument_probe(7,false);
#endif
}
static void fatal(const char *reason, const char *name) {
    __disable_irq();
    const char *parts[] = {"\r\nFATAL ", reason, " ", name, "\r\n"};
    /* IRQ TX may already own HAL's BUSY state. The fatal path has no scheduler
     * or HAL timeout: drain the physical transmitter directly. */
    for (unsigned i = 0; i < 5; ++i) for (const char *c=parts[i]; *c; ++c) {
        while (!(USART2->ISR & USART_ISR_TXE_TXFNF)) { }
        USART2->TDR=(uint8_t)*c;
    }
    while (!(USART2->ISR & USART_ISR_TC)) { }
    for (;;) { }
}
volatile uint32_t parcel_assert_line, parcel_assert_ipsr;
void parcel_assert(const char *file, unsigned line) {
    parcel_assert_line=line; parcel_assert_ipsr=__get_IPSR(); fatal("assert", file);
}
void vApplicationMallocFailedHook(void) { fatal("heap_exhausted", "heap_4"); }
void vApplicationStackOverflowHook(TaskHandle_t task, char *name) { (void)task; fatal("stack_overflow", name); }
void parcel_os_report(void) {
    static TaskStatus_t states[6];
    uint32_t total = 0;
    UBaseType_t n = uxTaskGetSystemState(states, 6, &total);
    printf("OS tick_hz=%u heap_total=%u heap_free=%u heap_min=%u switches=%lu runtime_total_us=%lu tick_n=%lu tick_avg_cycles=%lu tick_max_cycles=%lu switch_n=%lu switch_avg_cycles=%lu switch_max_cycles=%lu msp_watermark=%lu\r\n",
        configTICK_RATE_HZ, configTOTAL_HEAP_SIZE, (unsigned)xPortGetFreeHeapSize(),
        (unsigned)xPortGetMinimumEverFreeHeapSize(), (unsigned long)switches, (unsigned long)total,
        (unsigned long)tick_n, (unsigned long)(tick_n ? tick_sum/tick_n : 0), (unsigned long)tick_max,
        (unsigned long)switch_n, (unsigned long)(switch_n ? switch_sum/switch_n : 0),
        (unsigned long)switch_max, (unsigned long)instrument_stack());
    for (UBaseType_t i = 0; i < n; ++i)
        printf("TASK name=%s priority=%u runtime_us=%lu stack_free_min_bytes=%u handle=%lu\r\n", states[i].pcTaskName,
            (unsigned)states[i].uxCurrentPriority, (unsigned long)states[i].ulRunTimeCounter,
            (unsigned)states[i].usStackHighWaterMark * 4u, (unsigned long)states[i].xHandle);
    for (unsigned i = 0; i < trace_count; ++i)
        printf("TRACE us=%lu handle=%lu\r\n", (unsigned long)trace[i].us, (unsigned long)trace[i].task);
    trace_count = 0;
}
