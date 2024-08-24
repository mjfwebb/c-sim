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
  Vec2 current;
  Vec2 target;
  Spring spring_x;
  Spring spring_y;
} Position;

typedef struct {
  int interval;
  double accumulated;
} Timer;

typedef struct {
  u32 count;
  GPU_Image *textures[64];
} TextureAtlas;

typedef struct {
  Timer timer[2];
  // SDL_Renderer *renderer;
  SDL_Window *window;
  int window_w;
  int window_h;
  SDL_Color background_color;
  Camera camera;
  Font fonts[NUM_OF_FONTS];
  float fps;
  Position selection;
  const u8 *keyboard_state;
  TextureAtlas texture_atlas;
  GPU_Target *target;
  Uint32 shader_program;
} RenderContext;

RenderContext render_context = {0};