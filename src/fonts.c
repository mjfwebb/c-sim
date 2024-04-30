#include "headers.h"

static u32 BASIC_LATIN_SET[BASIC_LATIN_SET_COUNT] = {0};
static u32 LATIN_ONE_SUPPLEMENT_SET[LATIN_ONE_SUPPLEMENT_SET_COUNT] = {0};
static u32 LATIN_EXTENDED_A_SET[LATIN_EXTENDED_A_SET_COUNT] = {0};
static u32 LATIN_EXTENDED_B_SET[LATIN_EXTENDED_B_SET_COUNT] = {0};
static u32 HIRAGANA_SET[HIRAGANA_SET_COUNT] = {0};
static u32 KATAKANA_SET[KATAKANA_SET_COUNT] = {0};
static u32 EMOJI_SET[EMOJI_SET_COUNT] = {0};

static const u32 INVALID_CHARACTER_INDEX = (u32)-1;

static u8 in_basic_latin(const u32 character) {
  return character >= 32 && character <= 127;
}

static u8 in_latin_one_supplement(const u32 character) {
  return character >= 128 && character <= 255;
}

static u8 in_latin_extended_a(const u32 character) {
  return character >= 256 && character <= 383;
}

static u8 in_latin_extended_b(const u32 character) {
  return character >= 384 && character <= 563;
}

static u8 in_hiragana(const u32 character) {
  return character >= 12353 && character <= 12446;
}

static u8 in_katakana(const u32 character) {
  return character >= 12449 && character <= 12542;
}

static u8 in_emoji(const u32 character) {
  return character >= 128512 && character <= 128519;
}

static const u32 CHARACTER_SET_BITS[] = {
    BASIC_LATIN_BIT, LATIN_ONE_SUPPLEMENT_BIT, LATIN_EXTENDED_A_BIT, LATIN_EXTENDED_B_BIT, HIRAGANA_BIT, KATAKANA_BIT, EMOJI_BIT
};

static const u32 CHARACTER_SET_STARTS[] = {32, 128, 256, 384, 12353, 12449, 128512};

static const u32 CHARACTER_SET_ARRAY_COUNT[] = {BASIC_LATIN_SET_COUNT,
                                                LATIN_ONE_SUPPLEMENT_SET_COUNT,
                                                LATIN_EXTENDED_A_SET_COUNT,
                                                LATIN_EXTENDED_B_SET_COUNT,
                                                HIRAGANA_SET_COUNT,
                                                KATAKANA_SET_COUNT,
                                                EMOJI_SET_COUNT};

static const int UTF8_TRAILING = 0x80;
static const int UTF8_MASK_ONE = 0xC0;
static const int UTF8_MASK_TWO = 0xE0;
static const int UTF8_MASK_THREE = 0xF0;

static u8 (*CHARACTER_SET_CHECKER[])(const u32
) = {in_basic_latin, in_latin_one_supplement, in_latin_extended_a, in_latin_extended_b, in_hiragana, in_katakana, in_emoji};

static const u32 CHARACTER_SETS_COUNT = sizeof(CHARACTER_SET_BITS) / sizeof(u32);

static u32 get_index_in_font(const u32 character, const Font *font) {
  u32 offset_accumulation = 0;

  for (u32 i = 0; i < font->character_set_count; i++) {
    if (font->character_set_checker[i](character)) {
      return (character - font->character_set_starts[i]) + offset_accumulation;
    }
    offset_accumulation += font->character_set_array_count[i];
  }
  return INVALID_CHARACTER_INDEX;
}

void init_emoji_set() {
  for (int i = 0; i < EMOJI_SET_COUNT; i++) {
    EMOJI_SET[i] = i + 128512;
  }
}

void init_japanese_character_sets(const u32 bits) {
  if (bits & HIRAGANA_BIT) {
    for (int i = 0; i < HIRAGANA_SET_COUNT; i++) {
      HIRAGANA_SET[i] = i + 12353;
    }
  }
  if (bits & KATAKANA_BIT) {
    for (int i = 0; i < KATAKANA_SET_COUNT; i++) {
      KATAKANA_SET[i] = i + 12449;
    }
  }
}

void init_latin_character_sets(const u32 bits) {
  if (bits & BASIC_LATIN_BIT) {
    for (int i = 0; i < BASIC_LATIN_SET_COUNT; i++) {
      BASIC_LATIN_SET[i] = i + 32;
    }
  }
  if (bits & LATIN_ONE_SUPPLEMENT_BIT) {
    for (int i = 0; i < LATIN_ONE_SUPPLEMENT_SET_COUNT; i++) {
      LATIN_ONE_SUPPLEMENT_SET[i] = i + 128;
    }
  }
  if (bits & LATIN_EXTENDED_A_BIT) {
    for (int i = 0; i < LATIN_EXTENDED_A_SET_COUNT; i++) {
      LATIN_EXTENDED_A_SET[i] = i + 256;
    }
  }
  if (bits & LATIN_EXTENDED_B_BIT) {
    for (int i = 0; i < LATIN_EXTENDED_B_SET_COUNT; i++) {
      LATIN_EXTENDED_B_SET[i] = i + 384;
    }
  }
}

typedef struct {
  SDL_Surface **glyphs;
  SDL_Surface **outline_glyphs;
  GlyphMetrics *metrics;
  int atlas_width;
  int glyph_max_height;

} LoadGlyphSetData;

static void fill_glyph_set_data(LoadGlyphSetData *data, const u32 character, Font *font) {
  TTF_SetFontOutline(font->font_handle, 0);

  const SDL_Color white = {255, 255, 255, 255};
  const u32 index = get_index_in_font(character, font);
  GlyphMetrics *m = &data->metrics[index];
  m->valid = 0;
  if (TTF_GlyphMetrics32(font->font_handle, character, &m->min_x, &m->max_x, &m->min_y, &m->max_y, &m->advance) == -1) {
    return;
  }
  data->glyphs[index] = TTF_RenderGlyph32_Blended(font->font_handle, character, white);
  if (!data->glyphs[index]) {
    SDL_Log("%s\n", SDL_GetError());
    return;
  }
  m->valid = 1;
  m->source.size.x = (float)data->glyphs[index]->w;
  m->source.size.y = (float)data->glyphs[index]->h;
  if (data->glyphs[index]->h > data->glyph_max_height) {
    data->glyph_max_height = data->glyphs[index]->h;
  }
  if (font->outline_size > 0) {
    TTF_SetFontOutline(font->font_handle, font->outline_size);
    data->outline_glyphs[index] = TTF_RenderGlyph32_Blended(font->font_handle, character, white);
    data->atlas_width += data->outline_glyphs[index]->w + 2;
    SDL_SetSurfaceBlendMode(data->outline_glyphs[index], SDL_BLENDMODE_NONE);
  } else {
    data->atlas_width += data->glyphs[index]->w + 2;
  }
  SDL_SetSurfaceBlendMode(data->glyphs[index], SDL_BLENDMODE_NONE);
}

static void process_glyph_set(LoadGlyphSetData *data, const u32 *set, const u32 set_count, Font *font) {
  for (u32 i = 0; i < set_count; i++) {
    fill_glyph_set_data(data, set[i], font);
  }
}

static void build_texture_atlas(LoadGlyphSetData *data, Font *font) {
  const u32 format = SDL_PIXELFORMAT_BGRA32;

  const u32 atlas_max_width = 8192;  // this might have to be lower

  if (font->outline_size > 0) {
    data->glyph_max_height += data->glyph_max_height + font->outline_size * 2;
  }

  int atlas_count = data->atlas_width / atlas_max_width;
  if (atlas_count == 0) {
    atlas_count = 1;
  } else if (atlas_count > 1) {
    data->atlas_width = atlas_max_width;
  }

  {
    // position the glyph sources
    int current_pos = 0;
    for (int i = 0; i < font->glyph_count; i++) {
      if (font->glyph_metrics[i].valid) {
        font->glyph_metrics[i].source.position.y = 0;

        if ((font->outline_size > 0 && current_pos + data->outline_glyphs[i]->w > data->atlas_width) ||
            current_pos + data->glyphs[i]->w > data->atlas_width) {
          font->glyph_metrics[i].source.position.x = 0;
          if (font->outline_size) {
            current_pos = data->outline_glyphs[i]->w + 2;
          } else {
            current_pos = data->glyphs[i]->w + 2;
          }
        } else {
          font->glyph_metrics[i].source.position.x = (float)current_pos;
          if (font->outline_size) {
            current_pos += data->outline_glyphs[i]->w + 2;
          } else {
            current_pos += data->glyphs[i]->w + 2;
          }
        }
      }
    }
  }

  font->atlas_count = atlas_count;
  SDL_Texture **atlases = (SDL_Texture **)malloc(sizeof(SDL_Texture *) * atlas_count);
  SDL_Surface *glyph_cache;
  if (font->outline_size > 0) {
    font->outline_sources = (FRect *)malloc(sizeof(FRect) * font->glyph_count);
  }
  int glyph_counter = 0;
  for (int i = 0; i < atlas_count; i++) {
    int rendered_glyphs_counter = 0;
    glyph_cache = SDL_CreateRGBSurfaceWithFormat(0, data->atlas_width, data->glyph_max_height + 2, 32, format);
    assert(glyph_cache);
    SDL_SetSurfaceBlendMode(glyph_cache, SDL_BLENDMODE_NONE);
    for (; glyph_counter < font->glyph_count; glyph_counter++) {
      if (font->glyph_metrics[glyph_counter].valid) {
        SDL_Rect dst = {
            (int)font->glyph_metrics[glyph_counter].source.position.x,
            (int)font->glyph_metrics[glyph_counter].source.position.y,
            (int)frect_width(&font->glyph_metrics[glyph_counter].source),
            (int)frect_height(&font->glyph_metrics[glyph_counter].source),
        };

        if (rendered_glyphs_counter > 0 && dst.x == 0) {  // surface is full, time to create a new atlas
          break;
        }

        font->glyph_metrics[glyph_counter].atlas_index = i;
        SDL_BlitSurface(data->glyphs[glyph_counter], NULL, glyph_cache, &dst);
        if (font->outline_size > 0) {
          dst.y += dst.h + 2;
          dst.w = data->outline_glyphs[glyph_counter]->w;
          dst.h = data->outline_glyphs[glyph_counter]->h;
          font->outline_sources[glyph_counter] = (FRect){(float)dst.x, (float)dst.y, (float)dst.w, (float)dst.h};
          SDL_BlitSurface(data->outline_glyphs[glyph_counter], NULL, glyph_cache, &dst);
        }
        rendered_glyphs_counter++;
      }
    }
    atlases[i] = SDL_CreateTextureFromSurface(font->renderer, glyph_cache);
    assert(atlases[i]);
    SDL_SetTextureBlendMode(atlases[i], SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(glyph_cache);
  }

  font->atlas = atlases;
  for (int i = 0; i < font->glyph_count; i++) {
    SDL_FreeSurface(data->glyphs[i]);
  }
}

Font load_font(const char *path, const FontLoadParams loader) {
  assert(loader.size > 0);
  Font font = {0};

  TTF_Font *main_font = TTF_OpenFont(path, loader.size);

  if (!main_font) {
    return font;
  }

  font.renderer = loader.renderer;
  font.font_handle = main_font;
  font.character_sets = loader.character_sets;
  font.size = loader.size;
  font.outline_size = loader.outline_size;
  font.character_set_count = 0;

  for (u32 i = 0; i < CHARACTER_SETS_COUNT; i++) {
    if (font.character_sets & CHARACTER_SET_BITS[i]) {
      font.glyph_count += CHARACTER_SET_ARRAY_COUNT[i];
      font.character_set_starts[font.character_set_count] = CHARACTER_SET_STARTS[i];
      font.character_set_array_count[font.character_set_count] = CHARACTER_SET_ARRAY_COUNT[i];
      font.character_set_checker[font.character_set_count] = CHARACTER_SET_CHECKER[i];
      font.character_set_count++;
    }
  }

  font.glyph_metrics = (GlyphMetrics *)malloc(sizeof(GlyphMetrics) * font.glyph_count);
  LoadGlyphSetData glyph_set_data = {0};
  glyph_set_data.glyphs = (SDL_Surface **)malloc(sizeof(SDL_Surface *) * font.glyph_count);

  if (font.outline_size > 0) {
    glyph_set_data.outline_glyphs = (SDL_Surface **)malloc(sizeof(SDL_Surface *) * font.glyph_count);
  }
  glyph_set_data.metrics = font.glyph_metrics;

  u32 *character_set_arrays[] = {BASIC_LATIN_SET, LATIN_ONE_SUPPLEMENT_SET, LATIN_EXTENDED_A_SET, LATIN_EXTENDED_B_SET, HIRAGANA_SET, KATAKANA_SET,
                                 EMOJI_SET};

  for (u32 i = 0; i < CHARACTER_SETS_COUNT; i++) {
    if (font.character_sets & CHARACTER_SET_BITS[i]) {
      process_glyph_set(&glyph_set_data, character_set_arrays[i], CHARACTER_SET_ARRAY_COUNT[i], &font);
    }
  }

  font.line_skip = TTF_FontLineSkip(main_font);
  font.height = TTF_FontHeight(main_font);
  font.ascent = TTF_FontAscent(main_font);
  font.descent = TTF_FontDescent(main_font);

  build_texture_atlas(&glyph_set_data, &font);

  return font;
}

static void draw_text_internal(
    const char *text, const Vec2 position, const RGBA *color, const RGBA *outline_color, const Font *font, const u8 is_utf8, RenderBatcher *batcher
) {
  const float height = (float)font->height;
  const float outline = outline_color->a > 0 ? ((float)font->outline_size) : 0;
  const float line_increment = (height + (outline * 2));

  float xc = position.x;
  float yc = position.y;
  u32 prev_char = 0;
  u32 current_char = 0;
  u32 char_index;

  SDL_FRect dst = {0};
  SDL_FRect temp_dst;
  SDL_Rect source;

  FRect batcher_quad;
  GlyphMetrics *metrics;
  Vec2 uvs[4];
  for (const u8 *c = (u8 *)text; *c; c++) {
    if (is_utf8 && !in_basic_latin(*c)) {
      // there might be an optimal way to translate utf8 to unicode decimal value
      uint8_t copy = *c;
      if ((copy & UTF8_MASK_ONE) != UTF8_TRAILING) {
        if (copy & UTF8_TRAILING) {
          if (copy & UTF8_MASK_THREE) {
            copy &= ~UTF8_MASK_THREE;
          } else if (copy & UTF8_MASK_TWO) {
            copy &= ~UTF8_MASK_TWO;
          } else if (copy & UTF8_MASK_ONE) {
            copy &= ~UTF8_MASK_ONE;
          }
        }
        current_char = copy;
        continue;
      } else {
        copy &= ~UTF8_TRAILING;
        current_char <<= 6;
        current_char |= copy;
        const u8 *p = c;
        p++;
        if ((*p & UTF8_MASK_ONE) == UTF8_TRAILING) {
          continue;
        }
      }
    } else {
      current_char = *c;
    }

    if (current_char == '\n') {
      yc += line_increment;
      xc = position.x;
      current_char = 0;
      prev_char = 0;
      continue;
    }

    char_index = get_index_in_font(current_char, font);
    metrics = &font->glyph_metrics[char_index];

    if (metrics->valid && metrics->source.size.x > 0) {
      if (prev_char) {
        const int kern = TTF_GetFontKerningSizeGlyphs32(font->font_handle, prev_char, current_char);
        xc += kern;
      }

      dst = (SDL_FRect){xc, yc, metrics->source.size.x, metrics->source.size.y};
      dst.x = min(dst.x, dst.x + metrics->min_x);

      if (outline > 0 && outline_color->a > 0) {
        source = (SDL_Rect
        ){(int)font->outline_sources[char_index].position.x, (int)font->outline_sources[char_index].position.y,
          (int)font->outline_sources[char_index].size.x, (int)font->outline_sources[char_index].size.y};
        if (batcher) {
          batcher_quad = (FRect){
              .position.x = dst.x,
              .position.y = dst.y,
              .size.x = dst.x + metrics->source.size.x + outline * 2,
              .size.y = dst.y + metrics->source.size.y + outline * 2,
          };
          batcher_quad.position.x -= outline * 0.5f;
          batcher_quad.position.y -= outline * 0.5f;
          int atlas_width;
          int atlas_height;
          SDL_QueryTexture(font->atlas[metrics->atlas_index], NULL, NULL, &atlas_width, &atlas_height);
          uvs[0].x = (float)(source.x) / (float)atlas_width;
          uvs[0].y = (float)(source.y) / (float)atlas_height;
          uvs[1].x = (float)(source.x + source.w) / (float)atlas_width;
          uvs[1].y = (float)(source.y) / (float)atlas_height;
          uvs[2].x = (float)(source.x + source.w) / (float)atlas_width;
          uvs[2].y = (float)(source.y + source.h) / (float)atlas_height;
          uvs[3].x = (float)(source.x) / (float)atlas_width;
          uvs[3].y = (float)(source.y + source.h) / (float)atlas_height;
          render_batcher_copy_texture_quad(batcher, font->atlas[metrics->atlas_index], outline_color, &batcher_quad, uvs);
        } else {
          temp_dst = (SDL_FRect){.x = dst.x, .y = dst.y, .w = metrics->source.size.x + outline * 2, .h = metrics->source.size.y + outline * 2};
          temp_dst.x -= outline * 0.5f;
          temp_dst.y -= outline * 0.5f;

          SDL_SetTextureColorMod(
              font->atlas[metrics->atlas_index], (u8)(255 * outline_color->r), (u8)(255 * outline_color->g), (u8)(255 * outline_color->b)
          );
          SDL_SetTextureAlphaMod(font->atlas[metrics->atlas_index], (u8)(255 * outline_color->a));
          SDL_RenderCopyF(font->renderer, font->atlas[metrics->atlas_index], &source, &temp_dst);
        }
      }

      source = (SDL_Rect){(int)metrics->source.position.x, (int)metrics->source.position.y, (int)metrics->source.size.x, (int)metrics->source.size.y};
      if (batcher) {
        batcher_quad =
            (FRect){.position.x = dst.x, .position.y = dst.y, .size.x = dst.x + metrics->source.size.x, .size.y = dst.y + metrics->source.size.y};
        int atlas_width;
        int atlas_height;
        SDL_QueryTexture(font->atlas[metrics->atlas_index], NULL, NULL, &atlas_width, &atlas_height);
        uvs[0].x = (float)(source.x) / (float)atlas_width;
        uvs[0].y = (float)(source.y) / (float)atlas_height;
        uvs[1].x = (float)(source.x + source.w) / (float)atlas_width;
        uvs[1].y = (float)(source.y) / (float)atlas_height;
        uvs[2].x = (float)(source.x + source.w) / (float)atlas_width;
        uvs[2].y = (float)(source.y + source.h) / (float)atlas_height;
        uvs[3].x = (float)(source.x) / (float)atlas_width;
        uvs[3].y = (float)(source.y + source.h) / (float)atlas_height;
        render_batcher_copy_texture_quad(batcher, font->atlas[metrics->atlas_index], color, &batcher_quad, uvs);
      } else {
        SDL_SetTextureColorMod(font->atlas[metrics->atlas_index], (u8)(255 * color->r), (u8)(255 * color->g), (u8)(255 * color->b));
        SDL_SetTextureAlphaMod(font->atlas[metrics->atlas_index], (u8)(255 * color->a));
        SDL_RenderCopyF(font->renderer, font->atlas[metrics->atlas_index], &source, &dst);
      }
    }
    prev_char = current_char;
    current_char = 0;
    xc += metrics->advance + outline;
  }
}

void draw_text_utf8(const char *text, Vec2 position, const RGBA color, const Font *font) {
  draw_text_internal(text, position, &color, &(RGBA){0, 0, 0, 0}, font, 1, NULL);
}

void draw_text_outlined_utf8(const char *text, Vec2 position, const RGBA color, const RGBA outline_color, const Font *font) {
  draw_text_internal(text, position, &color, &outline_color, font, 1, NULL);
}

void draw_text(const char *text, Vec2 position, const RGBA color, const Font *font) {
  draw_text_internal(text, position, &color, &(RGBA){0, 0, 0, 0}, font, 0, NULL);
}

void draw_text_outlined(const char *text, Vec2 position, const RGBA color, const RGBA outline_color, const Font *font) {
  draw_text_internal(text, position, &color, &outline_color, font, 0, NULL);
}

void draw_text_utf8_batched(const char *text, Vec2 position, const RGBA color, const Font *font, RenderBatcher *batcher) {
  draw_text_internal(text, position, &color, &(RGBA){0, 0, 0, 0}, font, 1, batcher);
}

void draw_text_outlined_utf8_batched(
    const char *text, Vec2 position, const RGBA color, const RGBA outline_color, const Font *font, RenderBatcher *batcher
) {
  draw_text_internal(text, position, &color, &outline_color, font, 1, batcher);
}

void draw_text_batched(const char *text, Vec2 position, const RGBA color, const Font *font, RenderBatcher *batcher) {
  draw_text_internal(text, position, &color, &(RGBA){0, 0, 0, 0}, font, 0, batcher);
}

void draw_text_outlined_batched(
    const char *text, Vec2 position, const RGBA color, const RGBA outline_color, const Font *font, RenderBatcher *batcher
) {
  draw_text_internal(text, position, &color, &outline_color, font, 0, batcher);
}

int free_font(Font *atlas) {
  assert(atlas);
  free(atlas->glyph_metrics);
  for (int i = 0; i < atlas->atlas_count; i++) {
    SDL_DestroyTexture(atlas->atlas[i]);
  }
  if (atlas->font_handle) {
    TTF_CloseFont(atlas->font_handle);
  }
  return 0;
}

Vec2 get_text_size(const char *text, const Font *font, const u8 do_outline, const u8 is_utf8) {
  const int outline = do_outline ? font->outline_size : 0;
  const int height = font->height;

  int x = 0;
  int minx = 0;
  int maxx = 0;

  u32 prev_char = 0;
  u32 current_char = 0;

  Vec2 result = {-1, (float)(height + outline * 2)};
  GlyphMetrics *metric;

  for (const u8 *c = (u8 *)text; *c; c++) {
    if (is_utf8 && !in_basic_latin(*c)) {
      // there might be an optimal way to translate utf8 to unicode decimal value
      u8 copy = *c;
      if ((copy & UTF8_MASK_ONE) != UTF8_TRAILING) {
        if (copy & UTF8_TRAILING) {
          if (copy & UTF8_MASK_THREE) {
            copy &= ~UTF8_MASK_THREE;
          } else if (copy & UTF8_MASK_TWO) {
            copy &= ~UTF8_MASK_TWO;
          } else if (copy & UTF8_MASK_ONE) {
            copy &= ~UTF8_MASK_ONE;
          }
        }
        current_char = copy;
        continue;
      } else {
        copy &= ~UTF8_TRAILING;
        current_char <<= 6;
        current_char |= copy;
        const u8 *p = c;
        p++;
        if ((*p & UTF8_MASK_ONE) == UTF8_TRAILING) {
          continue;
        }
      }
    } else {
      current_char = *c;
    }

    if (current_char == '\n') {
      result.y += height + outline;
      result.x = (maxx - minx) > result.x ? (maxx - minx) : result.x;
      maxx = 0;
      minx = 0;
      x = 0;
      prev_char = 0;
      continue;
    }

    metric = &font->glyph_metrics[get_index_in_font(current_char, font)];

    if (prev_char && metric->valid) {
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
