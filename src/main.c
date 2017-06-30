#include <stdio.h>
#include <ntsid.h>
#include <memory.h>
#include <stdlib.h>
#include <stdbool.h>
#include "macros.h"
#include "compress.h"
#include "utils.h"

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
                printf("LZ_ITERATE 0x%02X, %zu\n", iterval, size);
                memset(dest, iterval, size);
                dest += size;
                break;
            case LZ_ALTERNATE:
                altervals[0] = *src++;
                altervals[1] = *src++;
                printf("LZ_ALTERNATE 0x%02X, 0x%02X, %zu\n", altervals[0], altervals[1], size);
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
                printf("LZ_REPEAT %d, %zu\n", offset, size);
                for (i=0; i<size; i++)
                    *dest++ = *rwsrc++;
                break;
            case LZ_FLIP:
                printf("LZ_FLIP %d, %zu\n", offset, size);
                for (i=0; i<size; i++)
                    *dest++ = bflp(*rwsrc++);
                break;
            case LZ_REVERSE:
                printf("LZ_REVERSE %d, %zu\n", offset, size);
                for (i=0; i<size; i++)
                    *dest++ = *rwsrc--;
                break;
        }
        outsize += size;
    }
    printf("LZ_END\n");
    return outsize;
}

int main(int argc, char *argv[])
{
    FILE *infile;
    FILE *outfile;
    size_t fsize;
    size_t osize;
    size_t (*method)(u8 *, const u8 *, size_t);
    if (argc != 4) {
        printf("Incorrect number of arguments, abort!\n");
        exit(1);
    }
    if (strcmp(argv[1], "unlz") == 0)
        method = unlz;
    else if (strcmp(argv[1], "lz") == 0)
        method = lz;
    else {
        printf("Unrecognized method, abort!\n");
        exit(1);
    }
    if ((infile = fopen(argv[2], "rb")) == NULL) {
        printf("IO error: input file not found!\n"
               "    Unable to find \"%s\"\n", argv[2]);
        exit(1);
    }
    if ((outfile = fopen(argv[3], "w+b")) == NULL) {
        printf("IO error: output file not found!\n"
               "    Unable to find \"%s\"\n", argv[3]);
        exit(1);
    }
    fseek(infile, 0, SEEK_END);
    fsize = ftell(infile);
    fseek(infile, 0, SEEK_SET);
    u8 src[fsize];
    u8 dest[0x2000];
    fread(src, 1, fsize, infile);
    fclose(infile);
    osize = method(dest, src, fsize);
    fwrite(dest, 1, osize, outfile);
    fclose(outfile);
    return 0;
}
