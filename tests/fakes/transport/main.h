#ifndef FAKE_TRANSPORT_MAIN_H
#define FAKE_TRANSPORT_MAIN_H
#include <stdint.h>
typedef struct { uint32_t ISR, ICR, CR1, CR2, TXDR, RXDR; } FakeI2c;
extern FakeI2c fake_i2c;
#define I2C1 (&fake_i2c)
#define I2C_ISR_NACKF (1u<<4)
#define I2C_ISR_BERR (1u<<8)
#define I2C_ISR_ARLO (1u<<9)
#define I2C_ISR_OVR (1u<<10)
#define I2C_ISR_BUSY (1u<<15)
#define I2C_ISR_TXIS (1u<<1)
#define I2C_ISR_RXNE (1u<<2)
#define I2C_ISR_STOPF (1u<<5)
#define I2C_ICR_STOPCF (1u<<5)
#define I2C_ICR_NACKCF (1u<<4)
#define I2C_ICR_BERRCF (1u<<8)
#define I2C_ICR_ARLOCF (1u<<9)
#define I2C_ICR_OVRCF (1u<<10)
#define I2C_CR1_PE 1u
#define I2C_CR2_STOP (1u<<14)
#define I2C_CR2_RD_WRN (1u<<10)
#define I2C_CR2_AUTOEND (1u<<25)
#define I2C_CR2_START (1u<<13)
uint32_t HAL_GetTick(void);
#endif
