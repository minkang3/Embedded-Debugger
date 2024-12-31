#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <boards/pico2.h>
#include <pico/time.h>
#include <stdint.h>

#define CLOCK_DELAY 100
#define CLOCK_DELAY_HALF 50

#define SWDIO 19
#define SWCLK 20

#define BUTTON_PIN 26

typedef struct _swd_req
{
    uint8_t start : 1;  // Always 1
    uint8_t APnDP : 1;  // Selects DPACC or APACC, 0 for DPACC, 1 for APACC
    uint8_t RnW : 1;    // Selects read or write, 0 for write, 1 for read
    uint8_t A : 2;      // Different meaning based on APnDP
    uint8_t parity : 1; // Parity check on APnDP,RnW,A bits, if num of 1's are even, parity is 0
    uint8_t stop : 1;   // Must always be 0 unless SWD is async which is never
    uint8_t park : 1;   // Always 1
} swd_req;

union Packet {
    swd_req req;
    uint16_t asInt;
};

void pin_setup();
void button_setup();
void led_setup();
void pulse_clock(uint32_t num_pulses);
void send_data(uint16_t data, uint8_t len);
void send_data_lsb(uint16_t data, uint8_t len);
void send_swd_packet(swd_req);

void wait_for_button();

void initialize_swd();


void setup() {
    pin_setup();
    button_setup();
    led_setup();
}

int main(void) {
    stdio_init_all();
    setup();

    wait_for_button();

    initialize_swd();

    // Read IDCODE from SWD-DP
    swd_req req = {
        .start = 1,
        .APnDP = 0,
        .RnW = 1,
        .A = 0b00,
        .parity = 1,
        .stop = 0,
        .park = 1,
    };

    send_swd_packet(req);
    pulse_clock(35);

    while (1);
}

// Misc Helpers {{{

void wait_for_button() {
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    while (gpio_get(BUTTON_PIN) == 1);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
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
// }}}

// Data transfer functions {{{
void pulse_clock(uint32_t num_pulses) {
    gpio_put(SWCLK, 0);
    sleep_us(CLOCK_DELAY);
    uint32_t i;
    for (i = 0; i < num_pulses - 1; ++i) {
        gpio_put(SWCLK, 1);
        sleep_us(CLOCK_DELAY);
        gpio_put(SWCLK, 0);
        sleep_us(CLOCK_DELAY);
    }
    sleep_us(CLOCK_DELAY);
    gpio_put(SWCLK, 1);
}

void send_data(uint16_t data, uint8_t len) {
    gpio_put(SWCLK, 0);
    int8_t pos;
    for (pos = len - 1; pos >= 0; --pos) {
        gpio_put(SWDIO, ((data & (1 << pos)) >> pos));
        sleep_us(CLOCK_DELAY);
        gpio_put(SWCLK, 1);
        sleep_us(CLOCK_DELAY);
        gpio_put(SWCLK, 0);
    }
    gpio_put(SWCLK, 1);
    gpio_put(SWDIO, 1);
}

void send_data_lsb(uint16_t data, uint8_t len) {
    gpio_put(SWCLK, 0);
    int i;
    for (i = 0; i < len; ++i) {
        gpio_put(SWDIO, data & 1);
        data >>= 1;
        sleep_us(CLOCK_DELAY);
        gpio_put(SWCLK, 1);
        sleep_us(CLOCK_DELAY);
        gpio_put(SWCLK, 0);
    }
    gpio_put(SWCLK, 1);
    gpio_put(SWDIO, 1);
}

void send_swd_packet(swd_req req) {
    uint16_t data = 0;
    data |= (req.start << 7);
    data |= (req.APnDP << 6);
    data |= (req.RnW << 5);
    data |= (req.A << 3);
    data |= (req.parity << 2);
    data |= (req.stop << 1);
    data |= (req.park << 0);
    send_data(data, 8);
}

//void send_read_data(uint16_t data, uint8_t len) {
//    gpio_put(SWCLK, 0);
//    int pos;
//    for (pos = len - 1; pos >= len; --pos) {
//        gpio_put(SWDIO, ((data & (1 << pos)) >> pos));
//        sleep_us(CLOCK_DELAY);
//        gpio_put(SWCLK, 1);
//        sleep_us(CLOCK_DELAY);
//        gpio_put(SWCLK, 0);
//    }
//    gpio_set_dir(SWDIO, GPIO_IN);
//}

// }}}

// SWD Functions for JTAG to SWD {{{

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
}

// }}}

