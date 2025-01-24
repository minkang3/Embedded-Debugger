#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <boards/pico2.h>
#include <pico/time.h>
#include <stdio.h>
#include <stdint.h>

#include "data_transfer.h"
#include "macros.h"
#include "setup.h"
#include "utils.h"
#include "swd_init.h"
#include "core.h"

#include "bin.h"


uint8_t read_pc(uint32_t* data);
uint8_t single_step();


// ***********************************
// ********** MAIN FUNCTION **********
// ***********************************
int main(void) {
    char c;
    stdio_init_all();
    setup();
    sleep_ms(1500);
START:
    printf("Enter any key to initialize SWD...\n");
    get_keypress();
    initialize_swd();

    printf("\033[36m***** SWD Initialized *****\033[0m\n\n");

    printf("DAP: \033[36mSetup DP and MEM-AP\033[0m\n");
    setup_dp_and_mem_ap();


    uint32_t data;
    uint8_t ack;
    // Read from DHCSR
    printf("\nCore Debug: \033[36mRead DHCSR\033[0m\n");
    ack = SWD_core_single_read_pf(0xe000edf0, &data, "DHCSR");
    CHECK_ACK_GT("Error in DHCSR Read");
    delay();

    // Write to DHCSR - Halt the core and enable Debug mode
    printf("\nCore Debug: \033[36mEnable Debug Mode\033[0m\n");
    ack = SWD_core_single_write_pf(0xe000edf0, 0xa05f0003, "DHCSR");
    CHECK_ACK_GT("");
    delay();

    // Check if write was successful
    printf("\nCore Debug: \033[36mRead DHCSR\033[0m\n");
    ack = SWD_core_single_read_pf(0xe000edf0, &data, "DHCSR");
    CHECK_ACK_GT("");
    if (data & (1 << 1)) {
        printf("\nCORE HALTED\n");
    }
    delay();

    // Enable Halt on Reset
    printf("\nCore Debug: \033[36mEnable Halt on Reset\033[0m\n");
    ack = SWD_core_single_write_pf(0xe000edfc, 0x00000001, "DEMCR");
    CHECK_ACK_GT("");
    delay();

    printf("\nNVIC.AIRCR: \033[36mIssue SYSRESET\033[0m\n");
    ack = SWD_core_single_write_pf(0xe000ed0c, 0x0afa0004, "AIRCR");
    CHECK_ACK_GT("");
    //ack = SWD_core_single_write(0xe000ed0c, 0xfa050004, "AIRCR");
    delay();

    // Read from DHCSR
    printf("\nCore Debug: \033[36mRead DHCSR\033[0m\n");
    ack = SWD_core_single_read_pf(0xe000edf0, &data, "DHCSR");
    CHECK_ACK_GT("");
    delay();

    // Setup PC
    printf("\nCore Debug: \033[36mSetup PC\033[0m\n");
    ack = SWD_core_single_write_pf(0xe000edf8, 0x20000041, "DCRDR");
    CHECK_ACK_GT("");
    delay();
    ack = SWD_core_single_write_pf(0xe000edf4, 0x0001000f, "DCRSR");
    delay();

    // Setup MSP
    printf("\nCore Debug: \033[36mSetup MSP\033[0m\n");
    ack = SWD_core_single_write_pf(0xe000edf8, 0x20004000, "DCRDR");
    delay();

    ack = SWD_core_single_write_pf(0xe000edf4, 0x0001000d, "DCRSR");
    delay();

    // Relocate VTOR to SRAM
    printf("\nVTOR: \033[36mRelocate VTOR to SRAM\033[0m\n");
    ack = SWD_core_single_write_pf(0xe000ed08, 0x20000000, "VTOR");


    // *******************************
    // ----- Write Bin to Ram ----- //
    // *******************************
    printf("\n\033[36mWriting bin to RAM\033[0m\n");

    // Enable auto increment
    ack = SWD_AP_write(0b00, 0x22000012);
    CHECK_ACK_GT("Error in CSW write");
    delay();
    // Write Target Address
    ack = SWD_AP_write(0b10, 0x20000000);
    CHECK_ACK_GT("Error in TAR write");
    delay();


    int index = 0;
    uint32_t word;
    for (index = 0; index < blink_bin_len; index += 4) {
        word = *(uint32_t *)&blink_bin[index];
        printf("CODE : %x 0x%.8x\n", index, word);
        ack = SWD_AP_write(0b11, word);
        CHECK_ACK_GT("Error in reading code");
        delay();
    }


    // ----- Verify Write to RAM ----- //
    printf("\n\033[36mReading bin from RAM\033[0m\n");
    ack = SWD_AP_write(0b10, 0x20000000);
    CHECK_ACK_GT("Error in TAR write");
    delay();

    ack = SWD_AP_read(0b11, &word);
    ack = SWD_AP_read(0b11, &word); // HACK: Why does reading twice make it work?
    for (index = 0; index < blink_bin_len; index += 4) {
        ack = SWD_AP_read(0b11, &word);
        CHECK_ACK_GT("Error in RAM Read");
        if (ack != 1) {
            printf("At index: %d\n", index);
        }
        if (word != *(uint32_t *)&blink_bin[index]) {
            printf("NOT EQUAL\n");
            printf("At index: %d\n", index);
            printf("0x%.8x != 0x%.8x\n", word, *(uint32_t *)&blink_bin[index]);
            goto LOOP;
        }
    }
    printf("\n\033[36mVerification Successful\033[0m\n");

    delay();

    // Read from DHCSR
    printf("\nCore Debug: \033[36mRead DHCSR\033[0m\n");
    ack = SWD_core_single_read_pf(0xe000edf0, &data, "DHCSR");
    delay();



    /* // Read PC */
    /* printf("\nPC: "); */
    /* ack = SWD_core_single_write(0xe000edf4, ((0 << 16) | 0x0F)); */
    /* CHECK_ACK_GT("DCRSR write"); */
    /* ack = SWD_core_single_read(0xe000edf8, &data); */
    /* CHECK_ACK_GT("DCRDR read"); */
    /* printf("\nPC: 0x%.8x", data); */

    /* // Write to DHCSR - Single step through the program */
    /* printf("\nCore Debug: \033[36mSingle Step through the program\033[0m\n"); */
    /* ack = SWD_core_single_write_pf(0xe000edf0, 0xa05f0005, "DHCSR"); */
    /* delay(); */

    // Read PC
    ack = read_pc(&data);
    CHECK_ACK_GT("PC Read");
    printf("PC: 0x%x\n", data);

    while (1) {
        wait_for_button();
        single_step();
        delay();
        ack = read_pc(&data);
        CHECK_ACK_GT("PC Read");
        printf("PC: 0x%x\n", data);
        sleep_ms(300);
    }



LOOP:
    sleep_ms(1000);

    printf("\n\n\n");
    printf("Resetting Pico...\n");
    goto START;
    while (1);
}


uint8_t read_pc(uint32_t* data) {
    uint8_t ack;
    ack = SWD_core_single_write(0xe000edf4, ((0 << 16) | 0x0F));
    CHECK_ACK_RT("DCRSR write");
    ack = SWD_core_single_read(0xe000edf8, data);
    CHECK_ACK_RT("DCRDR read");
    return ack;
}

uint8_t single_step() {
    // Write to DHCSR - Single step through the program
    return SWD_core_single_write(0xe000edf0, 0xa05f0005);
}
