#pragma once

#include "headers.h"
#include "personalities.h"

#define INVALID_ENTITY (-100000000)
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

typedef struct Spring {
  float target;
  float current;
  float velocity;
  float acceleration;
  float friction;
} Spring;

typedef struct {
  Vec2 current;
  Vec2 target;
  float target_zoom;
  float zoom;
  Spring zoom_spring;
  Spring pan_spring_x;
  Spring pan_spring_y;
} Camera;

typedef struct {
  Vec2 position;
  Vec2 target;
  Spring spring_x;
  Spring spring_y;
} Selection;

typedef struct {
  int w;
  int h;
  SDL_Texture *texture;
} Image;

typedef struct {
  int interval;
  double accumulated;
} Timer;

typedef struct {
  u32 count;
  SDL_Texture *textures[32];
  Vec2 size[32];
} TextureAtlas;

typedef struct {
  Timer timer[2];
  SDL_Renderer *renderer;
  SDL_Window *window;
  int window_w;
  int window_h;
  SDL_Color background_color;
  Camera camera;
  Font fonts[NUM_OF_FONTS];
  float fps;
  Selection selection;
  const u8 *keyboard_state;
  TextureAtlas texture_atlas;
} RenderContext;

RenderContext render_context = {0};