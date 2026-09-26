#include "mpu6050.h"
#include <stdbool.h>

#define MPU_ADDR 0x68u
#define MPU_WHO_AM_I 0x75u
#define MPU_PWR_MGMT_1 0x6bu
#define MPU_ACCEL_CONFIG 0x1cu
#define MPU_ACCEL_XOUT_H 0x3bu

#define I2C_PORT GPIOB
#define I2C_SCL GPIO_PIN_8
#define I2C_SDA GPIO_PIN_9

static void BusDelay(void)
{
  for (volatile uint32_t i = 0; i < 80u; ++i) {
    __NOP();
  }
}

static void SetLine(uint16_t pin, GPIO_PinState state)
{
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = pin;
  gpio.Mode = state == GPIO_PIN_SET ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(I2C_PORT, &gpio);
  BusDelay();
}

static void SetScl(GPIO_PinState state)
{
  SetLine(I2C_SCL, state);
}

static void SetSda(GPIO_PinState state)
{
  SetLine(I2C_SDA, state);
}

static bool Start(void)
{
  SetSda(GPIO_PIN_SET);
  SetScl(GPIO_PIN_SET);
  if (HAL_GPIO_ReadPin(I2C_PORT, I2C_SCL) != GPIO_PIN_SET ||
      HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA) != GPIO_PIN_SET) {
    return false;
  }
  SetSda(GPIO_PIN_RESET);
  SetScl(GPIO_PIN_RESET);
  return true;
}

static void Stop(void)
{
  SetSda(GPIO_PIN_RESET);
  SetScl(GPIO_PIN_SET);
  SetSda(GPIO_PIN_SET);
}

static bool WriteByte(uint8_t value)
{
  for (int bit = 7; bit >= 0; --bit) {
    SetSda((value & (1u << bit)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    SetScl(GPIO_PIN_SET);
    SetScl(GPIO_PIN_RESET);
  }
  SetSda(GPIO_PIN_SET);
  SetScl(GPIO_PIN_SET);
  bool ack = HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA) == GPIO_PIN_RESET;
  SetScl(GPIO_PIN_RESET);
  return ack;
}

static uint8_t ReadByte(bool acknowledge)
{
  uint8_t value = 0;
  SetSda(GPIO_PIN_SET);
  for (int bit = 7; bit >= 0; --bit) {
    SetScl(GPIO_PIN_SET);
    if (HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA) == GPIO_PIN_SET) {
      value |= (uint8_t)(1u << bit);
    }
    SetScl(GPIO_PIN_RESET);
  }
  SetSda(acknowledge ? GPIO_PIN_RESET : GPIO_PIN_SET);
  SetScl(GPIO_PIN_SET);
  SetScl(GPIO_PIN_RESET);
  SetSda(GPIO_PIN_SET);
  return value;
}

static HAL_StatusTypeDef ReadRegister(uint8_t reg, uint8_t *data, uint16_t size)
{
  if (!Start()) {
    return HAL_ERROR;
  }
  if (!WriteByte(MPU_ADDR << 1) || !WriteByte(reg) || !Start() ||
      !WriteByte((MPU_ADDR << 1) | 1u)) {
    Stop();
    return HAL_ERROR;
  }
  for (uint16_t i = 0; i < size; ++i) {
    data[i] = ReadByte(i + 1u < size);
  }
  Stop();
  return HAL_OK;
}

static HAL_StatusTypeDef WriteRegister(uint8_t reg, uint8_t value)
{
  if (!Start()) {
    return HAL_ERROR;
  }
  bool ok = WriteByte(MPU_ADDR << 1) && WriteByte(reg) && WriteByte(value);
  Stop();
  return ok ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef Mpu6050_Init(I2C_HandleTypeDef *i2c)
{
  /* Release I2C1, then use GPIO direction changes for the Wokwi C031 bus. */
  if (HAL_I2C_DeInit(i2c) != HAL_OK) {
    return HAL_ERROR;
  }
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_GPIO_WritePin(I2C_PORT, I2C_SCL | I2C_SDA, GPIO_PIN_RESET);
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = I2C_SCL | I2C_SDA;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(I2C_PORT, &gpio);

  uint8_t identity = 0;
  if (ReadRegister(MPU_WHO_AM_I, &identity, 1) != HAL_OK || identity != 0x68u) {
    return HAL_ERROR;
  }
  if (WriteRegister(MPU_PWR_MGMT_1, 0) != HAL_OK) {
    return HAL_ERROR;
  }
  return WriteRegister(MPU_ACCEL_CONFIG, 0);
}

HAL_StatusTypeDef Mpu6050_ReadAcceleration(I2C_HandleTypeDef *i2c,
                                           Mpu6050Acceleration *sample)
{
  (void)i2c;
  uint8_t bytes[6];
  if (ReadRegister(MPU_ACCEL_XOUT_H, bytes, sizeof bytes) != HAL_OK) {
    return HAL_ERROR;
  }
  sample->ax = (int16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
  sample->ay = (int16_t)(((uint16_t)bytes[2] << 8) | bytes[3]);
  sample->az = (int16_t)(((uint16_t)bytes[4] << 8) | bytes[5]);
  return HAL_OK;
}
