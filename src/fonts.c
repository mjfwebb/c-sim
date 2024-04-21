#include "fonts.h"
#include <stddef.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static u32 BASIC_LATIN_SET[BASIC_LATIN_SET_COUNT] = {0};
static u32 LATIN_ONE_SUPPLEMENT_SET[LATIN_ONE_SUPPLEMENT_SET_COUNT] = {0};
static u32 LATIN_EXTENDED_A_SET[LATIN_EXTENDED_A_SET_COUNT] = {0};
static u32 LATIN_EXTENDED_B_SET[LATIN_EXTENDED_B_SET_COUNT] = {0};
static u32 HIRAGANA_SET[HIRAGANA_SET_COUNT] = {0};
static u32 KATAKANA_SET[KATAKANA_SET_COUNT] = {0};
static u32 EMOJI_SET[EMOJI_SET_COUNT] = {0};

static const u32 INVALID_CHARACTER_INDEX = (u32)-1;

static u8 in_basic_latin(const u32 character)
{
    return character >= 32 && character <= 126;
}

static u8 in_latin_one_supplement(const u32 character)
{
    return character >= 128 && character <= 255;
}

static u8 in_latin_extended_a(const u32 character)
{
    return character >= 256 && character <= 383;
}

static u8 in_latin_extended_b(const u32 character)
{
    return character >= 384 && character <= 563;
}

static u8 in_hiragana(const u32 character)
{
    return character >= 12353 && character <= 12446;
}

static u8 in_katakana(const u32 character)
{
    return character >= 12449 && character <= 12542;
}

static u8 in_emoji(const u32 character)
{
    return character >= 128512 && character <= 128519;
}

static const u32 CHARACTER_SET_BITS[] = {BASIC_LATIN_BIT, 
                                         LATIN_ONE_SUPPLEMENT_BIT,
                                         LATIN_EXTENDED_A_BIT,
                                         LATIN_EXTENDED_B_BIT,
                                         HIRAGANA_BIT,
                                         KATAKANA_BIT,
                                         EMOJI_BIT};

static const u32 CHARACTER_SET_STARTS[] = {32, 128, 256, 384, 12353, 12449, 128512};

static const u32 CHARACTER_SET_ARRAY_COUNT[] = {BASIC_LATIN_SET_COUNT,
                                                LATIN_ONE_SUPPLEMENT_SET_COUNT,
                                                LATIN_EXTENDED_A_SET_COUNT,
                                                LATIN_EXTENDED_B_SET_COUNT,
                                                HIRAGANA_SET_COUNT,
                                                KATAKANA_SET_COUNT,
                                                EMOJI_SET_COUNT};

static const u32 CHARACTER_SETS_COUNT = sizeof(CHARACTER_SET_BITS) / sizeof(u32);

static u32 get_index_in_font(const u32 character, const Font* font)
{
    static u8(*set_checker[])(const u32) = {in_basic_latin, in_latin_one_supplement, in_latin_extended_a, in_latin_extended_b, in_hiragana, in_katakana, in_emoji};

    u32 offset_accumulation = 0;

    for(u32 i = 0; i < CHARACTER_SETS_COUNT; i++){
        if(set_checker[i](character)){
            if(font->character_sets & CHARACTER_SET_BITS[i]){
                return (character - CHARACTER_SET_STARTS[i]) + offset_accumulation;
            }
        }
        if(font->character_sets & CHARACTER_SET_BITS[i]){
            offset_accumulation += CHARACTER_SET_ARRAY_COUNT[i];
        }
    }
    return INVALID_CHARACTER_INDEX;
}

void init_emoji_set()
{
    for(int i = 0; i <= EMOJI_SET_COUNT; i++){
        EMOJI_SET[i] = i + 128512;
    }
}

void init_japanese_character_sets(const u32 bits)
{
    if(bits & HIRAGANA_BIT){
        for(int i = 0; i <= HIRAGANA_SET_COUNT; i++){
            HIRAGANA_SET[i] = i + 12353;
        }
    }
    if(bits & KATAKANA_BIT){
        for(int i = 0; i <= KATAKANA_SET_COUNT; i++){
            KATAKANA_SET[i] = i + 12449;
        }
    }
}

void init_latin_character_sets(const u32 bits)
{
    if(bits & BASIC_LATIN_BIT){
        for(int i = 0; i <= BASIC_LATIN_SET_COUNT; i++){
            BASIC_LATIN_SET[i] = i + 32;
        }
    }
    if(bits & LATIN_ONE_SUPPLEMENT_BIT){
        for(int i = 0; i <= LATIN_ONE_SUPPLEMENT_SET_COUNT; i++){
            LATIN_ONE_SUPPLEMENT_SET[i] = i + 128;
        }
    }
    if(bits & LATIN_EXTENDED_A_BIT){
        for(int i = 0; i <= LATIN_EXTENDED_A_SET_COUNT; i++){
            LATIN_EXTENDED_A_SET[i] = i + 256;
        }
    }
    if(bits & LATIN_EXTENDED_B_BIT){
        for(int i = 0; i <= LATIN_EXTENDED_B_SET_COUNT; i++){
            LATIN_EXTENDED_B_SET[i] = i + 384;
        }
    }
}

typedef struct
{
    SDL_Surface **glyphs;
    SDL_Surface **outline_glyphs;
    GlyphMetrics *metrics;
    GlyphMetrics *outline_metrics;
    int atlas_width;
    int outline_atlas_width;
    int glyph_max_height;
    int outline_glyph_max_height;

}LoadGlyphSetData;

static void fill_glyph_set_data(LoadGlyphSetData *data, const u32 character, Font* font, const u8 do_outline)
{
    SDL_Surface **glyphs;
    GlyphMetrics *metrics;
    int *atlas_width;
    int *glyph_max_height;

    if(do_outline){
        glyphs = data->outline_glyphs;
        metrics = data->outline_metrics;
        atlas_width = &data->outline_atlas_width;
        glyph_max_height = &data->outline_glyph_max_height;
    }
    else{
        glyphs = data->glyphs;
        metrics = data->metrics;
        atlas_width = &data->atlas_width;
        glyph_max_height = &data->glyph_max_height;
    }

    const SDL_Color white = {255, 255, 255, 255};
    const u32 index = get_index_in_font(character, font);
    GlyphMetrics *m = &metrics[index];
    m->valid = 0;
    if(TTF_GlyphMetrics32(font->font_handle, character, &m->min_x, &m->max_x, &m->min_y, &m->max_y, &m->advance) == -1){
        return;
    }
    SDL_Surface *g = TTF_RenderGlyph32_Blended(font->font_handle, character, white);
    if(!g){
        SDL_Log("%s\n", SDL_GetError());
        return;
    }
    m->valid = 1;
    m->source.x = (float)(*atlas_width);
    m->source.y = 0;
    m->source.w = (float)g->w;
    m->source.h = (float)g->h;
    if(g->h > *glyph_max_height){
        *glyph_max_height = g->h;
    }
    *atlas_width += g->w + 2;
    glyphs[index] = g;
    SDL_SetSurfaceBlendMode(g, SDL_BLENDMODE_NONE);
}

static void process_glyph_set(LoadGlyphSetData *data, const u32* set, const u32 set_count, Font *font)
{
    for(u32 i = 0; i < set_count; i++){
        TTF_SetFontOutline(font->font_handle, 0);
        fill_glyph_set_data(data, set[i], font, 0);
        if(font->outline_size > 0){
            TTF_SetFontOutline(font->font_handle, font->outline_size);
            fill_glyph_set_data(data, set[i], font, 1);
        }
    }
}

static void build_texture_atlas(LoadGlyphSetData *data, Font* font, const u8 do_outline)
{
    const u32 format = SDL_PIXELFORMAT_BGRA32;

    SDL_Surface *glyph_cache;
    GlyphMetrics *metrics;
    SDL_Surface **glyphs;
    SDL_Texture **texture;
    if(do_outline){
        metrics = font->outline_glyph_metrics;
        glyphs = data->outline_glyphs;
        glyph_cache = SDL_CreateRGBSurfaceWithFormat(0, data->outline_atlas_width, data->outline_glyph_max_height, 32, format);
        texture = &font->outline_atlas;
    }
    else{
        metrics = font->glyph_metrics;
        glyphs = data->glyphs;
        glyph_cache = SDL_CreateRGBSurfaceWithFormat(0, data->atlas_width, data->glyph_max_height, 32, format);
        texture = &font->atlas;
    }
    assert(glyph_cache);
    SDL_SetSurfaceBlendMode(glyph_cache, SDL_BLENDMODE_NONE);
    for(int i = 0; i < font->glyph_count; i++){

        if(metrics[i].valid){

            SDL_Rect dst = {(int)metrics[i].source.x,
                            (int)metrics[i].source.y,
                            (int)metrics[i].source.w,
                            (int)metrics[i].source.h};

            SDL_BlitSurface(glyphs[i], NULL, glyph_cache, &dst);
        }
    }
    *texture = SDL_CreateTextureFromSurface(font->renderer, glyph_cache);
    assert(*texture);
    SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(glyph_cache);
    for(int i = 0; i < font->glyph_count; i++){
        SDL_FreeSurface(glyphs[i]);
    }
}

Font load_font(const char* path, const FontLoadParams loader)
{
    assert(loader.size > 0);
    Font result = {0};

    TTF_Font *main_font = TTF_OpenFont(path, loader.size);

    if(!main_font){
        return result;
    }

    result.renderer = loader.renderer;
    result.font_handle = main_font;
    result.character_sets = loader.character_sets;
    result.size = loader.size;
    result.outline_size = loader.outline_size;

    for(u32 i = 0; i < CHARACTER_SETS_COUNT; i++){
        if(result.character_sets & CHARACTER_SET_BITS[i]){
            result.glyph_count += CHARACTER_SET_ARRAY_COUNT[i];
        }
    }

    result.glyph_metrics = (GlyphMetrics*)malloc(sizeof(GlyphMetrics) * result.glyph_count);
    LoadGlyphSetData glyph_set_data = {0};
    glyph_set_data.glyphs = (SDL_Surface**)malloc(sizeof(SDL_Surface*) * result.glyph_count);

    if(result.outline_size > 0){
        result.outline_glyph_metrics = (GlyphMetrics*)malloc(sizeof(GlyphMetrics) * result.glyph_count);
        glyph_set_data.outline_glyphs = (SDL_Surface**)malloc(sizeof(SDL_Surface*) * result.glyph_count);
    } 
    glyph_set_data.metrics = result.glyph_metrics;
    glyph_set_data.outline_metrics = result.outline_glyph_metrics;

    u32 *character_set_arrays[] = {BASIC_LATIN_SET,
                                   LATIN_ONE_SUPPLEMENT_SET,
                                   LATIN_EXTENDED_A_SET,
                                   LATIN_EXTENDED_B_SET,
                                   HIRAGANA_SET,
                                   KATAKANA_SET,
                                   EMOJI_SET};


    for(u32 i = 0; i < CHARACTER_SETS_COUNT; i++)
    {
        if(result.character_sets & CHARACTER_SET_BITS[i]){
            process_glyph_set(&glyph_set_data, character_set_arrays[i], CHARACTER_SET_ARRAY_COUNT[i], &result);
        }
    } 

	result.line_skip = TTF_FontLineSkip(main_font);
    result.height = TTF_FontHeight(main_font);
    result.ascent = TTF_FontAscent(main_font);
    result.descent = TTF_FontDescent(main_font);

    build_texture_atlas(&glyph_set_data, &result, 0);
    if(result.outline_size > 0){
        build_texture_atlas(&glyph_set_data, &result, 1);
    }

    return result;
}

static void draw_text_internal(const char* text, const FPoint position, const Color *color, const Color *outline_color, const Font* font, const u8 is_utf8)
{
    const float height = (float)font->height;
    const float outline = outline_color->a > 0 ? ((float)font->outline_size) : 0;
	const float line_increment = (height + (outline * 2));

    float xc = position.x;
    float yc = position.y;
    u32 prev_char = 0;
    u32 current_char = 0;

    u32 char_index;

    SDL_FRect dst;
    SDL_FRect temp_dst;

    const int trailing = 0x80;
    const int mask = 0xC0;
    const int mask2 = 0xE0;
    const int mask3 = 0xF0;

    for(const u8* c = (u8*)text; *c; c++)
    {
        if(is_utf8){
            // there might be an optimal way to translate utf8 to unicode decimal value
            u8 copy = *c;
            if((copy & mask) != trailing)
            {
                if(copy & trailing)
                {
                    if(copy & mask3){
                        copy &= ~mask3;
                    }
                    else if(copy & mask2){
                        copy &= ~mask2;
                    }
                    else if(copy & mask){
                        copy &= ~mask;
                    }
                }
                current_char = copy;
                if(*c > 127){
                    continue;
                }
            }
            else{
                copy &= ~trailing;
                current_char <<= 6;
                current_char |= copy;
                const u8* p = c;
                p++;
                if((*p & mask) == trailing){
                    continue;
                }
            }
        }
        else{
            current_char = *c;
        }

        if(current_char == '\n')
        {
			yc += line_increment;
			xc = position.x;
			current_char = 0; 
			prev_char = 0;
            continue;
        }
		
		char_index = get_index_in_font(current_char, font);
        const GlyphMetrics* metrics = &font->glyph_metrics[char_index];
		
        if(metrics->valid && metrics->source.w > 0)
        {
            if(prev_char)
            {
                const int kern = TTF_GetFontKerningSizeGlyphs32(font->font_handle, prev_char, current_char);
		        xc += kern;
            }
        
		    dst = (SDL_FRect){xc, yc, metrics->source.w, metrics->source.h};
            dst.x = min(dst.x, dst.x + metrics->min_x);
		    
            SDL_Rect source;
		    if(outline > 0) 
            {
                const GlyphMetrics* outline_metrics = &font->outline_glyph_metrics[char_index];
		    	
		    	temp_dst = (SDL_FRect){dst.x, dst.y, outline_metrics->source.w, outline_metrics->source.h};
		    	
		    	SDL_SetTextureColorMod(font->outline_atlas, (u8)(255 * outline_color->r),
                                                            (u8)(255 * outline_color->g),
                                                            (u8)(255 * outline_color->b));
		    	SDL_SetTextureAlphaMod(font->outline_atlas, (u8)(255 * outline_color->a));
                source = (SDL_Rect){(int)outline_metrics->source.x, (int)outline_metrics->source.y, (int)outline_metrics->source.w, (int)outline_metrics->source.h};
                SDL_RenderCopyF(font->renderer, font->outline_atlas, &source, &temp_dst);
		    }

	        dst.x += outline;
            dst.y += outline;

            source = (SDL_Rect){(int)metrics->source.x, (int)metrics->source.y, (int)metrics->source.w, (int)metrics->source.h};
            SDL_SetTextureColorMod(font->atlas, (u8)(255 * color->r),
                                                (u8)(255 * color->g),
                                                (u8)(255 * color->b));
            SDL_SetTextureAlphaMod(font->atlas, (u8)(255 * color->a));
            SDL_RenderCopyF(font->renderer, font->atlas, &source, &dst);

	    }	
		prev_char = current_char;
        current_char = 0;
        xc += metrics->advance + outline;
    }
}

void draw_text_utf8(const char* text, FPoint position, const Color color, const Font* font)
{
    draw_text_internal(text, position, &color, &(Color){0, 0, 0, 0}, font, 1);
}

void draw_text_outlined_utf8(const char* text, FPoint position, const Color color, const Color outline_color, const Font* font)
{
    draw_text_internal(text, position, &color, &outline_color, font, 1);
}

void draw_text(const char* text, FPoint position, const Color color, const Font* font)
{
    draw_text_internal(text, position, &color, &(Color){0, 0, 0, 0}, font, 0);
}

void draw_text_outlined(const char* text, FPoint position, const Color color, const Color outline_color, const Font* font)
{
    draw_text_internal(text, position, &color, &outline_color, font, 0);
}

FPoint get_text_size(const char* text, const Font* font, const u8 do_outline, const u8 is_utf8)
{
    const int outline = do_outline ? font->outline_size : 0;
    const int height = font->height;
 
    int x = 0;
    int minx = 0;
    int maxx = 0;
 
    u32 prev_char = 0;
    u32 current_char = 0;
    
    const int trailing = 0x80;
    const int mask = 0xC0;
    const int mask2 = 0xE0;
    const int mask3 = 0xF0;

    FPoint result = {-1, (float)(height * outline * 2)};

    for(const u8* c = (u8*)text; *c; c++)
    {
        if(is_utf8){
            // there might be an optimal way to translate utf8 to unicode decimal value
            u8 copy = *c;
            if((copy & mask) != trailing)
            {
                if(copy & trailing)
                {
                    if(copy & mask3){
                        copy &= ~mask3;
                    }
                    else if(copy & mask2){
                        copy &= ~mask2;
                    }
                    else if(copy & mask){
                        copy &= ~mask;
                    }
                }
                current_char = copy;
                if(*c > 127){
                    continue;
                }
            }
            else{
                copy &= ~trailing;
                current_char <<= 6;
                current_char |= copy;
                const u8* p = c;
                p++;
                if((*p & mask) == trailing){
                    continue;
                }
            }
        }
        else{
            current_char = *c;
        }

        if(current_char == '\n') 
        {
            result.y += height + outline;
            result.x = (maxx - minx) > result.x ? (maxx - minx) : result.x;
            maxx = 0;
            minx = 0;
            x = 0;
            prev_char = 0;
            continue;
        }
 
        const GlyphMetrics *metric = &font->glyph_metrics[get_index_in_font(current_char, font)];
 
        if(prev_char && metric->valid){
            x += TTF_GetFontKerningSizeGlyphs32(font->font_handle, prev_char, current_char);
        }
 
        minx = min(minx, x + metric->min_x);
        maxx = max(maxx, x + metric->max_x);
        maxx = max(maxx, x + metric->advance);
        maxx = max(maxx, x + metric->advance + outline);
        x += metric->advance + outline;
        prev_char = current_char;
    }
    result.x = result.x < (maxx - minx) ? (maxx - minx) : result.x;
    result.x += outline;
    return result;
}

int free_font(Font *atlas)
{
    assert(atlas);
    free(atlas->glyph_metrics);
    free(atlas->outline_glyph_metrics);
    if(atlas->atlas){
        SDL_DestroyTexture(atlas->atlas);
    }
    if(atlas->outline_atlas){
        SDL_DestroyTexture(atlas->outline_atlas);
    }
    if(atlas->font_handle){
        TTF_CloseFont(atlas->font_handle);
    }
    return 0;
}
