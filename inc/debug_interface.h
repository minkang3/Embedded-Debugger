/**
 * @file debug_interface.h
 * @author Min Kang
 * @brief Debug interface headers
 */
#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

/**
 * @brief Print help menu
 */
uint8_t show_help();

// TODO: Add Documentation
uint8_t debug_initialize_swd();

/**
 * @brief Show debug status
 *
 * @return ACK from SWD request
 */
uint8_t show_debug_status();

/**
 * @brief Halt the core
 *
 * @return ACK from SWD request
 */
uint8_t halt_core();

/**
 * @brief Continues core, if halted
 *
 * @return ACK from SWD request
 */
uint8_t continue_core();

/**
 * @brief Reset core and halts
 *
 * @return ACK from SWD request
 */
uint8_t reset_core();

/**
 * @brief Read current PC
 *
 * @param data Pointer to store PC
 *
 * @return ACK from SWD request
 */
uint8_t read_pc();

/**
 * @brief Performs a single step of instruction
 *
 * @return ACK from SWD request
 */
uint8_t single_step();

/**
 * @brief Verify written file integrity
 *
 * @param bin_arr Hex dump array of program
 * @param bin_len Length of array
 *
 * @return ACK from SWD request
 */
uint8_t verify_file(unsigned char* bin_arr, unsigned int bin_len);

/**
 * @brief Write file to RAM, set PC, and reset TARGET
 *
 * @param bin_arr Hex dump array of program
 * @param bin_len Length of array
 *
 * @return ACK from SWD request
 */
uint8_t load_file(unsigned char* bin_arr, unsigned int bin_len);

/**
 * @brief Set PC, stack pointer, and VTable
 *
 * @param pc PC to write
 * @param msp Stack pointer
 *
 * @return ACK of SWD request
 */
uint8_t init_file_execution(uint32_t pc, uint32_t msp);

uint8_t load_file_and_run(char** args, uint8_t num_args);

uint8_t set_mem(uint32_t address, uint32_t value);
uint8_t interface_set_mem(char** args, uint8_t num_args);
uint8_t read_mem(uint32_t address, uint32_t* data);
uint8_t interface_read_mem(char** args, uint8_t num_args);
#endif
