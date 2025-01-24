#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

void error(char* msg);
void error_ack(char* msg, uint8_t ack);
void delay();
void small_delay();
void wait_for_button();
char get_keypress();
uint8_t calc_parity(uint32_t data);

#endif
