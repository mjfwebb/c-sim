
#pragma once

#include "headers.h"

#define mouse_primary_down(mouse_state) \
  (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_RELEASED)

#define mouse_primary_pressed(mouse_state) \
  (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_PRESSED)

#define mouse_primary_released(mouse_state) \
  (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_RELEASED && mouse_state.prev_state == SDL_PRESSED)

typedef struct {
  int prev_state;
  int state;
  int button;
  Vec2 position;
  Vec2 prev_position;
  int clicks;
} MouseState;

MouseState mouse_state = {0};