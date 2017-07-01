//
// Created by Scott Norton on 6/29/17.
//

#ifndef GSCLZ_MACROS_H
#define GSCLZ_MACROS_H

typedef __uint8_t u8;
typedef __uint16_t u16;
typedef __uint32_t u32;
typedef __uint64_t u64;

typedef __int8_t s8;
typedef __int16_t s16;
typedef __int32_t s32;
typedef __int64_t s64;

typedef u8 bool8;
typedef u16 bool16;
typedef u32 bool32;
typedef u64 bool64;

enum LzCommands {
    LZ_LITERAL,
    LZ_ITERATE,
    LZ_ALTERNATE,
    LZ_ZERO,
    LZ_REPEAT,
    LZ_FLIP,
    LZ_REVERSE,
    LZ_LONG
};

#define LZ_RW 0x04
#define LZ_MAX_SIZE (1 << 10)
#define LZ_MAX_SHORT (1 << 5)
#define LZ_END 0xff

#endif //GSCLZ_MACROS_H
