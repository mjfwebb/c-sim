#pragma once

#include "headers.h"

typedef struct EntityDistance {
  float distance;
  int id;
} EntityDistance;

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
  char name[MAX_ENTITIES][128];
  bool selected[MAX_ENTITIES];
  bool single_entity_selected;
  bool hovered[MAX_ENTITIES];
  // FRect rect[MAX_ENTITIES];
  // int image[MAX_ENTITIES];
  Vec2 direction[MAX_ENTITIES];
  int personalities[MAX_ENTITIES][Personality_Count];
  int species[MAX_ENTITIES];  // 0 is human, 1 is tree
  int decision[MAX_ENTITIES];  // 0 is human, 1 is tree
  int experience[MAX_ENTITIES];
  int realm[MAX_ENTITIES];
  int entity_count;
  int action_countdown[MAX_ENTITIES];
  int decision_countdown[MAX_ENTITIES];
  SpeedComponent speed[MAX_ENTITIES];
  PositionComponent position[MAX_ENTITIES];
  TextureComponent texture[MAX_ENTITIES];
  bool game_is_still_running;
  bool in_pause_menu;
} GameContext;

GameContext game_context = {0};