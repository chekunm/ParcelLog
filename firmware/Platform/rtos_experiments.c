#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "main.h"
#include "board.h"
#include "parcel_print.h"
#include "instrument.h"

#ifdef LAB4_EXPERIMENT
extern uint32_t parcel_cycles(void);
extern void parcel_os_report(void);
static TaskHandle_t high_task, low_task, medium_task;
static SemaphoreHandle_t resource;
static QueueHandle_t queue;
static volatile unsigned done, received, lost;

static void work(unsigned loops) {
    for (volatile unsigned i=0; i<loops; ++i) __asm volatile ("nop");
}
static void low(void *arg) {
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        xSemaphoreTake(resource,portMAX_DELAY);
        xTaskNotifyGive(high_task);
        work(12000);
        xSemaphoreGive(resource);
    }
}
static void medium(void *arg) {
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        work(60000);
        xTaskNotifyGive(high_task);
    }
}
static void high(void *arg) {
    (void)arg;
    for (unsigned i=0;i<30;++i) {
        xTaskNotifyGive(low_task);
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        uint32_t start=parcel_cycles();
        xTaskNotifyGive(medium_task);
        xSemaphoreTake(resource,portMAX_DELAY);
        uint32_t delta=parcel_cycles()-start;
        xSemaphoreGive(resource);
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        parcel_printf("INVERSION case=%u n=%u wait_cycles=%lu\r\n",LAB4_EXPERIMENT,i,(unsigned long)delta);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    parcel_os_report(); parcel_printf("EXPERIMENT complete\r\n");
    vTaskSuspend(NULL);
}
__attribute__((noinline)) static void stack_work(void) {
    volatile uint8_t scratch[320];
    for (unsigned i=0;i<sizeof scratch;++i) scratch[i]=(uint8_t)i;
    /* Force a context switch with the full frame live for kernel checking. */
    vTaskDelay(1);
    if (scratch[319]!=63) parcel_printf("STACK corruption\r\n");
}
static void stack_task(void *arg) {
    (void)arg;
    for (unsigned i=0;i<30;++i) {stack_work();vTaskDelay(pdMS_TO_TICKS(100));}
    parcel_os_report();parcel_printf("EXPERIMENT complete\r\n");vTaskSuspend(NULL);
}
static void hog(void *arg) {
    (void)arg;
    uint32_t start=HAL_GetTick();
    while ((uint32_t)(HAL_GetTick()-start)<3000) work(100);
    done=1; vTaskSuspend(NULL);
}
static void observer(void *arg) {
    (void)arg;
    while (!done) {++received;vTaskDelay(1);}
    parcel_printf("STARVATION lower_releases=%u high_busy_ms=3000\r\n",received);
    parcel_os_report();parcel_printf("EXPERIMENT complete\r\n");vTaskSuspend(NULL);
}
static void producer(void *arg) {
    (void)arg;
    for (unsigned round=0;round<30;++round) {
        for (unsigned i=0;i<100;++i) if (xQueueSend(queue,&i,0)!=pdPASS) ++lost;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    done=1;vTaskSuspend(NULL);
}
static void consumer(void *arg) {
    (void)arg;unsigned value;
    for (;;) {
        if (xQueueReceive(queue,&value,pdMS_TO_TICKS(100))==pdPASS) ++received;
        if (done && uxQueueMessagesWaiting(queue)==0) break;
    }
    parcel_printf("QUEUE sent=3000 received=%u lost=%u block_ticks=0 capacity=4\r\n",received,lost);
    parcel_os_report();parcel_printf("EXPERIMENT complete\r\n");vTaskSuspend(NULL);
}
static void ping_high(void *arg) {
    (void)arg;
    for (unsigned i=0;i<30;++i) {
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        uint32_t finish=parcel_cycles();
        instrument_probe(8,true);
        extern volatile uint32_t parcel_ping_start;
        parcel_printf("NOTIFY n=%u cycles=%lu\r\n",i,(unsigned long)(finish-parcel_ping_start));
        instrument_probe(8,false);
        xTaskNotifyGive(low_task);
    }
    vTaskSuspend(NULL);
}
volatile uint32_t parcel_ping_start;
static void ping_low(void *arg) {
    (void)arg;
    for (unsigned i=0;i<30;++i) {
        instrument_probe(6,true);
        parcel_ping_start=parcel_cycles();xTaskNotifyGive(high_task);
        instrument_probe(6,false);
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);vTaskDelay(pdMS_TO_TICKS(100));
    }
    for (unsigned i=0;i<30;++i) {
        uint32_t before=parcel_cycles();uint32_t after=parcel_cycles();
        parcel_printf("COUNTER overhead_cycles=%lu\r\n",(unsigned long)(after-before));
    }
    /* Diagnostics follow the timed samples. Drain the bounded UART queue so
     * the counter dump plus OS report cannot discard the completion marker. */
    while (board_tx_free()<1900u) vTaskDelay(1);
    parcel_os_report();parcel_printf("EXPERIMENT complete\r\n");vTaskSuspend(NULL);
}
void parcel_experiment_start(void) {
    board_init();parcel_printf("EXPERIMENT case=%u tick_hz=%u\r\n",LAB4_EXPERIMENT,configTICK_RATE_HZ);
    if (LAB4_EXPERIMENT==1 || LAB4_EXPERIMENT==2) {
        resource=LAB4_EXPERIMENT==1 ? xSemaphoreCreateMutex() : xSemaphoreCreateBinary();
        configASSERT(resource);
        if (LAB4_EXPERIMENT==2) xSemaphoreGive(resource);
        configASSERT(xTaskCreate(low,"low",160,NULL,1,&low_task)==pdPASS);
        configASSERT(xTaskCreate(medium,"medium",160,NULL,3,&medium_task)==pdPASS);
        configASSERT(xTaskCreate(high,"high",192,NULL,4,&high_task)==pdPASS);
    } else if (LAB4_EXPERIMENT==3 || LAB4_EXPERIMENT==4 || LAB4_EXPERIMENT==9) {
        unsigned stack=LAB4_EXPERIMENT==9 ? 192 : 96;
        configASSERT(xTaskCreate(stack_task,"stack",stack,NULL,2,NULL)==pdPASS);
    } else if (LAB4_EXPERIMENT==5) {
        configASSERT(xTaskCreate(hog,"hog",128,NULL,4,NULL)==pdPASS);
        configASSERT(xTaskCreate(observer,"observer",224,NULL,1,NULL)==pdPASS);
    } else if (LAB4_EXPERIMENT==6) {
        queue=xQueueCreate(4,sizeof(unsigned));configASSERT(queue);
        configASSERT(xTaskCreate(producer,"producer",128,NULL,3,NULL)==pdPASS);
        configASSERT(xTaskCreate(consumer,"consumer",224,NULL,1,NULL)==pdPASS);
    } else if (LAB4_EXPERIMENT==7) {
        /* This build has configTOTAL_HEAP_SIZE=256: creation must fail. */
        configASSERT(xTaskCreate(observer,"heap_probe",1024,NULL,1,NULL)==pdPASS);
    } else {
        configASSERT(xTaskCreate(ping_high,"ping_high",160,NULL,4,&high_task)==pdPASS);
        configASSERT(xTaskCreate(ping_low,"ping_low",224,NULL,1,&low_task)==pdPASS);
    }
    vTaskStartScheduler();parcel_assert("experiment_scheduler",0);
}
#endif
