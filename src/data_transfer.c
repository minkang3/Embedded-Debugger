#include "data_transfer.h"
#include "pico/stdlib.h"
#include "macros.h"
#include "utils.h"

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

// Helpers
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
