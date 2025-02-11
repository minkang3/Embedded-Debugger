/**
 * @file   swd_init.h
 * @author Min Kang
 * @brief  SWD switch headers
 */
#ifndef SWD_INIT_H
#define SWD_INIT_H

#include <stdint.h>

/**
 * @brief Initializes SWD
 *
 * Resets DP, initiates the switching sequence, resets again and 
 * resets the line.
 */
void initialize_swd();

/**
 * @brief Initialize DP and Mem-AP
 *
 * Enables CSYSPWRUPREQ and CDBGPWRUPREQ in CTRL/STAT register of DP which
 * powers up the debug system.
 */
uint8_t setup_dp_and_mem_ap();

#endif
