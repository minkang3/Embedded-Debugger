/**
 * @file utils.c
 * @author Min Kang
 * @brief Miscellaneous utility functions
 */
#include "utils.h"
#include "macros.h"

#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdio.h>

/**
 * @brief Print a colored error message
 * 
 * @param msg c-string that contains error message
 */
void error(char* msg) {
    printf("\033[31merror:\033[0m %s\n", msg);
}

/**
 * @brief Print a colored error message as well as the ACK code
 *
 * @param msg c-string that contains error message
 * @param ack ACK to display after error message
 */
void error_ack(char* msg, uint8_t ack) {
    printf("\033[31merror:\033[0m %s ACK: %d\n", msg, ack);
}

/**
 * @brief Waits a DELAY_MS amount of time
 */
void delay() {
    sleep_ms(DELAY_MS);
}

/**
 * @brief Blocks until button is pressed
 * 
 * Gets stuck in loop until high voltage is received in BUTTON_PIN
 */
void wait_for_button() {
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    while (gpio_get(BUTTON_PIN) == 1);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
}

/**
 * @brief Get single keypress from uart
 *
 * @return Key that was pressed
 */
char get_keypress() {
    char c;
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    scanf("%c", &c);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    return c;
}

/**
 * @brief Get a line of text from user
 * 
 * Receive text from user until new line is received
 * Can also handle backspaces
 */
void get_line(char* buf, int8_t buf_size) {
    char c;
    int8_t i = 0;
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    printf("> ");
    do {
        scanf("%c", &c);
        printf("%c", c);
        if (c == 127 || c == '\b') {
            if (i != 0) {
                i--;
                printf("\b \b");
            }
        } else if (c != '\n' && c != '\r')
            buf[i++] = c;
    } while ((c != '\n' && c != '\r') && i < buf_size - 1);
    if (i != 0)
        buf[i] = '\0';
    printf("\n");
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
}

/**
 * @brief Calculate parity of data
 *
 * @param data Data you want to calculate parity for
 *
 * @return Even parity (0 if number of 1's even)
 */
uint8_t calc_parity(uint32_t data) {
    uint8_t counter = 0;
    while (data) {
        data &= (data - 1);
        counter += 1;
    }
    return counter % 2;
}

/**
 * @brief Compares just the first character of a string and if str is len of 1
 *
 * @param str c-string to compare
 * @param c char to compare against
 *
 * @return 0 if equal, 1 if not
 */
uint8_t single_cmp(char* str, char c) {
    return !(str[0] == c && str[1] == '\0');
}

/**
 * @brief Tokenize a cstring in place
 * 
 * @param buf String to tokenize
 * @param buf_max_len Maximum length of buffer
 * @param tokens Array of char pointers that will hold the tokens
 * @param token_max_len Maximum length of token array
 * @param num_tokens Stores number of tokens
 *
 * @return 1 for error, 0 for success
 */
uint8_t tokenize(char* buf, uint8_t buf_max_len, char** tokens, int8_t token_max_len, uint8_t* num_tokens) {
    if (token_max_len < 1 || buf_max_len < 1) return 1;
    *num_tokens = 0;
    *tokens++ = buf; ++(*num_tokens);
    uint8_t i = 0;
    while (*buf != '\0' && i < buf_max_len && *num_tokens < token_max_len) {
        ++buf; ++i;
        while (*buf == ' ') {
            *buf = '\0';
            ++buf; ++i;
            if (*buf != ' ') {
                *tokens++ = buf; ++(*num_tokens);
            }
        }
    }
    *tokens = NULL;
    return 0;
}

int power(int base, int pow) {
    int result = base;
    if (pow == 0) return 1;
    while (--pow) {
        result *= base;
    }
    return result;
}

int8_t parse_char_to_hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int parse_str_to_hex(char* str, uint32_t* hex) {
    int val;
    if (*str != '0' && *(str+1) != 'x') return 1;
    *hex = 0;
    str += 2;
    for (uint8_t i = 0; i < 8; ++i) {
        if (str[i] == '\0') return 1;
        val = parse_char_to_hex(str[i]);
        if (val < 0) return 1;
        *hex += power(16, 7 - i) * val;
    }
    return 0;
}
