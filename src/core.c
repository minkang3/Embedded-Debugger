#include "core.h"
#include "data_transfer.h"
#include "utils.h"
#include <stdio.h>

// Core Functions {{{

uint8_t SWD_core_single_read(uint32_t addr, uint32_t* data) {
    uint8_t ack;
    // ------ Write to CSW of MEM-AP ----- //
    // Disable auto increment
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

    return ack;
}

uint8_t SWD_core_single_read_pf(uint32_t addr, uint32_t* data, char* reg_name) {
    uint8_t ack;
    // ------ Write to CSW of MEM-AP ----- //
    // Disable auto increment
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

uint8_t SWD_core_single_write(uint32_t addr, uint32_t data) {
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

uint8_t SWD_core_single_write_pf(uint32_t addr, uint32_t data, char* reg_name) {
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
// }}}
