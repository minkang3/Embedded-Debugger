/**
 * @file   swd_init.c
 * @author Min Kang
 * @brief  Logic to switch from JTAG to SWD
 */
#include "swd_init.h"
#include "macros.h"
#include "data_transfer.h"
#include "utils.h"
#include "hardware/gpio.h"
#include <stdio.h>

/**
 * @brief Sends 50 clock pulses to reset DP
 */
void reset_dp() {
    pulse_clock(50);
}

/**
 * @brief Sends special sequence that switches from JTAG to SWD
 *
 * Must be done after DP is reset
 */
void jtag_to_swd_bit_seq() {
    // Send special JTAG-to-SWD select sequence
    // 0b1110011110011110
    send_data_lsb(0xE79E, 16);
}

/**
 * @brief Clears JTAG State
 *
 * Sends 12 clock signals with SWDIO low
 */
void line_reset() {
    gpio_put(SWDIO, 0);
    pulse_clock(12);
}

/**
 * @brief Initializes SWD
 *
 * Resets DP, initiates the switching sequence, resets again and 
 * resets the line.
 */
void initialize_swd() {
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

/**
 * @brief Initialize DP and Mem-AP
 *
 * Enables CSYSPWRUPREQ and CDBGPWRUPREQ in CTRL/STAT register of DP which
 * powers up the debug system.
 */
uint8_t setup_dp_and_mem_ap() {
    uint32_t data;
    uint8_t ack;
    // ------ Read IDCODE from SWD-DP ----- //
    ack = SWD_DP_read(0b00, &data);
    CHECK_ACK_RT("Failed to read IDCODE.");
    printf("IDCODE: 0x%x ACK: %d\r\n", data, ack);
    delay();

    // ------ CTRL/STAT Write ----- //
    // Set CSYSPWRUPREQ and CDBGPWRUPREQ to bring rest of system online
    ack = SWD_DP_write(0b10, 0x50000000);
    CHECK_ACK_RT("Failed to write to CTRL/STAT");
    printf("CTRL/STAT Write: 0x5000.0000 ACK: %d\n", ack);
    delay();

    // ----- CTRL/STAT Read ----- //
    ack = SWD_DP_read(0b10, &data);
    CHECK_ACK_RT("Error in CTRL/STAT read");
    printf("CTRL/STAT Read: 0x%x ACK: %d\n", data, ack);
    delay();

    // ------ SELECT Write ----- //
    // Set APSEL to 0x00
    // Set APBANKSEL to 0b00
    // To access Memory AP
	// 0x00 is the MEM-AP on MOST targets but is *IMPLEMENTATION DEFINED*
	ack = SWD_DP_write(0b01, 0x00000000);
	CHECK_ACK_RT("Error in SELECT write");
    printf("SELECT Write: 0x0000.0000 ACK: %d\n", ack);
    delay();

    // ------ Write to CSW of MEM-AP ----- //
    // To enable privledged master access over AHB-AP
    // Enable auto increment for TAR
    // 32-bit IO over DRW
    // Auto Increment
    ack = SWD_AP_write(0b00, 0x22000012);
	CHECK_ACK_RT("Error in CSW write");
    printf("CSW Write: 0x22000012 ACK: %d\n", ack);
    delay();
}

