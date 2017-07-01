//
// Created by Scott Norton on 6/30/17.
//

#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <stdio.h>
#include "macros.h"
#include "compress.h"
#include "utils.h"

u8 *writeLzCommand(u8 *dest, u8 cmd, size_t size, int offset, const u8 *src, const u8 *cursrc) {
    if (size > LZ_MAX_SIZE) size = LZ_MAX_SIZE;
    if (size <= LZ_MAX_SHORT)
        *dest++ = (cmd << 5) | (size - 1);
    else {
        *dest++ = (LZ_LONG << 5) | (cmd << 2) | (((size - 1) >> 8) & 0x03);
        *dest++ = size - 1;
    }
    printf("%s %zu", CommandStrings[cmd], size);
    if (cmd == LZ_LITERAL) {
        memcpy(dest, cursrc, size);
    } else if (cmd >= LZ_RW) {
        if (cursrc - (src + offset) < 128)
        {
            *dest++ = (cursrc - (src + offset)) | 0x80;
        }
        else {
            *dest++ = offset >> 8;
            *dest++ = offset;
        }
        printf(", %d", offset);
    } else if (cmd == LZ_ITERATE)
    {
        *dest++ = *cursrc;
        printf(", 0x%02X", *cursrc);
    }
    else if (cmd == LZ_ALTERNATE) {
        *dest++ = cursrc[0];
        *dest++ = cursrc[1];
        printf(", 0x%02X, 0x%02X", cursrc[0], cursrc[1]);
    }
    dest += size;
    printf("\n");
    return dest;
}

struct LzRW arrmax(int *array, size_t size) {
    struct LzRW output = {.score = -1};
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
    if (iterval != 0)
        while (src < end && *src++ == iterval) cnt++;
    if (cnt < 3) return -1;
    return cnt;
}

int scoreLzAlternate(const u8 *src, const u8 *end) {
    int cnt = 2;
    char altval[2];
    altval[0] = *src++;
    altval[1] = *src++;
    for (int i = 0; src < end && *src++ != altval[i % 2]; i++) cnt++;
    if (cnt < 4) return -1;
    return cnt;
}

int scoreLzZero(const u8 *src, const u8 *end) {
    int cnt = 0;
    while (src < end && *src++ == 0) cnt++;
    if (cnt < 2) return -1;
    return cnt;
}

struct LzRW scoreLzRepeat(const u8 *src, const u8 *init, const u8 *end) {
    size_t moff = src - init;
    int scores[moff];
    for (int i = 0; i < moff; i ++) {
        scores[i] = 0;
        while (src + scores[i] < end && src[scores[i]] == init[i + scores[i]]) scores[i]++;
    }
    struct LzRW retval = arrmax(scores, moff);
    if (retval.score < 3) retval.score = -1;
    return retval;
}

struct LzRW scoreLzFlip(const u8 *src, const u8 *init, const u8 *end) {
    size_t moff = src - init;
    int scores[moff];
    for (int i = 0; i < moff; i ++) {
        scores[i] = 0;
        while (src + scores[i] < end && src[scores[i]] == bflp(init[i + scores[i]])) scores[i]++;
    }
    struct LzRW retval = arrmax(scores, moff);
    if (retval.score < 3) retval.score = -1;
    return retval;
}

struct LzRW scoreLzReverse(const u8 *src, const u8 *init, const u8 *end) {
    size_t moff = src - init;
    int scores[moff];
    for (int i = 0; i < moff; i ++) {
        scores[i] = 0;
        while (src + scores[i] < end && src[scores[i]] == init[moff - i - scores[i] - 1]) scores[i]++;
    }
    struct LzRW retval = arrmax(scores, moff);
    if (retval.score < 3) retval.score = -1;
    return retval;
}

size_t lz(u8 *dest, const u8 *src, size_t insize) {
    const u8 *end = src + insize;
    const u8 *lastsrc = src;
    u8 *initdest = dest;
    int iterateScore = 0;
    int alternateScore = 0;
    int zeroScore = 0;
    int offsets[7] = {};
    int prevScores[7] = {};
    int prevOffsets[7] = {};
    struct LzRW repeatScore;
    struct LzRW flipScore;
    struct LzRW reverseScore;
    struct LzRW finalScore;
    int scores[7] = {};
    int last_winner = LZ_LITERAL;
    for (const u8 *cursrc = src; cursrc < end; cursrc++) {
        iterateScore = scoreLzIterate(cursrc, end);
        alternateScore = scoreLzAlternate(cursrc, end);
        zeroScore = scoreLzZero(cursrc, end);
        repeatScore = scoreLzRepeat(cursrc, src, end);
        flipScore = scoreLzFlip(cursrc, src, end);
        reverseScore = scoreLzReverse(cursrc, src, end);
        scores[LZ_LITERAL] = cursrc - lastsrc + 1;
        scores[LZ_ITERATE] = iterateScore;
        scores[LZ_ALTERNATE] = alternateScore;
        scores[LZ_ZERO] = zeroScore;
        scores[LZ_REPEAT] = repeatScore.score;
        scores[LZ_FLIP] = flipScore.score;
        scores[LZ_REVERSE] = reverseScore.score;
        offsets[LZ_REPEAT] = repeatScore.offset;
        offsets[LZ_FLIP] = flipScore.offset;
        offsets[LZ_REVERSE] = reverseScore.offset;
        finalScore = arrmax(scores, 7);
        if (finalScore.offset != last_winner) {
            if (cursrc > src) {
                dest = writeLzCommand(dest, last_winner, prevScores[last_winner], prevOffsets[last_winner], src, lastsrc);
                cursrc += prevScores[last_winner] - 1;
                lastsrc = cursrc + 1;
            }
            last_winner = finalScore.offset;
        }
        memcpy(prevScores, scores, sizeof scores);
        memcpy(prevOffsets, offsets, sizeof offsets);
    }
    if (lastsrc < end) {
        dest = writeLzCommand(dest, finalScore.offset, scores[finalScore.offset], offsets[finalScore.offset], src, lastsrc);
    }
    *dest++ = LZ_END;
    printf("LZ_END\n");
    return dest - initdest;
}

