/**
 * @file data_transfer.h
 * @author Min Kang
 * @brief AP and DP read and write function headers
 */
#ifndef DATA_TRANSFER_H
#define DATA_TRANSFER_H

#include <stdint.h>
#include "swd_request.h"

// ------------------------- MAIN INTERFACE ------------------------- //
/**
 * @brief Read request to DP
 *
 * @param A 2 bits, used to select register
 * @param data Pointer to where data should be read into
 * 
 * @return ACK of request
 */
uint8_t SWD_DP_read(uint8_t A, uint32_t* data);

/**
 * @brief Write request to DP
 * 
 * @param A 2 bit register selector
 * @param data Data to be written
 *
 * @return ACK of request
 */
uint8_t SWD_DP_write(uint8_t A, uint32_t data);

/**
 * @brief Read request to AP
 *
 * @param A 2 bits, used to select register
 * @param data Pointer to where data should be read into
 * 
 * @return ACK of request
 */
uint8_t SWD_AP_read(uint8_t A, uint32_t* data);

/**
 * @brief Write request to AP
 * 
 * @param A 2 bits, used to select registor
 * @param data Data to be written, 32 bits.
 *
 * @return ACK of request
 */
uint8_t SWD_AP_write(uint8_t A, uint32_t data);

// ------------------------- HELPERS ------------------------- //

/**
 * @brief Send a single clock pulse
 */
void single_pulse();

/**
 * @prief Pulse the clock a certain number of times
 * 
 * Useful for resetting DP
 */
void pulse_clock(uint32_t num_pulses);

/**
 * @brief Sends data while pulsing clock
 * 
 * @param data Data to be sent
 * @param len Length of data in bits
 */
void send_data(uint16_t data, uint8_t len);

/**
 * @brief Sends data lsb first
 * 
 * SWD protocol uses little endian, so this is used frequently.
 *
 * @param data Data to be sent
 * @param len Length of data in bits
 */
void send_data_lsb(uint32_t data, uint8_t len);

/**
 * @brief Reads a certain number of bits (max 32) from SWDIO, used after a read request
 * @param len Length of read, max 32
 * @return Data read
 */
uint32_t read_data(uint8_t len);

/**
 * @brief Creates an swd packet
 *
 * Automatically calculates parity
 * 
 * @param APnDP 1 bit, selects AP or DP. 0 for DP, 1 for AP.
 * @param RnW 1 bit, selects a read or write request. 0 for write, 1 for read.
 * @param A 2 bit register selector (for most types of requests).
 */
swd_req_t create_swd_packet(uint8_t APnDP, uint8_t RnW, uint8_t A);

/**
 * @brief Converts an swd_req_t into a binary format
 *
 * @param swd_req_t An SWD request struct
 * @return The request packet in binary 
 */
uint8_t conv_swd_packet_to_bin(swd_req_t packet);

/**
 * @brief Creates an SWD packet and sends it over SWDIO
 * 
 * @param APnDP 1 bit, selects AP or DP. 0 for DP, 1 for AP.
 * @param RnW 1 bit, selects a read or write request. 0 for write, 1 for read.
 * @param A 2 bit register selector (for most types of requests).
 */
void send_swd_packet(uint8_t APnDP, uint8_t RnW, uint8_t A);

#endif
