#ifndef SHIPMENT_LOG_H
#define SHIPMENT_LOG_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { SHIPMENT_SLOT_BYTES = 32, SHIPMENT_SLOTS = 128 };
typedef struct {
    uint32_t sequence, uptime_ms, boot;
    uint8_t kind, mode;
    uint16_t valid, light;
    int16_t ax, ay, az;
} ShipmentEntry;
typedef struct {
    void *context;
    bool (*read)(void *, uint32_t, uint8_t *, size_t);
    bool (*program8)(void *, uint32_t, const uint8_t *);
    bool (*erase)(void *);
} ShipmentStorage;
typedef struct {
    ShipmentStorage storage;
    uint32_t last_sequence, boot;
    uint16_t next_slot, intact, damaged;
    bool accessible;
} ShipmentLog;
void shipment_open(ShipmentLog *, ShipmentStorage);
bool shipment_read(ShipmentLog *, uint16_t, ShipmentEntry *);
bool shipment_append(ShipmentLog *, ShipmentEntry *);
bool shipment_erase(ShipmentLog *);
#endif
