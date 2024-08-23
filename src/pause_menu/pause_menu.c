#include "headers.h"

#include "pause_menu_video.c"
// #include "pause_menu_audio.c"
#include "pause_menu_controls.c"

int element_id_start = 9999;
int number_of_elements = 0;

float element_height = 60.0f;
float element_gap = 30.0f;

PauseMenu pause_menu = {.current_screen = PAUSE_MENU_MAIN};

FRect get_container(float* element_sizes, int element_count) {
  float container_height = 0;
  float container_width = 300.0f;
  for (int i = 0; i < element_count; i++) {
    container_height += (element_sizes[i] + element_gap);
  }
  container_height -= element_gap;

  float middle_of_screen_container_x = ((float)render_context.window_w / 2) - (container_width / 2);
  float y = ((float)render_context.window_h / 2) - (container_height / 2);

  FRect container = {
      .left = middle_of_screen_container_x,
      .top = y,
      .right = middle_of_screen_container_x + container_width,
      .bottom = y + container_height,
  };

  return container;
}

float current_element_position_y(float* element_heights, int current_element) {
  float y_position = 0.0f;

  for (int i = 0; i < current_element; i++) {
    y_position += element_heights[i] + element_gap;
  }

  return y_position;
}

void toggle_pause_menu(void) {
  game_context.in_pause_menu = !game_context.in_pause_menu;

  if (game_context.in_pause_menu && physics_context.simulation_speed > 0) {
    physics_context.prev_simulation_speed = physics_context.simulation_speed;
    physics_context.simulation_speed = 0;
  }

  if (!game_context.in_pause_menu && physics_context.prev_simulation_speed > 0) {
    physics_context.simulation_speed = physics_context.prev_simulation_speed;
    physics_context.prev_simulation_speed = 0;
  }
}

bool pause_menu_title(PauseMenuTitle title) {
  Vec2 text_size = get_text_size(title.text, &render_context.fonts[2], false, false);
  float container_width = title.rect.right - title.rect.left;
  Vec2 text_centered = {
      .x = title.rect.left + (container_width / 2) - (text_size.x / 2),
      .y = title.rect.top,
  };

  draw_text_utf8(title.text, (Vec2){.x = text_centered.x, .y = text_centered.y}, (RGBA){1, 1, 1, 1}, &render_context.fonts[2]);

  return true;
}

bool draw_pause_menu_dropdown(PauseMenuDropdown dropdown) {
  // Then draw text on top
  Vec2 dropdown_dimensions = {
      .x = dropdown.rect.right - dropdown.rect.left,
      .y = dropdown.rect.bottom - dropdown.rect.top,
  };

  bool dropdown_is_hovered = pause_menu.input_mode == PAUSE_MENU_MOUSE_MODE && gfx_frect_contains_point(&dropdown.rect, &mouse_state.position);
  bool dropdown_is_focused = pause_menu.input_mode == PAUSE_MENU_KEYBOARD_MODE && pause_menu.focused_id == dropdown.id;
  if (dropdown_is_hovered) {
    pause_menu.focused_id = dropdown.id;
  }

  bool dropdown_is_hot = dropdown_is_hovered || dropdown_is_focused;

  // double current_time = SDL_GetTicks64() / 500.0;

  if (pause_menu.hovered_id == dropdown.id) {
    if (!dropdown_is_hovered) {
      pause_menu.hovered_id = 0;
      pause_menu.hover_start_time = 0.0;
    }
  } else {
    if (dropdown_is_hovered) {
      pause_menu.hovered_id = dropdown.id;
      pause_menu.hover_start_time = SDL_GetTicks64() / 500.0;
    }
  }

  // float hovered_time = (float)current_time - (float)pause_menu.hover_start_time;

  gfx_draw_frect_filled(&dropdown.rect, &(RGBA){1, 1, 1, 1});

  Vec2 text_size = get_text_size(dropdown.text, &render_context.fonts[1], false, false);
  Vec2 text_centered = {
      .x = (dropdown_dimensions.x / 2) - (text_size.x / 2),
      .y = (dropdown_dimensions.y / 2) - (text_size.y / 2),
  };
  draw_text_utf8(
      dropdown.text, (Vec2){.x = dropdown.rect.left + 30.0f, .y = dropdown.rect.top + text_centered.y},
      dropdown_is_hot ? (RGBA){0.5f, 0.5f, 0.8f, 1} : (RGBA){0, 0, 0, 1}, &render_context.fonts[1]
  );

  if (dropdown_is_hovered && mouse_primary_down(mouse_state)) {
    return true;
  }

  if (dropdown_is_hot && render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_RETURN)]) {
    return true;
  }

  return false;
}

bool pause_menu_button(PauseMenuButton button) {
  // Then draw text on top
  Vec2 button_dimensions = {
      .x = button.rect.right - button.rect.left,
      .y = button.rect.bottom - button.rect.top,
  };

  bool button_is_hovered = pause_menu.input_mode == PAUSE_MENU_MOUSE_MODE && gfx_frect_contains_point(&button.rect, &mouse_state.position);
  bool button_is_focused = pause_menu.input_mode == PAUSE_MENU_KEYBOARD_MODE && pause_menu.focused_id == button.id;
  if (button_is_hovered) {
    pause_menu.focused_id = button.id;
  }

  bool button_is_hot = button_is_hovered || button_is_focused;

  double current_time = SDL_GetTicks64() / 500.0;

  if (pause_menu.hovered_id == button.id) {
    if (!button_is_hovered) {
      pause_menu.hovered_id = 0;
      pause_menu.hover_start_time = 0.0;
    }
  } else {
    if (button_is_hovered) {
      pause_menu.hovered_id = button.id;
      pause_menu.hover_start_time = SDL_GetTicks64() / 500.0;
    }
  }

  float hovered_time = (float)current_time - (float)pause_menu.hover_start_time;

  FRect button_rect = button.rect;
  if (button_is_hot) {
    button_rect.left = max(button.rect.left - 6.0f, button_rect.left - hovered_time * 35.0f);
    button_rect.top = max(button.rect.top - 6.0f, button_rect.top - hovered_time * 35.0f);
    button_rect.right = min(button.rect.right + 6.0f, button_rect.right + hovered_time * 35.0f);
    button_rect.bottom = min(button.rect.bottom + 6.0f, button_rect.bottom + hovered_time * 35.0f);
  }

  gfx_draw_frect_filled(&button_rect, &(RGBA){1, 1, 1, 1});

  Vec2 text_size = get_text_size(button.text, &render_context.fonts[1], false, false);
  Vec2 text_centered = {
      .x = (button_dimensions.x / 2) - (text_size.x / 2),
      .y = (button_dimensions.y / 2) - (text_size.y / 2),
  };
  draw_text_utf8(
      button.text, (Vec2){.x = button.rect.left + text_centered.x, .y = button.rect.top + text_centered.y},
      button_is_hot ? (RGBA){0.5f, 0.5f, 0.8f, 1} : (RGBA){0, 0, 0, 1}, &render_context.fonts[1]
  );

  if (button_is_hovered && mouse_primary_down(mouse_state)) {
    return true;
  }

  if (button_is_hot && render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_RETURN)]) {
    return true;
  }

  return false;
}

void pause_menu_draw_main(void) {
  number_of_elements = 5;
  int element_count = 0;

  float element_heights[] = {element_height, element_height, element_height, element_height, element_height};
  FRect container = get_container(element_heights, array_count(element_heights));

  if (pause_menu_button((PauseMenuButton){
          .id = element_id_start + element_count,
          .rect = {.left = container.left, .top = container.top, .right = container.right, .bottom = container.top + element_height},
          .text = "Continue",
      })) {
    // Then we have clicked continue
    toggle_pause_menu();
  }

  element_count++;
  {
    float current_element_y = current_element_position_y(element_heights, element_count);
    if (pause_menu_button((PauseMenuButton){
            .id = element_id_start + element_count,
            .rect =
                {.left = container.left,
                 .top = container.top + current_element_y,
                 .right = container.right,
                 .bottom = container.top + current_element_y + element_height},
            .text = "Video",
        })) {
      pause_menu.current_screen = PAUSE_MENU_VIDEO;
    }
  }

  element_count++;
  {
    float current_element_y = current_element_position_y(element_heights, element_count);
    if (pause_menu_button((PauseMenuButton){
            .id = element_id_start + element_count,
            .rect =
                {.left = container.left,
                 .top = container.top + current_element_y,
                 .right = container.right,
                 .bottom = container.top + current_element_y + element_height},
            .text = "Audio",
        })) {
      // Then we have clicked continue
      print("TODO: implement audio screen");
    }
  }

  element_count++;
  {
    float current_element_y = current_element_position_y(element_heights, element_count);
    if (pause_menu_button((PauseMenuButton){
            .id = element_id_start + element_count,
            .rect =
                {.left = container.left,
                 .top = container.top + current_element_y,
                 .right = container.right,
                 .bottom = container.top + current_element_y + element_height},
            .text = "Controls",
        })) {
      // Then we have clicked continue
      pause_menu.current_screen = PAUSE_MENU_CONTROLS;
    }
  }

  element_count++;
  {
    float current_element_y = current_element_position_y(element_heights, element_count);
    if (pause_menu_button((PauseMenuButton){
            .id = element_id_start + element_count,
            .rect =
                {.left = container.left,
                 .top = container.top + current_element_y,
                 .right = container.right,
                 .bottom = container.top + current_element_y + element_height},
            .text = "Quit",
        })) {
      // Then we have clicked continue
      game_context.game_is_still_running = 0;
    }
  }
}

void pause_menu_draw(void) {
  if (!game_context.in_pause_menu) {
    return;
  }

  gfx_set_blend_mode_blend();

  // Draw full pause_menu rect
  FRect pause_menu_rect = {
      .left = 0.0f,
      .top = 0.0f,
      .right = (float)render_context.window_w,
      .bottom = (float)render_context.window_h,
  };

  gfx_draw_frect_filled(&pause_menu_rect, &(RGBA){0, 0, 0, 0.9f});

  switch (pause_menu.current_screen) {
    case PAUSE_MENU_MAIN:
      pause_menu_draw_main();
      break;
    case PAUSE_MENU_VIDEO:
      pause_menu_draw_video(number_of_elements, element_id_start, element_height);
      break;
    case PAUSE_MENU_AUDIO:
      printf("TODO: add audio pause menu\n");
      break;
    case PAUSE_MENU_CONTROLS:
      pause_menu_draw_controls(number_of_elements, element_id_start, element_height);
      break;
    default:
      pause_menu_draw_main();
      break;
  }

  gfx_set_blend_mode_none();
}

void pause_menu_handle_input(SDL_Event* event) {
  if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP || event->type == SDL_MOUSEMOTION) {
    pause_menu.input_mode = PAUSE_MENU_MOUSE_MODE;
  }

  if (event->type == SDL_KEYDOWN) {
    pause_menu.input_mode = PAUSE_MENU_KEYBOARD_MODE;
    switch (event->key.keysym.sym) {
      case SDLK_ESCAPE:
        toggle_pause_menu();
        break;
      case SDLK_UP:
        pause_menu.hover_start_time = SDL_GetTicks64() / 500.0;
        if (pause_menu.focused_id > element_id_start) {
          pause_menu.focused_id--;
          break;
        }
        pause_menu.focused_id = element_id_start + number_of_elements - 1;
        break;
      case SDLK_DOWN:
        pause_menu.hover_start_time = SDL_GetTicks64() / 500.0;
        if (pause_menu.focused_id > element_id_start + number_of_elements - 1) {
          pause_menu.focused_id = element_id_start;
          break;
        }
        if (pause_menu.focused_id >= element_id_start) {
          pause_menu.focused_id++;
          break;
        }
        pause_menu.focused_id = element_id_start;
        break;
      case SDLK_TAB: {
        console_open();
        break;
      }
      default:
        break;
    }
  }
}