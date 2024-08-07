#include "headers.h"

float get_entity_velocity(int entity_id) {
  float velocity = game_context.speed[entity_id].velocity;
  int realm = game_context.realm[entity_id] + 1;
  float velocity_multiplier = ((float)realm / 10);
  velocity += velocity * velocity_multiplier;

  return velocity;
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

void create_entity(float entity_width, int texture_id, Stat health, Stat hunger, Stat thirst, char* name, Species species, Vec2 position) {
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

  game_context.position[game_context.entity_count] = (Position){
      .current = position,
      .target = position,
      .spring_x =
          {
              .target = position.x,
              .current = position.x,
              .velocity = 0.0f,
              .acceleration = 0.5f,
              .friction = 0.1f,
          },
      .spring_y =
          {
              .target = position.y,
              .current = position.y,
              .velocity = 0.0f,
              .acceleration = 0.5f,
              .friction = 0.1f,
          },
  };
}

void create_tree(void) {
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
  create_entity(500.0f, random_int_between(GFX_TEXTURE_TREE_1, GFX_TEXTURE_TREE_6), health, hunger, thirst, "tree", Species__Tree, position);

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
  create_entity(100.0f, GFX_TEXTURE_ROCK, health, hunger, thirst, "rock", Species__Rock, position);

  game_context.entity_count++;
}

void create_human(char* name) {
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

  create_entity(100.0f, random_int_between(0, 7), health, hunger, thirst, name, Species__Human, position);

  set_random_entity_direction(game_context.entity_count, BASE_VELOCITY);

  int random_amount_of_personalities = random_int_between(5, 10);
  for (int i = 0; i < random_amount_of_personalities; i++) {
    int personality = random_int_between(0, Personality_Count);
    game_context.personalities[game_context.entity_count][personality] = random_int_between(0, 100);
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
