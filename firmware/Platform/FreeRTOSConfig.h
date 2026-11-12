#ifndef PARCEL_FREERTOS_CONFIG_H
#define PARCEL_FREERTOS_CONFIG_H
#include <stdint.h>
extern uint32_t SystemCoreClock;
uint32_t instrument_now(void);
void parcel_assert(const char *, unsigned);
void parcel_trace_in(void *);
void parcel_trace_out(void *);
#define configENABLE_MPU 0
#define configCPU_CLOCK_HZ SystemCoreClock
#ifndef configTICK_RATE_HZ
#define configTICK_RATE_HZ 1000
#endif
#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configMAX_PRIORITIES 6
#define configMINIMAL_STACK_SIZE 80
#define configMAX_TASK_NAME_LEN 12
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_COUNTING_SEMAPHORES 0
#define configUSE_TASK_NOTIFICATIONS 1
#define configUSE_TIMERS 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION 0
#ifndef configTOTAL_HEAP_SIZE
#define configTOTAL_HEAP_SIZE 5120
#endif
#define configUSE_MALLOC_FAILED_HOOK 1
#ifndef configCHECK_FOR_STACK_OVERFLOW
#define configCHECK_FOR_STACK_OVERFLOW 2
#endif
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_TRACE_FACILITY 1
#define configGENERATE_RUN_TIME_STATS 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() ((void)0)
#define portGET_RUN_TIME_COUNTER_VALUE() instrument_now()
#define traceTASK_SWITCHED_IN() parcel_trace_in((void *)pxCurrentTCB)
#define traceTASK_SWITCHED_OUT() parcel_trace_out((void *)pxCurrentTCB)
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskDelayUntil 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_xTaskGetIdleTaskHandle 1
#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define configASSERT(x) do { if (!(x)) parcel_assert(__FILE__, __LINE__); } while (0)
#define SysTick_Handler xPortSysTickHandler
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#endif
