#ifndef MPU6050_H
#define MPU6050_H

#include "stm32c0xx_hal.h"
#include <stdint.h>

typedef struct {
  int16_t ax;
  int16_t ay;
  int16_t az;
} Mpu6050Acceleration;

HAL_StatusTypeDef Mpu6050_Init(I2C_HandleTypeDef *i2c);
HAL_StatusTypeDef Mpu6050_ReadAcceleration(I2C_HandleTypeDef *i2c,
                                           Mpu6050Acceleration *sample);

#endif
