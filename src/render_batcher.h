#pragma once

#include "headers.h"

/*
    this was made for batched font rendering but
    this can be also used for any sprite/shape OkayChamp
*/

typedef struct {
  struct SDL_Vertex *vertex_buffer;
  struct SDL_Texture *current_texture;
  struct SDL_Renderer *renderer;
  int count;
  int cursor;
  int capacity;
} RenderBatcher;

// this will allocate (count * 6) vertex data
RenderBatcher new_render_batcher(int count, struct SDL_Renderer *renderer);

void flush_render_batcher(RenderBatcher *batcher);

void render_batcher_copy_vertex_data(RenderBatcher *batcher, struct SDL_Texture *texture, const struct SDL_Vertex *data, const int vertex_count);

void render_batcher_copy_quad(RenderBatcher *batcher, const void *color, FRect *quad);

// color as void* for now because compiler giving warning for some fucking reason
// uvs is an array of 4 points that corresponds to the portion of the texture this cory is referring to
void render_batcher_copy_texture_quad(RenderBatcher *batcher, struct SDL_Texture *texture, const void *color, FRect *quad, const struct Vec2 *uvs);

void free_render_batcher(RenderBatcher *batcher);
