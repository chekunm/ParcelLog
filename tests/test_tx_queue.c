#include "tx_queue.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void) {
    ParcelTxQueue q = {0}; uint16_t n; const uint8_t *p;
    assert(parcel_tx_free(&q)==2047 && parcel_tx_begin(&q,&n)==NULL && n==0);
    for (unsigned i=0;i<2047;i++) assert(parcel_tx_append(&q,(uint8_t)i));
    assert(!parcel_tx_append(&q,7) && q.dropped);
    p=parcel_tx_begin(&q,&n); assert(p && n==2047 && p[0]==0 && p[2046]==(uint8_t)2046);
    assert(parcel_tx_begin(&q,&n)==NULL && n==0);
    parcel_tx_cancel(&q); p=parcel_tx_begin(&q,&n); assert(p && n==2047);
    parcel_tx_complete(&q); assert(parcel_tx_free(&q)==2047);
    for (unsigned i=0;i<4;i++) assert(parcel_tx_append(&q,(uint8_t)(0xa0+i)));
    p=parcel_tx_begin(&q,&n); assert(n==1 && p[0]==0xa0); parcel_tx_complete(&q);
    p=parcel_tx_begin(&q,&n); assert(n==3 && p[0]==0xa1 && p[2]==0xa3); parcel_tx_complete(&q);
    assert(parcel_tx_free(&q)==2047); puts("TX queue: capacity, cancellation, in-flight ownership and wrap passed");
}
