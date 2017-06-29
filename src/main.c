#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "macros.h"

size_t unlz(u8 *dest, const u8 *src, size_t insize)
{
    u8 rval;
    u8 cmd;
    s8 altset;
    int i;
    size_t size;
    const u8 *initsrc;
    u8 *initdst;
    u8 *rwsrc;
    s8 offset;
    u8 flipout;
    u8 j;
    size_t outsize;

    initsrc = src;
    initdst = dest;
    outsize = 0;
    while ((rval = *src++) != 0xff && (outsize < 0x2000) && (src - initsrc < insize)) {
        cmd = rval >> 5;
        if (cmd == LZ_LONG) {
            cmd = (rval >> 2) & 0x07;
            size = (((rval & 0x03) << 8) | *src++) + 1;
        } else {
            size = (rval & 0x1f) + 1;
        }
        if (cmd & LZ_RW) {
            offset = *src++;
            rwsrc = offset >= 0 ? initdst + offset : dest - (offset & 0x7f);
        } else {
            rwsrc = NULL;
        }
        switch (cmd) {
            case LZ_LITERAL:
                memcpy(dest, src, size);
                dest += size;
                src += size;
                break;
            case LZ_ITERATE:
                memset(dest, *src++, size);
                dest += size;
                break;
            case LZ_ALTERNATE:
                altset = 1;
                for (i=0; i<size; i++)
                {
                    *dest++ = *(src += altset);
                    altset = -altset;
                }
                if (altset == 1) src++;
                src++;
                break;
            case LZ_ZERO:
                memset(dest, 0, size);
                dest += size;
                break;
            default:
            case LZ_REPEAT:
                memcpy(dest, rwsrc, size);
                dest += size;
                break;
            case LZ_FLIP:
                for (i=0; i<size; i++) {
                    flipout = 0;
                    for (j=0; j<8; j++) {
                        flipout |= (((*rwsrc) & (1 << j)) >> j) << (7 - j);
                    }
                    rwsrc++;
                    *dest++ = flipout;
                }
                break;
            case LZ_REVERSE:
                for (i=0; i<size; i++)
                    *dest++ = *rwsrc--;
                break;
        }
        outsize += size;
    }
    return outsize;
}

size_t lz(u8 *dest, const u8 *src, size_t insize) {
    return 0;
}

int main(int argc, char *argv[])
{
    u8 *src;
    u8 *dest;
    u8 *decompend;
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
    dest = malloc(0x2000);
    src = malloc(fsize);
    fread(src, 1, fsize, infile);
    fclose(infile);
    if ((osize = method(dest, src, fsize)) <= fsize) {
        printf("Packed size exceeds unpacked size!\n");
        exit(1);
    }
    fwrite(dest, 1, osize, outfile);
    fclose(outfile);
    free(src);
    free(dest);
    return 0;
}
