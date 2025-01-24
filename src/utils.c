#include "utils.h"
#include "macros.h"

#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdio.h>


void error(char* msg) {
    printf("\033[31merror:\033[0m %s\n", msg);
}

void error_ack(char* msg, uint8_t ack) {
    printf("\033[31merror:\033[0m %s ACK: %d\n", msg, ack);
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

char get_keypress() {
    char c;
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    scanf("%c", &c);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    return c;
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
