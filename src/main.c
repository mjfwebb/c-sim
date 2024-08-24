#define SDL_MAIN_HANDLED

#include "headers.h"

#include "defs.c"
#include "gfx.c"
#include "audio.c"
#include "fonts.c"
#include "seed.c"
#include "console.c"
#include "entities.c"
#include "pause_menu/pause_menu.c"
#include "decisions.c"
#include "spring.c"

#define VA_ARGS(...) , ##__VA_ARGS__  // For variadic macros

#define MAX_TILES 1000
GFXTextureID terrains[MAX_TILES][MAX_TILES];

int game_is_still_running = 1;

Vec2 vec2_world_to_screen(Vec2 point) {
  return (Vec2){
      .x = (point.x - render_context.camera.current.x) * render_context.camera.zoom + (render_context.window_w * 0.5f),
      .y = (point.y - render_context.camera.current.y) * render_context.camera.zoom + (render_context.window_h * 0.5f),
  };
}

Vec2 vec2_screen_to_world(Vec2 point) {
  return (Vec2){
      .x = (point.x - render_context.window_w * 0.5f) / render_context.camera.zoom + render_context.camera.current.x,
      .y = (point.y - render_context.window_h * 0.5f) / render_context.camera.zoom + render_context.camera.current.y,
  };
}

FRect frect_world_to_screen(FRect rect) {
  Vec2 position = vec2_world_to_screen((Vec2){.x = rect.left, .y = rect.top});
  Vec2 size = vec2_world_to_screen((Vec2){.x = rect.right, .y = rect.bottom});
  return (FRect){
      .top = position.y,
      .left = position.x,
      .right = size.x,
      .bottom = size.y,
  };
}

FRect frect_screen_to_world(FRect rect) {
  Vec2 position = vec2_screen_to_world((Vec2){.x = rect.left, .y = rect.top});
  Vec2 size = vec2_screen_to_world((Vec2){.x = rect.right, .y = rect.bottom});
  return (FRect){
      .top = position.y,
      .left = position.x,
      .right = size.x,
      .bottom = size.y,
  };
}

FRect get_camera_world_rect(void) {
  return (FRect){
      .left = render_context.camera.current.x - (render_context.window_w * 0.5f) / render_context.camera.zoom,
      .top = render_context.camera.current.y - (render_context.window_h * 0.5f) / render_context.camera.zoom,
      .right = render_context.camera.current.x + (render_context.window_w * 0.5f) / render_context.camera.zoom,
      .bottom = render_context.camera.current.y + (render_context.window_h * 0.5f) / render_context.camera.zoom,
  };
}

FRect get_entity_render_rect(int entity_id) {
  FRect texture_rect = {0};
  texture_rect.left = game_context.position[entity_id].current.x;
  texture_rect.top = game_context.position[entity_id].current.y;
  texture_rect.right = texture_rect.left + game_context.texture[entity_id].size.x;
  texture_rect.bottom = texture_rect.top + game_context.texture[entity_id].size.y;

  return texture_rect;
}

void draw_debug_text(int index, char *str, ...) {
  char text_buffer[128];
  va_list args;
  va_start(args, str);
  int chars_written = vsnprintf(text_buffer, sizeof(text_buffer), str, args);
  assert(chars_written > 0);
  va_end(args);

  draw_text_outlined_utf8(text_buffer, (Vec2){10.0f, (32.0f * index)}, (RGBA){0, 1, 0, 1}, (RGBA){0, 0, 0, 1}, &render_context.fonts[0]);
}

FRect get_selection_rect(void) {
  FRect rect = {
      .left = min(mouse_state.position.x, render_context.selection.current.x),
      .top = min(mouse_state.position.y, render_context.selection.current.y),
      .right = fabsf(mouse_state.position.x - render_context.selection.current.x),
      .bottom = fabsf(mouse_state.position.y - render_context.selection.current.y),
  };

  rect.right = rect.left + rect.right;
  rect.bottom = rect.top + rect.bottom;

  return rect;
}

void render_debug_info(void) {
  int index = 0;
  draw_debug_text(index++, "fps: %.2f", render_context.fps);
  draw_debug_text(index++, "mouse state: %d, button: %d, clicks: %d", mouse_state.state, mouse_state.button, mouse_state.clicks);
  draw_debug_text(index++, "prev mouse state: %d", mouse_state.prev_state);
  draw_debug_text(index++, "camera zoom: %.1f", render_context.camera.target_zoom);
  draw_debug_text(index++, "game speed: %d", simulation_speeds[physics_context.simulation_speed]);
  draw_debug_text(
      index++, "camera: current x,y: %.2f,%.2f target x,y: %.2f,%.2f", render_context.camera.current.x, render_context.camera.current.y,
      render_context.camera.target.x, render_context.camera.target.y
  );
  FRect selection_rect = get_selection_rect();
  draw_debug_text(
      index++, "selection: current x,y: %.2f,%.2f target x,y: %.2f,%.2f", selection_rect.left, selection_rect.top, render_context.selection.target.x,
      render_context.selection.target.y
  );
}

void draw_selection_box(void) {
  if (!mouse_primary_pressed(mouse_state) || game_context.in_pause_menu) {
    return;
  }

  FRect selection_rect = get_selection_rect();

  gfx_draw_frect(&selection_rect, &(RGBA){1, 1, 1, 1});
}

char *cultivation_realm_name(int cultivation_realm) {
  switch (cultivation_realm) {
    case 1:
      return "Qi Condensation";
    case 2:
      return "Foundation Establishment";
    case 3:
      return "Core Formation";
    case 4:
      return "Nascent Soul";
    default:
      return "Mortal";
  }
}

void buffer_text(char *text_buffer, float *max_width, char *format, ...) {
  Font *font = &render_context.fonts[0];
  va_list args;
  va_start(args, format);
  vsnprintf(text_buffer, 128, format, args);
  va_end(args);

  float current_text_width = get_text_size(text_buffer, font, true, false).x;

  if (current_text_width > *max_width) {
    *max_width = current_text_width;
  }
}

void draw_name(int entity_id) {
  FRect entity_hit_box_rect = get_entity_hit_box_rect(entity_id);
  FRect entity_hit_box_screen_rect = frect_world_to_screen(entity_hit_box_rect);

  Font *font = &render_context.fonts[0];
  RGBA color = (RGBA){1, 1, 1, 1};
  float name_y = (entity_hit_box_screen_rect.top - (45.0f));
  if (game_context.hovered[entity_id]) {
    name_y -= 10.0f;  // move the text up a little when using the bigger font
    color = (RGBA){1, 1, 0, 1};
    font = &render_context.fonts[1];
  }
  Vec2 name_size = get_text_size(game_context.name[entity_id], font, false, true);
  float diff = ((entity_hit_box_screen_rect.right - entity_hit_box_screen_rect.left) / 2) - (name_size.x / 2);
  float name_x = entity_hit_box_screen_rect.left + diff;

  draw_text_outlined_utf8(game_context.name[entity_id], (Vec2){.x = name_x, .y = name_y}, color, (RGBA){0, 0, 0, 1}, font);
}

void draw_entity_info(int entity_id) {
  FRect top_right_screen_rect = {0};

  // Draw the personalities list
  Font *font = &render_context.fonts[0];
  float font_size = (float)font->size;
  char text_buffer[64][128];
  int line_number = 0;
  float max_text_width = 0.0f;
  float y_start = 20.0f;
  int realm = game_context.realm[entity_id] + 1;

  buffer_text(text_buffer[line_number], &max_text_width, "ID: %d", entity_id);
  buffer_text(text_buffer[++line_number], &max_text_width, "Target ID: %d", game_context.target_entity_id[entity_id]);
  buffer_text(text_buffer[++line_number], &max_text_width, "Realm: %s", cultivation_realm_name(game_context.realm[entity_id]));
  buffer_text(text_buffer[++line_number], &max_text_width, "Experience: %d/%d", game_context.experience[entity_id], (realm * 50) << realm);
  buffer_text(
      text_buffer[++line_number], &max_text_width, "Hunger: %d/%d", game_context.hunger_current[entity_id], game_context.hunger_max[entity_id]
  );
  buffer_text(
      text_buffer[++line_number], &max_text_width, "Thirst: %d/%d", game_context.thirst_current[entity_id], game_context.thirst_max[entity_id]
  );
  buffer_text(text_buffer[++line_number], &max_text_width, "Species: %s", Species__Strings[game_context.species[entity_id]]);
  buffer_text(text_buffer[++line_number], &max_text_width, "Decision: %s", Decisions__Strings[game_context.decision[entity_id]]);
  if (game_context.health_current[entity_id] <= 0) {
    buffer_text(text_buffer[++line_number], &max_text_width, "Killed by: %s", game_context.name[game_context.killed_by[entity_id]]);
  }
  buffer_text(text_buffer[++line_number], &max_text_width, "Aggressive score: %d", aggressive_personality_score(entity_id));
  buffer_text(text_buffer[++line_number], &max_text_width, "Velocity: %f", get_entity_velocity(entity_id));

  for (int personality_index = 0; personality_index < game_context.sorted_personalities_length[entity_id]; personality_index++) {
    int personality_value = game_context.personalities[entity_id][game_context.sorted_personalities[entity_id][personality_index]];

    if (personality_value == 0) {
      break;
    }

    buffer_text(
        text_buffer[++line_number], &max_text_width, "%s: %d", Personality__Strings[game_context.sorted_personalities[entity_id][personality_index]],
        personality_value
    );
  }

  for (int i = 0; i <= line_number; i++) {
    draw_text_outlined_utf8(
        text_buffer[i],
        (Vec2){.x = (float)(render_context.window_w - (max_text_width + 40.0f)), (top_right_screen_rect.bottom + 10.0f + y_start + (font_size * i))},
        (RGBA){1, 1, 1, 1}, (RGBA){0, 0, 0, 1}, font
    );
  }
}

void draw_health_bar(int entity_id, FRect entity_rect) {
  if (game_context.health_current[entity_id] == game_context.health_max[entity_id]) {
    return;
  }

  const float y = (entity_rect.top - 15.0f * min(render_context.camera.zoom, 1.0f));
  const float h = (10.0f * min(render_context.camera.zoom, 1.0f));

  FRect total_health_rect = {
      .left = entity_rect.left,
      .top = y,
      .right = entity_rect.right,
      .bottom = y + h,
  };
  gfx_draw_frect_filled(&total_health_rect, &(RGBA){0, 0, 0, 1});

  float size_x = frect_width(&entity_rect);
  float health_width = (game_context.health_max[entity_id] - game_context.health_current[entity_id]) * size_x / game_context.health_max[entity_id];
  FRect current_health_rect = {
      .left = entity_rect.left,
      .top = y,
      .right = entity_rect.right - health_width,
      .bottom = y + h,
  };
  gfx_draw_frect_filled(&current_health_rect, &(RGBA){1, 0, 0, 1});
}

void draw_border(FRect around, float gap_width, float border_width) {
  FRect borders[4];

  //         1
  //   |-----------|
  //   |           |
  // 0 |           | 2
  //   |           |
  //   |-----------|
  //         3
  for (int i = 0; i < 4; i++) {
    borders[i] = around;

    float width = frect_width(&around);
    float height = frect_height(&around);

    if (i == 0) {  // Left (0)
      borders[i].left += -(gap_width + border_width);
      borders[i].top -= gap_width + border_width;
      borders[i].right = borders[i].left + border_width;
      borders[i].bottom = borders[i].top + height + (gap_width + border_width) * 2;
    } else if (i == 1) {  // Top (1)
      borders[i].left -= gap_width + border_width;
      borders[i].top += -(gap_width + border_width);
      borders[i].right = borders[i].left + width + (gap_width + border_width) * 2;
      borders[i].bottom = borders[i].top + border_width;
    } else if (i == 2) {  // right (2)
      borders[i].left = around.left + width + gap_width;
      borders[i].top -= gap_width + border_width;
      borders[i].right = borders[i].left + border_width;
      borders[i].bottom = borders[i].top + height + (gap_width + border_width) * 2;
    } else {  // bottom (3)
      borders[i].left -= gap_width + border_width;
      borders[i].top = around.top + height + gap_width;
      borders[i].right = borders[i].left + width + (gap_width + border_width) * 2;
      borders[i].bottom = borders[i].top + border_width;
    }

    gfx_draw_frect_filled(&borders[i], &(RGBA){1, 1, 1, 1});
  }
}

void move_entity(int entity_id) {
  if (game_context.speed[entity_id].velocity == 0.0f) {
    return;
  }

  game_context.speed[entity_id].previous_direction = game_context.speed[entity_id].current_direction;
  float velocity = get_entity_velocity(entity_id);

  game_context.position[entity_id].target.x += (game_context.speed[entity_id].current_direction.x) * velocity *
                                               (float)(physics_context.delta_time * (simulation_speeds[physics_context.simulation_speed]));
  game_context.position[entity_id].target.y += (game_context.speed[entity_id].current_direction.y) * velocity *
                                               (float)(physics_context.delta_time * (simulation_speeds[physics_context.simulation_speed]));
}

int dead_entity_texture(int entity_id) {
  switch (game_context.species[entity_id]) {
    case Species__Rock:
      return GFX_TEXTURE_ROCK_SMASHED;
    case Species__Tree:
      return game_context.texture[entity_id].texture_id + 6;
    default:
      return GFX_TEXTURE_TOMBSTONE;
  }
}

void render_entity(int entity_id) {
  FRect entity_render_rect = get_entity_render_rect(entity_id);
  FRect entity_screen_rect = frect_world_to_screen(entity_render_rect);
  FRect entity_shadow_rect = entity_screen_rect;

  float entity_shadow_height = entity_shadow_rect.bottom - entity_shadow_rect.top;
  entity_shadow_rect.top += (entity_shadow_height * 0.8f);
  entity_shadow_rect.bottom += (entity_shadow_height * 0.05f);

  float entity_shadow_width = entity_shadow_rect.right - entity_shadow_rect.left;
  entity_shadow_rect.left += (entity_shadow_width * 0.2f);
  entity_shadow_rect.right -= (entity_shadow_width * 0.2f);

  if (game_context.health_current[entity_id] <= 0) {
    gfx_draw_texture(dead_entity_texture(entity_id), entity_screen_rect.left, entity_screen_rect.top);
  } else {
    gfx_draw_texture(GFX_TEXTURE_SHADOW, entity_shadow_rect.left, entity_shadow_rect.top);
    gfx_draw_texture(game_context.texture[entity_id].texture_id, entity_screen_rect.left, entity_screen_rect.top);
  }
}

void generate_grass_textures(void) {
  // GFX_TEXTURE_ID assigned_textures[16][16];
  // int column = 0;
  // int row = 0;

  for (int y = 0; y < MAX_TILES; y++) {
    for (int x = 0; x < MAX_TILES; x++) {
      // Now get the next possible texture_id based on the current one. Loop over the textures.
      int previous_right_grass_type = (LONG_GRASS_RIGHT | OVERGROWN_GRASS_RIGHT | SHORT_GRASS_RIGHT);
      if (x > 0) {
        // We're on the top column, we don't care about what was above us.
        previous_right_grass_type = grass_textures[terrains[y][x - 1]] & (LONG_GRASS_RIGHT | OVERGROWN_GRASS_RIGHT | SHORT_GRASS_RIGHT);
      }

      int previous_bottom_grass_type = (LONG_GRASS_BOTTOM | OVERGROWN_GRASS_BOTTOM | SHORT_GRASS_BOTTOM);
      if (y > 0) {
        // We're no longer on the top, we need to consider what was above.
        previous_bottom_grass_type = grass_textures[terrains[y - 1][x]] & (LONG_GRASS_BOTTOM | OVERGROWN_GRASS_BOTTOM | SHORT_GRASS_BOTTOM);
      }

      int possible_textures[64] = {0};
      int possible_textures_count = 0;
      for (int texture_id = GFX_TEXTURE_GRASS_LONG_CENTER; texture_id <= GFX_TEXTURE_GRASS_SHORT_OVERGROWN_TOP; texture_id++) {
        // { return tile.type & rightType != 0 && tile.type & topType != 0;});
        if ((grass_textures[texture_id] & (previous_right_grass_type << 2)) != 0 &&
            (grass_textures[texture_id] & (previous_bottom_grass_type >> 2)) != 0) {
          possible_textures[possible_textures_count] = texture_id;
          possible_textures_count++;
        }
      }

      int chosen_texture_id = possible_textures[random_int_between(0, possible_textures_count - 1)];
      terrains[y][x] = chosen_texture_id;
    }
  }
}

void draw_terrain(void) {
  // TODO: Fix this
  // gfx_set_blend_mode_blend();
  float original_grid_size = 256.0f;
  float zoom = render_context.camera.zoom;
  Vec2 camera = render_context.camera.current;
  float grid_size = original_grid_size * zoom;
  int window_w = render_context.window_w;
  int window_h = render_context.window_h;

  int x_start = (int)((camera.x * zoom - window_w / 2) / grid_size);
  int y_start = (int)((camera.y * zoom - window_h / 2) / grid_size);

  // The amount of tiles that can fit in the whole screen
  int screen_tiles_x = (int)(window_w / grid_size);
  int screen_tiles_y = (int)(window_h / grid_size);

  // The amount of tiles that is drawn beyond the calculated screen tiles
  int padding = 2;
  // RGBA color = {1, 1, 1, 1};

  for (int y = y_start - padding; y < y_start + screen_tiles_y + padding; y++) {
    for (int x = x_start - padding; x < x_start + screen_tiles_x + padding; x++) {
      float grid_pos_x = x * grid_size;
      float grid_pos_y = y * grid_size;
      int terrain_y = (MAX_TILES / 2) + y;
      int terrain_x = (MAX_TILES / 2) + x;
      if (terrain_y < 0 || terrain_y >= MAX_TILES || terrain_x < 0 || terrain_x >= MAX_TILES) {
        continue;
      }
      int terrain_id = terrains[terrain_y][terrain_x];
      gfx_draw_texture(terrain_id, grid_pos_x - (camera.x * zoom - window_w / 2), grid_pos_y - (camera.y * zoom - window_h / 2));
    }
  }
  // TODO: Fix this
  // gfx_set_blend_mode_none();
}

void mouse_control_camera(void) {
  if (mouse_state.button == SDL_BUTTON_RIGHT && mouse_state.state == SDL_PRESSED) {
    if (mouse_state.prev_position.x != mouse_state.position.x || mouse_state.prev_position.y != mouse_state.position.y) {
      float delta_x = mouse_state.position.x - mouse_state.prev_position.x;
      float delta_y = mouse_state.position.y - mouse_state.prev_position.y;
      mouse_state.prev_position.x = mouse_state.position.x;
      mouse_state.prev_position.y = mouse_state.position.y;

      render_context.camera.target.x -= delta_x / render_context.camera.zoom;
      render_context.camera.target.y -= delta_y / render_context.camera.zoom;
    }
  }
}

void deselect_all_entities(void) {
  // reset spring position
  render_context.camera.pan_spring_x.current = render_context.camera.current.x;
  render_context.camera.pan_spring_y.current = render_context.camera.current.y;

  loop(game_context.entity_count, entity_id) {
    game_context.selected[entity_id] = false;
  }
}

// Camera movement and selection rect movement
void keyboard_control_camera(void) {
  float camera_keyboard_movement_speed = 15.0f;
  if (render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_w)]) {
    deselect_all_entities();
    render_context.camera.target.y -= camera_keyboard_movement_speed / render_context.camera.zoom;
    render_context.selection.target.y += camera_keyboard_movement_speed;
  }
  if (render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_s)]) {
    deselect_all_entities();
    render_context.camera.target.y += camera_keyboard_movement_speed / render_context.camera.zoom;
    render_context.selection.target.y -= camera_keyboard_movement_speed;
  }
  if (render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_a)]) {
    deselect_all_entities();
    render_context.camera.target.x -= camera_keyboard_movement_speed / render_context.camera.zoom;
    render_context.selection.target.x += camera_keyboard_movement_speed;
  }
  if (render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_d)]) {
    deselect_all_entities();
    render_context.camera.target.x += camera_keyboard_movement_speed / render_context.camera.zoom;
    render_context.selection.target.x -= camera_keyboard_movement_speed;
  }
}

int get_singly_selected_entity(void) {
  int result = INVALID_ENTITY;
  int selected_count = 0;
  loop(game_context.entity_count, entity_id) {
    if (game_context.selected[entity_id]) {
      selected_count += 1;
      result = entity_id;
      if (selected_count > 1) {
        break;
      }
    }
  }
  return selected_count == 1 ? result : INVALID_ENTITY;
}

// Set selected on any entity within the selection_rect
void select_entities_within_selection_rect(void) {
  loop(game_context.entity_count, entity_id) {
    FRect entity_hit_box_rect = get_entity_hit_box_rect(entity_id);
    FRect entity_screen_rect = frect_world_to_screen(entity_hit_box_rect);

    Vec2 point_top_left = {
        .x = entity_screen_rect.left,
        .y = entity_screen_rect.top,
    };
    Vec2 point_bottom_right = {
        .x = entity_screen_rect.right,
        .y = entity_screen_rect.bottom,
    };

    FRect selection_rect = get_selection_rect();

    if (selection_rect.right > 30.0f) {
      if (gfx_frect_contains_point(&selection_rect, &point_top_left) && gfx_frect_contains_point(&selection_rect, &point_bottom_right)) {
        game_context.selected[entity_id] = true;
      } else {
        if (!render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_LSHIFT)]) {
          game_context.selected[entity_id] = false;
        }
      }
    }
  }
}

bool is_entity_under_mouse(int entity_id) {
  FRect entity_hit_box_rect = get_entity_hit_box_rect(entity_id);
  FRect rect = frect_world_to_screen(entity_hit_box_rect);

  return gfx_frect_contains_point(&rect, &mouse_state.position);
}

void update(void) {
  // Spring the camera zoom
  render_context.camera.zoom = spring_update(&render_context.camera.zoom_spring, render_context.camera.target_zoom);

  // Spring the console position
  console.y = spring_update(&console.y_spring, console.target_y);

  mouse_control_camera();

  if (game_context.in_pause_menu) {
    return;
  }

  if (!console_is_open()) {
    keyboard_control_camera();
  }

  if (physics_context.simulation_speed > 0) {
    loop(game_context.entity_count, entity_id) {
      reduce_countdowns(entity_id);
      move_entity(entity_id);

      game_context.position[entity_id].current.x =
          spring_update(&game_context.position[entity_id].spring_x, game_context.position[entity_id].target.x);
      game_context.position[entity_id].current.y =
          spring_update(&game_context.position[entity_id].spring_y, game_context.position[entity_id].target.y);
    }
  }

  // Spring the selection box
  render_context.selection.current.x = spring_update(&render_context.selection.spring_x, render_context.selection.target.x);
  render_context.selection.current.y = spring_update(&render_context.selection.spring_y, render_context.selection.target.y);
}

void handle_input(void) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
      mouse_state.prev_state = mouse_state.state;
      mouse_state.state = event.button.state;
      mouse_state.button = event.button.button;
      mouse_state.clicks = event.button.clicks;
      if (mouse_state.prev_state != SDL_PRESSED) {
        // Set selection target to the current mouse position
        render_context.selection.target.x = mouse_state.position.x;
        render_context.selection.target.y = mouse_state.position.y;
        // Reset selection spring so it doesn't spring between the old selection and the new one
        render_context.selection.spring_x.current = render_context.selection.target.x;
        render_context.selection.spring_y.current = render_context.selection.target.y;
      }
    }
    if (event.type == SDL_MOUSEMOTION) {
      mouse_state.prev_state = mouse_state.state;
      mouse_state.prev_position.x = mouse_state.position.x;
      mouse_state.prev_position.y = mouse_state.position.y;
      mouse_state.position.x = (float)event.motion.x;
      mouse_state.position.y = (float)event.motion.y;
    }
    if (event.type == SDL_QUIT) {
      game_context.game_is_still_running = 0;
      return;
    }
    if (console_is_open()) {
      console_handle_input(&event);
      return;
    }
    if (game_context.in_pause_menu) {
      pause_menu_handle_input(&event);
      return;
    }
    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
          bool entity_had_selection = false;

          // 1. If anything is selected, then deselect it and break.
          reverse_loop(game_context.entity_count, entity_id) {
            if (game_context.selected[entity_id]) {
              entity_had_selection = true;
              game_context.selected[entity_id] = false;
            }
          }

          if (entity_had_selection) {
            game_context.single_entity_selected = false;
            break;
          }

          // 2. If nothing was deselected, then open the pause menu.
          game_context.in_pause_menu = true;
          pause_menu.current_screen = PAUSE_MENU_MAIN;

          if (physics_context.simulation_speed > 0) {
            physics_context.prev_simulation_speed = physics_context.simulation_speed;
            physics_context.simulation_speed = 0;
          }

          break;
        case SDLK_UP:
          // TODO: if paused, then up should return to pre-paused speed
          if (physics_context.prev_simulation_speed > 0) {
            physics_context.simulation_speed = physics_context.prev_simulation_speed;
            physics_context.prev_simulation_speed = 0;
          } else {
            physics_context.simulation_speed += 1;
            physics_context.simulation_speed = min(physics_context.simulation_speed, MAX_SIMULATION_SPEED_INDEX);
          }
          break;
        case SDLK_DOWN:
          // TODO: if paused, then do nothing?
          if (physics_context.prev_simulation_speed == 0) {
            physics_context.simulation_speed -= 1;
            physics_context.simulation_speed = max(physics_context.simulation_speed, 0);
          }
          break;
        case SDLK_SPACE:
          if (physics_context.prev_simulation_speed > 0) {
            physics_context.simulation_speed = physics_context.prev_simulation_speed;
            physics_context.prev_simulation_speed = 0;
          } else {
            physics_context.prev_simulation_speed = physics_context.simulation_speed;
            physics_context.simulation_speed = 0;
          }
          break;
        case SDLK_TAB: {
          console_open();
          break;
        }
        default:
          break;
      }
    }
    if (event.type == SDL_MOUSEWHEEL) {
      if (event.wheel.y > 0) {
        // zoom in
        render_context.camera.target_zoom = min(render_context.camera.target_zoom + 0.1f, 2.0f);
        audio_set_sound_volume(audio_context.sound_volume);
      } else if (event.wheel.y < 0) {
        // zoom out
        render_context.camera.target_zoom = max(render_context.camera.target_zoom - 0.1f, 0.1f);
        audio_set_sound_volume(audio_context.sound_volume);
      }
    }

    // Two loops needed so we can have a case where multiple entities can be hovered over, but only one can be selected
    reverse_loop(num_of_visible_entities, index) {
      game_context.hovered[visible_entities[index]] = is_entity_under_mouse(visible_entities[index]);
    }

    // TODO: This needs to change when we take spatial partitioning into account
    if (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_RELEASED) {
      bool any_entity_selected = false;
      int entity_id_to_click = -1;

      reverse_loop(num_of_visible_entities, index) {
        int entity_id = visible_entities[index];
        if (is_entity_under_mouse(entity_id)) {
          if (entity_id_to_click == -1) {
            entity_id_to_click = entity_id;
          } else {
            if (game_context.species[entity_id] < game_context.species[entity_id_to_click]) {
              entity_id_to_click = entity_id;
            }
          }
        }
      }
      if (entity_id_to_click > -1) {
        game_context.selected[entity_id_to_click] = !game_context.selected[entity_id_to_click];
        any_entity_selected = true;
        break;
      }

      game_context.single_entity_selected = any_entity_selected;
    }
  }
}

void move_camera(void) {
  Vec2 camera_spring_distance = {
      .x = fabsf(render_context.camera.target.x - render_context.camera.current.x),
      .y = fabsf(render_context.camera.target.y - render_context.camera.current.y),
  };

  int entity_to_follow = get_singly_selected_entity();

  if (!game_context.single_entity_selected && mouse_primary_pressed(mouse_state)) {
    select_entities_within_selection_rect();
  } else {
    if (entity_to_follow != INVALID_ENTITY) {
      FRect entity_render_rect = get_entity_render_rect(entity_to_follow);
      render_context.camera.target.x = entity_render_rect.left + ((entity_render_rect.right - entity_render_rect.left) / 2);
      render_context.camera.target.y = entity_render_rect.top + ((entity_render_rect.bottom - entity_render_rect.top) / 2);

      if (camera_spring_distance.x < 0.5f && camera_spring_distance.y < 0.5f) {
        render_context.camera.current.x = entity_render_rect.left + ((entity_render_rect.right - entity_render_rect.left) / 2);
        render_context.camera.current.y = entity_render_rect.top + ((entity_render_rect.bottom - entity_render_rect.top) / 2);
      }
    }
  }

  if (camera_spring_distance.x > 0.5f || camera_spring_distance.y > 0.5f) {
    // Spring the camera position
    render_context.camera.current.x = spring_update(&render_context.camera.pan_spring_x, render_context.camera.target.x);
    render_context.camera.current.y = spring_update(&render_context.camera.pan_spring_y, render_context.camera.target.y);
  }
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

void render(void) {
  gfx_clear_screen();

  draw_terrain();

  FRect camera_rect = get_camera_world_rect();

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

  loop(num_of_visible_entities, index) {
    render_entity(visible_entities[index]);
  }

  if (render_context.camera.zoom > 0.5f) {
    loop(num_of_visible_entities, index) {
      draw_name(visible_entities[index]);
    }
  }

  int single_entity = get_singly_selected_entity();
  if (single_entity != INVALID_ENTITY) {
    draw_entity_info(single_entity);
  }

  if (render_context.camera.zoom > 0.5f) {
    loop(num_of_visible_entities, index) {
      int entity_id = visible_entities[index];
      FRect entity_hit_box_rect = get_entity_hit_box_rect(entity_id);
      FRect entity_hit_box_screen_rect = frect_world_to_screen(entity_hit_box_rect);
      draw_health_bar(entity_id, entity_hit_box_screen_rect);
      if (game_context.selected[entity_id]) {
        draw_border(entity_hit_box_screen_rect, 5.0f, 4.0f);
      }
    }
  }

  if (mouse_primary_pressed(mouse_state)) {
    // Draw the selection box
    draw_selection_box();
  }

  render_debug_info();

  pause_menu_draw();

  console_draw();

  gfx_render_present();
}

void update_timer(Timer *timer, double frame_time) {
  timer->accumulated += frame_time;
  if (timer->accumulated >= timer->interval) {
    timer->accumulated -= timer->interval;
  }
}

int main(int argc, char *args[]) {
  srand(create_seed("I like calculating widths"));

  render_context.background_color = (SDL_Color){35, 127, 178, 255};
  render_context.camera = (Camera){
      .zoom = 1.0f,
      .target_zoom = 1.0f,
      .pan_spring_x =
          {
              .target = 1.0f,
              .current = 1.0f,
              .velocity = 0.0f,
              .acceleration = 2.0f,
              .friction = 0.05f,
          },
      .pan_spring_y =
          {
              .target = 1.0f,
              .current = 1.0f,
              .velocity = 0.0f,
              .acceleration = 2.0f,
              .friction = 0.05f,
          },
      .zoom_spring =
          {
              .target = 1.0f,
              .current = 1.0f,
              .velocity = 0.0f,
              .acceleration = 0.4f,
              .friction = 0.1f,
          },
  };
  render_context.selection =
      (Position){
          .spring_x =
              {
                  .target = 1.0f,
                  .current = 1.0f,
                  .velocity = 0.0f,
                  .acceleration = 0.5f,
                  .friction = 0.1f,
              },
          .spring_y =
              {
                  .target = 1.0f,
                  .current = 1.0f,
                  .velocity = 0.0f,
                  .acceleration = 0.5f,
                  .friction = 0.1f,
              },
      },
  console.y_spring = (Spring){
      .target = 0.0f,
      .current = 0.0f,
      .velocity = 0.1f,
      .acceleration = 0.5f,
      .friction = 0.1f,
  };

  int gfx_init_result = gfx_init();
  if (gfx_init_result == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  int audio_init_result = audio_init();
  if (audio_init_result == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  game_context.game_is_still_running = 1;
  physics_context = (PhysicsContext){.delta_time = 0.01, .simulation_speed = 3};
  render_context.timer[0] = (Timer){.interval = 100};  // Second timer
  render_context.timer[1] = (Timer){.interval = 60000};  // Minute timer

  gfx_load_textures();

  audio_load_sounds();

  load_fonts();

  create_entities();
  generate_grass_textures();

  u32 start_ticks = SDL_GetTicks();
  int frame_count = 0;

  // Here we keep the frame_time within a reasonable bound. If a frame_time exceeds 250ms, we "give up" and drop simulation frames
  // This is necessary as if our frame_time were to become too large, we would effectively lock ourselves in an update cycle
  // and the simulation would fall completely out of sync with the physics being rendered
  float max_frame_time_threshold = 0.25;
  double accumulator = 0.0;
  double current_time = SDL_GetTicks64() / 1000.0;

  while (game_context.game_is_still_running) {
    double new_time = SDL_GetTicks64() / 1000.0;
    double frame_time = new_time - current_time;

    update_timer(&render_context.timer[0], frame_time);

    frame_count++;
    if (SDL_GetTicks() - start_ticks >= 1000) {
      render_context.fps = (float)frame_count;
      frame_count = 0;
      start_ticks = SDL_GetTicks();
    }

    gfx_get_window_size(&render_context.window_w, &render_context.window_h);
    render_context.keyboard_state = SDL_GetKeyboardState(NULL);

    if (frame_time > max_frame_time_threshold) {
      frame_time = max_frame_time_threshold;
    }

    current_time = new_time;
    accumulator += frame_time;

    handle_input();

    while (accumulator >= physics_context.delta_time) {
      update();
      accumulator -= physics_context.delta_time;
    }

    physics_context.alpha = min(accumulator / physics_context.delta_time, 1.0);

    // Set the alpha to 1.0 so that rendering is consistent when the simulation speed is 0.
    if (physics_context.simulation_speed == 0) {
      physics_context.alpha = 1.0;
    }

    move_camera();

    render();
  }

  gfx_destroy();

  return EXIT_SUCCESS;
}
