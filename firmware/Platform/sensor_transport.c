#include "board.h"
#include "main.h"

/* A transaction has one deadline, including both phases of a register read.
 * I2C1 belongs to the foreground loop; IRQ handlers do not use this bus. */
typedef struct { uint32_t began; } Transfer;
enum { TRANSFER_LIMIT_MS = 10, WIRE_BYTE_US = 120 };

#ifdef PARCEL_TRANSPORT_TEST
void transport_test_step(unsigned event, uint32_t value);
#define OBSERVE(event, value) transport_test_step(event, value)
#else
#define OBSERVE(event, value) ((void)0)
#endif
static void wire_settle(void) {
#ifdef PARCEL_TRANSPORT_TEST
    OBSERVE(1, WIRE_BYTE_US);
#else
    uint32_t cycles = WIRE_BYTE_US * (SystemCoreClock / 1000000u);
    uint32_t prev = SysTick->VAL;
    while (cycles) {
        uint32_t now = SysTick->VAL;
        uint32_t passed = prev >= now ? prev - now : prev + SysTick->LOAD + 1u - now;
        if (passed >= cycles) return;
        cycles -= passed;
        prev = now;
    }
#endif
}
static BusResult await_flag(const Transfer *t, uint32_t expected) {
    for (;;) {
        OBSERVE(2, expected);
        uint32_t flags = I2C1->ISR;
        if (flags & I2C_ISR_NACKF) return BUS_NACK;
        if (flags & (I2C_ISR_BERR | I2C_ISR_ARLO | I2C_ISR_OVR)) return BUS_ERROR;
        if ((uint32_t)(HAL_GetTick() - t->began) >= TRANSFER_LIMIT_MS) return BUS_TIMEOUT;
        if (flags & expected) return BUS_OK;
    }
}
static void acknowledge_flags(void) {
    I2C1->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF | I2C_ICR_OVRCF;
}
/* Clearing sticky flags and CR2 terminates every local transaction. A normal
 * NACK with an idle bus does not need a peripheral reset. A still-busy bus or
 * a transport fault does: ParcelLog owns I2C1 and has a 10 ms deadline. */
static BusResult close_transfer(BusResult result) {
    bool busy = (I2C1->ISR & I2C_ISR_BUSY) != 0;
    if (busy) {
        I2C1->CR2 |= I2C_CR2_STOP;
        OBSERVE(5, I2C1->CR2);
    }
    if (busy || result == BUS_TIMEOUT || result == BUS_ERROR) {
        I2C1->CR1 &= ~I2C_CR1_PE;
        I2C1->CR1 |= I2C_CR1_PE;
        OBSERVE(6, I2C1->CR1);
    }
    acknowledge_flags();
    I2C1->CR2 = 0;
    return result;
}
static void begin_phase(uint8_t address, size_t count, bool read) {
    acknowledge_flags();
    I2C1->CR2 = ((uint32_t)address << 1) | ((uint32_t)count << 16) |
        (read ? I2C_CR2_RD_WRN : 0) | I2C_CR2_AUTOEND | I2C_CR2_START;
    /* 100 kHz: address plus ACK requires at least 90 us on the wire. */
    OBSERVE(3, I2C1->CR2);
    wire_settle();
}
static BusResult write_byte(const Transfer *t, uint8_t value) {
    BusResult result = await_flag(t, I2C_ISR_TXIS);
    if (result != BUS_OK) return result;
    I2C1->TXDR = value;
    OBSERVE(4, value);
    wire_settle();
    return BUS_OK;
}
static BusResult stop_phase(const Transfer *t) {
    BusResult result = await_flag(t, I2C_ISR_STOPF);
    if (result == BUS_OK) { acknowledge_flags(); wire_settle(); }
    return result;
}
static BusResult sensor_write(void *unused, uint8_t address, uint8_t reg, const uint8_t *src, size_t length) {
    (void)unused;
    if (!src || !length || length > 254 || address > 127) return BUS_ERROR;
    Transfer transfer = { HAL_GetTick() };
    begin_phase(address, length + 1, false);
    BusResult result = write_byte(&transfer, reg);
    for (size_t i = 0; result == BUS_OK && i < length; ++i) result = write_byte(&transfer, src[i]);
    if (result == BUS_OK) result = stop_phase(&transfer);
    return close_transfer(result);
}
static BusResult sensor_read(void *unused, uint8_t address, uint8_t reg, uint8_t *dst, size_t length) {
    (void)unused;
    if (!dst || !length || length > 255 || address > 127) return BUS_ERROR;
    Transfer transfer = { HAL_GetTick() };
    begin_phase(address, 1, false);
    BusResult result = write_byte(&transfer, reg);
    if (result == BUS_OK) result = stop_phase(&transfer);
    if (result == BUS_OK) {
        begin_phase(address, length, true);
        for (size_t i = 0; result == BUS_OK && i < length; ++i) {
            result = await_flag(&transfer, I2C_ISR_RXNE);
            if (result == BUS_OK) dst[i] = (uint8_t)I2C1->RXDR;
        }
        if (result == BUS_OK) result = stop_phase(&transfer);
    }
    return close_transfer(result);
}
SensorBus board_sensor_bus(void) { return (SensorBus){NULL, sensor_read, sensor_write}; }
