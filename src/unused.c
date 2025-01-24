/*
void dump_flash() {
LOOP:
    // ------ SELECT Write ----- //
    // Set APSEL to 0x00
    // Set APBANKSEL to 0b00
    // To access Memory AP
    uint8_t ack;
    uint32_t data;
    if (ack = SWD_DP_write(0b01, 0x00000000)) {
        printf("Error in SELECT write\n");
        goto LOOP;
    }
    printf("SELECT Write: 0x0000.0000 ACK: %d\n", ack);
    delay();

    // ------ Write to CSW of MEM-AP ----- //
    // To enable privledged master access over AHB-AP
    // Enable auto increment for TAR
    // 32-bit IO over DRW
    if (ack = SWD_AP_write(0b00, 0x22000012)) {
        printf("Error in CSW write\n");
        goto LOOP;
    }
    printf("CSW Write: 0x22000012 ACK: %d\n", ack);

    delay();

    // Write to TAR to set address to read from
    if (ack = SWD_AP_write(0b10, 0x00000000)) {
        printf("Error in TAR write\n");
        goto LOOP;
    }
    printf("TAR Write: 0x00000000 ACK: %d\n", ack);
    delay();

    // Attempt to read flash
    if (ack = SWD_AP_read(0b11, &data)) {
        // If WAIT received
        if (ack == 0b010) {
            single_pulse();
            sleep_ms(5);
            // Read from RDBUFF from DP (Since we are getting WAIT ACK)
            if (SWD_DP_read(0b11, &data)) {
                printf("Error in RDBUFF read\n");
            } else {
                printf("RDBUFF Read: 0x%.8x ACKed\n", data);
            }
        }
    } else {
        printf("%x\n", data);
    }
    delay();


    printf("\nFLASH:\n");
    int i;
    for (i = 0; i < 20; ++i) {
        // Read flash
        if (SWD_AP_read(0b11, &data)) {
            printf("Error in flash read\n");
        } else {
            printf("%.8x\n", data);
        }
        delay();
    }
}
*/
