#include "main.h"
#include "instrument.h"
#include <assert.h>
#include <stdio.h>
FakeGpio fake_gpio;
FakeTimer fake_timer;
uint32_t _estack,_Min_Stack_Size;
static FakeSysTick systick;
static FakeScb scb;
static uint32_t milliseconds,mask;
static unsigned scenario,scb_reads,timer_reads;
uint32_t HAL_GetTick(void) {return milliseconds;}
uint32_t __get_PRIMASK(void) {return mask;}
uintptr_t __get_MSP(void) {return (uintptr_t)&_estack;}
void __disable_irq(void) {mask=1;}
void __enable_irq(void) {mask=0;}
void __WFI(void) { }
void HAL_GPIO_Init(void *port,const GPIO_InitTypeDef *config) {(void)port;(void)config;}
FakeScb *fake_scb_read(void) {
    ++scb_reads;
    if (scenario==4 && scb_reads==2) {scb.ICSR=SCB_ICSR_PENDSTSET_Msk;systick.VAL=47951;}
    return &scb;
}
FakeSysTick *fake_systick_read(void) {
    ++timer_reads;
    if (scenario==3 && timer_reads==1) {scb.ICSR=SCB_ICSR_PENDSTSET_Msk;systick.VAL=47951;}
    if (scenario==3 && timer_reads==2) systick.VAL=47903;
    return &systick;
}
static void setup(unsigned which,uint32_t ms,uint32_t load,uint32_t value,bool pending) {
    scenario=which;milliseconds=ms;systick=(FakeSysTick){load,value};
    scb.ICSR=pending?SCB_ICSR_PENDSTSET_Msk:0;mask=0;scb_reads=timer_reads=0;
}
int main(void) {
    setup(1,10,47999,38399,false);assert(instrument_now()==10000 && mask==0);
    setup(2,10,47999,38399,true);assert(instrument_now()==10000 && mask==0);
    setup(3,10,47999,0,false);assert(instrument_now()==10000 && mask==0);
    setup(4,10,47999,0,false);assert(instrument_now()==10000 && mask==0);
    setup(1,10,47999,38399,false);mask=1;assert(instrument_now()==10000 && mask==1);
    setup(2,120,479999,479951,true);assert(instrument_now()==120000);
    setup(2,10,47999,0,true);assert(instrument_now()==10000);
    setup(2,10,47999,47999,true);assert(instrument_now()==10000);
    setup(1,4294967u,47999,19199,false);assert(instrument_now()==4294967000u);
    setup(1,4294968u,47999,19199,false);assert(instrument_now()==704u);
    puts("Shared coarse HAL clock: 1-ms quantization, pending independence, IRQ mask and 32-bit us wrap checked; sub-ms task runtime is unresolved");
}
