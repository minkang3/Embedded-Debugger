#ifndef MACROS_H
#define MACROS_H

#define CLOCK_DELAY 100
#define DELAY_MS 3
#define SMALL_DELAY_MS 10

#define SWDIO 19
#define SWCLK 20

#define BUTTON_PIN 26


#define CHECK_ACK(error_message) do { \
    if (ack != 1) { \
        if (ack == 0b010) { error("WAIT received"); } \
        error_ack(error_message, ack); \
    } \
} while (0)

#define CHECK_ACK_GT(error_message) do { \
    if (ack != 1) { \
        if (ack == 0b010) { error("WAIT received"); } \
        error_ack(error_message, ack); \
        goto LOOP; \
    } \
} while (0)

#define CHECK_ACK_RT(error_message) do { \
    if (ack != 1) { \
        if (ack == 0b010) { error("WAIT received"); } \
        error_ack(error_message, ack); \
        return ack; \
    } \
} while (0)

#endif
