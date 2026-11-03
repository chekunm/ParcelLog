#include <assert.h>
#include <string.h>
#include "register_bank.chip.c"
static i2c_config_t device;
pin_t pin_init(const char *name, uint32_t mode) { assert(mode == INPUT); return strcmp(name, "SCL") ? 2 : 1; }
uint32_t attr_init(const char *name, uint32_t value) { (void)value; return !strcmp(name, "initial_nacks") ? 1 : 2; }
uint32_t attr_read(uint32_t id) { return id == 1 ? 2 : 1; }
i2c_dev_t i2c_init(const i2c_config_t *config) { device = *config; return 1; }
int main(void) {
    chip_init(); void *b = device.user_data;
    assert(device.address == 0x68);
    assert(!device.connect(b, 0x68, false)); assert(!device.connect(b, 0x68, false));
    assert(device.connect(b, 0x68, false)); assert(device.write(b, 0x75)); device.disconnect(b);
    assert(device.connect(b, 0x68, true)); assert(device.read(b) == 0); device.disconnect(b);
    assert(device.connect(b, 0x68, false)); assert(device.write(b, 0x75)); device.disconnect(b);
    assert(device.connect(b, 0x68, true)); assert(device.read(b) == 0x68); device.disconnect(b);
    assert(device.connect(b, 0x68, false)); assert(device.write(b, 0x1b)); assert(device.write(b, 0x08)); device.disconnect(b);
    assert(device.connect(b, 0x68, false)); assert(device.write(b, 0x1b)); device.disconnect(b);
    assert(device.connect(b, 0x68, true)); assert(device.read(b) == 0x08); device.disconnect(b);
    return 0;
}
