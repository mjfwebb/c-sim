#include "headers.h"

float calculate_distance_squared(Vec2 a, Vec2 b) {
  Vec2 distance = {.x = a.x - b.x, .y = a.y - b.y};

  float distance_squared = distance.x * distance.x + distance.y * distance.y;

  return distance_squared;
}

Vec2 normalize_vec2(Vec2 vec) {
  float length = sqrtf(vec.x * vec.x + vec.y * vec.y);

  if (length == 0.0f) {
    return vec;
  }

  Vec2 normalized_vec = {
      .x = vec.x / length,
      .y = vec.y / length,
  };

  return normalized_vec;
}

Vec2 get_direction_vec2(Vec2 a, Vec2 b) {
  Vec2 direction_vector = {
      .x = b.x - a.x,
      .y = b.y - a.y,
  };

  return normalize_vec2(direction_vector);
}

EntityDistance find_closest_entity_of_species(int current_entity_id, Species species) {
  float closest_distance = FLT_MAX;  // Default to a big number. This is the search radius.
  int closest_tree_id = -1;

  loop(game_context.entity_count, entity_id) {
    if (current_entity_id != entity_id && game_context.species[entity_id] == species && game_context.health_current[entity_id] > 0) {
      float distance_between_entities =
          calculate_distance_squared(game_context.position[current_entity_id].current, game_context.position[entity_id].current);

      if (distance_between_entities < closest_distance) {
        closest_distance = distance_between_entities;
        closest_tree_id = entity_id;
      }
    }
  }

  return (EntityDistance){closest_distance, closest_tree_id};
}

void make_action(int entity_id) {
  game_context.action_countdown[entity_id] -= simulation_speeds[physics_context.simulation_speed];
  if (game_context.health_current[entity_id] > 0 && game_context.action_countdown[entity_id] <= 0) {
    switch (game_context.decision[entity_id]) {
      case Decisions__Find_Tree: {
        EntityDistance closest_tree = find_closest_entity_of_species(entity_id, Species__Tree);
        if (closest_tree.id > -1) {
          game_context.speed[entity_id].current_direction =
              get_direction_vec2(game_context.position[entity_id].current, game_context.position[closest_tree.id].current);
          game_context.speed[entity_id].current_velocity = BASE_VELOCITY;
        }
        if (closest_tree.distance < 500.0f) {
          game_context.speed[entity_id].current_velocity = 0;
          game_context.decision[entity_id] = Decisions__Chop_Tree;
        }
      } break;
      case Decisions__Chop_Tree: {
        EntityDistance closest_tree = find_closest_entity_of_species(entity_id, Species__Tree);
        if (closest_tree.id > -1 && closest_tree.distance < 500.0f) {
          game_context.health_current[closest_tree.id] = max(0, game_context.health_current[closest_tree.id] - 100);
          print("%s chopped %s", game_context.name[entity_id], game_context.name[closest_tree.id]);
        } else {
          game_context.decision[entity_id] = Decisions__Find_Tree;
        }
      } break;
      case Decisions__Find_Human: {
        EntityDistance closest_human = find_closest_entity_of_species(entity_id, Species__Human);
        if (closest_human.id > -1) {
          game_context.speed[entity_id].current_direction =
              get_direction_vec2(game_context.position[entity_id].current, game_context.position[closest_human.id].current);
          game_context.speed[entity_id].current_velocity = BASE_VELOCITY;
          if (closest_human.distance < 500.0f) {
            game_context.speed[entity_id].current_velocity = 0;
            game_context.decision[entity_id] = Decisions__Attack_Human;
          }
        }
      } break;
      case Decisions__Attack_Human: {
        EntityDistance closest_human = find_closest_entity_of_species(entity_id, Species__Human);
        if (closest_human.id > -1 && closest_human.distance < 500.0f) {
          game_context.health_current[closest_human.id] = max(0, game_context.health_current[closest_human.id] - 10);
          print("%s attacked %s", game_context.name[entity_id], game_context.name[closest_human.id]);
        } else {
          game_context.decision[entity_id] = Decisions__Find_Human;
        }
      } break;
      case Decisions__Find_Rock: {
        EntityDistance closest_rock = find_closest_entity_of_species(entity_id, Species__Rock);
        if (closest_rock.id > -1) {
          game_context.speed[entity_id].current_direction =
              get_direction_vec2(game_context.position[entity_id].current, game_context.position[closest_rock.id].current);
          game_context.speed[entity_id].current_velocity = BASE_VELOCITY;
        }
        if (closest_rock.distance < 500.0f) {
          game_context.speed[entity_id].current_velocity = 0;
          game_context.decision[entity_id] = Decisions__Mine_Rock;
        }
      } break;
      case Decisions__Mine_Rock: {
        EntityDistance closest_rock = find_closest_entity_of_species(entity_id, Species__Rock);
        if (closest_rock.id > -1 && closest_rock.distance < 500.0f) {
          game_context.health_current[closest_rock.id] = max(0, game_context.health_current[closest_rock.id] - 100);
          print("%s mined %s", game_context.name[entity_id], game_context.name[closest_rock.id]);
        } else {
          game_context.decision[entity_id] = Decisions__Find_Rock;
        }
      } break;
      // case Decisions__Wander:
      //   set_random_entity_direction(entity_id, BASE_VELOCITY);
      //   break;
      case Decisions__Flee:
        set_random_entity_direction(entity_id, BASE_VELOCITY);
        break;
      case Decisions__Wait:
        game_context.speed[entity_id].current_velocity = 0;
        break;

      default:
        break;
    }

    game_context.action_countdown[entity_id] = TICKS_TO_NEXT_ACTION;
  }
}

void make_decision(int entity_id) {
  game_context.decision_countdown[entity_id] -= simulation_speeds[physics_context.simulation_speed];
  if (game_context.decision_countdown[entity_id] <= 0) {
    // Let's see if the entity should find a new direction
    // i.e. a new target, like a tree to chop down
    game_context.decision[entity_id] = Decisions__Wait;

    if (game_context.health_current[entity_id] > 0 && game_context.species[entity_id] == Species__Human) {
      int random_chance = random_int_between(0, 100);
      if (random_chance <= 20) {
        game_context.decision[entity_id] = Decisions__Wander;
        set_random_entity_direction(entity_id, BASE_VELOCITY);
      } else if (random_chance > 70) {
        game_context.decision[entity_id] = Decisions__Find_Tree;
      } else {
        game_context.decision[entity_id] = Decisions__Find_Human;
      }
    }
    game_context.decision_countdown[entity_id] = TICKS_TO_NEXT_DECISION;
  }
}
