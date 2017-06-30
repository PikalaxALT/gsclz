//
// Created by Scott Norton on 6/30/17.
//

#include "utils.h"

unsigned char bflp(unsigned char inbits) {
    unsigned char outbits = 0;
    for (int j = 0; j < 8; j ++) outbits |= ((inbits & (1 << j)) >> j) << (7 - j);
    return outbits;
}
