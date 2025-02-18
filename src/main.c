/**
 * @file main.c
 * @author Min Kang
 * @brief Main file for embedded debugger
 */
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <boards/pico2.h>
#include <pico/time.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "data_transfer.h"
#include "macros.h"
#include "setup.h"
#include "utils.h"
#include "swd_init.h"
#include "mem.h"
#include "debug_interface.h"

typedef struct {
    char* cmd;
    uint8_t has_args;
    uint8_t single_char;
    union {
        uint8_t (*no_arg_func)();
        uint8_t (*arg_func)(char** args, uint8_t num_args);
    } func_ptr;
} Command;

Command commands[] = {
    { "help",     .has_args = 0, .single_char = 1, .func_ptr.no_arg_func = show_help },
    { "init",     .has_args = 0, .single_char = 1, .func_ptr.no_arg_func = debug_initialize_swd },
    { "status",   .has_args = 0, .single_char = 0, .func_ptr.no_arg_func = show_debug_status },
    { "halt",     .has_args = 0, .single_char = 0, .func_ptr.no_arg_func = halt_core },
    { "continue", .has_args = 0, .single_char = 1, .func_ptr.no_arg_func = continue_core },
    { "reset",    .has_args = 0, .single_char = 0, .func_ptr.no_arg_func = reset_core },
    { "step",     .has_args = 0, .single_char = 1, .func_ptr.no_arg_func = single_step },
    { "pc",       .has_args = 0, .single_char = 0, .func_ptr.no_arg_func = read_pc },

    { "load",     .has_args = 1, .single_char = 0, .func_ptr.arg_func = load_file_and_run },
    { "set",      .has_args = 1, .single_char = 0, .func_ptr.arg_func = interface_set_mem },
    { "read",     .has_args = 1, .single_char = 0, .func_ptr.arg_func = interface_read_mem },
};

void parse_cmd(char** args, uint8_t num_args) {
    if (num_args == 0) {
        return;
    }

    int cmd_size = sizeof(commands) / sizeof(Command);
    for (int i = 0; i < cmd_size; ++i) {
        if (strcmp(commands[i].cmd, args[0])) {
            if (!commands[i].single_char)
                continue;
            if (commands[i].single_char && single_cmp(args[0], commands[i].cmd[0]))
                continue;
        }
        if (!commands[i].has_args)
            commands[i].func_ptr.no_arg_func();
        else
            commands[i].func_ptr.arg_func(args, num_args);
    }
}

void init(void) {
    stdio_init_all();
    setup();
}

// *********************************** //
// ********** MAIN FUNCTION ********** //
// *********************************** //
int main(void) {
    char c;
    //uint32_t data;
    //uint8_t ack;

    char buf[BUF_LEN];
    char* tokens[TOKENS_LEN];
    uint8_t num_tokens;

    init();
    sleep_ms(1500);

    printf("Enter any key to start entering commands...\n");
    get_keypress();

    printf("Enter help or h to get available commands\n");


    while (1) {
        get_line(buf, BUF_LEN);
        tokenize(buf, BUF_LEN, tokens, TOKENS_LEN, &num_tokens);
        parse_cmd(tokens, num_tokens);
    }
}
