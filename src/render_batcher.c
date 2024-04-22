#include "render_batcher.h"

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "defs.h"
#include "colors.h"

RenderBatcher new_render_batcher(int count, SDL_Renderer* renderer)
{
    RenderBatcher result = {0};
    result.renderer = renderer;
    result.current_texture = NULL;
    result.vertex_buffer = (SDL_Vertex*)malloc(sizeof(SDL_Vertex) * 6 * count);
    result.count = 0;
    result.cursor = 0;
    result.capacity = count;
    return result;
}
void free_render_batcher(RenderBatcher *batcher)
{
    free(batcher->vertex_buffer);
}

void flush_render_batcher(RenderBatcher *batcher)
{
    if(batcher->cursor == 0){
        return;
    }
    SDL_RenderGeometry(batcher->renderer, batcher->current_texture, batcher->vertex_buffer, batcher->cursor, NULL, 0);
    batcher->cursor = 0;
    batcher->current_texture = NULL;
}

void render_batcher_copy_vertex_data(RenderBatcher *batcher, SDL_Texture *texture, const SDL_Vertex *data, const int vertex_count)
{
    assert(batcher->renderer && batcher->capacity > 0);
    if(batcher->current_texture && batcher->current_texture != texture || batcher->cursor * 6 == batcher->capacity){
        flush_render_batcher(batcher);
    }
    memcpy(batcher->vertex_buffer + batcher->cursor, data, vertex_count * sizeof(SDL_Vertex));
    batcher->cursor += vertex_count;
    batcher->current_texture = texture;
}

void render_batcher_copy_quad(RenderBatcher *batcher, const void *color, const struct FRect *quad)
{
    assert(batcher->renderer && batcher->capacity > 0);

    RGBA *c = (RGBA*)color;
    const SDL_Color vertex_color = (SDL_Color){(u8)(255 * c->r), (u8)(255 * c->g), (u8)(255 * c->b), (u8)(255 * c->a)};
    SDL_Vertex vertex_data[6];

    vertex_data[0].position.x = quad->x;
    vertex_data[0].position.y = quad->y;
    vertex_data[0].color = vertex_color;

    vertex_data[1].position.x = quad->x + quad->w;
    vertex_data[1].position.y = quad->y;
    vertex_data[1].color = vertex_color;

    vertex_data[2].position.x = quad->x;
    vertex_data[2].position.y = quad->y + quad->h;
    vertex_data[2].color = vertex_color;

    vertex_data[3].position.x = quad->x + quad->w;
    vertex_data[3].position.y = quad->y;
    vertex_data[3].color = vertex_color;

    vertex_data[4].position.x = quad->x;
    vertex_data[4].position.y = quad->y + quad->h;
    vertex_data[4].color = vertex_color;

    vertex_data[5].position.x = quad->x + quad->w;
    vertex_data[5].position.y = quad->y + quad->h;
    vertex_data[5].color = vertex_color;

    render_batcher_copy_vertex_data(batcher, NULL, vertex_data, 6);
}

void render_batcher_copy_texture_quad(RenderBatcher *batcher, SDL_Texture *texture, const void *color, const FRect *quad, const FPoint *uvs)
{
    assert(batcher->renderer && batcher->capacity > 0);

    RGBA *c = (RGBA*)color;
    const SDL_Color vertex_color = (SDL_Color){(u8)(255 * c->r), (u8)(255 * c->g), (u8)(255 * c->b), (u8)(255 * c->a)};
    SDL_Vertex vertex_data[6];

    vertex_data[0].position.x = quad->x;
    vertex_data[0].position.y = quad->y;
    vertex_data[0].color = vertex_color;

    vertex_data[1].position.x = quad->x + quad->w;
    vertex_data[1].position.y = quad->y;
    vertex_data[1].color = vertex_color;

    vertex_data[2].position.x = quad->x;
    vertex_data[2].position.y = quad->y + quad->h;
    vertex_data[2].color = vertex_color;

    vertex_data[3].position.x = quad->x + quad->w;
    vertex_data[3].position.y = quad->y;
    vertex_data[3].color = vertex_color;

    vertex_data[4].position.x = quad->x;
    vertex_data[4].position.y = quad->y + quad->h;
    vertex_data[4].color = vertex_color;

    vertex_data[5].position.x = quad->x + quad->w;
    vertex_data[5].position.y = quad->y + quad->h;
    vertex_data[5].color = vertex_color;

    if(uvs){
        vertex_data[0].tex_coord = (SDL_FPoint){uvs[0].x, uvs[0].y};
        vertex_data[1].tex_coord = (SDL_FPoint){uvs[1].x, uvs[1].y};
        vertex_data[2].tex_coord = (SDL_FPoint){uvs[2].x, uvs[2].y};
        vertex_data[3].tex_coord = (SDL_FPoint){uvs[1].x, uvs[1].y};
        vertex_data[4].tex_coord = (SDL_FPoint){uvs[2].x, uvs[2].y};
        vertex_data[5].tex_coord = (SDL_FPoint){uvs[5].x, uvs[5].y};
    }
    else{
        vertex_data[0].tex_coord = (SDL_FPoint){0, 0};
        vertex_data[1].tex_coord = (SDL_FPoint){1, 0};
        vertex_data[2].tex_coord = (SDL_FPoint){0, 1};
        vertex_data[3].tex_coord = (SDL_FPoint){1, 0};
        vertex_data[4].tex_coord = (SDL_FPoint){0, 1};
        vertex_data[5].tex_coord = (SDL_FPoint){1, 1};
    }
    render_batcher_copy_vertex_data(batcher, texture, vertex_data, 6);
}

