#ifndef DATA_TRANSFER_H
#define DATA_TRANSFER_H

#include <stdint.h>
#include "swd_request.h"

// Main data transfer functions
uint8_t SWD_DP_read(uint8_t A, uint32_t* data);
uint8_t SWD_DP_write(uint8_t A, uint32_t data);
uint8_t SWD_AP_read(uint8_t A, uint32_t* data);
uint8_t SWD_AP_write(uint8_t A, uint32_t data);

// Helpers
void single_pulse();
void pulse_clock(uint32_t num_pulses);
void send_data(uint16_t data, uint8_t len);
void send_data_lsb(uint32_t data, uint8_t len);
uint32_t read_data(uint8_t len);
uint32_t reverse(uint32_t num, uint8_t len);
swd_req_t create_swd_packet(uint8_t APnDP, uint8_t RnW, uint8_t A);
uint8_t conv_swd_packet_to_bin(swd_req_t packet);
void send_swd_packet(uint8_t APnDP, uint8_t RnW, uint8_t A);

#endif
