#include "headers.h"

// NOTES: You need to reset game_context.target_entity_id[entity_id] on new decision probably.

// TODO: put this is in a nicer file
void play_entity_sound(int entity_id, SoundEffect sound_effect) {
  if (!entity_is_visible(entity_id)) {
    return;
  }

  audio_play_sound(sound_effect);
}

void play_decision_sound(int entity_id, Decisions decision) {
  switch (decision) {
    case Decisions__Cultivate:
      play_entity_sound(entity_id, SOUND_CULTIVATE_1);
      break;

    case Decisions__Heal_Human:
      play_entity_sound(entity_id, SOUND_HEAL_1);
      break;

    case Decisions__Flee:
      // play_entity_sound(entity_id, random_int_between(SOUND_FLEE_1, SOUND_FLEE_15));
      break;

    default:
      break;
  }
}

int aggressive_personality_score(int entity_id) {
  int aggressive_score = 0;

  for (int i = 0; i < Personality_Count; i++) {
    if (game_context.personalities[entity_id][i] > 0) {
      switch (i) {
        case Personalities__Assertive:
        case Personalities__Brave:
        case Personalities__Confident:
        case Personalities__Cynical:
        case Personalities__Deceptive:
        case Personalities__Detached:
        case Personalities__Emotional:
        case Personalities__Impulsive:
        case Personalities__Insecure:
        case Personalities__Jaded:
        case Personalities__Passionate:
        case Personalities__Rebellious:
        case Personalities__Sloppy:
        case Personalities__Unreliable:
        case Personalities__Selfish:
        case Personalities__Stingy:
        case Personalities__Indifferent: {
          aggressive_score += game_context.personalities[entity_id][i];
          break;
        }

        case Personalities__Cruel: {
          aggressive_score += 3 * game_context.personalities[entity_id][i];
          break;
        }

        case Personalities__Hostile:
        case Personalities__Aggressive: {
          aggressive_score += 5 * game_context.personalities[entity_id][i];
          break;
        }

        default:
          break;
      }
    }
  }

  return aggressive_score;
}

TargetEntity find_closest_entity_of_species(int current_entity_id, Species species) {
  float closest_distance = FLT_MAX;  // Default to a big number. This is the search radius.
  int closest_tree_id = -1;

  loop(game_context.entity_count, entity_id) {
    if (current_entity_id != entity_id && game_context.species[entity_id] == species && game_context.health_current[entity_id] > 0) {
      float distance_between_entities = vec2_distance(get_entity_origin_point(current_entity_id), get_entity_origin_point(entity_id));

      if (distance_between_entities < closest_distance) {
        closest_distance = distance_between_entities;
        closest_tree_id = entity_id;
      }
    }
  }

  return (TargetEntity){closest_distance, closest_tree_id};
}

void play_entity_death_sound(int entity_id) {
  Species species = game_context.species[entity_id];

  switch (species) {
    case Species__Human:
      play_entity_sound(entity_id, SOUND_KILL_ORGANIC_1);
      break;

    case Species__Tree:
      play_entity_sound(entity_id, random_int_between(SOUND_HIT_WOOD_1, SOUND_HIT_WOOD_3));
      break;

    case Species__Rock:
      play_entity_sound(entity_id, random_int_between(SOUND_HIT_ROCK_1, SOUND_HIT_ROCK_3));
      break;

    default:
      break;
  }
}

void play_entity_hit_sound(int entity_id) {
  Species species = game_context.species[entity_id];

  switch (species) {
    case Species__Human:
      play_entity_sound(entity_id, random_int_between(SOUND_HIT_ORGANIC_1, SOUND_HIT_ORGANIC_3));
      break;

    case Species__Tree:
      play_entity_sound(entity_id, random_int_between(SOUND_HIT_WOOD_1, SOUND_HIT_WOOD_3));
      break;

    case Species__Rock:
      play_entity_sound(entity_id, random_int_between(SOUND_HIT_ROCK_1, SOUND_HIT_ROCK_3));
      break;

    default:
      break;
  }
}

void handle_death(int entity_id) {
  play_entity_death_sound(entity_id);
  Species species = game_context.species[entity_id];

  switch (species) {
    case Species__Human:
      break;

    case Species__Tree: {
      Vec2 lumber_position = (Vec2){
          .x = game_context.position[entity_id].target.x + (game_context.texture[entity_id].size.x / 2) - 50,
          .y = game_context.position[entity_id].target.y + (game_context.texture[entity_id].size.y / 2) - 50,
      };
      create_lumber(lumber_position);
      break;
    }

    case Species__Rock: {
      Vec2 quarried_rock_position = (Vec2){
          .x = game_context.position[entity_id].target.x + (game_context.texture[entity_id].size.x / 2) - 50,
          .y = game_context.position[entity_id].target.y + (game_context.texture[entity_id].size.y / 2) - 50,
      };
      create_quarried_rock(quarried_rock_position);
      break;
    } break;

    default:
      break;
  }
}

void handle_attack(int entity_id, int attacker_id, int damage) {
  game_context.health_current[entity_id] = max(0, game_context.health_current[entity_id] - damage);

  if (game_context.health_current[entity_id] == 0) {
    game_context.speed[entity_id].velocity = 0.0f;
    game_context.decision[entity_id] = Decisions__Wait;
    game_context.killed_by[entity_id] = attacker_id;
    handle_death(entity_id);
    return;
  } else {
    play_entity_hit_sound(entity_id);
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

// QUESTION: Do I want to use the bool return, mutate input style here?
TargetEntity find_entity_of_species(int entity_id, Species species) {
  TargetEntity target = {
      .distance = FLT_MAX,
      .id = game_context.target_entity_id[entity_id],
  };
  if (target.id == INVALID_ENTITY) {
    TargetEntity closest_entity = find_closest_entity_of_species(entity_id, species);
    if (closest_entity.id > -1) {
      target.id = closest_entity.id;
      target.distance = closest_entity.distance;
    }
  } else {
    if (game_context.health_current[target.id] <= 0 || game_context.species[target.id] != species) {
      target.id = INVALID_ENTITY;
    } else {
      target.distance = vec2_distance(get_entity_origin_point(entity_id), get_entity_origin_point(target.id));
    }
  }

  return target;
}

bool get_current_target(int entity_id, float valid_distance, TargetEntity* target) {
  int target_id = game_context.target_entity_id[entity_id];

  if (target_id == INVALID_ENTITY) {
    return false;
  }

  target->id = target_id;
  target->distance = vec2_distance(get_entity_origin_point(entity_id), get_entity_origin_point(target_id));

  if (game_context.health_current[target->id] <= 0 || target->distance > valid_distance) {
    game_context.target_entity_id[entity_id] = INVALID_ENTITY;
    return false;
  }

  return true;
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
      TargetEntity target = find_entity_of_species(entity_id, Species__Tree);
      game_context.target_entity_id[entity_id] = target.id;

      if (target.id == INVALID_ENTITY) {
        break;
      }

      game_context.speed[entity_id].velocity = BASE_VELOCITY;
      game_context.speed[entity_id].current_direction = vec2_direction(get_entity_origin_point(entity_id), get_entity_origin_point(target.id));

      if (target.distance < 100.0f) {
        game_context.speed[entity_id].velocity = 0;
        game_context.decision[entity_id] = Decisions__Chop_Tree;
      }
    } break;
    case Decisions__Mine_Rock:
    case Decisions__Chop_Tree: {
      TargetEntity target;
      bool valid_target = get_current_target(entity_id, 100.0f, &target);
      if (valid_target) {
        int damage = random_int_between(50, 100);
        handle_attack(target.id, entity_id, damage);
      } else {
        game_context.decision[entity_id] = Decisions__Wait;
      }
    } break;
    case Decisions__Find_Human: {
      TargetEntity target = find_entity_of_species(entity_id, Species__Human);
      game_context.target_entity_id[entity_id] = target.id;

      if (target.id == INVALID_ENTITY) {
        break;
      }

      game_context.speed[entity_id].velocity = BASE_VELOCITY;
      game_context.speed[entity_id].current_direction = vec2_direction(get_entity_origin_point(entity_id), get_entity_origin_point(target.id));

      if (target.distance < 100.0f) {
        game_context.speed[entity_id].velocity = 0;

        int aggressive_score = aggressive_personality_score(entity_id);
        // If you are a bastard, then attack.
        if (aggressive_score > 100) {
          game_context.decision[entity_id] = Decisions__Attack_Human;
          break;
        }

        // If you are nice, and the target hasn't got full health, heal them?
        if (game_context.health_current[target.id] < 50) {
          // Rub them!
          game_context.decision[entity_id] = Decisions__Heal_Human;
          break;
        }

        // If you are nice, and the target is at full health, give them a kiss? OwO
        game_context.decision[entity_id] = Decisions__Socialise;
      }

    } break;
    case Decisions__Heal_Human: {
      TargetEntity target;
      bool valid_target = get_current_target(entity_id, 100.0f, &target);
      if (valid_target) {
        int heal = random_int_between(3, 7) * (game_context.realm[entity_id] + 1);
        game_context.health_current[target.id] = max(game_context.health_max[target.id], game_context.health_current[target.id] + heal);
        play_decision_sound(entity_id, Decisions__Heal_Human);
      } else {
        game_context.decision[entity_id] = Decisions__Wait;
      }
    } break;
    case Decisions__Attack_Human: {
      TargetEntity target;
      bool valid_target = get_current_target(entity_id, 100.0f, &target);
      if (valid_target) {
        int damage = random_int_between(5, 15) * (game_context.realm[entity_id] + 1);
        handle_attack(target.id, entity_id, damage);
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
      TargetEntity target = find_entity_of_species(entity_id, Species__Rock);
      game_context.target_entity_id[entity_id] = target.id;

      if (target.id == INVALID_ENTITY) {
        break;
      }

      game_context.speed[entity_id].velocity = BASE_VELOCITY;
      game_context.speed[entity_id].current_direction = vec2_direction(get_entity_origin_point(entity_id), get_entity_origin_point(target.id));

      if (target.distance < 100.0f) {
        game_context.speed[entity_id].velocity = 0;
        game_context.decision[entity_id] = Decisions__Mine_Rock;
      }
    } break;
    case Decisions__Cultivate:
      int exp_gain = random_int_between(1, 2);
      play_decision_sound(entity_id, Decisions__Cultivate);
      game_context.experience[entity_id] += exp_gain;
      handle_cultivation(entity_id);
      game_context.speed[entity_id].velocity = 0;
      break;

    case Decisions__Wait:
      game_context.speed[entity_id].velocity = 0;
      break;

    default:
      break;
  }
}

void make_action(int entity_id) {
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

void reduce_countdowns(int entity_id) {
  if (game_context.health_current[entity_id] > 0) {
    game_context.action_countdown[entity_id] -= 1;
    if (game_context.action_countdown[entity_id] <= 0) {
      make_action(entity_id);
    }

    game_context.hunger_countdown[entity_id] -= 1;
    if (game_context.hunger_countdown[entity_id] <= 0) {
      game_context.hunger_current[entity_id] = max(0, game_context.hunger_current[entity_id] - 1);
      game_context.hunger_countdown[entity_id] = TICKS_TO_HUNGER;
    }

    game_context.thirst_countdown[entity_id] -= 1;
    if (game_context.thirst_countdown[entity_id] <= 0) {
      game_context.thirst_current[entity_id] = max(0, game_context.thirst_current[entity_id] - 1);
      game_context.thirst_countdown[entity_id] = TICKS_TO_THIRST;
    }
  }

  game_context.decision_countdown[entity_id] -= 1;
  if (game_context.decision_countdown[entity_id] <= 0) {
    make_decision(entity_id);
  }
}