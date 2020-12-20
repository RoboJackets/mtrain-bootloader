#include "dfu_target.h"
#include "usbdfu.h"
#include "ch.h"
#include "hal.h"
#include <string.h>

static size_t last_program_addr = 0;

size_t target_get_max_fw_size(void) {
	return MAX_FLASH_ADDR - APP_BASE;
}

uint16_t target_get_timeout(void) {
  if (last_program_addr == 0) {
    return 6000;
  }
  return 25;
}

void target_flash_unlock(void) {
	osalSysLockFromISR();
  FLASH->KEYR = 0x45670123; // KEY 1
  FLASH->KEYR = 0xCDEF89AB; // KEY 2
}

enum dfu_status target_flash_write(uint8_t* dst, uint8_t* src, size_t len) {
  last_program_addr = (size_t)dst;
  if (FLASH->CR & FLASH_CR_LOCK) {
    return DFU_STATUS_ERR_CHECK_ERASED; // Still locked after unlock. Can't recover from this unless reset.
  }
  // We are 3.3V powered, we need to write to 4 byte aligned address
  if (((size_t)dst % sizeof(uint32_t) != 0) ||
      ((size_t)src % sizeof(uint32_t) != 0) ||
      (len % sizeof(uint32_t) != 0)) {
    return DFU_STATUS_ERR_ADDRESS;
  }

  while(FLASH->SR & FLASH_SR_BSY) {}
  FLASH->CR = FLASH_CR_PG | FLASH_CR_PSIZE_1;
  for (size_t i = 0; i < len / sizeof(uint32_t); ++i)
  {
    ((uint32_t*)dst)[i] = ((uint32_t*)src)[i];
    while(FLASH->SR & FLASH_SR_BSY){}
  }
  FLASH->CR = 0;
  return DFU_STATUS_OK;
}

enum dfu_status target_prepare_flash(void) {
  // Erase the whole flash from sector 1 onward (Sector 1 is bootloader itself)
  if (FLASH->CR & FLASH_CR_LOCK) {
    return DFU_STATUS_ERR_CHECK_ERASED; // Still locked after unlock. Can't recover from this unless reset.
  }
  while(FLASH->SR & FLASH_SR_BSY){} // Wait for not busy
  for (uint32_t i = 1; i < 12; ++i)
  {
    FLASH->CR = FLASH_CR_PSIZE_1 | FLASH_CR_SER | (i << FLASH_CR_SNB_Pos) | FLASH_CR_STRT;
    while(FLASH->SR & FLASH_SR_BSY){}
    if (FLASH->SR & (0xF << FLASH_SR_WRPERR_Pos)) {
      *(volatile int*)i;
      return DFU_STATUS_ERR_ERASE; // Error while erasing
    }
  }
  return DFU_STATUS_OK;
}

void target_flash_lock(void) {
  FLASH->CR = FLASH_CR_LOCK; // LOCK Flash
	osalSysUnlockFromISR();
}

enum dfu_status target_complete_programming(void) {
  return DFU_STATUS_OK;
}
