#include "board.h"
#include "main.h"
#include "instrument.h"
#include "tx_queue.h"
#include "parcel_light.h"

extern UART_HandleTypeDef huart2;
static volatile uint16_t rx_head, rx_tail;
static volatile bool rx_overflow;
static uint8_t rx_byte, rx_ring[256];
static uint32_t rx_times[256], rx_taken_at;
static volatile uint32_t pir_rose_at;
static ParcelTxQueue tx;
static void transmit_next(void) {
    uint16_t count;
    const uint8_t *data = parcel_tx_begin(&tx, &count);
    if (count && HAL_UART_Transmit_IT(&huart2, (uint8_t *)data, count) != HAL_OK) {
        parcel_tx_cancel(&tx);
        tx.dropped = true;
    }
}
int board_put_char(int value) {
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    bool accepted = parcel_tx_append(&tx, (uint8_t)value);
    transmit_next();
    if (!mask) __enable_irq();
    return accepted ? value : -1;
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart) {
    if (uart != &huart2) return;
    parcel_tx_complete(&tx);
    transmit_next();
}
bool board_tx_overflow(void) {
    uint32_t mask = __get_PRIMASK();
    __disable_irq(); bool value = tx.dropped; tx.dropped = false;
    if (!mask) __enable_irq();
    return value;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart) {
    if (uart != &huart2) return;
    uint16_t next = (uint16_t)((rx_head + 1u) & 255u);
    if (next == rx_tail) rx_overflow = true;
    else { rx_ring[rx_head] = rx_byte; rx_times[rx_head] = instrument_now(); rx_head = next; }
    (void)HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart) {
    if (uart == &huart2) { rx_overflow = true; (void)HAL_UART_Receive_IT(&huart2, &rx_byte, 1); }
}
int board_get_char(void) {
    if (rx_head == rx_tail) return -1;
    uint8_t value = rx_ring[rx_tail];
    rx_taken_at = rx_times[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1u) & 255u);
    return value;
}
bool board_rx_overflow(void) {
    uint32_t mask = __get_PRIMASK();
    __disable_irq(); bool value = rx_overflow; rx_overflow = false;
    if (!mask) __enable_irq();
    return value;
}
bool board_storage(void) { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET; }
bool board_motion(void) { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET; }
void board_led(bool on) { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, on ? GPIO_PIN_SET : GPIO_PIN_RESET); }
bool board_read_light(uint16_t *value) { return parcel_light_read(value); }
void board_init(void) {
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_0; gpio.Mode = GPIO_MODE_INPUT; gpio.Pull = GPIO_PULLDOWN; HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_1; gpio.Mode = GPIO_MODE_IT_RISING; HAL_GPIO_Init(GPIOB, &gpio);
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 1, 0); HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
    gpio.Pin = GPIO_PIN_4; gpio.Mode = GPIO_MODE_OUTPUT_PP; gpio.Pull = GPIO_NOPULL; HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_0; gpio.Mode = GPIO_MODE_ANALOG; HAL_GPIO_Init(GPIOA, &gpio);
    HAL_NVIC_SetPriority(USART2_IRQn, 2, 0); HAL_NVIC_EnableIRQ(USART2_IRQn);
    (void)HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

unsigned board_tx_free(void) {
    uint32_t mask = __get_PRIMASK(); __disable_irq();
    unsigned free_bytes = parcel_tx_free(&tx);
    if (!mask) __enable_irq();
    return free_bytes;
}
uint32_t board_rx_timestamp(void) { return rx_taken_at; }
uint32_t board_motion_timestamp(void) { return pir_rose_at; }
void HAL_GPIO_EXTI_Rising_Callback(uint16_t pin) {
    if (pin == GPIO_PIN_1) pir_rose_at = instrument_now();
}
void EXTI0_1_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1); }
