#ifndef CORE_H
#define CORE_H
#include <stdint.h>

uint8_t SWD_core_single_read(uint32_t addr, uint32_t* data);
uint8_t SWD_core_single_read_pf(uint32_t addr, uint32_t* data, char* reg_name);

uint8_t SWD_core_single_write(uint32_t addr, uint32_t data);
uint8_t SWD_core_single_write_pf(uint32_t addr, uint32_t data, char* reg_name);

#endif
