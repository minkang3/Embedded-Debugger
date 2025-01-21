#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <boards/pico2.h>
#include <pico/time.h>
#include <stdio.h>
#include <stdint.h>

#include "bin.h"

#define CLOCK_DELAY 100
#define DELAY_MS 3
#define SMALL_DELAY_MS 10

#define SWDIO 19
#define SWCLK 20

#define BUTTON_PIN 26

// Macros {{{

#define CHECK_ACK(error_message) do { \
    if (ack != 1) { \
        if (ack == 0b010) { error("WAIT received"); } \
        error(error_message); \
    } \
} while (0)

#define CHECK_ACK_GT(error_message) do { \
    if (ack != 1) { \
        if (ack == 0b010) { error("WAIT received"); } \
        error(error_message); \
        goto LOOP; \
    } \
} while (0)

// }}}

// Type Definitions {{{
typedef struct _swd_req
{
    uint8_t start : 1;  // Always 1
    uint8_t APnDP : 1;  // Selects DPACC or APACC, 0 for DPACC, 1 for APACC
    uint8_t RnW : 1;    // Selects read or write, 0 for write, 1 for read
    uint8_t A : 2;      // Different meaning based on APnDP
    uint8_t parity : 1; // Parity check on APnDP,RnW,A bits, if num of 1's are even, parity is 0
    uint8_t stop : 1;   // Must always be 0 unless SWD is async which is never
    uint8_t park : 1;   // Always 1
} swd_req_t;

union Packet {
    swd_req_t req;
    uint16_t asInt;
};
// }}}

// Headers {{{

// ----- Setup ----- //
void pin_setup();
void button_setup();
void led_setup();
void setup();

// ----- JTAG to SWD switch ----- //
void initialize_swd();
uint8_t setup_dp_and_mem_ap();

// ----- Helpers ----- //
void error(char* msg);
void delay();
void small_delay();
void wait_for_button();
uint8_t calc_parity(uint32_t data);

// ----- Data Transfer Helpers ----- //
void single_pulse();
void pulse_clock(uint32_t num_pulses);
void send_data(uint16_t data, uint8_t len);
void send_data_lsb(uint32_t data, uint8_t len);
uint32_t read_data(uint8_t len);
uint32_t reverse(uint32_t num, uint8_t len);
swd_req_t create_swd_packet(uint8_t APnDP, uint8_t RnW, uint8_t A);
uint8_t conv_swd_packet_to_bin(swd_req_t packet);
void send_swd_packet(uint8_t APnDP, uint8_t RnW, uint8_t A);

// ----- Data Transfer ----- //
uint8_t SWD_DP_read(uint8_t A, uint32_t* data);
uint8_t SWD_DP_write(uint8_t A, uint32_t data);
uint8_t SWD_AP_read(uint8_t A, uint32_t* data);
uint8_t SWD_AP_write(uint8_t A, uint32_t data);

// ----- Core Functions ----- //
uint8_t SWD_core_single_read(uint32_t addr, uint32_t* data, char* reg_name);
uint8_t SWD_core_single_write(uint32_t addr, uint32_t data, char* reg_name);


// }}}

// ***** Main Function ***** {{{
int main(void) {
    stdio_init_all();
    setup();

    wait_for_button();
START:
    initialize_swd();

    printf("\033[36m***** SWD Initialized *****\033[0m\n\n");

    printf("DAP: \033[36mSetup DP and MEM-AP\033[0m\n");
    setup_dp_and_mem_ap();


    uint32_t data;
    uint8_t ack;
    // Read from DHCSR
    printf("\nCore Debug: \033[36mRead DHCSR\033[0m\n");
    ack = SWD_core_single_read(0xe000edf0, &data, "DHCSR");
    CHECK_ACK_GT("Error in DHCSR Read");
    delay();

    // Write to DHCSR - Halt the core and enable Debug mode
    printf("\nCore Debug: \033[36mEnable Debug Mode\033[0m\n");
    ack = SWD_core_single_write(0xe000edf0, 0xa05f0003, "DHCSR");
    CHECK_ACK_GT("");
    delay();

    // Check if write was successful
    printf("\nCore Debug: \033[36mRead DHCSR\033[0m\n");
    ack = SWD_core_single_read(0xe000edf0, &data, "DHCSR");
    CHECK_ACK_GT("");
    if (data & (1 << 1)) {
        error("\nCORE HALTED\n");
    }
    delay();

    // Enable Halt on Reset
    printf("\nCore Debug: \033[36mEnable Halt on Reset\033[0m\n");
    ack = SWD_core_single_write(0xe000edfc, 0x00000001, "DEMCR");
    CHECK_ACK_GT("");
    delay();

    printf("\nNVIC.AIRCR: \033[36mIssue SYSRESET\033[0m\n");
    ack = SWD_core_single_write(0xe000ed0c, 0x0afa0004, "AIRCR");
    CHECK_ACK_GT("");
    //ack = SWD_core_single_write(0xe000ed0c, 0xfa050004, "AIRCR");
    delay();

    // Read from DHCSR
    printf("\nCore Debug: \033[36mRead DHCSR\033[0m\n");
    ack = SWD_core_single_read(0xe000edf0, &data, "DHCSR");
    CHECK_ACK_GT("");
    delay();

    // Setup PC
    printf("\nCore Debug: \033[36mSetup PC\033[0m\n");
    ack = SWD_core_single_write(0xe000edf8, 0x20000041, "DCRDR");
    CHECK_ACK_GT("");
    delay();
    ack = SWD_core_single_write(0xe000edf4, 0x0001000f, "DCRSR");
    delay();

    // Setup MSP
    printf("\nCore Debug: \033[36mSetup MSP\033[0m\n");
    ack = SWD_core_single_write(0xe000edf8, 0x20004000, "DCRDR");
    delay();

    ack = SWD_core_single_write(0xe000edf4, 0x0001000d, "DCRSR");
    delay();

    // Relocate VTOR to SRAM
    printf("\nVTOR: \033[36mRelocate VTOR to SRAM\033[0m\n");
    ack = SWD_core_single_write(0xe000ed08, 0x20000000, "VTOR");




    
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
    ack = SWD_core_single_read(0xe000edf0, &data, "DHCSR");
    delay();


    // Write to DHCSR - Halt the core and enable Debug mode
    printf("\nCore Debug: \033[36mDisable Debug Mode and run core\033[0m\n");
    ack = SWD_core_single_write(0xe000edf0, 0xa05f0000, "DHCSR");
    delay();

    // Read from DHCSR
    printf("\nCore Debug: \033[36mRead DHCSR\033[0m\n");
    ack = SWD_core_single_read(0xe000edf0, &data, "DHCSR");
    if (data & (1 << 19)) {
        error("Processor locked because of an unrecoverable exception");
    }
    delay();


LOOP:
    sleep_ms(1000);
    wait_for_button();
    printf("\n\n\n");
    goto START;
    while (1);
}
// }}}

// Setup Functions {{{
void pin_setup() {
    // Initialize SWCLK and SWDIO for GPIO
    gpio_init(SWCLK);
    gpio_init(SWDIO);
    // Set GPIO direction to out
    gpio_set_dir(SWCLK, GPIO_OUT);
    gpio_set_dir(SWDIO, GPIO_OUT);
    // Initialize SWCLK and SWDIO high
    gpio_put(SWCLK, 1);
    gpio_put(SWDIO, 1);
}

void button_setup() {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
}

void led_setup() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}
void setup() {
    pin_setup();
    button_setup();
    led_setup();
}
// }}}

// Functions for JTAG to SWD switch {{{

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

// }}}

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
    if ((ack = SWD_DP_read(0b00, &data)) != 1) { 
        error("Error in IDCODE read");
        return ack;
    }
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

// Misc Helpers {{{

void error(char* msg) {
    printf("\033[31merror:\033[0m %s\n", msg);
}

void delay() {
    sleep_ms(DELAY_MS);
}

void small_delay() {
    sleep_ms(SMALL_DELAY_MS);
}

void wait_for_button() {
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    while (gpio_get(BUTTON_PIN) == 1);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
}

uint8_t calc_parity(uint32_t data) {
    uint8_t counter = 0;
    while (data) {
        data &= (data - 1);
        counter += 1;
    }
    return counter % 2;
}

// }}}

// Unused Functions {{{
void dump_flash() {
LOOP:
    // ------ SELECT Write ----- //
    // Set APSEL to 0x00
    // Set APBANKSEL to 0b00
    // To access Memory AP
    uint8_t ack;
    uint32_t data;
    if (ack = SWD_DP_write(0b01, 0x00000000)) {
        printf("Error in SELECT write\n");
        goto LOOP;
    }
    printf("SELECT Write: 0x0000.0000 ACK: %d\n", ack);
    delay();

    // ------ Write to CSW of MEM-AP ----- //
    // To enable privledged master access over AHB-AP
    // Enable auto increment for TAR
    // 32-bit IO over DRW
    if (ack = SWD_AP_write(0b00, 0x22000012)) {
        printf("Error in CSW write\n");
        goto LOOP;
    }
    printf("CSW Write: 0x22000012 ACK: %d\n", ack);

    delay();

    // Write to TAR to set address to read from
    if (ack = SWD_AP_write(0b10, 0x00000000)) {
        printf("Error in TAR write\n");
        goto LOOP;
    }
    printf("TAR Write: 0x00000000 ACK: %d\n", ack);
    delay();

    // Attempt to read flash
    if (ack = SWD_AP_read(0b11, &data)) {
        // If WAIT received
        if (ack == 0b010) {
            single_pulse();
            sleep_ms(5);
            // Read from RDBUFF from DP (Since we are getting WAIT ACK)
            if (SWD_DP_read(0b11, &data)) {
                printf("Error in RDBUFF read\n");
            } else {
                printf("RDBUFF Read: 0x%.8x ACKed\n", data);
            }
        }
    } else {
        printf("%x\n", data);
    }
    delay();


    printf("\nFLASH:\n");
    int i;
    for (i = 0; i < 20; ++i) {
        // Read flash
        if (SWD_AP_read(0b11, &data)) {
            printf("Error in flash read\n");
        } else {
            printf("%.8x\n", data);
        }
        delay();
    }
}
// }}}

// Data Transfer Helpers {{{

void single_pulse() {
    gpio_put(SWCLK, 0);
    sleep_us(CLOCK_DELAY);
    gpio_put(SWCLK, 1);
    sleep_us(CLOCK_DELAY);
}

void pulse_clock(uint32_t num_pulses) {
    uint32_t i;
    for (i = 0; i < num_pulses; ++i) {
        gpio_put(SWCLK, 0);
        sleep_us(CLOCK_DELAY);
        gpio_put(SWCLK, 1);
        sleep_us(CLOCK_DELAY);
    }
}

void send_data(uint16_t data, uint8_t len) {
    gpio_set_dir(SWDIO, GPIO_OUT);
    int8_t pos;
    for (pos = len - 1; pos >= 0; --pos) {
        gpio_put(SWCLK, 0);
        gpio_put(SWDIO, ((data & (1 << pos)) >> pos));
        sleep_us(CLOCK_DELAY);
        gpio_put(SWCLK, 1);
        sleep_us(CLOCK_DELAY);
    }
}

void send_data_lsb(uint32_t data, uint8_t len) {
    gpio_set_dir(SWDIO, GPIO_OUT);
    int i;
    for (i = 0; i < len; ++i) {
        gpio_put(SWCLK, 0);
        gpio_put(SWDIO, data & 1);
        data >>= 1;
        sleep_us(CLOCK_DELAY);
        gpio_put(SWCLK, 1);
        sleep_us(CLOCK_DELAY);
    }
    gpio_put(SWDIO, 1);
}

uint32_t read_data(uint8_t len) {
    gpio_set_dir(SWDIO, GPIO_IN);
    uint32_t data = 0;
    int8_t pos;
    for (pos = 0; pos < len; ++pos) {
        gpio_put(SWCLK, 0);
        sleep_us(CLOCK_DELAY);
        data |= (gpio_get(SWDIO) << pos);
        gpio_put(SWCLK, 1);
        sleep_us(CLOCK_DELAY);
    }
    return data;
}

uint32_t reverse(uint32_t num, uint8_t len) {
    uint32_t reversed = 0;
    int8_t pos;
    for (pos = 0; pos < len; ++pos) {
        reversed <<= 1; reversed |= (num & 1);
        num >>= 1;
    }
    return reversed;
}

swd_req_t create_swd_packet(uint8_t APnDP, uint8_t RnW, uint8_t A) {
    // For calculating parity
    uint8_t num_1s = 0;
    num_1s += APnDP;
    num_1s += RnW;
    num_1s += ((A & (1 << 1)) >> 1) + (A & 1);

    swd_req_t packet = {
        .start = 1,
        .APnDP = APnDP,
        .RnW = RnW,
        .A = A,
        .parity = num_1s % 2,
        .stop = 0,
        .park = 1,
    };

    return packet;
}

uint8_t conv_swd_packet_to_bin(swd_req_t packet) {
    uint8_t data = 0;
    data |= (packet.start << 7);
    data |= (packet.APnDP << 6);
    data |= (packet.RnW << 5);
    data |= (packet.A << 3);
    data |= (packet.parity << 2);
    data |= (packet.stop << 1);
    data |= (packet.park << 0);
    return data;
}

void send_swd_packet(uint8_t APnDP, uint8_t RnW, uint8_t A) {
    swd_req_t packet = create_swd_packet(APnDP, RnW, A);
    uint8_t data = conv_swd_packet_to_bin(packet);
    send_data(data, 8);
}

// }}}

// Data Transfer Functions {{{

uint8_t SWD_DP_read(uint8_t A, uint32_t* data) {
    // Parameters (in order):
    //   APnDP - 0 for DP, 1 for AP
    //   RnW - 0 for write, 1 for read
    //   A
    send_swd_packet(0, 1, A);

    // Change GPIO dir of SWDIO to read in ACK
    gpio_set_dir(SWDIO, GPIO_IN);

    // Turnaround cycle while passing control to TARGET
    single_pulse();

    // Read ACK
    uint8_t ack = read_data(3);
    // If ACK isn't OK, return Error (indicated by 1)
    if (ack != 0b001) {
        return ack;
    }

    // Read data sent from TARGET
    *data = read_data(32);
    // TODO: Confirm parity of data
    pulse_clock(2);

    gpio_set_dir(SWDIO, GPIO_OUT);
    gpio_put(SWDIO, 1);

    return ack;
}

uint8_t SWD_DP_write(uint8_t A, uint32_t data) {
    // Calculate data parity for later
    uint8_t data_parity = calc_parity(data);

    // Parameters (in order):
    //   APnDP - 0 for DP, 1 for AP
    //   RnW - 0 for write, 1 for read
    //   A
    send_swd_packet(0, 0, A);

    // Change GPIO dir of SWDIO to read in ACK
    gpio_set_dir(SWDIO, GPIO_IN);

    // Turnaround cycle while passing control to TARGET
    single_pulse();

    // Read ACK
    uint8_t ack = read_data(3);
    // If ACK isn't OK, return Error (indicated by 1)
    if (ack != 0b001) {
        return ack;
    }

    // Turnaround cycle for control to return back to HOST
    single_pulse();

    // Send data to target
    send_data_lsb(data, 32);
    send_data(data_parity, 1);

    gpio_set_dir(SWDIO, GPIO_OUT);
    gpio_put(SWDIO, 1);

    return ack;
}

uint8_t SWD_AP_read(uint8_t A, uint32_t* data) {
    // Parameters (in order):
    //   APnDP - 0 for DP, 1 for AP
    //   RnW - 0 for write, 1 for read
    //   A
    send_swd_packet(1, 1, A);

    // Change GPIO dir of SWDIO to read in ACK
    gpio_set_dir(SWDIO, GPIO_IN);

    // Turnaround cycle while passing control to TARGET
    single_pulse();

    // Read ACK
    uint8_t ack = read_data(3);
    // If ACK isn't OK, return Error (indicated by 1)
    if (ack != 0b001) {
        if (ack == 0b010) single_pulse();
        return ack;
    }

    // Read data sent from TARGET
    *data = read_data(32);
    // TODO: Confirm parity of data
    pulse_clock(2);

    gpio_set_dir(SWDIO, GPIO_OUT);
    gpio_put(SWDIO, 1);

    return ack;
}

// TODO: Maybe change ack to be a parameter, so that return
    // can be just 0 or 1 (to be simpler)
uint8_t SWD_AP_write(uint8_t A, uint32_t data) {
    // Calculate data parity for later
    uint8_t data_parity = calc_parity(data);

    // Parameters (in order):
    //   APnDP - 0 for DP, 1 for AP
    //   RnW - 0 for write, 1 for read
    //   A
    send_swd_packet(1, 0, A);

    // Change GPIO dir of SWDIO to read in ACK
    gpio_set_dir(SWDIO, GPIO_IN);

    // Turnaround cycle while passing control to TARGET
    single_pulse();

    // Read ACK
    uint8_t ack = read_data(3);
    // If ACK isn't OK, return Error (indicated by 1)
    if (ack != 0b001) {
        return ack;
    }

    // Turnaround cycle for control to return back to HOST
    single_pulse();

    // Send data to target
    send_data_lsb(data, 32);
    send_data(data_parity, 1);

    gpio_set_dir(SWDIO, GPIO_OUT);
    gpio_put(SWDIO, 1);

    return ack;
}

// }}}

// Core Functions {{{

uint8_t SWD_core_single_read(uint32_t addr, uint32_t* data, char* reg_name) {
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

uint8_t SWD_core_single_write(uint32_t addr, uint32_t data, char* reg_name) {
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
