#pragma once

#include "headers.h"

bool entity_is_visible(int entity_id);
FRect get_entity_hit_box_rect(int entity_id);
FRect get_entity_hit_box_rect_target(int entity_id);
Vec2 get_entity_origin_point(int entity_id);
FRect get_entity_render_rect(int entity_id);
void calculate_visible_entities(FRect camera_rect);
void create_lumber(Vec2 position);
void create_quarried_rock(Vec2 position);