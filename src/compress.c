//
// Created by Scott Norton on 6/30/17.
//

#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include "macros.h"
#include "compress.h"
#include "utils.h"

u8 *writeLzCommand(u8 *dest, u8 cmd, size_t size) {
    if (size > LZ_MAX_SIZE) size = LZ_MAX_SIZE;
    if (size <= LZ_MAX_SHORT)
        *dest++ = (cmd << 5) | (size - 1);
    else {
        *dest++ = (LZ_LONG << 5) | (cmd << 2) | (((size - 1) >> 8) & 0x03);
        *dest++ = size - 1;
    }
    return dest;
}

u8 *checkLiteral(u8 *dest, const u8 *src, const u8 *lastsrc) {
    if (src > lastsrc) {
        size_t diff = src - lastsrc;
        dest = writeLzCommand(dest, LZ_LITERAL, diff);
        memcpy(dest, src, diff);
        dest += diff;
    }
    return dest;
}

struct LzRW arrmax(int *array, size_t size) {
    struct LzRW output = {};
    for (int i = 0; i < size; i ++) {
        if (array[i] > output.score) {
            output.score = array[i];
            output.offset = i;
        }
    }
    return output;
}

int scoreLzIterate(const u8 *src, const u8 *end) {
    int cnt = 1;
    char iterval = *src++;
    while (src < end && *src++ == iterval) cnt++;
    return cnt;
}

int scoreLzAlternate(const u8 *src, const u8 *end) {
    int cnt = 2;
    char altval[2];
    altval[0] = *src++;
    altval[1] = *src++;
    for (int i = 0; src < end && *src++ != altval[i % 2]; i++) cnt++;
    return cnt;
}

int scoreLzZero(const u8 *src, const u8 *end) {
    int cnt = 0;
    while (src < end && *src++ == 0) cnt++;
    return cnt;
}

struct LzRW scoreLzRepeat(const u8 *src, const u8 *init, const u8 *end) {
    size_t moff = src - init;
    int scores[moff];
    for (int i = 0; i < moff; i ++) {
        scores[i] = 0;
        while (src + scores[i] < end && src[scores[i]] == init[i + scores[i]]) scores[i]++;
    }
    return arrmax(scores, moff);
}

struct LzRW scoreLzFlip(const u8 *src, const u8 *init, const u8 *end) {
    size_t moff = src - init;
    int scores[moff];
    for (int i = 0; i < moff; i ++) {
        scores[i] = 0;
        while (src + scores[i] < end && src[scores[i]] == bflp(init[i + scores[i]])) scores[i]++;
    }
    return arrmax(scores, moff);
}

struct LzRW scoreLzReverse(const u8 *src, const u8 *init, const u8 *end) {
    size_t moff = src - init;
    int scores[moff];
    for (int i = 0; i < moff; i ++) {
        scores[i] = 0;
        while (src + scores[i] < end && src[scores[i]] == init[moff - i - scores[i] - 1]) scores[i]++;
    }
    return arrmax(scores, moff);
}

size_t lz(u8 *dest, const u8 *src, size_t insize) {
    const u8 *cursrc = src;
    const u8 *end = src + insize;
    const u8 *lastsrc = src;
    u8 *initdest = dest;
    int literalScore = 0;
    int iterateScore = 0;
    int alternateScore = 0;
    int zeroScore = 0;
    int whichCommand;
    int offset;
    int offsets[3] = {};
    struct LzRW repeatScore = {};
    struct LzRW flipScore = {};
    struct LzRW reverseScore = {};
    struct LzRW finalScore = {};
    int scores[7] = {};
    while (cursrc < end) {
        literalScore ++;
        iterateScore = scoreLzIterate(cursrc, end);
        alternateScore = scoreLzAlternate(cursrc, end);
        zeroScore = scoreLzZero(cursrc, end);
        repeatScore = scoreLzRepeat(cursrc, src, end);
        flipScore = scoreLzFlip(cursrc, src, end);
        reverseScore = scoreLzReverse(cursrc, src, end);
        scores[LZ_LITERAL] = literalScore;
        scores[LZ_ITERATE] = iterateScore;
        scores[LZ_ALTERNATE] = alternateScore;
        scores[LZ_ZERO] = zeroScore;
        scores[LZ_REPEAT] = repeatScore.score;
        scores[LZ_FLIP] = flipScore.score;
        scores[LZ_REVERSE] = reverseScore.score;
        offsets[LZ_REPEAT - LZ_RW] = repeatScore.offset;
        offsets[LZ_FLIP - LZ_RW] = flipScore.offset;
        offsets[LZ_REVERSE - LZ_RW] = reverseScore.offset;
        finalScore = arrmax(scores, sizeof scores);
        if (finalScore.offset == LZ_LITERAL)
            cursrc++;
        else {
            dest = checkLiteral(dest, cursrc, lastsrc);
            lastsrc = cursrc;
            literalScore = 0;
            dest = writeLzCommand(dest, finalScore.offset, finalScore.score);
            if (finalScore.offset >= LZ_RW) {
                offset = offsets[finalScore.offset - LZ_RW];
                if (cursrc - (src + offset) < 128)
                    *dest++ = (cursrc - (src + offset)) | 0x80;
                else {
                    *dest++ = offset >> 8;
                    *dest++ = offset;
                }
            } else if (finalScore.offset == LZ_ITERATE)
                *dest++ = *cursrc;
            else if (finalScore.offset == LZ_ALTERNATE) {
                *dest++ = *cursrc++;
                *dest++ = *cursrc--;
            }
            cursrc += finalScore.score;
        }
    }
    *dest++ = LZ_END;
    return dest - initdest;
}

