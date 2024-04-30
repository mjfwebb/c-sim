#pragma once

#include "headers.h"

typedef struct {
  double delta_time;
  double alpha;  // This is the interpolation factor between the current and previous states. Used for rendering accurate physics steps
  double simulation_speed;
  double prev_simulation_speed;
} PhysicsContext;

PhysicsContext physics_context = {0};