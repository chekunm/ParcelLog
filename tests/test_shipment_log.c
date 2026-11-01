#include "shipment_log.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t flash[4096];
static int remaining = -1;
static bool read_flash(void *c, uint32_t at, uint8_t *p, size_t n) { (void)c; assert(at+n<=sizeof flash); memcpy(p,flash+at,n); return true; }
static bool program(void *c, uint32_t at, const uint8_t *p) {
    (void)c; assert(!(at&7) && at+8<=sizeof flash);
    for (unsigned i=0;i<8;++i) {
        if (remaining==0) return false;
        if (remaining>0) --remaining;
        assert((flash[at+i]&p[i])==p[i]); flash[at+i]&=p[i];
    }
    return true;
}
static bool erase(void *c) { (void)c; memset(flash,255,sizeof flash); return true; }
static ShipmentStorage backend = {NULL,read_flash,program,erase};
static ShipmentEntry entry(uint8_t kind) { return (ShipmentEntry){.uptime_ms=1234,.kind=kind,.mode=1,.valid=3,.light=2345,.ax=-32768,.ay=32767,.az=16384}; }
int main(void) {
    ShipmentLog j,recovered; ShipmentEntry first=entry(2),next=entry(9),got;
    erase(NULL); shipment_open(&j,backend); assert(j.boot==1 && j.next_slot==0);
    assert(shipment_append(&j,&first));
    uint8_t prefix[32]; memcpy(prefix,flash,32);
    for (int cut=0;cut<=32;++cut) {
        erase(NULL); memcpy(flash,prefix,32); shipment_open(&j,backend);
        remaining=cut; bool complete=shipment_append(&j,&next); remaining=-1;
        shipment_open(&recovered,backend);
        assert(!memcmp(prefix,flash,32)); assert(shipment_read(&recovered,0,&got));
        assert(got.sequence==1 && got.ax==-32768 && got.light==2345);
        assert(complete==(cut==32)); assert(recovered.intact==(cut==32?2:1));
        assert(shipment_read(&recovered,1,&got)==(cut==32));
        ShipmentEntry after=entry(4); assert(shipment_append(&recovered,&after));
        assert(after.sequence==(cut==32?3:2));
    }
    for (unsigned bit=0;bit<256;++bit) {
        erase(NULL); memcpy(flash,prefix,32); flash[bit/8]^=1u<<(bit%8);
        shipment_open(&j,backend); assert(j.intact==0 && j.damaged==1 && j.next_slot==1);
    }
    erase(NULL); shipment_open(&j,backend);
    for (unsigned i=0;i<128;++i) { next=entry(i%11); assert(shipment_append(&j,&next)); }
    uint8_t full[4096]; memcpy(full,flash,sizeof full);
    assert(!shipment_append(&j,&next) && !memcmp(full,flash,sizeof full));
    shipment_open(&recovered,backend); assert(recovered.intact==128 && recovered.last_sequence==128 && recovered.boot==2);
    assert(shipment_erase(&recovered) && recovered.next_slot==0);
    puts("shipment_log: 33 byte-cut boundaries, 256 corruptions, restart and capacity preservation passed (host NOR model)");
    return 0;
}
