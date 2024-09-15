#include "headers.h"

int visible_entities[10000] = {0};
int num_of_visible_entities = 0;

bool entity_is_visible(int entity_id) {
  loop(num_of_visible_entities, index) {
    if (visible_entities[index] == entity_id) {
      return true;
    }
  }
  return false;
}

int compare_entities_y(const void *a, const void *b) {
  int a_id = *(int *)a;
  int b_id = *(int *)b;

  FRect entity_a_rect = get_entity_render_rect(a_id);
  FRect entity_b_rect = get_entity_render_rect(b_id);

  float bottom = entity_a_rect.bottom - entity_b_rect.bottom;

  if (bottom < 0) {
    return -1;
  }

  if (bottom > 0) {
    return 1;
  }

  return a_id - b_id;
}

FRect get_entity_render_rect(int entity_id) {
  FRect texture_rect = {0};
  texture_rect.left = game_context.position[entity_id].current.x;
  texture_rect.top = game_context.position[entity_id].current.y;
  texture_rect.right = texture_rect.left + game_context.texture[entity_id].size.x;
  texture_rect.bottom = texture_rect.top + game_context.texture[entity_id].size.y;

  return texture_rect;
}

void calculate_visible_entities(FRect camera_rect) {
  memset(visible_entities, 0, sizeof(visible_entities));
  num_of_visible_entities = 0;
  // Create a array of entities which are "visible"
  loop(game_context.entity_count, entity_id) {
    FRect entity_render_rect = get_entity_render_rect(entity_id);
    if (gfx_frect_intersects_frect(&entity_render_rect, &camera_rect)) {
      visible_entities[num_of_visible_entities] = entity_id;
      num_of_visible_entities++;
    }
  }

  qsort(visible_entities, num_of_visible_entities, sizeof(int), compare_entities_y);
}

FRect get_entity_hit_box_rect(int entity_id) {
  FRect hit_box_rect = {.top = game_context.position[entity_id].current.y, .left = game_context.position[entity_id].current.x};
  hit_box_rect.left += game_context.hit_box_offset_position[entity_id].x;
  hit_box_rect.top += game_context.hit_box_offset_position[entity_id].y;
  hit_box_rect.right =
      game_context.position[entity_id].current.x + game_context.texture[entity_id].size.x - game_context.hit_box_offset_size[entity_id].x;
  hit_box_rect.bottom =
      game_context.position[entity_id].current.y + game_context.texture[entity_id].size.y - game_context.hit_box_offset_size[entity_id].y;

  return hit_box_rect;
}

FRect get_entity_hit_box_rect_target(int entity_id) {
  FRect hit_box_rect = {.top = game_context.position[entity_id].target.y, .left = game_context.position[entity_id].target.x};
  hit_box_rect.left += game_context.hit_box_offset_position[entity_id].x;
  hit_box_rect.top += game_context.hit_box_offset_position[entity_id].y;
  hit_box_rect.right =
      game_context.position[entity_id].target.x + game_context.texture[entity_id].size.x - game_context.hit_box_offset_size[entity_id].x;
  hit_box_rect.bottom =
      game_context.position[entity_id].target.y + game_context.texture[entity_id].size.y - game_context.hit_box_offset_size[entity_id].y;

  return hit_box_rect;
}

Vec2 get_entity_origin_point(int entity_id) {
  FRect entity_hit_box_rect = get_entity_hit_box_rect_target(entity_id);

  return (Vec2){
      .x = entity_hit_box_rect.left + (entity_hit_box_rect.right - entity_hit_box_rect.left) * 0.5f,
      .y = entity_hit_box_rect.top + (entity_hit_box_rect.bottom - entity_hit_box_rect.top) * 0.5f,
  };
}

float get_entity_velocity(int entity_id) {
  float velocity = game_context.speed[entity_id].velocity;
  int realm = game_context.realm[entity_id] + 1;
  float velocity_multiplier = ((float)realm / 10);
  velocity += velocity * velocity_multiplier;

  return velocity * 0.01f;
}

void set_random_entity_direction(int entity_id, float velocity) {
  float angle = (float)(random_int_between(0, 360) * ATH_PI / 180);

  game_context.speed[entity_id] = (SpeedComponent){
      .current_direction.x = cosf(angle),
      .current_direction.y = sinf(angle),
      .velocity = velocity,
  };
}

typedef struct {
  int current;
  int max;
} Stat;

void create_entity(
    float entity_width, int texture_id, Stat health, Stat hunger, Stat thirst, char *name, Species species, Vec2 position,
    Vec2 hit_box_offset_position, Vec2 hit_box_offset_size
) {
  game_context.target_entity_id[game_context.entity_count] = INVALID_ENTITY;
  game_context.texture[game_context.entity_count] = (TextureComponent){.texture_id = texture_id, .size = {.x = entity_width}};

  float scale = entity_width / render_context.texture_atlas.size[texture_id].x;
  game_context.texture[game_context.entity_count].size.y = (float)(render_context.texture_atlas.size[texture_id].y * scale);

  game_context.health_current[game_context.entity_count] = health.current;
  game_context.health_max[game_context.entity_count] = health.max;
  game_context.hunger_current[game_context.entity_count] = hunger.current;
  game_context.hunger_max[game_context.entity_count] = hunger.max;
  game_context.thirst_current[game_context.entity_count] = thirst.current;
  game_context.thirst_max[game_context.entity_count] = thirst.max;

  strcpy(game_context.name[game_context.entity_count], name);  // FIXME: Use the safe version strcpy_s. PRs welcome

  game_context.selected[game_context.entity_count] = false;
  game_context.hovered[game_context.entity_count] = false;

  game_context.realm[game_context.entity_count] = 0;
  game_context.experience[game_context.entity_count] = 0;
  game_context.species[game_context.entity_count] = species;

  game_context.decision_countdown[game_context.entity_count] = random_int_between(0, TICKS_TO_NEXT_DECISION);
  game_context.action_countdown[game_context.entity_count] = random_int_between(0, TICKS_TO_NEXT_ACTION);
  game_context.hunger_countdown[game_context.entity_count] = random_int_between(0, TICKS_TO_HUNGER);
  game_context.thirst_countdown[game_context.entity_count] = random_int_between(0, TICKS_TO_THIRST);
  game_context.decision[game_context.entity_count] = Decisions__Wait;

  game_context.hit_box_offset_position[game_context.entity_count] = hit_box_offset_position;
  game_context.hit_box_offset_size[game_context.entity_count] = hit_box_offset_size;

  game_context.position[game_context.entity_count] = (Position){
      .current = position,
      .target = position,
      .spring_x =
          {
              .target = position.x,
              .current = position.x,
              .velocity = 0.0f,
              .acceleration = 0.01f,
              .friction = 0.5f,
          },
      .spring_y =
          {
              .target = position.y,
              .current = position.y,
              .velocity = 0.0f,
              .acceleration = 0.01f,
              .friction = 0.5f,
          },
  };
}

bool entity_has_personality(int entity_index, Personality personality) {
  return game_context.personalities[entity_index][personality] > 0;
}

void create_tree(void) {
  float entity_width = 500.0f;
  int texture_id = random_int_between(GFX_TEXTURE_TREE_1, GFX_TEXTURE_TREE_6);
  Vec2 position = {.x = (float)random_int_between(-400, 400) * 100, .y = (float)random_int_between(-400, 400) * 100};
  float scale = entity_width / render_context.texture_atlas.size[texture_id].x;
  Vec2 texture_size = {
      .x = entity_width,
      .y = (float)(render_context.texture_atlas.size[texture_id].y * scale),
  };

  // These values get added to the current position
  Vec2 position_offset = {.x = 150.0f, .y = texture_size.y - 200.0f};

  // These values get subtracted from (current position + texture_size)
  Vec2 size_offset = {.x = 150.0f, .y = 0.0f};

  Stat health = {
      .current = 1000,
      .max = 1000,
  };
  Stat hunger = {
      .current = 0,
      .max = 0,
  };
  Stat thirst = {
      .current = 0,
      .max = 0,
  };
  create_entity(entity_width, texture_id, health, hunger, thirst, "tree", Species__Tree, position, position_offset, size_offset);

  game_context.entity_count++;
}

void create_rock(void) {
  Vec2 position = {.x = (float)random_int_between(-400, 400) * 100, .y = (float)random_int_between(-400, 400) * 100};
  Stat health = {
      .current = 1000,
      .max = 1000,
  };
  Stat hunger = {
      .current = 0,
      .max = 0,
  };
  Stat thirst = {
      .current = 0,
      .max = 0,
  };
  create_entity(100.0f, GFX_TEXTURE_ROCK, health, hunger, thirst, "rock", Species__Rock, position, (Vec2){0}, (Vec2){0});

  game_context.entity_count++;
}

void create_human(char *name) {
  Vec2 position = {.x = (float)random_int_between(-1000, 1000), .y = (float)random_int_between(-1000, 1000)};
  Stat health = {
      .current = 100,
      .max = 100,
  };
  Stat hunger = {
      .current = 100,
      .max = 100,
  };
  Stat thirst = {
      .current = 100,
      .max = 100,
  };

  create_entity(100.0f, random_int_between(0, 7), health, hunger, thirst, name, Species__Human, position, (Vec2){0}, (Vec2){0});

  int entity_id = game_context.entity_count;
  set_random_entity_direction(entity_id, BASE_VELOCITY);

  int random_amount_of_personalities = random_int_between(5, 10);
  for (int i = 0; i < random_amount_of_personalities; i++) {
    int personality = random_int_between(0, Personality_Count);
    game_context.personalities[entity_id][personality] = random_int_between(0, 100);
  }

  int length = 0;
  for (int personality_i = 0; personality_i < Personality_Count; personality_i++) {
    if (entity_has_personality(entity_id, personality_i)) {
      if (length == 0) {
        game_context.sorted_personalities[entity_id][0] = personality_i;
        length++;
        continue;
      }

      int score = game_context.personalities[entity_id][personality_i];

      for (int i = 0; i < length; i++) {
        if (score > game_context.personalities[entity_id][game_context.sorted_personalities[entity_id][i]]) {
          memmove(game_context.sorted_personalities[entity_id] + i + 1, game_context.sorted_personalities[entity_id] + i, (length - i) * sizeof(int));
          game_context.sorted_personalities[entity_id][i] = personality_i;
          length++;
          break;
        }

        if (i == length - 1) {
          game_context.sorted_personalities[entity_id][length] = personality_i;
          length++;
          break;
        }
      }
    }

    game_context.sorted_personalities_length[entity_id] = length;
  }

  game_context.entity_count++;
}

void create_entities(void) {
  for (int name_index = 0; name_index < array_count(entity_names); name_index++) {
    create_human(entity_names[name_index]);
  }

  for (int i = 0; i < 9999; i++) {
    create_tree();
  }

  for (int i = 0; i < 9999; i++) {
    create_rock();
  }
}
