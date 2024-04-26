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
  FPoint current;
  FPoint target;
  float target_zoom;
  float zoom;
  Spring zoom_spring;
  Spring pan_spring_x;
  Spring pan_spring_y;
} Camera;

typedef struct {
  FPoint position;
  FPoint target;
  Spring spring_x;
  Spring spring_y;
} Selection;

typedef struct {
  int w;
  int h;
  SDL_Texture *texture;
} Image;

typedef struct {
  float speed;
  float prev_speed;
  float delta_time;
  float animated_time;
  SDL_Renderer *renderer;
  int window_w;
  int window_h;
  SDL_Color background_color;
  Camera camera;
  Font fonts[NUM_OF_FONTS];
  float fps;
  Selection selection;
  const u8 *keyboard_state;
  Image *images;
} RenderContext;

typedef struct {
  int health[MAX_ENTITIES];
  char names[MAX_ENTITIES][128];
  bool selected[MAX_ENTITIES];
  bool hovered[MAX_ENTITIES];
  FRect rect[MAX_ENTITIES];
  int image[MAX_ENTITIES];
  FPoint direction[MAX_ENTITIES];
  int personalities[MAX_ENTITIES][Personality_Count];
  int entity_count;
  bool game_is_still_running;
} GameContext;

GameContext game_context = {0};
RenderContext render_context = {0};