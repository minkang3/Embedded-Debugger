/**
 * @file data_transfer.c
 * @author Min Kang
 * @brief AP and DP read and write functions
 */
#include "data_transfer.h"
#include "pico/stdlib.h"
#include "macros.h"
#include "utils.h"

/**
 * @brief Read request to DP
 *
 * @param A 2 bits, used to select register
 * @param data Pointer to where data should be read into
 * 
 * @return ACK of request
 */
uint8_t SWD_DP_read(uint8_t A, uint32_t* data) {
	// Send SWD packet
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
    // NOTE: Parity isn't being checked so we pulse twice instead of once
    pulse_clock(2);

    gpio_set_dir(SWDIO, GPIO_OUT);
    gpio_put(SWDIO, 1);

    return ack;
}

/**
 * @brief Write request to DP
 * 
 * @param A 2 bit register selector
 * @param data Data to be written
 *
 * @return ACK of request
 */
uint8_t SWD_DP_write(uint8_t A, uint32_t data) {
    // Calculate data parity for later
    uint8_t data_parity = calc_parity(data);

	// Send SWD packet
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

/**
 * @brief Read request to AP
 *
 * @param A 2 bits, used to select register
 * @param data Pointer to where data should be read into
 * 
 * @return ACK of request
 */
uint8_t SWD_AP_read(uint8_t A, uint32_t* data) {
	// Send SWD packet
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
    // NOTE: Parity isn't being checked so we pulse twice instead of once
    pulse_clock(2);

    gpio_set_dir(SWDIO, GPIO_OUT);
    gpio_put(SWDIO, 1);

    return ack;
}

/**
 * @brief Write request to AP
 * 
 * @param A 2 bits, used to select registor
 * @param data Data to be written, 32 bits.
 *
 * @return ACK of request
 */
uint8_t SWD_AP_write(uint8_t A, uint32_t data) {
    // Calculate data parity
    uint8_t data_parity = calc_parity(data);

	// Send SWD packet
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

	// Set GPIO dir back to out
    gpio_set_dir(SWDIO, GPIO_OUT);
    gpio_put(SWDIO, 1);

    return ack;
}

/**
 * @brief Send a single clock pulse
 */
void single_pulse() {
    gpio_put(SWCLK, 0);
    sleep_us(CLOCK_DELAY);
    gpio_put(SWCLK, 1);
    sleep_us(CLOCK_DELAY);
}

/**
 * @prief Pulse the clock a certain number of times
 * 
 * Useful for resetting DP
 */
void pulse_clock(uint32_t num_pulses) {
    uint32_t i;
    for (i = 0; i < num_pulses; ++i) {
        gpio_put(SWCLK, 0);
        sleep_us(CLOCK_DELAY);
        gpio_put(SWCLK, 1);
        sleep_us(CLOCK_DELAY);
    }
}

/**
 * @brief Sends data while pulsing clock
 * 
 * @param data Data to be sent
 * @param len Length of data in bits
 */
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

/**
 * @brief Sends data lsb first
 * 
 * SWD protocol uses little endian, so this is used frequently.
 *
 * @param data Data to be sent
 * @param len Length of data in bits
 */
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

/**
 * @brief Reads a certain number of bits (max 32) from SWDIO, used after a read request
 * @param len Length of read, max 32
 * @return Data read
 */
uint32_t read_data(uint8_t len) {
	if (len > 32) {
		error("Attempt to read data larger than 32 bits");
		return 0;
	}
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

/**
 * @brief Creates an swd packet
 *
 * Automatically calculates parity
 * 
 * @param APnDP 1 bit, selects AP or DP. 0 for DP, 1 for AP.
 * @param RnW 1 bit, selects a read or write request. 0 for write, 1 for read.
 * @param A 2 bit register selector (for most types of requests).
 */
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

/**
 * @brief Converts an swd_req_t into a binary format
 *
 * @param swd_req_t An SWD request struct
 * @return The request packet in binary 
 */
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

/**
 * @brief Creates an SWD packet and sends it over SWDIO
 * 
 * @param APnDP 1 bit, selects AP or DP. 0 for DP, 1 for AP.
 * @param RnW 1 bit, selects a read or write request. 0 for write, 1 for read.
 * @param A 2 bit register selector (for most types of requests).
 */
void send_swd_packet(uint8_t APnDP, uint8_t RnW, uint8_t A) {
    swd_req_t packet = create_swd_packet(APnDP, RnW, A);
    uint8_t data = conv_swd_packet_to_bin(packet);
    send_data(data, 8);
}
