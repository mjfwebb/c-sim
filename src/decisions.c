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
          calculate_distance_squared(game_context.position[current_entity_id].target, game_context.position[entity_id].target);

      if (distance_between_entities < closest_distance) {
        closest_distance = distance_between_entities;
        closest_tree_id = entity_id;
      }
    }
  }

  return (EntityDistance){closest_distance, closest_tree_id};
}

void handle_attack(int entity_id, int attacker_id) {
  if (game_context.health_current[entity_id] == 0) {
    game_context.speed[entity_id].current_velocity = 0.0f;
    game_context.decision[entity_id] = Decisions__Wait;
    game_context.killed_by[entity_id] = attacker_id;
    return;
  }
}

void handle_cultivation(int entity_id) {
  int realm = game_context.realm[entity_id] + 1;
  int exp_required_to_go_up_realm = (realm * 50) << realm;

  if (game_context.experience[entity_id] >= exp_required_to_go_up_realm) {
    game_context.realm[entity_id]++;
    game_context.experience[entity_id] = 0;
  }
}

void make_action_human(int entity_id) {
  if ((game_context.health_current[entity_id] > 0) && (game_context.health_current[entity_id] < (game_context.health_max[entity_id] * 0.2)) &&
      game_context.decision[entity_id] != Decisions__Flee) {
    // print("%s flees!", game_context.name[entity_id]);
    set_random_entity_direction(entity_id, BASE_VELOCITY);
    game_context.decision[entity_id] = Decisions__Flee;
    return;
  }

  switch (game_context.decision[entity_id]) {
    case Decisions__Find_Tree: {
      EntityDistance closest_tree = find_closest_entity_of_species(entity_id, Species__Tree);
      if (closest_tree.id > -1) {
        game_context.speed[entity_id].current_direction =
            get_direction_vec2(game_context.position[entity_id].target, game_context.position[closest_tree.id].target);
        game_context.speed[entity_id].current_velocity = BASE_VELOCITY;
        if (closest_tree.distance < 5000.0f) {
          game_context.speed[entity_id].current_velocity = 0;
          game_context.decision[entity_id] = Decisions__Chop_Tree;
        }
      }
    } break;
    case Decisions__Chop_Tree: {
      EntityDistance closest_tree = find_closest_entity_of_species(entity_id, Species__Tree);
      if (closest_tree.id > -1 && closest_tree.distance < 5000.0f) {
        game_context.health_current[closest_tree.id] = max(0, game_context.health_current[closest_tree.id] - 100);
        // print("%s chopped %s", game_context.name[entity_id], game_context.name[closest_tree.id]);
      } else {
        game_context.decision[entity_id] = Decisions__Wait;
      }
    } break;
    case Decisions__Find_Human: {
      EntityDistance closest_human = find_closest_entity_of_species(entity_id, Species__Human);
      if (closest_human.id > -1) {
        game_context.speed[entity_id].current_direction =
            get_direction_vec2(game_context.position[entity_id].target, game_context.position[closest_human.id].target);
        game_context.speed[entity_id].current_velocity = BASE_VELOCITY;
        if (closest_human.distance < 5000.0f) {
          game_context.speed[entity_id].current_velocity = 0;
          game_context.decision[entity_id] = Decisions__Attack_Human;
        }
      }
    } break;
    case Decisions__Attack_Human: {
      EntityDistance closest_human = find_closest_entity_of_species(entity_id, Species__Human);
      if (closest_human.id > -1 && closest_human.distance < 5000.0f) {
        int damage = random_int_between(5, 15) * (game_context.realm[entity_id] + 1);
        game_context.health_current[closest_human.id] = max(0, game_context.health_current[closest_human.id] - damage);
        // print("%s attacked %s", game_context.name[entity_id], game_context.name[closest_human.id]);
        handle_attack(closest_human.id, entity_id);
      } else {
        int should_continue_the_hunt = random_int_between(0, 1);
        if (should_continue_the_hunt) {
          game_context.decision[entity_id] = Decisions__Find_Human;
        } else {
          game_context.decision[entity_id] = Decisions__Wait;
        }
      }
    } break;
    case Decisions__Find_Rock: {
      EntityDistance closest_rock = find_closest_entity_of_species(entity_id, Species__Rock);
      if (closest_rock.id > -1) {
        game_context.speed[entity_id].current_direction =
            get_direction_vec2(game_context.position[entity_id].target, game_context.position[closest_rock.id].target);
        game_context.speed[entity_id].current_velocity = BASE_VELOCITY;
        if (closest_rock.distance < 5000.0f) {
          game_context.speed[entity_id].current_velocity = 0;
          game_context.decision[entity_id] = Decisions__Mine_Rock;
        }
      }
    } break;
    case Decisions__Mine_Rock: {
      EntityDistance closest_rock = find_closest_entity_of_species(entity_id, Species__Rock);
      if (closest_rock.id > -1 && closest_rock.distance < 5000.0f) {
        game_context.health_current[closest_rock.id] = max(0, game_context.health_current[closest_rock.id] - 100);
        // print("%s mined %s", game_context.name[entity_id], game_context.name[closest_rock.id]);
      } else {
        game_context.decision[entity_id] = Decisions__Wait;
      }
    } break;
    case Decisions__Cultivate:
      int exp_gain = random_int_between(1, 2);
      game_context.experience[entity_id] += exp_gain;
      handle_cultivation(entity_id);
      game_context.speed[entity_id].current_velocity = 0;
      break;

    case Decisions__Wait:
      game_context.speed[entity_id].current_velocity = 0;
      break;

    default:
      break;
  }
}

void make_action(int entity_id) {
  game_context.action_countdown[entity_id] -= simulation_speeds[physics_context.simulation_speed];
  if (game_context.health_current[entity_id] > 0 && game_context.action_countdown[entity_id] <= 0) {
    // Passive regeneration!
    game_context.health_current[entity_id] = min(game_context.health_max[entity_id], game_context.health_current[entity_id] + 1);

    switch (game_context.species[entity_id]) {
      case Species__Human:
        make_action_human(entity_id);
        break;

      default:
        break;
    }

    game_context.action_countdown[entity_id] = TICKS_TO_NEXT_ACTION;
  }
}

void make_decision_human(int entity_id) {
  switch (game_context.decision[entity_id]) {
    case Decisions__Wait:
    case Decisions__Wander: {
      if (game_context.health_current[entity_id] > 0) {
        int random_chance = random_int_between(0, 5);

        if (random_chance == 1) {
          game_context.decision[entity_id] = Decisions__Cultivate;
          break;
        }

        if (random_chance == 2) {
          // print("%s is wandering", game_context.name[entity_id]);
          game_context.decision[entity_id] = Decisions__Wander;
          set_random_entity_direction(entity_id, BASE_VELOCITY);
          break;
        }

        if (random_chance == 3) {
          // print("%s is looking for a tree", game_context.name[entity_id]);
          game_context.decision[entity_id] = Decisions__Find_Tree;
          break;
        }

        if (random_chance == 4) {
          // print("%s is looking for someone", game_context.name[entity_id]);
          game_context.decision[entity_id] = Decisions__Find_Human;
          break;
        }

        // print("%s is looking for a rock", game_context.name[entity_id]);
        game_context.decision[entity_id] = Decisions__Find_Rock;
      }
    } break;

    case Decisions__Find_Rock:
    case Decisions__Mine_Rock:
    case Decisions__Find_Tree:
    case Decisions__Find_Human:
    case Decisions__Attack_Human:
    case Decisions__Chop_Tree: {
      break;
    }

    case Decisions__Flee: {
      if (game_context.health_current[entity_id] > (game_context.health_max[entity_id] * 0.2)) {
        game_context.decision[entity_id] = Decisions__Wander;
      }
    } break;

    default:
      // print("%s is waiting", game_context.name[entity_id]);
      game_context.decision[entity_id] = Decisions__Wait;
      break;
  }
}

void make_decision(int entity_id) {
  game_context.decision_countdown[entity_id] -= simulation_speeds[physics_context.simulation_speed];
  if (game_context.decision_countdown[entity_id] <= 0) {
    // Let's see if the entity should find a new direction
    // i.e. a new target, like a tree to chop down

    switch (game_context.species[entity_id]) {
      case Species__Human:
        make_decision_human(entity_id);
        break;

      default:
        game_context.decision[entity_id] = Decisions__Wait;
        break;
    }

    game_context.decision_countdown[entity_id] = TICKS_TO_NEXT_DECISION;
  }
}
