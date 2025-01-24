#include "setup.h"
#include "macros.h"
#include "pico/stdlib.h"

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
