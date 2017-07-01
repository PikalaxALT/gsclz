//
// Created by scott on 6/30/2017.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include "macros.h"
#include "utils.h"
#include "decompress.h"

size_t unlz(u8 *dest, const u8 *src, size_t insize)
{
    u8 rval;
    u8 cmd;
    int i;
    size_t size;
    const u8 *initsrc;
    u8 *initdst;
    u8 *rwsrc;
    s32 offset;
    size_t outsize;
    bool is_rw;
    bool is_long;
    bool is_fwd;
    u8 iterval;
    u8 altervals[2];

    initsrc = src;
    initdst = dest;
    outsize = 0;
    while ((rval = *src++) != LZ_END && (outsize < 0x2000) && (src - initsrc < insize)) {
        offset = -1;
        cmd = rval >> 5;
        is_long = cmd == LZ_LONG;
        if (is_long) {
            cmd = (rval >> 2) & 0x07;
            size = (((rval & 0x03) << 8) | *src++) + 1;
        } else {
            size = (rval & 0x1f) + 1;
        }
        is_rw = ((cmd & LZ_RW) != 0);
        if (is_rw) {
            offset = *src++;
            if (offset & 0x80)
            {
                is_fwd = false;
                offset = ~(offset & 0x7f);
                rwsrc = dest + offset;
            }
            else {
                is_fwd = true;
                offset <<= 8;
                offset |= (*src++);
                rwsrc = initdst + offset;
            }
            if (rwsrc >= dest) {
                printf("Error in calculating offset!\n"
                           "dest = %p\n"
                           "rwsrc = %p\n"
                           "offset = 0x%04x\n"
                           "direction = %s\n", &dest, &rwsrc, offset, is_fwd ? "forward" : "reverse");
                exit(1);
            }
        } else {
            rwsrc = NULL;
        }
        switch (cmd) {
            case LZ_LITERAL:
                printf("LZ_LITERAL %zu\n", size);
                memcpy(dest, src, size);
                dest += size;
                src += size;
                break;
            case LZ_ITERATE:
                iterval = *src++;
                printf("LZ_ITERATE %zu, 0x%02X\n", size, iterval);
                memset(dest, iterval, size);
                dest += size;
                break;
            case LZ_ALTERNATE:
                altervals[0] = *src++;
                altervals[1] = *src++;
                printf("LZ_ALTERNATE, %zu 0x%02X, 0x%02X\n", size, altervals[0], altervals[1]);
                for (i=0; i<size; i++)
                {
                    *dest++ = altervals[i % 2];
                }
                break;
            case LZ_ZERO:
                printf("LZ_ZERO %zu\n", size);
                memset(dest, 0, size);
                dest += size;
                break;
            default:
            case LZ_REPEAT:
                printf("LZ_REPEAT %zu, %d\n", size, offset);
                for (i=0; i<size; i++)
                    *dest++ = *rwsrc++;
                break;
            case LZ_FLIP:
                printf("LZ_FLIP %zu, %d\n", size, offset);
                for (i=0; i<size; i++)
                    *dest++ = bflp(*rwsrc++);
                break;
            case LZ_REVERSE:
                printf("LZ_REVERSE %zu, %d\n", size, offset);
                for (i=0; i<size; i++)
                    *dest++ = *rwsrc--;
                break;
        }
        outsize += size;
    }
    printf("LZ_END\n");
    return outsize;
}

