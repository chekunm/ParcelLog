#ifndef TEST_LIGHT_MAIN_H
#define TEST_LIGHT_MAIN_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
typedef struct { uint32_t ClockPrescaler, Resolution, DataAlign, ScanConvMode, EOCSelection, NbrOfConversion, ExternalTrigConv, ExternalTrigConvEdge, Overrun, SamplingTimeCommon1, SamplingTimeCommon2, TriggerFrequencyMode; } ADC_InitTypeDef;
typedef struct { void *Instance; ADC_InitTypeDef Init; } ADC_HandleTypeDef;
typedef struct { uint32_t Channel, Rank, SamplingTime; } ADC_ChannelConfTypeDef;
#define ADC1 ((void *)1)
#define ADC_CLOCK_SYNC_PCLK_DIV2 2
#define ADC_RESOLUTION_12B 12
#define ADC_DATAALIGN_RIGHT 0
#define ADC_SCAN_SEQ_FIXED 0
#define ADC_EOC_SINGLE_CONV 0
#define ADC_SOFTWARE_START 0
#define ADC_EXTERNALTRIGCONVEDGE_NONE 0
#define ADC_OVR_DATA_OVERWRITTEN 0
#define ADC_SAMPLETIME_160CYCLES_5 160
#define ADC_TRIGGER_FREQ_LOW 0
#define ADC_CHANNEL_0 0
#define ADC_RANK_CHANNEL_NUMBER 0
#define ADC_SAMPLINGTIME_COMMON_1 0
#define GPIOA ((void *)2)
#define GPIO_PIN_4 4
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *, ADC_ChannelConfTypeDef *);
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *, uint32_t);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef *);
void HAL_GPIO_WritePin(void *, unsigned, int);
#endif
