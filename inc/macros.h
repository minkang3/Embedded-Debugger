#ifndef MACROS_H
#define MACROS_H

#define BUF_LEN 64
#define TOKENS_LEN 16

#define CORE_DHCSR 0xe000edf0
#define CORE_DCRDR 0xe000edf8
#define CORE_DCRSR 0xe000edf4
#define CORE_DEMCR 0xe000edfc 

#define CORE_VTOR 0xe000ed08
#define NVIC_AIRCR 0xe000ed0c



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
