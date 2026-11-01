/* Deterministic functional execution on the host. Fake microseconds are a test
 * input, not measured MCU time, a flash timing model, or a one-hour endurance run. */
#include "runtime.h"
#include "board.h"
#include "instrument.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
static uint32_t us;
uint32_t SystemCoreClock=48000000;
static uint8_t flash[4096];
static unsigned reads, adc_reads, failures, events, inspections, nearby, status_lines, writes;
static bool pir, storage, sensor_ok=true, rx_overrun;
static unsigned tx_space=2047;
static uint32_t pir_stamp;
static char incoming[256], last_metrics[1024];
static size_t in_at;
uint32_t HAL_GetTick(void) { return us/1000; }
uint32_t instrument_now(void) { return us; }
void instrument_init(void) {}
uint32_t instrument_stack(void) { return 0; }
uint32_t instrument_heap(void) { return 0; }
void instrument_heap_usage(uint32_t *used,uint32_t *free_bytes) { *used=*free_bytes=0; }
uint64_t instrument_sleep_us(void) { return 0; }
void instrument_probe(unsigned pin,bool high) {(void)pin;(void)high;}
void board_init(void) {}
unsigned board_tx_free(void) { return tx_space; }
uint32_t board_rx_timestamp(void) { return us; }
uint32_t board_motion_timestamp(void) { return pir_stamp; }
int board_get_char(void) { return incoming[in_at]?(unsigned char)incoming[in_at++]:-1; }
bool board_rx_overflow(void) { bool v=rx_overrun;rx_overrun=false;return v; }
bool board_tx_overflow(void) { return false; }
bool board_motion(void) { return pir; }
bool board_storage(void) { return storage; }
void board_led(bool v) {(void)v;}
bool board_read_light(uint16_t *v) { ++adc_reads;*v=1000;return true; }
static bool rd(void*c,uint32_t a,uint8_t*d,size_t n) {(void)c;memcpy(d,flash+a,n);return true;}
static bool wr(void*c,uint32_t a,const uint8_t*d) {(void)c;for(unsigned i=0;i<8;++i)flash[a+i]&=d[i];++writes;return true;}
static bool er(void*c) {(void)c;memset(flash,255,sizeof flash);return true;}
ShipmentStorage board_shipment_storage(void) { return (ShipmentStorage){NULL,rd,wr,er}; }
static BusResult bus_read(void*c,uint8_t a,uint8_t reg,uint8_t*d,size_t n) {
    (void)c;(void)a;if(!sensor_ok) {++failures;return BUS_TIMEOUT;}
    if(reg==0x75) {assert(n==1);*d=0x68;}
    else {++reads;assert(n==14);memset(d,0,n);d[4]=0x40;}
    return BUS_OK;
}
static BusResult bus_write(void*c,uint8_t a,uint8_t reg,const uint8_t*d,size_t n) {(void)c;(void)a;(void)reg;(void)d;(void)n;return BUS_OK;}
SensorBus board_sensor_bus(void) {return (SensorBus){NULL,bus_read,bus_write};}
int runtime_test_printf(const char *fmt,...) {
    char text[1024];va_list a;va_start(a,fmt);int n=vsnprintf(text,sizeof text,fmt,a);va_end(a);assert(n>=0 && n<(int)sizeof text);
    if(strstr(text,"event=")) ++events;
    if(strstr(text,"event=inspection")) ++inspections;
    if(strstr(text,"event=nearby_motion")) ++nearby;
    if(strstr(text,"tick [")) ++status_lines;
    if(strstr(text,"METRICS ")) strcpy(last_metrics,text);
    return n;
}
static void send(const char *cmd) {assert(!incoming[in_at]);strcpy(incoming,cmd);in_at=0;}
static void run(unsigned milliseconds) {for(unsigned i=0;i<milliseconds;++i){us+=1000;runtime_poll();}}
int main(void) {
    er(NULL);runtime_init();runtime_poll();
    unsigned before=reads;run(200000);assert(reads-before==2000);
    printf("HOST S1 fake_time_ms=200000 samples=%u events=%u\n",reads-before,events);
    before=reads;storage=true;
    for(unsigned second=0;second<3600;++second) {
        if(second%120==0) send("mark\n");
        if(second%60==1) {pir=true;pir_stamp=us;}
        if(second%60==2) pir=false;
        run(1000);
    }
    assert(reads-before==36000 && inspections==30 && nearby==60);
    printf("HOST S2 fake_time_ms=3600000 samples=%u inspection=%u nearby=%u, NOT wall-clock endurance\n",reads-before,inspections,nearby);
    /* Test command input arriving from an adapter and parser recovery, rather
     * than pretending that the fake adapter reproduces a real UART wire. */
    before=reads;sensor_ok=false;run(4200);assert(failures>=3 && reads==before);
    sensor_ok=true;run(2100);assert(reads>before);
    unsigned old=inspections;
    for(unsigned i=0;i<40;++i) {send("mark\n");run(1);}
    assert(inspections-old==40);
    tx_space=0;send("mark\n");run(500);assert(inspections-old==40);
    rx_overrun=true;tx_space=2047;run(10);assert(inspections-old==40);
    send("mark\n");run(10);assert(inspections-old==41);
    for(unsigned i=0;i<100;++i) {send("mark\n");run(1);}
    send("stats\n");run(100);assert(strstr(last_metrics,"rx_overflows=1") && strstr(last_metrics,"next=128") && !strstr(last_metrics,"save_failed=0 "));
    ShipmentLog j;shipment_open(&j,board_shipment_storage());assert(j.intact==128 && j.next_slot==128);
    printf("HOST S3 command_burst=40 tx_backpressure=recovered rx_overflow=1 sensor_faults=%u journal_intact=%u program8_calls=%u\n",failures,j.intact,writes);
    us=UINT32_MAX-500000u; runtime_init(); runtime_poll(); before=reads;run(1000);assert(reads-before==10);
    puts("HOST TIM3 uint32 wrap: 10 samples across overflow passed");
    puts("HOST all scenarios passed; no MCU timing/CPU/stack measurements were made");
    return 0;
}
