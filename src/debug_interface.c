/**
 * @file debug_interface.c
 * @author Min Kang
 * @brief Debug interface
 */
#include "debug_interface.h"
#include "swd_init.h"
#include "mem.h"
#include "macros.h"
#include "utils.h"
#include "data_transfer.h"
#include <stdio.h>
#include <string.h>

#include "blink_bin.h"
#include "simple_bin.h"

/**
 * @brief Print help menu
 */
uint8_t show_help() {
    printf("DEBUG HELP:\n\n");
    printf("    init - Initialize SWD Debug (Must be run first)\n");
    printf("    status - Show debug status\n");
    printf("    halt - Halt core\n");
    printf("    reset - Reset core\n");
    printf("    step - Single step\n");
    printf("    load - Load file and initialize execution\n");
    printf("    pc - read current pc\n");
    printf("    load [program] - load precompiled program\n");
    printf("    set <address> <value> - set a memory address\n");
    printf("    read <address> - set a value from memory address\n");
}

uint8_t debug_initialize_swd() {
    uint8_t ack;
    initialize_swd();
    ack = setup_dp_and_mem_ap();
    if (ack == 0b001) {
        printf("Debug initialized\n");
    } else {
        error("Debug unable to initialize");
    }
}

/**
 * @brief Show debug status
 *
 * @return ACK from SWD request
 */
uint8_t show_debug_status() {
    uint32_t data;
    uint8_t ack;
    ack = mem_read(CORE_DHCSR, &data);
    CHECK_ACK_RT("Failed getting debug status");

    if (data & (1 << 19)) {
        printf("Processor locked up because of an unrecoverable exception\n");
        return ack;
    } if (data & (1 << 18)) {
        printf("Processor sleeping.\n");
    } if (data & (1 << 17)) {
        printf("In Debug Mode\n");
    } else {
        printf("Core currently running\n");
    }
    delay();
}

/**
 * @brief Halt the core
 *
 * @return ACK from SWD request
 */
uint8_t halt_core() {
    uint8_t ack;
    uint32_t data;
    ack = mem_write(CORE_DHCSR, 0xa05f0003);
    CHECK_ACK_RT("Failed halting core");
    ack = mem_read(CORE_DHCSR, &data);
    if (data & (1 << 17)) {
        printf("Core successfully halted\n");
    } else {
        error("Halt failed");
    }
}

/**
 * @brief Continues core, if halted
 *
 * @return ACK from SWD request
 */
uint8_t continue_core() {
    uint8_t ack;
    uint32_t data;
    ack = mem_write(CORE_DHCSR, 0xa05f0000);
    CHECK_ACK_RT("Failed continuing core");
    ack = mem_read(CORE_DHCSR, &data);
    if (data & (1 << 17)) {
        error("Continue failed");
    }
}

/**
 * @brief Reset core and halts
 *
 * @return ACK from SWD request
 */
uint8_t reset_core() {
    uint8_t ack;
    uint32_t data;

    // Enable halt on reset
    ack = mem_write(CORE_DEMCR, 0x00000001);
    CHECK_ACK_RT("Failed enabling halt on reset");

    // Reset core by writing to NVIC AIRCR
    ack = mem_write(NVIC_AIRCR, 0x0afa0004);
    CHECK_ACK_RT("Failed writing to NVIC.AIRCR");

    // Check status
    ack = mem_read(CORE_DHCSR, &data);
    CHECK_ACK_RT("Failed reading status after reset");

    printf("Successfully reset core\n");
}

/**
 * @brief Read current PC
 *
 * @param data Pointer to store PC
 *
 * @return ACK from SWD request
 */
uint8_t read_pc() {
    uint8_t ack;
    uint32_t data;
    ack = mem_write(0xe000edf4, ((0 << 16) | 0x0F));
    CHECK_ACK_RT("DCRSR write");
    ack = mem_read(0xe000edf8, &data);
    CHECK_ACK_RT("DCRDR read");
    printf("PC: 0x%.8x\n", data);
    return ack;
}

/**
 * @brief Performs a single step of instruction
 *
 * @return ACK from SWD request
 */
uint8_t single_step() {
    uint32_t data; uint8_t ack;
    ack = mem_write(CORE_DHCSR, 0xa05f0005);
    CHECK_ACK_RT("Failed single stepping");
    delay();
    read_pc();
    delay();
    return ack;
}

/**
 * @brief Verify written file integrity
 *
 * @param bin_arr Hex dump array of program
 * @param bin_len Length of array
 *
 * @return ACK from SWD request
 */
uint8_t verify_file(unsigned char* bin_arr, unsigned int bin_len) {
    uint32_t data; uint8_t ack;

    ack = SWD_AP_write(0b10, 0x20000000);
    CHECK_ACK_RT("Failed writing target address");
    delay();

    uint32_t word, index = 0;
    ack = SWD_AP_read(0b11, &word);
    ack = SWD_AP_read(0b11, &word); // HACK: Why does reading twice make it work?
    for (index = 0; index < bin_len; index += 4) {
        ack = SWD_AP_read(0b11, &word);
        CHECK_ACK_RT("Failed reading SRAM");
        if (word != *(uint32_t *)&bin_arr[index]) {
            error("Verification failed");
            printf("At index: %d\n", index);
            printf("0x%.8x != 0x%.8x\n", word, *(uint32_t *)&bin_arr[index]);
            return 0b100;
        }
    }
    printf("Verification success\n");
}

/**
 * @brief Write file to RAM, set PC, and reset TARGET
 *
 * @param bin_arr Hex dump array of program
 * @param bin_len Length of array
 *
 * @return ACK from SWD request
 */
uint8_t load_file(unsigned char* bin_arr, unsigned int bin_len) {
    uint32_t data; uint8_t ack;
    
    // Enable auto increment
    ack = SWD_AP_write(0b00, 0x22000012);
    CHECK_ACK_RT("Error setting auto increment");
    delay();

    // Write Target Address
    ack = SWD_AP_write(0b10, 0x20000000);
    CHECK_ACK_RT("Error writing target address");
    delay();

    uint32_t word, index = 0;
    for (; index < bin_len; index += 4) {
        word = *(uint32_t *)&bin_arr[index];
        ack = SWD_AP_write(0b11, word);
        printf("CODE : %x 0x%.8x\n", index, word);
        CHECK_ACK_RT("Error in writing code");
        delay();
    }
}

/**
 * @brief Set PC, stack pointer, and VTable
 *
 * @param pc PC to write
 * @param msp Stack pointer
 *
 * @return ACK of SWD request
 */
uint8_t init_file_execution(uint32_t pc, uint32_t msp) {
    uint32_t data; uint8_t ack;
    // Write PC
    ack = mem_write(CORE_DCRDR, pc);
    CHECK_ACK_RT("Failed writing PC (DCRDR)");
    delay();
    ack = mem_write(CORE_DCRSR, 0x0001000f);
    CHECK_ACK_RT("Failed writing PC (DCRSR)");
    delay();

    // Set MSP
    ack = mem_write(CORE_DCRDR, msp);
    CHECK_ACK_RT("Failed writing MSP");
    delay();
    ack = mem_write(CORE_DCRSR, 0x0001000d);
    CHECK_ACK_RT("Failed writing MSP");
    delay();

    // Relocate VTOR to SRAM
    ack = mem_write(0xe000ed08, 0x20000000);
    CHECK_ACK_RT("Failed relocating VTOR");
    delay();

    return 1;
}

uint8_t load_file_and_run(char** args, uint8_t num_args) {
    unsigned char* bin_arr;
    unsigned int bin_len;
    if (!strncmp(args[1], "blink", 5)) {
        bin_arr = blink_bin;
        bin_len = blink_bin_len;
    } else if (!strncmp(args[1], "simple", 5)) {
        bin_arr = simple_bin;
        bin_len = simple_bin_len;
    } else {
        bin_arr = simple_bin;
        bin_len = simple_bin_len;
    }
    halt_core();
    load_file(bin_arr, bin_len);
    printf("\n");
    verify_file(bin_arr, bin_len);
    reset_core();
    init_file_execution(0x20000041, 0x20004000);
}

uint8_t set_mem(uint32_t address, uint32_t value) {
    uint8_t ack;
    ack = SWD_AP_write(0b10, address);
    CHECK_ACK_RT("Failed writing target address");

    ack = SWD_AP_write(0b11, value);
    CHECK_ACK_RT("Failed writing value");

    return 1;
}

uint8_t interface_set_mem(char** args, uint8_t num_args) {
    uint32_t addr, val;
    int err = 0;
    if (num_args != 3) {
            printf("Incorrect number of arguments. Format should be:\n");
            printf("set <address> <value>\n");
            return 1;
    }
    err += parse_str_to_hex(args[1], &addr);
    err += parse_str_to_hex(args[2], &val);
    if (err) {
            printf("Incorrect format. Address and val should be in hex format like 0x12341234\n");
            return 1;
    }
    set_mem(addr, val);
    printf("Wrote 0x%.8x to address 0x%.8x\n", val, addr);
}

uint8_t read_mem(uint32_t address, uint32_t* data) {
    uint8_t ack;
    ack = SWD_AP_write(0b10, address);
    CHECK_ACK_RT("Failed writing target address");
    delay();

    ack = SWD_AP_read(0b11, data);
    delay();
    if (ack == 0b010) {
        ack = SWD_AP_read(0b11, data);
        CHECK_ACK_RT("Failed reading memory");
    }
    ack - SWD_DP_read(0b11, data);
    CHECK_ACK_RT("Failed reading memory");
    delay();
}

void to_uppercase(char* str) {
    while (*str != '\0') {
        if ('a' <= *str && *str <= 'z') {
            *str -= 'a' - 'A';
        }
        str++;
    }
}

int32_t str_to_int(char* str) {
    int ret = 0;
    while (*str != '\0') {
        if (!('0' <= *str && *str <= '9'))
            return -1;
        ret *= 10;
        ret += *str - '0';
        ++str;
    }
    return ret;
}

/*
 * @brief Parse a string like "$r5" into the REGSEL val for DCRSR
 * 
 * @param reg_str c-string in the format of "$r0" or "$pc"
 *
 * @return the REGSEL val or negative value for fail
 */
int8_t parse_reg_str_to_DCRSR_REGSEL(char* reg_str) {
    int8_t ret;
    uint8_t len = strlen(reg_str);

    if (len < 3) {
        printf("Unknown register\n");
        return -1;
    }

    reg_str++; // Get rid of first character '$'

    to_uppercase(reg_str);
    if (reg_str[0] == 'R' && '0' <= reg_str[1] && reg_str[1] <= '9') {
        int32_t i = str_to_int(reg_str + 1);
        if (i < 0 || i > 15) {
            printf("Register not valid or not within bounds, must be within 0 to 15\n");
            return -1;
        }
        ret = i;
    } else if (reg_str[0] == 'S' && '0' <= reg_str[1] && reg_str[1] <= '9') {
        int32_t i = str_to_int(reg_str + 1);
        if (i < 0 || i > 31) {
            printf("Register not valid or not within bounds, must be within 0 to 31\n");
            return -1;
        }
        ret = i;
    } else {
        struct name_to_val {
            char* name;
            uint8_t val;
        };
        struct name_to_val ntv_arr[] = {
            { "SP",        0x0D },
            { "LR",        0x0E },
            { "PC",        0x0F },
            { "XPSR",      0x10 },
            { "MSP",       0x11 },
            { "PSP",       0x12 },
            { "CONTROL",   0x14 },
            { "FAULTMASK", 0x14 },
            { "BASEPRI",   0x14 },
            { "PRIMASK",   0x14 },
            { "FPCSR",     0x21 },
        };

        ret = 0;
        for (int i = 0; i < sizeof(ntv_arr) / sizeof(struct name_to_val); ++i) {
            if (strcmp(reg_str, ntv_arr[i].name) == 0) {
                ret = ntv_arr[i].val;
                break;
            }
        }
        if (ret == 0) {
            printf("Unknown register\n");
            return -1;
        }
    }
    return ret;
}

/*
 * @brief Receive a string like "$r5" and return the value of the specified 
 * register
 *
 * @param reg_str c-string in the format of "$r0" or "$pc"
 *
 * @return 0 for success, 1 for unknown register
 */
uint8_t read_register(char* reg_str, uint32_t* data) {
    int8_t ack = 0;
    int8_t regsel = parse_reg_str_to_DCRSR_REGSEL(reg_str);
    if (regsel < 0) {
        return 1;
    }

    ack = set_mem(CORE_DCRSR, regsel);
    CHECK_ACK_RT("Failed to write regsel");
    ack = read_mem(CORE_DCRDR, data);
    CHECK_ACK_RT("Failed to read register");

    return 0;
}

uint8_t interface_read_mem(char** args, uint8_t num_args) {
    uint32_t addr, val;
    int err = 0;
    if (num_args != 2) {
        printf("Incorrect number of arguments. Format should be:\n");
        printf("read <address>\n");
        return 1;
    }
    if (args[1][0] == '$') {
        err += read_register(args[1], &val);
        if (err) {
            printf("Error in read_register\n");
            return 1;
        }
        printf("%s: 0x%.8x\n", args[1] + 1, val);
    } else {
        err += parse_str_to_hex(args[1], &addr);
        if (err) {
            printf("Incorrect format. Address should be in hex format like 0x12341234\n");
            return 1;
        }
        read_mem(addr, &val);
        printf("0x%.8x: 0x%.8x\n", addr, val);
    }
}

// TODO: Refactor all above helping functions into different files
