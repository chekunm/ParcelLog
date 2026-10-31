#include "board.h"
#include "main.h"
#include <string.h>

#define SHIPMENT_BASE 0x08007000u
/* Actual controller evidence, readable through GDB even when UART stalls.
 * stage: 0 untouched, 1 unlock, 2 program, 3 verify, 4 erase, 5 completed. */
volatile struct {
    uint32_t stage, hal_status, hal_error, page_error, sr, cr, offset;
    uint32_t unlock_status, program_status, erase_status;
} shipment_flash_diag = {0, UINT32_MAX, 0, UINT32_MAX, 0, 0, 0,
                        UINT32_MAX, UINT32_MAX, UINT32_MAX};
static void diagnostic(uint32_t stage, HAL_StatusTypeDef status) {
    shipment_flash_diag.stage = stage;
    shipment_flash_diag.hal_status = status;
    shipment_flash_diag.hal_error = HAL_FLASH_GetError();
    shipment_flash_diag.sr = FLASH->SR; shipment_flash_diag.cr = FLASH->CR;
    if (stage == 1) shipment_flash_diag.unlock_status = status;
}
static bool read_bytes(void *context, uint32_t offset, uint8_t *data, size_t n) {
    (void)context;
    if (offset > 4096 || n > 4096 - offset) return false;
    memcpy(data, (const void *)(SHIPMENT_BASE + offset), n);
    return true;
}
static bool program8(void *context, uint32_t offset, const uint8_t *data) {
    (void)context;
    if ((offset & 7u) || offset > 4088) return false;
    shipment_flash_diag.offset = offset;
    HAL_StatusTypeDef status = HAL_FLASH_Unlock(); diagnostic(1, status);
    if (status != HAL_OK) return false;
    uint64_t word; memcpy(&word, data, sizeof word);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, SHIPMENT_BASE + offset, word);
    shipment_flash_diag.program_status = status;
    diagnostic(2, status);
    (void)HAL_FLASH_Lock();
    bool ok = status == HAL_OK && !memcmp((const void *)(SHIPMENT_BASE + offset), data, 8);
    shipment_flash_diag.stage = ok ? 5u : 3u;
    return ok;
}
static bool erase(void *context) {
    (void)context;
    HAL_StatusTypeDef status = HAL_FLASH_Unlock(); diagnostic(1, status);
    if (status != HAL_OK) return false;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    FLASH_EraseInitTypeDef request = {0};
    request.TypeErase = FLASH_TYPEERASE_PAGES; request.Page = 14; request.NbPages = 2;
    uint32_t page_error = UINT32_MAX;
    shipment_flash_diag.page_error = page_error;
    status = HAL_FLASHEx_Erase(&request, &page_error);
    shipment_flash_diag.erase_status = status;
    shipment_flash_diag.page_error = page_error;
    diagnostic(4, status);
    (void)HAL_FLASH_Lock();
    return status == HAL_OK;
}
ShipmentStorage board_shipment_storage(void) { return (ShipmentStorage){NULL, read_bytes, program8, erase}; }
