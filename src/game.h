#pragma once

#include "headers.h"

typedef struct {
  Vec2 current_position;
  Vec2 previous_position;
} PositionComponent;

typedef struct {
  float current_velocity;
  float previous_velocity;
  Vec2 current_direction;
  Vec2 previous_direction;
} SpeedComponent;

typedef struct {
  u32 texture_id;
  Vec2 size;
} TextureComponent;

typedef struct {
  int health_current[MAX_ENTITIES];
  int health_max[MAX_ENTITIES];
  char names[MAX_ENTITIES][128];
  bool selected[MAX_ENTITIES];
  bool hovered[MAX_ENTITIES];
  // FRect rect[MAX_ENTITIES];
  // int image[MAX_ENTITIES];
  Vec2 direction[MAX_ENTITIES];
  int personalities[MAX_ENTITIES][Personality_Count];
  int entity_count;
  SpeedComponent speeds[MAX_ENTITIES];
  PositionComponent positions[MAX_ENTITIES];
  TextureComponent textures[MAX_ENTITIES];
  bool game_is_still_running;
  bool in_pause_menu;
} GameContext;

GameContext game_context = {0};