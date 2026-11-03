#include "../wokwi-flash-trigger/wokwi-api.h"
#include <stdint.h>

/* Minimal deterministic register-bank test double, not a physical MPU model. */
typedef struct {
    uint8_t registers[128];
    uint8_t cursor;
    bool expect_cursor;
    uint32_t nack_remaining;
    uint32_t bad_identity_remaining;
} RegisterBank;
static RegisterBank bank;

static bool connect_device(void *context, uint32_t address, bool read) {
    RegisterBank *b = context;
    (void)address;
    if (b->nack_remaining) { --b->nack_remaining; return false; }
    if (!read) b->expect_cursor = true;
    return true;
}
static bool write_byte(void *context, uint8_t value) {
    RegisterBank *b = context;
    if (b->expect_cursor) { b->cursor = value & 0x7f; b->expect_cursor = false; }
    else { b->registers[b->cursor] = value; b->cursor = (b->cursor + 1u) & 0x7f; }
    return true;
}
static uint8_t read_byte(void *context) {
    RegisterBank *b = context;
    uint8_t address = b->cursor;
    b->cursor = (b->cursor + 1u) & 0x7f;
    if (address == 0x75 && b->bad_identity_remaining) { --b->bad_identity_remaining; return 0; }
    return b->registers[address];
}
static void disconnect_device(void *context) { (void)context; }
void chip_init(void) {
    bank.nack_remaining = attr_read(attr_init("initial_nacks", 0));
    bank.bad_identity_remaining = attr_read(attr_init("bad_identity_reads", 0));
    bank.registers[0x75] = 0x68;
    bank.registers[0x3f] = 0x40; /* Z acceleration = +1 g at reset scale */
    const i2c_config_t config = { .user_data = &bank, .address = 0x68,
        .scl = pin_init("SCL", INPUT), .sda = pin_init("SDA", INPUT),
        .connect = connect_device, .read = read_byte, .write = write_byte,
        .disconnect = disconnect_device };
    i2c_init(&config);
}
