/**
 * @file mem.c
 * @author Min Kang
 * @brief Functions to write and read to debug core registers
 */
#include "mem.h"
#include "data_transfer.h"
#include "utils.h"
#include "macros.h"
#include <stdio.h>

/**
 * @brief Read from address of TARGET
 *
 * @param addr Register to read from
 * @param data Pointer to data you want to read to
 *
 * @return ACK of request
 */
uint8_t mem_read(uint32_t addr, uint32_t* data) {
    uint8_t ack;
    // ------ Write to CSW of MEM-AP ----- //
    // Enable auto increment
	ack = SWD_AP_write(0b00, 0x22000012);
	CHECK_ACK_RT("Failed reading core while writing to CSW");
    delay();

    // Write to TAR to set address to read from
	ack = SWD_AP_write(0b10, addr);
	CHECK_ACK_RT("Failed setting target address while reading core");
    delay();

    // Read from DRW and handle WAIT ACK
    if ((ack = SWD_AP_read(0b11, data)) == 0b010) {
		// First read returns nothing of use
        ack = SWD_DP_read(0b11, data);
		CHECK_ACK_RT("Bad ACK while reading core");
        ack = SWD_AP_read(0b11, data);
		CHECK_ACK_RT("Bad ACK while reading core");
        if ((ack = SWD_DP_read(0b11, data)) != 1) {
            error("Bad ACK in SWD_core_single_read while reading from RDBUFF");
            return ack;
        }
    } else {
        error("Bad ACK while reading from DRW in SWD_core_single_read");
        return ack;
    }

    return ack;
}

/**
 * @brief Read from address of TARGET while printing debug messages
 *
 * @param addr Register to read from
 * @param data Pointer to data you want to read to
 * @param reg_name Name of register for print statement
 *
 * @return ACK of request
 */
uint8_t mem_read_db(uint32_t addr, uint32_t* data, char* reg_name) {
    uint8_t ack;
    // ------ Write to CSW of MEM-AP ----- //
    // Enable auto increment
    if ((ack = SWD_AP_write(0b00, 0x22000012)) != 1) {
        error("Error in CSW write");
        return ack;
    }
    delay();

    // Write to TAR to set address to read from
    if ((ack = SWD_AP_write(0b10, addr)) != 1) {
        error("Bad ACK in SWD core single read");
        return ack;
    }
    printf("%s ADDR: 0x%.8x ACK: \033[33m%d\033[0m\n", reg_name, addr, ack);
    delay();

    // Read from DRW and handle WAIT ACK
    if ((ack = SWD_AP_read(0b11, data)) == 0b010) {
        ack = SWD_DP_read(0b11, data);
        if (ack != 1) {
            error("Error");
            return ack;
        }
        // HACK Handle corrupted read on first AP-read of TARGET
        //printf("Handling corrupted read\n");
        ack = SWD_AP_read(0b11, data);
        //return ack;
        if ((ack = SWD_DP_read(0b11, data)) != 1) {
            error("Bad ACK in SWD_core_single_read while reading from RDBUFF");
            return ack;
        }
    } else {
        error("Bad ACK while reading from DRW in SWD_core_single_read");
        return ack;
    }
    printf("%s VALUE: 0x%.8x ACK: \033[33m%d\033[0m\n", reg_name, *data, ack);

    return ack;
}

/**
 * @brief Write to address of TARGET
 *
 * @param addr Address to write to
 * @param data Data to write
 *
 * @return ACK of request
 */
uint8_t mem_write(uint32_t addr, uint32_t data) {
    uint8_t ack;
    if ((ack = SWD_AP_write(0b10, addr)) != 1) {
        error ("Bad ACK in SWD_core_single_write");
        return ack;
    }
    if ((ack = SWD_AP_write(0b11, data)) != 1) {
        error ("Bad ACK in SWD_core_single_write");
        return ack;
    }
    delay();
    return ack;
}

/**
 * @brief Write to address of TARGET while printing debug messages
 *
 * @param addr Address to write to
 * @param data Data to write
 * @param reg_name Name of register for print statements
 *
 * @return ACK of request
 */
uint8_t mem_write_db(uint32_t addr, uint32_t data, char* reg_name) {
    uint8_t ack;
    if ((ack = SWD_AP_write(0b10, addr)) != 1) {
        error ("Bad ACK in SWD_core_single_write");
        return ack;
    }
    printf("%s ADDR: 0x%.8x ACK: \033[33m%d\033[0m\n", reg_name, addr, ack);
    if ((ack = SWD_AP_write(0b11, data)) != 1) {
        error ("Bad ACK in SWD_core_single_write");
        return ack;
    }
    printf("%s ADDR: 0x%.8x ACK: \033[33m%d\033[0m\n", reg_name, data, ack);
    delay();
    return ack;
}
