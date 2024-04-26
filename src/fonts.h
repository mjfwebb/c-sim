#pragma once

#include "headers.h"
#include "render_batcher.h"

#define BASIC_LATIN_SET_COUNT (126 - 32)
#define LATIN_ONE_SUPPLEMENT_SET_COUNT (255 - 128)
#define LATIN_EXTENDED_A_SET_COUNT (383 - 256)
#define LATIN_EXTENDED_B_SET_COUNT (563 - 384)
#define HIRAGANA_SET_COUNT (12446 - 12353)
#define KATAKANA_SET_COUNT (12542 - 12449)
#define EMOJI_SET_COUNT (128519 - 128512)

enum CHARACTER_SETS {
  BASIC_LATIN_BIT = 1,
  LATIN_ONE_SUPPLEMENT_BIT = 1 << 1,
  LATIN_EXTENDED_A_BIT = 1 << 2,
  LATIN_EXTENDED_B_BIT = 1 << 3,
  HIRAGANA_BIT = 1 << 4,
  KATAKANA_BIT = 1 << 5,
  EMOJI_BIT = 1 << 6
};

typedef struct {
  FRect source;
  int min_x;
  int max_x;
  int advance;
  int min_y;
  int max_y;
  u32 atlas_index;
  u8 valid;
} GlyphMetrics;

typedef struct {
  struct SDL_Texture** atlas;
  GlyphMetrics* glyph_metrics;
  FRect* outline_sources;
  struct SDL_Renderer* renderer;
  struct _TTF_Font* font_handle;
  int size;
  int line_skip;
  int height;
  int ascent;
  int descent;
  int outline_size;
  int glyph_count;
  int atlas_count;
  u32 character_sets;
  u32 character_set_count;
  u32 character_set_starts[10];  // hardcoded to 10 for now
  u32 character_set_array_count[10];  // hardcoded to 10 for now
  u8 (*character_set_checker[10])(const u32);  // hardcoded to 10 for now
} Font;

typedef struct {
  struct SDL_Renderer* renderer;
  int size;
  int outline_size;
  u32 character_sets;
} FontLoadParams;
