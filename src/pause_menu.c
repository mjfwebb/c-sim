#include "headers.h"

typedef enum {
  PAUSE_MENU_MAIN,
  PAUSE_MENU_VIDEO,
  PAUSE_MENU_AUDIO,
} PauseMenuScreen;

typedef enum {
  PAUSE_MENU_MOUSE_MODE,
  PAUSE_MENU_KEYBOARD_MODE,
} PauseMenuInputMode;

typedef struct {
  int id;
  FRect rect;
  char text[32];
} PauseMenuButton;

typedef struct {
  int hovered_id;
  int focused_id;
  double hover_start_time;
  PauseMenuInputMode input_mode;
  PauseMenuScreen current_screen;
} PauseMenu;

int button_id_start = 9999;
int number_of_buttons = 5;

PauseMenu pause_menu = {.current_screen = PAUSE_MENU_MAIN};

void toggle_pause_menu() {
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

bool pause_menu_button(PauseMenuButton button) {
  // Then draw text on top
  Vec2 button_dimensions = {
      .x = button.rect.size.x - button.rect.position.x,
      .y = button.rect.size.y - button.rect.position.y,
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
    button_rect.position.x = max(button.rect.position.x - 6.0f, button_rect.position.x - hovered_time * 35.0f);
    button_rect.position.y = max(button.rect.position.y - 6.0f, button_rect.position.y - hovered_time * 35.0f);
    button_rect.size.x = min(button.rect.size.x + 6.0f, button_rect.size.x + hovered_time * 35.0f);
    button_rect.size.y = min(button.rect.size.y + 6.0f, button_rect.size.y + hovered_time * 35.0f);
  }

  gfx_draw_frect_filled(&button_rect, &(RGBA){1, 1, 1, 1});

  Vec2 text_size = get_text_size(button.text, &render_context.fonts[1], false, false);
  Vec2 text_centered = {
      .x = (button_dimensions.x / 2) - (text_size.x / 2),
      .y = (button_dimensions.y / 2) - (text_size.y / 2),
  };
  draw_text_utf8(
      button.text, (Vec2){.x = button.rect.position.x + text_centered.x, .y = button.rect.position.y + text_centered.y},
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

void pause_menu_draw_main() {
  float button_height = 60.0f;
  float button_width = 300.0f;
  float button_gap = 30.0f;

  int button_num = 0;
  float button_container_height = (number_of_buttons * (button_height + button_gap)) - button_gap;
  float middle_of_screen_button_x = ((float)render_context.window_w / 2) - 150.0f;
  Vec2 button_container_position = {
      .x = middle_of_screen_button_x,
      .y = ((float)render_context.window_h / 2) - (button_container_height / 2),
  };

  if (pause_menu_button((PauseMenuButton){
          .id = button_id_start + button_num,
          .rect =
              {.position.x = button_container_position.x,
               .position.y = button_container_position.y,
               .size.x = button_container_position.x + button_width,
               .size.y = button_container_position.y + button_height},
          .text = "Continue",
      })) {
    // Then we have clicked continue
    toggle_pause_menu();
  }

  button_num++;
  if (pause_menu_button((PauseMenuButton){
          .id = button_id_start + button_num,
          .rect =
              {.position.x = button_container_position.x,
               .position.y = button_container_position.y + (button_num * (button_height + button_gap)),
               .size.x = button_container_position.x + button_width,
               .size.y = button_container_position.y + (button_num * (button_height + button_gap)) + button_height},
          .text = "Video",
      })) {
    pause_menu.current_screen = PAUSE_MENU_VIDEO;
  }

  button_num++;
  if (pause_menu_button((PauseMenuButton){
          .id = button_id_start + button_num,
          .rect =
              {.position.x = button_container_position.x,
               .position.y = button_container_position.y + (button_num * (button_height + button_gap)),
               .size.x = button_container_position.x + button_width,
               .size.y = button_container_position.y + (button_num * (button_height + button_gap)) + button_height},
          .text = "Audio",
      })) {
    // Then we have clicked continue
    game_context.game_is_still_running = 0;
  }

  button_num++;
  if (pause_menu_button((PauseMenuButton){
          .id = button_id_start + button_num,
          .rect =
              {.position.x = button_container_position.x,
               .position.y = button_container_position.y + (button_num * (button_height + button_gap)),
               .size.x = button_container_position.x + button_width,
               .size.y = button_container_position.y + (button_num * (button_height + button_gap)) + button_height},
          .text = "Controls",
      })) {
    // Then we have clicked continue
    game_context.game_is_still_running = 0;
  }

  button_num++;
  if (pause_menu_button((PauseMenuButton){
          .id = button_id_start + button_num,
          .rect =
              {.position.x = button_container_position.x,
               .position.y = button_container_position.y + (button_num * (button_height + button_gap)),
               .size.x = button_container_position.x + button_width,
               .size.y = button_container_position.y + (button_num * (button_height + button_gap)) + button_height},
          .text = "Quit",
      })) {
    // Then we have clicked continue
    game_context.game_is_still_running = 0;
  }
}
void pause_menu_draw_video() {
  Vec2 middle_of_screen = {
      .x = ((float)render_context.window_w / 2),
      .y = ((float)render_context.window_h / 2),
  };

  Vec2 text_size = get_text_size("Video", &render_context.fonts[2], false, false);
  Vec2 text_centered = {
      .x = middle_of_screen.x - (text_size.x / 2),
      .y = middle_of_screen.y - (text_size.y / 2),
  };

  draw_text_utf8("Video", (Vec2){.x = text_centered.x, .y = text_centered.y}, (RGBA){1, 1, 1, 1}, &render_context.fonts[2]);
}

void pause_menu_draw() {
  if (!game_context.in_pause_menu) {
    return;
  }

  gfx_set_blend_mode_blend();

  // Draw full pause_menu rect
  FRect pause_menu_rect = {
      .position.x = 0.0f,
      .position.y = 0.0f,
      .size.x = (float)render_context.window_w,
      .size.y = (float)render_context.window_h,
  };

  gfx_draw_frect_filled(&pause_menu_rect, &(RGBA){0, 0, 0, 0.9f});

  switch (pause_menu.current_screen) {
    case PAUSE_MENU_MAIN:
      pause_menu_draw_main();
      break;
    case PAUSE_MENU_VIDEO:
      pause_menu_draw_video();
      break;
    // case PAUSE_MENU_AUDIO:
    //   pause_menu_draw_audio();
    //   break;
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
        if (pause_menu.focused_id > button_id_start) {
          pause_menu.focused_id--;
          break;
        }
        pause_menu.focused_id = button_id_start + number_of_buttons - 1;
        break;
      case SDLK_DOWN:
        pause_menu.hover_start_time = SDL_GetTicks64() / 500.0;
        if (pause_menu.focused_id > button_id_start + number_of_buttons - 1) {
          pause_menu.focused_id = button_id_start;
          break;
        }
        if (pause_menu.focused_id >= button_id_start) {
          pause_menu.focused_id++;
          break;
        }
        pause_menu.focused_id = button_id_start;
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