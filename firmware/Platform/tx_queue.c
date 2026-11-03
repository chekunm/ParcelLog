#include "tx_queue.h"
enum { TX_SIZE = 2048, TX_MASK = TX_SIZE - 1 };
unsigned parcel_tx_free(const ParcelTxQueue *q) { return (q->read - q->write - 1u) & TX_MASK; }
bool parcel_tx_append(ParcelTxQueue *q, uint8_t value) {
    if (!parcel_tx_free(q)) { q->dropped = true; return false; }
    q->bytes[q->write] = value;
    q->write = (uint16_t)((q->write + 1u) & TX_MASK);
    return true;
}
const uint8_t *parcel_tx_begin(ParcelTxQueue *q, uint16_t *length) {
    if (q->inflight || q->read == q->write) { *length = 0; return NULL; }
    q->inflight = q->write > q->read ? (uint16_t)(q->write - q->read) : (uint16_t)(TX_SIZE - q->read);
    *length = q->inflight;
    return &q->bytes[q->read];
}
void parcel_tx_cancel(ParcelTxQueue *q) { q->inflight = 0; }
void parcel_tx_complete(ParcelTxQueue *q) {
    q->read = (uint16_t)((q->read + q->inflight) & TX_MASK);
    q->inflight = 0;
}
