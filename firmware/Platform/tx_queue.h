#ifndef PARCEL_TX_QUEUE_H
#define PARCEL_TX_QUEUE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t bytes[2048];
    uint16_t read, write, inflight;
    bool dropped;
} ParcelTxQueue;
unsigned parcel_tx_free(const ParcelTxQueue *q);
bool parcel_tx_append(ParcelTxQueue *q, uint8_t value);
const uint8_t *parcel_tx_begin(ParcelTxQueue *q, uint16_t *length);
void parcel_tx_cancel(ParcelTxQueue *q);
void parcel_tx_complete(ParcelTxQueue *q);
#endif
