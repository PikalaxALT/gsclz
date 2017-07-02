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

const int CmdSizes[] = {
    -1, // LZ_LITERAL = variable length of literal
     1, // LZ_ITERATE = value
     2, // LZ_ALTERNATE = value 1, value 2
     0, // LZ_ZERO = no parameter
    -2, // LZ_REPEAT = offset
    -2, // LZ_FLIP = offset
    -2  // LZ_REVERSE = offset
};

int checkOffsetWidth(const void *cursrc, const void *src, int offset) {
    if (cursrc - (src + offset) < 128) return 1;
    return 2;
}

u8 *writeLzCommand(u8 *dest, u8 cmd, size_t size, int offset, const u8 *src, const u8 *cursrc) {
    if (size <= LZ_MAX_SHORT)
        *dest++ = (cmd << 5) | (size - 1);
    else {
        *dest++ = (LZ_LONG << 5) | (cmd << 2) | (((size - 1) >> 8) & 0x03);
        *dest++ = size - 1;
    }
    printf("%s %zu", CommandStrings[cmd], size);
    if (cmd == LZ_LITERAL) {
        memcpy(dest, cursrc, size);
        dest += size;
    } else if (cmd >= LZ_RW) {
        if (checkOffsetWidth(cursrc, src, offset) == 1)
        {
            offset = cursrc - (src + offset) - 1;
            *dest++ = offset | 0x80;
            offset = ~offset;
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
    printf(" # %zu\n", cursrc - src + size);
    return dest;
}

struct LzCommand createLzCommand(u8 type, size_t size, int offset, const u8 *src, const u8 *cursrc) {
    struct LzCommand command;
    command.type = type;
    command.size = size;
    switch (type) {
        case LZ_LITERAL:
            command.data.literal = src;
            break;
        case LZ_ITERATE:
            command.data.iterate = cursrc[0];
            break;
        case LZ_ALTERNATE:
            command.data.alternate[0] = cursrc[0];
            command.data.alternate[1] = cursrc[1];
            break;
        case LZ_ZERO:
        case LZ_END:
            break;
        case LZ_REPEAT:
        case LZ_FLIP:
        case LZ_REVERSE:
            if (checkOffsetWidth(cursrc, src, offset) == 1)
                offset += src - cursrc;
            command.data.offset = offset; // The structs are all identical here.
            break;
    }
    return command;
}

size_t makeLzCommandsReverse(u8 *dest, const struct LzCommand *commands, size_t ncommands) {
    size_t outputsize = 0;
    size_t inputsize = 0;
    for (int i = ncommands - 1; i >= 0; i --)
    {
        outputsize++;
        if (commands[i].type == LZ_END) {
            printf("LZ_END\n");
            *dest++ = LZ_END;
            break;
        }
        if (commands[i].size > LZ_MAX_SHORT) {
            *dest++ = (LZ_LONG << 5) | (commands[i].type << 2) | ((commands[i].size >> 8) & 3);
            *dest++ = commands[i].size;
            outputsize++;
        } else {
            *dest++ = (commands[i].type << 5) | commands[i].size;
        }
        printf("%s %zu", CommandStrings[commands[i].type], commands[i].size);
        switch (commands[i].type) {
            case LZ_LITERAL:
                memcpy(dest, commands[i].data.literal, commands[i].size);
                dest += commands[i].size;
                outputsize += commands[i].size;
                break;
            case LZ_ITERATE:
                printf(", 0x%02X", commands[i].data.iterate);
                *dest++ = commands[i].data.iterate;
                outputsize ++;
                break;
            case LZ_ALTERNATE:
                printf(", 0x%02X, 0x%02X", commands[i].data.alternate[0], commands[i].data.alternate[1]);
                *dest++ = commands[i].data.alternate[0];
                *dest++ = commands[i].data.alternate[1];
                outputsize += 2;
                break;
            case LZ_ZERO:
                break;
            case LZ_REPEAT:
            case LZ_FLIP:
            case LZ_REVERSE:
                printf(", %d", commands[i].data.offset);
                if (commands[i].data.offset < 0)
                    *dest++ = (~commands[i].data.offset) | 0x80;
                else {
                    *dest++ = (commands[i].data.offset >> 8);
                    *dest++ = commands[i].data.offset;
                    outputsize++;
                }
                outputsize++;
                break;
        }
        inputsize += commands[i].size;
        printf(" # %zu\n", inputsize);
    }
    return outputsize;
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
    if (src[0] == 0)
        cnt = -1; // Penalize this to prefer ZERO
    else
        while (src + cnt < end && src[cnt] == src[0]) cnt++;
    return cnt;
}

int scoreLzAlternate(const u8 *src, const u8 *end) {
    int cnt = 2;
    if (src[0] == src[1])
        cnt = -2; // Penalize this to prefer ITERATE
    else
        while (src + cnt < end && src[cnt] == src[cnt % 2]) cnt++;
    return cnt;
}

int scoreLzZero(const u8 *src, const u8 *end) {
    int cnt = 0;
    while (src + cnt < end && src[cnt] == 0) cnt++;
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
        while (src + scores[i] < end && i - scores[i] >= 0 && src[scores[i]] == init[i - scores[i]]) scores[i]++;
    }
    return arrmax(scores, moff);
}

size_t lz(u8 *dest, const u8 *src, size_t insize) {
    const u8 *end = src + insize;
    const u8 *lastsrc = src;
    u8 *initdest = dest;
    int i;
    size_t size;
    int iterateScore = 0;
    int alternateScore = 0;
    int zeroScore = 0;
    int offsets[7] = {};
    struct LzRW repeatScore;
    struct LzRW flipScore;
    struct LzRW reverseScore;
    struct LzRW finalScore;
    int scores[7];
    int adjscores[7];
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

        for (i = 0; i < 7; i ++) {
            adjscores[i] = scores[i] - 1;
            if (scores[i] > 32) adjscores[i]--; // 10-bit sizes are LZ_LONG
            switch (CmdSizes[i])
            {
                case -1: // subtract off the score (LZ_LITERAL always gets a score of -1 or -2)
                    adjscores[i] -= scores[i];
                    break;
                case -2: // subtract off the offset
                    adjscores[i] -= checkOffsetWidth(cursrc, src, offsets[i]);
                    break;
                default: // subtract off 1 byte for iterate, 2 for alternate, and 0 for zero
                    adjscores[i] -= CmdSizes[i];
                    break;
            }
        }
        finalScore = arrmax(adjscores, 7);
        if (finalScore.score <= 0) continue;
        if (scores[finalScore.offset] > LZ_MAX_SIZE) scores[finalScore.offset] = LZ_MAX_SIZE;
        if (finalScore.offset != LZ_LITERAL) {
            if (cursrc > lastsrc) {
                size = cursrc - lastsrc;
                while (size > LZ_MAX_SIZE) {
                    dest = writeLzCommand(dest, LZ_LITERAL, LZ_MAX_SIZE, 0, src, lastsrc);
                    size -= LZ_MAX_SIZE;
                    lastsrc += LZ_MAX_SIZE;
                }
                dest = writeLzCommand(dest, LZ_LITERAL, size, 0, src, lastsrc);
            }
            dest = writeLzCommand(dest, finalScore.offset, scores[finalScore.offset], offsets[finalScore.offset], src, cursrc);
            cursrc += scores[finalScore.offset] - 1;
            lastsrc = cursrc + 1;
        }
    }
    if (lastsrc < end) {
        size = end - lastsrc;
        while (size > LZ_MAX_SIZE) {
            dest = writeLzCommand(dest, LZ_LITERAL, LZ_MAX_SIZE, 0, src, lastsrc);
            size -= LZ_MAX_SIZE;
            lastsrc += LZ_MAX_SIZE;
        }
        dest = writeLzCommand(dest, LZ_LITERAL, size, 0, src, lastsrc);
    }
    *dest++ = LZ_END;
    printf("LZ_END\n");
    return dest - initdest;
}

