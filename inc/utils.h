/**
 * @file utils.h
 * @author Min Kang
 * @brief Utilities header
 */
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

/**
 * @brief Print a colored error message
 * 
 * @param msg c-string that contains error message
 */
void error(char* msg);

/**
 * @brief Print a colored error message as well as the ACK code
 *
 * @param msg c-string that contains error message
 * @param ack ACK to display after error message
 */
void error_ack(char* msg, uint8_t ack);

/**
 * @brief Waits a DELAY_MS amount of time
 */
void delay();

/**
 * @brief Blocks until button is pressed
 * 
 * Gets stuck in loop until high voltage is received in BUTTON_PIN
 */
void wait_for_button();

/**
 * @brief Get single keypress from uart
 *
 * @return Key that was pressed
 */
char get_keypress();

/**
 * @brief Get a line of text from user
 * 
 * Receive text from user until new line is received
 * Can also handle backspaces
 */
void get_line(char* buf, int8_t buf_size);

/**
 * @brief Calculate parity of data
 *
 * @param data Data you want to calculate parity for
 *
 * @return Even parity (0 if number of 1's even)
 */
uint8_t calc_parity(uint32_t data);

/**
 * @brief Compares just the first character of a string and if str is len of 1
 *
 * @param str c-string to compare
 * @param c char to compare against
 *
 * @return 0 if equal, 1 if not
 */
uint8_t single_cmp(char* a, char b);

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
uint8_t tokenize(char* buf, uint8_t buf_max_len, char** tokens, int8_t token_max_len, uint8_t* num_tokens);

int power(int base, int pow);
int8_t parse_char_to_hex(char c);
int parse_str_to_hex(char* str, uint32_t* hex);
#endif
