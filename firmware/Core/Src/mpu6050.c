#include "mpu6050.h"

#define MPU_ADDR (0x68u << 1)
#define MPU_WHO_AM_I 0x75u
#define MPU_PWR_MGMT_1 0x6bu
#define MPU_ACCEL_CONFIG 0x1cu
#define MPU_ACCEL_XOUT_H 0x3bu
#define MPU_TIMEOUT_MS 20u

HAL_StatusTypeDef Mpu6050_Init(I2C_HandleTypeDef *i2c)
{
  uint8_t identity = 0;
  uint8_t config = 0;

  if (HAL_I2C_Mem_Read(i2c, MPU_ADDR, MPU_WHO_AM_I, I2C_MEMADD_SIZE_8BIT,
                       &identity, 1, MPU_TIMEOUT_MS) != HAL_OK ||
      identity != 0x68u) {
    return HAL_ERROR;
  }
  if (HAL_I2C_Mem_Write(i2c, MPU_ADDR, MPU_PWR_MGMT_1, I2C_MEMADD_SIZE_8BIT,
                        &config, 1, MPU_TIMEOUT_MS) != HAL_OK) {
    return HAL_ERROR;
  }
  /* ACCEL_CONFIG = 0 selects the +/-2 g range. */
  return HAL_I2C_Mem_Write(i2c, MPU_ADDR, MPU_ACCEL_CONFIG,
                           I2C_MEMADD_SIZE_8BIT, &config, 1, MPU_TIMEOUT_MS);
}

HAL_StatusTypeDef Mpu6050_ReadAcceleration(I2C_HandleTypeDef *i2c,
                                           Mpu6050Acceleration *sample)
{
  uint8_t bytes[6];
  if (HAL_I2C_Mem_Read(i2c, MPU_ADDR, MPU_ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT,
                       bytes, sizeof bytes, MPU_TIMEOUT_MS) != HAL_OK) {
    return HAL_ERROR;
  }

  sample->ax = (int16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
  sample->ay = (int16_t)(((uint16_t)bytes[2] << 8) | bytes[3]);
  sample->az = (int16_t)(((uint16_t)bytes[4] << 8) | bytes[5]);
  return HAL_OK;
}
