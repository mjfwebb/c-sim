#include "headers.h"

float vec2_length(Vec2 v) {
  return sqrtf((v.x * v.x) + (v.y * v.y));
}

float vec2_distance(Vec2 a, Vec2 b) {
  Vec2 difference = {
      .x = b.x - a.x,
      .y = b.y - a.y,
  };

  return vec2_length(difference);
}

Vec2 vec2_normalize(Vec2 v) {
  float length = vec2_length(v);

  if (length < ath_epsilon) {
    return (Vec2){0};
  }

  return (Vec2){
      .x = v.x / length,
      .y = v.y / length,
  };
}

Vec2 vec2_direction(Vec2 a, Vec2 b) {
  Vec2 direction_vector = {
      .x = b.x - a.x,
      .y = b.y - a.y,
  };

  return vec2_normalize(direction_vector);
}