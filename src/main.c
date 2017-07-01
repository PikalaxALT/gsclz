#include <stdio.h>
#ifdef __APPLE__
#include <ntsid.h>
#endif
#include <memory.h>
#include <stdlib.h>
#include <stdbool.h>
#include "macros.h"
#include "utils.h"
#include "decompress.h"
#include "compress.h"

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
