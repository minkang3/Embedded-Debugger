#include "swd_init.h"
#include "macros.h"
#include "data_transfer.h"
#include "utils.h"
#include "hardware/gpio.h"
#include <stdio.h>

void reset_dp() {
    pulse_clock(50);
}

void jtag_to_swd_bit_seq() {
    // Send special JTAG-to-SWD select sequence
    // 0b1110011110011110
    send_data_lsb(0xE79E, 16);
}

void line_reset() {
    gpio_put(SWDIO, 0);
    pulse_clock(12);
}
// SWD Command Sequences {{{

void initialize_swd() {
    const unsigned int SLEEP_DUR = 10;

    // ----- Enable SWD ----- //
    // Reset current DP (currently JTAG-DP)
    reset_dp();
    delay();

    // Send special JTAG-to-SWD select sequence
    jtag_to_swd_bit_seq();
    delay();

    // Reset DP again (which is now SWD-DP)
    reset_dp();
    delay();

    // Reset the line by sending 12 clocks with SWDIO low
    line_reset();
    delay();
}

uint8_t setup_dp_and_mem_ap() {
    uint32_t data;
    uint8_t ack;
    // ------ Read IDCODE from SWD-DP ----- //
    ack = SWD_DP_read(0b00, &data);
    CHECK_ACK("Failed to read IDCODE.");
    printf("IDCODE: 0x%x ACK: %d\r\n", data, ack);
    delay();

    // ------ CTRL/STAT Write ----- //
    // Set CSYSPWRUPREQ and CDBGPWRUPREQ to bring rest of system online
    if ((ack = SWD_DP_write(0b10, 0x50000000)) != 1) {
        error("Error in CTRL/STAT write");
        return ack;
    }
    printf("CTRL/STAT Write: 0x5000.0000 ACK: %d\n", ack);
    delay();

    // ----- CTRL/STAT Read ----- //
    if ((ack = SWD_DP_read(0b10, &data)) != 1) {
        error("Error in CTRL/STAT read");
        return ack;
    }
    printf("CTRL/STAT Read: 0x%x ACK: %d\n", data, ack);
    delay();

    // ------ SELECT Write ----- //
    // Set APSEL to 0x00
    // Set APBANKSEL to 0b00
    // To access Memory AP
    if ((ack = SWD_DP_write(0b01, 0x00000000)) != 1) {
        error("Error in SELECT write");
        return ack;
    }
    printf("SELECT Write: 0x0000.0000 ACK: %d\n", ack);
    delay();

    // ------ Write to CSW of MEM-AP ----- //
    // To enable privledged master access over AHB-AP
    // Enable auto increment for TAR
    // 32-bit IO over DRW
    // Auto Increment
    //if (ack = SWD_AP_write(0b00, 0x22000012)) {
    // No Auto Increment
    if ((ack = SWD_AP_write(0b00, 0x22000002)) != 1) {
        error("Error in CSW write");
        return ack;
    }
    //printf("CSW Write: 0x22000012 ACK: %d\n", ack);
    printf("CSW Write: 0x22000002 ACK: %d\n", ack);
    delay();
}

// }}}
