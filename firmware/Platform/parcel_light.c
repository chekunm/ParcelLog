#include "parcel_light.h"
#include "main.h"

/* One ADC1 channel is owned by ParcelLog's foreground sampling task. The
 * runtime provides the one-second retry and calls this every 100 ms when ready. */
static ADC_HandleTypeDef light_adc;
static bool configured;

bool parcel_light_open(void) {
    light_adc = (ADC_HandleTypeDef){0};
    ADC_ChannelConfTypeDef input = {0};
    light_adc.Instance = ADC1;
    light_adc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    light_adc.Init.Resolution = ADC_RESOLUTION_12B;
    light_adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    light_adc.Init.ScanConvMode = ADC_SCAN_SEQ_FIXED;
    light_adc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    light_adc.Init.NbrOfConversion = 1;
    light_adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    light_adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    light_adc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    light_adc.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_160CYCLES_5;
    light_adc.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_160CYCLES_5;
    light_adc.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_LOW;
    input.Channel = ADC_CHANNEL_0;
    input.Rank = ADC_RANK_CHANNEL_NUMBER;
    input.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    configured = false;
    if (HAL_ADC_Init(&light_adc) == HAL_OK &&
        HAL_ADC_ConfigChannel(&light_adc, &input) == HAL_OK &&
        HAL_ADCEx_Calibration_Start(&light_adc) == HAL_OK) {
        configured = true;
    } else {
        (void)HAL_ADC_DeInit(&light_adc);
    }
    return configured;
}

bool parcel_light_read(uint16_t *raw) {
    if (!raw) return false;
    if (!configured && !parcel_light_open()) return false;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_StatusTypeDef started = HAL_ADC_Start(&light_adc);
    HAL_StatusTypeDef ready = started == HAL_OK ? HAL_ADC_PollForConversion(&light_adc, 2) : HAL_ERROR;
    uint16_t measured = ready == HAL_OK ? (uint16_t)HAL_ADC_GetValue(&light_adc) : 0;
    HAL_StatusTypeDef stopped = HAL_ADC_Stop(&light_adc);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    if (ready != HAL_OK || stopped != HAL_OK) {
        (void)HAL_ADC_DeInit(&light_adc);
        configured = false;
        return false;
    }
    *raw = measured;
    return true;
}
