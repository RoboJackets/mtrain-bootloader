#include "firmware_update.h"

uint8_t* firmware_buffer = (uint8_t*) 0xC0000000;
// uint8_t buf[1024*100];
// uint8_t* firmware_buffer = buf;
size_t firmware_size = 1;
