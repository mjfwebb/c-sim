#pragma once

#include "headers.h"

#define NUM_OF_FONTS 8
#define MAX_CONSOLE_INPUT_LENGTH 1024
#define MAX_ENTITIES 1000000

#define array_count(static_array) (sizeof(static_array) / sizeof((static_array)[0]))
#define print(format, ...)            \
  printf(format "\n", ##__VA_ARGS__); \
  fflush(stdout)

#define entity_loop(index_name) for (int index_name = 0; index_name < game_context.entity_count; index_name++)
#define reverse_entity_loop(index_name) for (int index_name = game_context.entity_count - 1; index_name >= 0; index_name--)

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
#define abs(x) (((x) >= 0) ? (x) : -(x))
#define fabs(x) (((x) >= 0.0) ? (x) : -(x))
#define fabsf(x) (((x) >= 0.0f) ? (x) : -(x))
#define ath_epsilon 1E-6
