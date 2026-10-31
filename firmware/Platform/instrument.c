#include "instrument.h"
#include "main.h"
#include "tick_clock.h"

static volatile uint32_t timer_high;
static uint64_t slept;
static volatile uint32_t irq_in_sleep;
static volatile bool sleeping;
static uint32_t irq_started;
/* Diagnostic gate: GDB may enable WFI only after counters/clock are observed.
 * A busy super-loop is a real execution mode, not an artificial flash delay. */
volatile uint32_t instrument_use_wfi;
volatile uint32_t instrument_timer_irqs;
extern uint32_t _estack, _Min_Stack_Size;

void instrument_early(void) {
    uint32_t *p = (uint32_t *)((uintptr_t)&_estack - (uintptr_t)&_Min_Stack_Size);
    uint32_t *safe_end = (uint32_t *)__get_MSP() - 16;
    while (p < safe_end) *p++ = 0xdeadbeefu;
}
uint32_t instrument_stack(void) {
    uint32_t *p = (uint32_t *)((uintptr_t)&_estack - (uintptr_t)&_Min_Stack_Size);
    while (p < &_estack && *p == 0xdeadbeefu) ++p;
    return (uint32_t)((uintptr_t)&_estack - (uintptr_t)p);
}
void instrument_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
#ifndef PARCEL_CLOCK_SYSTICK
    __HAL_RCC_TIM3_CLK_ENABLE();
    TIM3->CR1 = 0; TIM3->PSC = SystemCoreClock / 1000000u - 1u;
    TIM3->ARR = 65535; TIM3->EGR = TIM_EGR_UG; TIM3->SR = 0;
    timer_high = 0; TIM3->DIER = TIM_DIER_UIE;
    HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0); HAL_NVIC_EnableIRQ(TIM3_IRQn);
    TIM3->CR1 = TIM_CR1_CEN;
#endif
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8;
    gpio.Mode = GPIO_MODE_OUTPUT_PP; gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH; HAL_GPIO_Init(GPIOA, &gpio);
}
uint32_t instrument_now(void) {
#ifdef PARCEL_CLOCK_SYSTICK
    return tick_clock_us(HAL_GetTick());
#else
    uint32_t mask = __get_PRIMASK(); __disable_irq();
    uint32_t high = timer_high, low = TIM3->CNT;
    if (TIM3->SR & TIM_SR_UIF) { high += 65536u; low = TIM3->CNT; }
    if (!mask) __enable_irq();
    return high + low;
#endif
}
void TIM3_IRQHandler(void) {
    ++instrument_timer_irqs;
    if (TIM3->SR & TIM_SR_UIF) { TIM3->SR = 0; timer_high += 65536u; }
}
void instrument_probe(unsigned pin, bool high) { GPIOA->BSRR = 1u << (pin + (high ? 0 : 16)); }
void instrument_irq_enter(void) { if (sleeping) irq_started = instrument_now(); }
void instrument_irq_leave(void) { if (sleeping) irq_in_sleep += instrument_now() - irq_started; }
void instrument_idle(void) {
    if (!instrument_use_wfi) return;
    /* Only time actually asleep counts as idle. UART/SysTick handler body time
     * is subtracted. Entry/exit and TIM3 IRQ overhead remain measurement error. */
    uint32_t start = instrument_now(), before = irq_in_sleep;
    sleeping = true; __WFI(); sleeping = false;
    slept += (uint32_t)(instrument_now() - start) - (uint32_t)(irq_in_sleep - before);
}
uint64_t instrument_sleep_us(void) { return slept; }
