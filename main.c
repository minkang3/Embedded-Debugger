#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include <boards/pico2.h>
#include <pico/time.h>
#include <stdint.h>

#define CLOCK_DELAY 100
#define CLOCK_DELAY_HALF 50

#define SWDIO 19
#define SWCLK 20

#define BUTTON_PIN 26

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

// ----- Helpers ----- //
void wait_for_button();
uint8_t calc_parity(uint32_t data);

// ----- Data Transfer ----- //
void single_pulse();
void pulse_clock(uint32_t num_pulses);
void send_data(uint16_t data, uint8_t len);
void send_data_lsb(uint32_t data, uint8_t len);
uint32_t read_data(uint8_t len);
uint32_t reverse(uint32_t num, uint8_t len);
swd_req_t create_swd_packet(uint8_t APnDP, uint8_t RnW, uint8_t A);
uint8_t conv_swd_packet_to_bin(swd_req_t packet);
void send_swd_packet(uint8_t APnDP, uint8_t RnW, uint8_t A);

uint8_t SWD_DP_read(uint8_t A, uint32_t* data);
uint8_t SWD_DP_write(uint8_t A, uint32_t data);
uint8_t SWD_AP_read(uint8_t A, uint32_t* data);
uint8_t SWD_AP_write(uint8_t A, uint32_t data);


// }}}

// ***** Main Function ***** {{{
int main(void) {
    stdio_init_all();
    setup();

START:
    wait_for_button();
    initialize_swd();

    // ------ Read IDCODE from SWD-DP ----- //
    uint32_t data;
    if (SWD_DP_read(0b00, &data)) { 
        printf("Error in IDCODE read\n");
        goto LOOP;
    }
    printf("IDCODE: 0x%x ACKed\r\n", data);
    sleep_ms(10);


    // ------ CTRL/STAT Write ----- //
    // Set CSYSPWRUPREQ and CDBGPWRUPREQ to bring rest of system online
    if (SWD_DP_write(0b10, 0x50000000)) {
        printf("Error in CTRL/STAT write\n");
        goto LOOP;
    }
    printf("CTRL/STAT Write: 0x5000.0000 ACKed\n");
    sleep_ms(10);

    // ----- CTRL/STAT Read ----- //
    if (SWD_DP_read(0b10, &data)) {
        printf("Error in CTRL/STAT read\n");
        goto LOOP;
    }
    printf("CTRL/STAT Read: 0x%x ACKed\n", data);
    sleep_ms(10);

    // ------ SELECT Write ----- //
    // Set APSEL to 0x00
    // Set APBANKSEL to 0b00
    // To access Memory AP
    if (SWD_DP_write(0b01, 0x00000000)) {
        printf("Error in SELECT write\n");
        goto LOOP;
    }
    printf("SELECT Write: 0x0000.0000 ACKed\n");
    sleep_ms(10);

    // ------ Write to CSW of MEM-AP ----- //
    // To enable privledged master access over AHB-AP
    // Enable auto increment for TAR
    // 32-bit IO over DRW
    if (SWD_AP_write(0b00, 0x22000012)) {
        printf("Error in CSW write\n");
        goto LOOP;
    }
    printf("CSW Write: 0x22000012 ACKed\n");

    sleep_ms(10);

    // Write to TAR to set address to read from
    if (SWD_AP_write(0b10, 0x00000000)) {
        printf("Error in TAR write\n");
        goto LOOP;
    }
    printf("TAR Write: 0x00000000 ACKed\n");
    sleep_ms(10);

    // Attempt to read flash
    uint8_t ack;
    if (ack = SWD_AP_read(0b11, &data)) {
        // If WAIT received
        if (ack == 0b010) {
            single_pulse();
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
    sleep_ms(10);


    printf("\nFLASH:\n");
    int i;
    for (i = 0; i < 20; ++i) {
        // Read flash
        if (SWD_AP_read(0b11, &data)) {
            printf("Error in flash read\n");
        } else {
            printf("%.8x\n", data);
        }
        sleep_ms(10);
    }


LOOP:
    sleep_ms(1000);
    goto START;
    while (1);
}
// }}}

// Setup functions {{{
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

void initialize_swd() {
    const unsigned int SLEEP_DUR = 10;

    // ----- Enable SWD ----- //
    // Reset current DP (currently JTAG-DP)
    reset_dp();
    sleep_ms(SLEEP_DUR);

    // Send special JTAG-to-SWD select sequence
    jtag_to_swd_bit_seq();
    sleep_ms(SLEEP_DUR);

    // Reset DP again (which is now SWD-DP)
    reset_dp();
    sleep_ms(SLEEP_DUR);

    // Reset the line by sending 12 clocks with SWDIO low
    line_reset();
    sleep_ms(10);

    printf("** SWD Initialized **\r\n");
}

// }}}

// Misc Helpers {{{

void wait_for_button() {
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    while (gpio_get(BUTTON_PIN) == 1);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
}

uint8_t calc_parity(uint32_t data) {
    uint8_t parity = 0;
    while (data) {
        parity ^= (data & 1);
        data &= (data - 1);
    }
    return parity;
}

// }}}

// Data transfer functions {{{

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

    return 0;
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

    return 0;
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
        return ack;
    }

    // Read data sent from TARGET
    *data = read_data(32);
    // TODO: Confirm parity of data
    pulse_clock(2);

    gpio_set_dir(SWDIO, GPIO_OUT);
    gpio_put(SWDIO, 1);

    return 0;
}

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

    return 0;
}

// }}}

