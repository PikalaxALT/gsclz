//
// Created by Scott Norton on 6/30/17.
//

#include "utils.h"

const char *const CommandStrings[] = {
    "LZ_LITERAL",
    "LZ_ITERATE",
    "LZ_ALTERNATE",
    "LZ_ZERO",
    "LZ_REPEAT",
    "LZ_FLIP",
    "LZ_REVERSE"
};

unsigned char bflp(unsigned char inbits) {
    unsigned char outbits = 0;
    for (int j = 0; j < 8; j ++) outbits |= ((inbits & (1 << j)) >> j) << (7 - j);
    return outbits;
}
