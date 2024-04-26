#include "headers.h"

typedef bool (*CommandCallback)(char*);

typedef struct {
  char* name;
  CommandCallback callback;
  bool close_console_on_success;
} ConsoleCommand;

typedef struct {
  float y;
  float target_y;
  Spring y_spring;
  char input[MAX_CONSOLE_INPUT_LENGTH];
  int input_length;
} Console;

bool quit(char* text) {
  game_context.game_is_still_running = 0;
  return true;
}

bool follow_entity(char* text) {
  // we need to deselect all entities
  int found_entity = -1;

  entity_loop(entity_i) {
    if (_strcmpi(game_context.names[entity_i], text) == 0) {
      found_entity = entity_i;
    }
  }
  // we need find the entity with name that is the input and select it
  // if no entity is found then we report an error in the console output

  if (found_entity >= 0) {
    print("Found entity with name %s", game_context.names[found_entity]);

    entity_loop(entity_i) {
      game_context.selected[entity_i] = false;
    };

    game_context.selected[found_entity] = true;

    return true;
  } else {
    print("No entity found with the name %s", text);
  }
  return false;
}

ConsoleCommand console_commands[] = {
    {
        .name = "quit",
        .callback = quit,
        .close_console_on_success = true,
    },
    {
        .name = "follow",
        .callback = follow_entity,
        .close_console_on_success = true,
    }
};

Console console = {0};

void handle_console_input() {
  if (console.input_length == 0) {
    return;
  }

  bool found_command = false;
  for (int i = 0; i < array_count(console_commands); i++) {
    int command_length = (int)strlen(console_commands[i].name);
    if (command_length > console.input_length) {
      continue;
    }
    if (strncmp(console_commands[i].name, console.input, command_length) == 0) {
      bool result = console_commands[i].callback(console.input + command_length + 1);
      if (console_commands[i].close_console_on_success && result) {
        console.target_y = 0.0f;
      }

      found_command = true;
    }
  }

  if (!found_command) {
    print("no command found for %.*s", console.input_length, console.input);
  }

  console.input[0] = 0;
  console.input_length = 0;
  return;
}

void draw_console() {
  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_BLEND);

  SDL_FRect console_rect = {
      .h = console.y_spring.current,
      .w = (float)render_context.window_w,
      .x = 0.0f,
      .y = 0.0f,
  };

  SDL_SetRenderDrawColor(render_context.renderer, 255, 255, 255, 200);
  SDL_RenderFillRectF(render_context.renderer, &console_rect);

  float input_y = console.y_spring.current - 64.0f;
  SDL_FRect console_input_rect = {
      .h = 64.0f,
      .w = (float)render_context.window_w,
      .x = 0.0f,
      .y = input_y,
  };

  SDL_SetRenderDrawColor(render_context.renderer, 255, 255, 255, 200);
  SDL_RenderFillRectF(render_context.renderer, &console_input_rect);

  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_NONE);
  draw_text_utf8(console.input, (FPoint){.x = 8.0f, .y = input_y + 7.0f}, (RGBA){0, 0, 0, 1}, &render_context.fonts[1]);
}

void append_console_input(char* new_input) {
  assert(console.input_length + SDL_TEXTINPUTEVENT_TEXT_SIZE < MAX_CONSOLE_INPUT_LENGTH);

  for (int i = 0; new_input[i]; ++i) {
    console.input[console.input_length] = new_input[i];
    console.input_length++;
  }

  console.input[console.input_length] = 0;
}