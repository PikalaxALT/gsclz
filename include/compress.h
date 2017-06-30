//
// Created by Scott Norton on 6/30/17.
//

#ifndef GSCLZ_COMPRESS_H
#define GSCLZ_COMPRESS_H

struct LzRW {
    int score;
    int offset;
};

size_t lz(u8 *dest, const u8 *src, size_t insize);

#endif //GSCLZ_COMPRESS_H
