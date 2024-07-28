#pragma once

#include "headers.h"

#define NUM_OF_FONTS 8
#define MAX_CONSOLE_INPUT_LENGTH 1024
#define MAX_ENTITIES 1000000
#define TICKS_TO_NEXT_DECISION 5000
#define TICKS_TO_NEXT_ACTION 500
#define BASE_VELOCITY 20
#define DEAD_ENTITY_TEXTURE 38

#define array_count(static_array) (sizeof(static_array) / sizeof((static_array)[0]))
#define print(format, ...)            \
  printf(format "\n", ##__VA_ARGS__); \
  fflush(stdout)

#define loop(upper_bound, index_name) for (int index_name = 0; index_name < upper_bound; index_name++)
#define reverse_loop(lower_bound, index_name) for (int index_name = lower_bound - 1; index_name >= 0; index_name--)

typedef struct Vec2 {
  float x;
  float y;
} Vec2;

typedef struct {
  Vec2 position;
  Vec2 size;
} FRect;

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

bool floats_equal(float a, float b);

float frect_width(FRect *rect);

float frect_height(FRect *rect);
