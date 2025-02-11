/**
 * @file setup.h
 * @author Min Kang
 * @brief Setup headers
 */
#ifndef SETUP_H
#define SETUP_H

/**
 * @brief Setup SWCLK and SWDIO pins
 */
void pin_setup();

/**
 * @brief Setup pin to detect button press
 */
void button_setup();

/**
 * @brief Initialize LED
 */
void led_setup();

/**
 * @brief Call all setup functions
 */
void setup();

#endif
