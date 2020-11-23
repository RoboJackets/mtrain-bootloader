#pragma once

#include "board.h"

#define DRAM_START  ((void*) 0xC0000000)

void dram_init(void);
