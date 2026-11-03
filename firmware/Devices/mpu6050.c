#include "mpu6050.h"

static int32_t signed_word(const uint8_t *data) {
    uint32_t word = ((uint32_t)data[0] << 8) | data[1];
    return word < 32768u ? (int32_t)word : (int32_t)word - 65536;
}

void mpu6050_decode(const uint8_t bytes[14], ImuSample *sample) {
    for (size_t axis = 0; axis < 3; ++axis) {
        sample->accel_raw[axis] = (int16_t)signed_word(bytes + 2 * axis);
        sample->accel_mg[axis] = signed_word(bytes + 2 * axis) * 1000 / 16384;
        sample->gyro_mdps[axis] = signed_word(bytes + 8 + 2 * axis) * 1000 / 131;
    }
    sample->temperature_mc = signed_word(bytes + 6) * 1000 / 340 + 36530;
}

/* Configuration is deliberately table-driven: identify the device before any
 * write, and stop at the first transport failure. A failed sample never
 * overwrites the last complete measurement. */
BusResult mpu6050_init(Mpu6050 *device) {
    if (!device || !device->bus.read || !device->bus.write || device->address > 0x7f) return BUS_ERROR;
    uint8_t identity;
    BusResult status = device->bus.read(device->bus.context, device->address, 0x75, &identity, 1);
    if (status != BUS_OK) return status;
    if (identity != 0x68) return BUS_ERROR;
    static const struct { uint8_t reg, value; } setup[] = {
        {0x6b, 1}, {0x19, 9}, {0x1a, 3}, {0x1b, 0}, {0x1c, 0}
    };
    for (size_t i = 0; i < sizeof(setup) / sizeof(setup[0]); ++i) {
        status = device->bus.write(device->bus.context, device->address, setup[i].reg, &setup[i].value, 1);
        if (status != BUS_OK) break;
    }
    return status;
}

BusResult mpu6050_read(Mpu6050 *device, ImuSample *sample) {
    if (!device || !sample || !device->bus.read || device->address > 0x7f) return BUS_ERROR;
    uint8_t frame[14];
    BusResult status = device->bus.read(device->bus.context, device->address, 0x3b, frame, sizeof(frame));
    if (status != BUS_OK) return status;
    ImuSample complete;
    mpu6050_decode(frame, &complete);
    *sample = complete;
    return BUS_OK;
}
