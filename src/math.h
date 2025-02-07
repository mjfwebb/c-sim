#pragma once

#include "headers.h"

typedef struct Vec2 {
  float x;
  float y;
} Vec2;

float vec2_length(Vec2 v);
float vec2_distance(Vec2 a, Vec2 b);
Vec2 vec2_normalize(Vec2 v);
Vec2 vec2_direction(Vec2 a, Vec2 b);