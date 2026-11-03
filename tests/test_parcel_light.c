#include "parcel_light.h"
#include "main.h"
#include <assert.h>
#include <stdio.h>
static int fail, opens, starts, polls, stops, resets, probe, reads;
static uint16_t simulated_raw=1777;
enum { INIT=1, CHANNEL, CALIBRATE, START, POLL, STOP };
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *a) { ++opens; assert(a->Instance==ADC1 && a->Init.Resolution==12 && a->Init.NbrOfConversion==1); return fail==INIT ? HAL_ERROR : HAL_OK; }
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *a, ADC_ChannelConfTypeDef *c) { (void)a; assert(c->Channel==0); return fail==CHANNEL ? HAL_ERROR : HAL_OK; }
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *a) {(void)a;return fail==CALIBRATE?HAL_ERROR:HAL_OK;}
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *a) {(void)a;++starts;return fail==START?HAL_ERROR:HAL_OK;}
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *a,uint32_t timeout) {(void)a;assert(timeout==2);++polls;return fail==POLL?HAL_ERROR:HAL_OK;}
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *a) {(void)a;++reads;return simulated_raw;}
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *a) {(void)a;++stops;return fail==STOP?HAL_ERROR:HAL_OK;}
HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef *a) {(void)a;++resets;return HAL_OK;}
void HAL_GPIO_WritePin(void *p,unsigned pin,int v) {assert(p==GPIOA && pin==4);probe=v;}
int main(void) {
 uint16_t raw=0xface;
 assert(!parcel_light_read(NULL));
 for(int i=INIT;i<=CALIBRATE;i++) {fail=i; assert(!parcel_light_open()); assert(!parcel_light_read(&raw)); assert(raw==0xface && !probe);}
 fail=0;assert(parcel_light_open());assert(parcel_light_read(&raw) && raw==1777 && !probe);
 for(int i=START;i<=STOP;i++) {int before=opens;fail=i;raw=123;assert(!parcel_light_read(&raw));assert(raw==123 && !probe);fail=0;assert(parcel_light_read(&raw) && raw==1777);assert(opens==before+1);}
 const uint16_t boundary[]={0,2048,4095};
 for(unsigned i=0;i<3;++i) { simulated_raw=boundary[i]; assert(parcel_light_read(&raw) && raw==boundary[i]); }
 assert(polls<starts && reads<=polls && resets>=6 && stops>=4);
 puts("Parcel light: configuration, bounded conversion, atomic sample and recovery passed");
}
