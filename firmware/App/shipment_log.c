#include "shipment_log.h"
#include <string.h>

/* Format 1 is explicitly little endian. A slot is never reused before erase.
 * CRC covers all 24 payload bytes; the final doubleword commits CRC + magic. */
#define COMMITTED 0x314c4350u
static void put32(uint8_t *p, uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (8 * i));
}
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static uint16_t get16(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }
static uint32_t crc(const uint8_t *p) {
    uint32_t v = UINT32_MAX;
    for (unsigned i = 0; i < 24; ++i) {
        v ^= p[i];
        for (unsigned bit = 0; bit < 8; ++bit) v = (v >> 1) ^ (0xedb88320u & (0u - (v & 1u)));
    }
    return ~v;
}
static bool unpack(const uint8_t *b, ShipmentEntry *e) {
    if (get32(b + 28) != COMMITTED || get32(b + 24) != crc(b)) return false;
    *e = (ShipmentEntry){ .sequence = get32(b), .uptime_ms = get32(b + 4), .boot = get32(b + 8),
        .kind = b[12], .mode = b[13], .valid = get16(b + 14), .light = get16(b + 16),
        .ax = (int16_t)get16(b + 18), .ay = (int16_t)get16(b + 20), .az = (int16_t)get16(b + 22) };
    return e->sequence != 0 && e->boot != 0 && e->kind <= 10 && e->mode <= 1 && e->valid <= 3;
}
bool shipment_read(ShipmentLog *j, uint16_t slot, ShipmentEntry *e) {
    uint8_t bytes[SHIPMENT_SLOT_BYTES];
    return slot < SHIPMENT_SLOTS && j->storage.read(j->storage.context, slot * SHIPMENT_SLOT_BYTES, bytes, sizeof bytes) && unpack(bytes, e);
}
void shipment_open(ShipmentLog *j, ShipmentStorage storage) {
    *j = (ShipmentLog){ .storage = storage, .accessible = true, .boot = 1 };
    for (uint16_t slot = 0; slot < SHIPMENT_SLOTS; ++slot) {
        uint8_t b[SHIPMENT_SLOT_BYTES]; ShipmentEntry e;
        if (!storage.read(storage.context, slot * SHIPMENT_SLOT_BYTES, b, sizeof b)) { j->accessible = false; return; }
        bool empty = true;
        for (unsigned i = 0; i < sizeof b; ++i) if (b[i] != 0xff) empty = false;
        if (empty) continue;
        j->next_slot = slot + 1;
        if (!unpack(b, &e)) { ++j->damaged; continue; }
        ++j->intact;
        if (e.sequence > j->last_sequence) j->last_sequence = e.sequence;
        if (e.boot >= j->boot) j->boot = e.boot + 1;
    }
}
bool shipment_append(ShipmentLog *j, ShipmentEntry *e) {
    if (!j->accessible || j->next_slot == SHIPMENT_SLOTS || j->last_sequence == UINT32_MAX || !j->boot) return false;
    uint8_t b[SHIPMENT_SLOT_BYTES];
    e->sequence = j->last_sequence + 1; e->boot = j->boot;
    put32(b, e->sequence); put32(b + 4, e->uptime_ms); put32(b + 8, e->boot);
    b[12] = e->kind; b[13] = e->mode; put16(b + 14, e->valid); put16(b + 16, e->light);
    put16(b + 18, (uint16_t)e->ax); put16(b + 20, (uint16_t)e->ay); put16(b + 22, (uint16_t)e->az);
    put32(b + 24, crc(b)); put32(b + 28, COMMITTED);
    uint32_t offset = j->next_slot++ * SHIPMENT_SLOT_BYTES;
    for (unsigned i = 0; i < sizeof b; i += 8) {
        if (!j->storage.program8(j->storage.context, offset + i, b + i)) { ++j->damaged; return false; }
    }
    ShipmentEntry verified;
    if (!shipment_read(j, (uint16_t)(j->next_slot - 1), &verified)) { ++j->damaged; return false; }
    j->last_sequence = e->sequence; ++j->intact;
    return true;
}
bool shipment_erase(ShipmentLog *j) {
    if (!j->storage.erase(j->storage.context)) { shipment_open(j, j->storage); return false; }
    shipment_open(j, j->storage);
    return j->accessible && j->next_slot == 0;
}
