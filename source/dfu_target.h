#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "usbdfu.h"

size_t target_get_max_fw_size(void);
uint16_t target_get_timeout(void);
void target_flash_unlock(void);
enum dfu_status target_flash_write(uint8_t* dst, uint8_t* src, size_t len);
enum dfu_status target_prepare_flash(void);
void target_flash_lock(void);
enum dfu_status target_complete_programming(void);
