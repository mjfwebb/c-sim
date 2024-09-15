#pragma once

#include "headers.h"

int simulation_speeds[] = {0, 1, 2, 4, 8, 16, 32};

typedef struct {
  double delta_time;
  double alpha;  // This is the interpolation factor between the current and previous states. Used for rendering accurate physics steps
  int simulation_speed;
  int prev_simulation_speed;
} PhysicsContext;

PhysicsContext physics_context = {0};