//
// Created by Scott Norton on 6/30/17.
//

#ifndef GSCLZ_COMPRESS_H
#define GSCLZ_COMPRESS_H

struct LzRW {
    int score;
    int offset;
};

struct LzLiteralCmd {
    const u8 *data;
};

struct LzIterateCmd {
    u8 value;
};

struct LzAlternateCmd {
    u8 val1;
    u8 val2;
};

struct LzZeroCmd {};

struct LzRepeatCmd {
    s16 offset;
};

struct LzFlipCmd {
    s16 offset;
};

struct LzReverseCmd {
    s16 offset;
};

struct LzEndCmd {};

union LzCommandData {
    const u8 *literal;
    u8 iterate;
    u8 alternate[2];
    s16 offset;
};

struct LzCommand {
    u8 type;
    size_t size;
    union LzCommandData data;
};

size_t lz(u8 *dest, const u8 *src, size_t insize);

#endif //GSCLZ_COMPRESS_H
