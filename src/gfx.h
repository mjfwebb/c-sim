#pragma once

#include "headers.h"

int gfx_init();

void gfx_render_present();

void gfx_get_window_size(int *out_w, int *out_h);

void gfx_draw_frect(FRect *rect, RGBA *color);

void gfx_draw_frect_filled(FRect *rect, RGBA *color);

void gfx_load_textures();

void gfx_set_blend_mode_blend();

void gfx_set_blend_mode_none();

void gfx_destroy();

void gfx_draw_line(float x1, float y1, float x2, float y2, RGBA *color);

void gfx_clear_screen();

bool gfx_frect_intersects_frect(FRect *rect_a, FRect *rect_b);

bool gfx_frect_contains_point(FRect *rect, Vec2 *point);