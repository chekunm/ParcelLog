#include "board.h"
#include "main.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
FakeI2c fake_i2c;
static uint32_t tick, failure;
static unsigned events[128], values[128], count, read_index;
uint32_t HAL_GetTick(void) { return tick; }
void transport_test_step(unsigned event, uint32_t value) {
    assert(count < 128); events[count]=event; values[count++]=value;
    if (event==1) tick++;
    if (event==2) {
        fake_i2c.ISR = failure ? failure : value;
        if (failure==I2C_ISR_BUSY) { fake_i2c.ISR=0; tick++; }
        if (value==I2C_ISR_RXNE) fake_i2c.RXDR=0x40+read_index++;
    }
}
static unsigned observed(unsigned event) { unsigned n=0; for(unsigned i=0;i<count;i++) if(events[i]==event) ++n; return n; }
static void reset(void) { memset(&fake_i2c,0,sizeof(fake_i2c)); tick=failure=count=read_index=0; fake_i2c.CR1=I2C_CR1_PE; }
int main(void) {
    SensorBus bus=board_sensor_bus();uint8_t data[2]={0};
    reset(); assert(bus.read(NULL,0x68,0x75,data,2)==BUS_OK && data[0]==0x40 && data[1]==0x41);
    assert(events[0]==3 && events[1]==1 && values[1]==120 && events[2]==2 && events[3]==4);
    reset(); uint8_t src[2]={3,4};assert(bus.write(NULL,0x68,0x19,src,2)==BUS_OK);
    unsigned written=0;for(unsigned i=0;i<count;i++)if(events[i]==4){assert(values[i]==(written==0?0x19:written==1?3:4));written++;assert(events[i+1]==1);}assert(written==3);
    reset();failure=I2C_ISR_NACKF;assert(bus.read(NULL,0x68,0x75,data,1)==BUS_NACK && fake_i2c.CR2==0); assert(observed(5)==0 && observed(6)==0);
    reset();failure=I2C_ISR_NACKF | I2C_ISR_BUSY;assert(bus.read(NULL,0x68,0x75,data,1)==BUS_NACK && fake_i2c.CR2==0); assert(observed(5)==1 && observed(6)==1);
    reset();failure=I2C_ISR_BERR;assert(bus.write(NULL,0x68,1,src,1)==BUS_ERROR && fake_i2c.CR2==0); assert(observed(6)==1);
    reset();failure=I2C_ISR_BUSY;assert(bus.read(NULL,0x68,1,data,1)==BUS_TIMEOUT && tick>=10 && fake_i2c.CR2==0); assert(observed(6)==1);
    assert(bus.read(NULL,0x80,1,data,1)==BUS_ERROR && bus.write(NULL,0x68,1,NULL,1)==BUS_ERROR);
    puts("I2C transport: address and byte spacing, reads, NACK, bus error and timeout passed (host fake)");
}
