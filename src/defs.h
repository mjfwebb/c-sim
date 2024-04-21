#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct FRect {
  float x;
  float y;
  float w;
  float h;
} FRect;

typedef struct FPoint {
  float x;
  float y;
} FPoint;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/***
 * Taken from SDL.
 * NOTE: these double-evaluate their arguments, so you should never have side effects in the parameters
 *      (e.g. SDL_min(x++, 10) is bad).
 */
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define clamp(x, a, b) (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))
#define ath_epsilon 1E-6

bool floats_equal(float a, float b);