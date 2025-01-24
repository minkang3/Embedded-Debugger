#ifndef SWD_REQUEST_H
#define SWD_REQUEST_H

#include <stdint.h>

typedef struct _swd_req
{
    uint8_t start : 1;  // Always 1
    uint8_t APnDP : 1;  // Selects DPACC or APACC, 0 for DPACC, 1 for APACC
    uint8_t RnW : 1;    // Selects read or write, 0 for write, 1 for read
    uint8_t A : 2;      // Different meaning based on APnDP
    uint8_t parity : 1; // Parity check on APnDP,RnW,A bits, if num of 1's are even, parity is 0
    uint8_t stop : 1;   // Must always be 0 unless SWD is async which is never
    uint8_t park : 1;   // Always 1
} swd_req_t;

union Packet {
    swd_req_t req;
    uint16_t asInt;
};

#endif
