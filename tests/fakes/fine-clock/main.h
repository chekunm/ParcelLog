#ifndef FINE_CLOCK_FAKE_MAIN_H
#define FINE_CLOCK_FAKE_MAIN_H
#include <stdint.h>
#include <stddef.h>
typedef struct { volatile uint32_t LOAD, VAL; } FakeSysTick;
typedef struct { volatile uint32_t ICSR; } FakeScb;
typedef struct { volatile uint32_t BSRR; } FakeGpio;
typedef struct { volatile uint32_t SR, CNT; } FakeTimer;
typedef struct { uint32_t Pin,Mode,Pull,Speed; } GPIO_InitTypeDef;
extern FakeGpio fake_gpio;
extern FakeTimer fake_timer;
FakeSysTick *fake_systick_read(void);
FakeScb *fake_scb_read(void);
#define SysTick fake_systick_read()
#define SCB fake_scb_read()
#define GPIOA (&fake_gpio)
#define TIM3 (&fake_timer)
#define SCB_ICSR_PENDSTSET_Msk (1u<<26)
#define GPIO_PIN_6 (1u<<6)
#define GPIO_PIN_7 (1u<<7)
#define GPIO_PIN_8 (1u<<8)
#define GPIO_MODE_OUTPUT_PP 0u
#define GPIO_NOPULL 0u
#define GPIO_SPEED_FREQ_HIGH 0u
#define TIM_SR_UIF 1u
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
uint32_t HAL_GetTick(void);
uint32_t __get_PRIMASK(void);
uintptr_t __get_MSP(void);
void __disable_irq(void);
void __enable_irq(void);
void __WFI(void);
void HAL_GPIO_Init(void *,const GPIO_InitTypeDef *);
#endif
