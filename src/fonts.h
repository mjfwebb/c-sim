#pragma once

#include "type_defs.h"
#include "math_types.h"

#define BASIC_LATIN_SET_COUNT          (126 - 32)
#define LATIN_ONE_SUPPLEMENT_SET_COUNT (255 - 128)
#define LATIN_EXTENDED_A_SET_COUNT     (383 - 256)
#define LATIN_EXTENDED_B_SET_COUNT     (563 - 384)
#define HIRAGANA_SET_COUNT             (12446 - 12353)
#define KATAKANA_SET_COUNT             (12542 - 12449)
#define EMOJI_SET_COUNT                (128519 - 128512)

enum CHARACTER_SETS
{
    BASIC_LATIN_BIT          = 1,
    LATIN_ONE_SUPPLEMENT_BIT = 1 << 1,
    LATIN_EXTENDED_A_BIT     = 1 << 2,
    LATIN_EXTENDED_B_BIT     = 1 << 3,
    HIRAGANA_BIT             = 1 << 4,
    KATAKANA_BIT             = 1 << 5,
    EMOJI_BIT                = 1 << 6
};

typedef struct
{
    float r;
    float g;
    float b;
    float a;
}Color;

typedef struct
{
    FRect source;
    int min_x;
    int max_x;
    int advance;
    int min_y;
    int max_y; 
    u8 valid;
}GlyphMetrics;

typedef struct
{
    struct SDL_Texture *atlas;
    struct SDL_Texture *outline_atlas;
    GlyphMetrics *glyph_metrics;
    GlyphMetrics *outline_glyph_metrics;
    struct SDL_Renderer* renderer;
    void* font_handle;
    int size;
    int line_skip;
    int height;
    int ascent;
    int descent;
    int outline_size;
    int glyph_count;
    u32 character_sets;
}Font;

typedef struct
{
    struct SDL_Renderer* renderer;
    int size;
    int outline_size;
    u32 character_sets;
}FontLoadParams;

void init_emoji_set();

void init_japanese_character_sets(const u32 bits);

void init_latin_character_sets(const u32 bits);

void draw_text_utf8(const char* text, FPoint position, const Color color, const Font* font);

void draw_text_outlined_utf8(const char* text, FPoint position, const Color color, const Color outline_color, const Font* font);

void draw_text(const char* text, FPoint position, const Color color, const Font* font);

void draw_text_outlined(const char* text, FPoint position, const Color color, const Color outline_color, const Font* font);

FPoint get_text_size(const char* text, const Font* font, const u8 do_outline, const u8 is_utf8);

Font load_font(const char* path, const FontLoadParams loader);
int free_font(Font *atlas);
