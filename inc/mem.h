/**
 * @file mem.h
 * @author Min Kang
 * @brief Memory read and write functions header
 */
#ifndef MEM_H
#define MEM_H
#include <stdint.h>

/**
 * @brief Read from address of TARGET
 *
 * @param addr Register to read from
 * @param data Pointer to data you want to read to
 *
 * @return ACK of request
 */
uint8_t mem_read(uint32_t addr, uint32_t* data);

/**
 * @brief Read from address of TARGET while printing debug messages
 *
 * @param addr Register to read from
 * @param data Pointer to data you want to read to
 * @param reg_name Name of register for print statement
 *
 * @return ACK of request
 */
uint8_t mem_read_db(uint32_t addr, uint32_t* data, char* reg_name);


/**
 * @brief Write to address of TARGET
 *
 * @param addr Address to write to
 * @param data Data to write
 *
 * @return ACK of request
 */
uint8_t mem_write(uint32_t addr, uint32_t data);

/**
 * @brief Write to address of TARGET while printing debug messages
 *
 * @param addr Address to write to
 * @param data Data to write
 * @param reg_name Name of register for print statements
 *
 * @return ACK of request
 */
uint8_t mem_write_db(uint32_t addr, uint32_t data, char* reg_name);

#endif
