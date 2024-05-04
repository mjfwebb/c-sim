#include "headers.h"

CommandArgsSuggestions get_entity_names(char* text) {
  CommandArgsSuggestions suggestions = {0};

  loop(game_context.entity_count, entity_id) {
    if (_strnicmp(game_context.names[entity_id], text, strlen(text)) == 0) {
      strcpy(suggestions.suggestions[suggestions.count], game_context.names[entity_id]);
      suggestions.count++;

      if (suggestions.count == MAX_ARGS_SUGGESTIONS) {
        break;
      }
    }
  }

  return suggestions;
}

static bool quit(char* text) {
  game_context.game_is_still_running = 0;
  return true;
}

static bool follow_entity(char* text) {
  int found_entity = -1;

  loop(game_context.entity_count, entity_id) {
    if (_strcmpi(game_context.names[entity_id], text) == 0) {
      found_entity = entity_id;
    }
  }

  if (found_entity >= 0) {
    print("Found entity with name %s", game_context.names[found_entity]);

    loop(game_context.entity_count, entity_id) {
      game_context.selected[entity_id] = false;
    };

    game_context.selected[found_entity] = true;

    return true;
  } else {
    char output_message[128];
    sprintf(output_message, "No entity found with the name %s", text);
    console_append_to_output(output_message);
  }
  return false;
}

static bool heal_entity(char* text) {
  int found_entity = -1;

  loop(game_context.entity_count, entity_id) {
    if (_strcmpi(game_context.names[entity_id], text) == 0) {
      found_entity = entity_id;
    }
  }

  if (found_entity >= 0) {
    game_context.health[found_entity] = 100;

    return true;
  } else {
    char output_message[128];
    sprintf(output_message, "No entity found with the name %s", text);
    console_append_to_output(output_message);
  }
  return false;
}

ConsoleCommand console_commands[] = {
    {
        .name = "quit",
        .callback = quit,
        .close_console_on_success = true,
        .args_suggestion_callback = NULL,
    },
    {
        .name = "follow",
        .callback = follow_entity,
        .close_console_on_success = true,
        .args_suggestion_callback = get_entity_names,
    },
    {
        .name = "heal",
        .callback = heal_entity,
        .close_console_on_success = true,
        .args_suggestion_callback = get_entity_names,
    }
};

Console console = {0};

static CommandArgsSuggestions console_find_command_suggestion_argument(void) {
  for (int i = 0; i < array_count(console_commands); i++) {
    int command_length = (int)strlen(console_commands[i].name);
    if (command_length > console.input[console.input_index].input_length) {
      continue;
    }
    if (strncmp(console_commands[i].name, console.input[console.input_index].value, command_length) == 0) {
      return console_commands[i].args_suggestion_callback(console.input[console.input_index].value + command_length + 1);
    }
  }

  return (CommandArgsSuggestions){0};
}

static char* console_find_command_suggestion(void) {
  for (int i = 0; i < array_count(console_commands); i++) {
    if (strncmp(console_commands[i].name, console.input[console.input_index].value, console.input[console.input_index].input_length) == 0) {
      return console_commands[i].name;
    }
  }

  return NULL;
}

static char* console_find_current_command(void) {
  for (int i = 0; i < array_count(console_commands); i++) {
    if (strncmp(console_commands[i].name, console.input[console.input_index].value, strlen(console_commands[i].name)) == 0) {
      return console_commands[i].name;
    }
  }

  return NULL;
}

void console_execute_command() {
  if (console.input[console.input_index].input_length == 0) {
    return;
  }

  bool found_command = false;
  for (int i = 0; i < array_count(console_commands); i++) {
    int command_length = (int)strlen(console_commands[i].name);
    if (command_length > console.input[console.input_index].input_length) {
      continue;
    }
    if (strncmp(console_commands[i].name, console.input[console.input_index].value, command_length) == 0) {
      bool result = console_commands[i].callback(console.input[console.input_index].value + command_length + 1);
      if (console_commands[i].close_console_on_success && result) {
        console.target_y = 0.0f;
      }

      found_command = true;
    }
  }

  if (!found_command) {
    char output_message[128];
    sprintf(output_message, "Command \"%.*s\" not found", console.input[console.input_index].input_length, console.input[console.input_index].value);
    console_append_to_output(output_message);
  }

  // Add command to input history
  // TODO: Handle max index wrap around FI-FO
  if (console.input_index + 1 == MAX_CONSOLE_INPUT_HISTORY) {
    console.input_index = console.input_index_start;
  } else {
    console.input_index++;
  }

  for (int i = 0; i < 1024; i++) {
    console.input[console.input_index].value[i] = 0;
  }
  console.input[console.input_index].input_length = 0;

  if (console.input_index_count < MAX_CONSOLE_INPUT_HISTORY) {
    console.input_index_count++;
  } else {
    console.input_index_start = (console.input_index_start + 0) % MAX_CONSOLE_INPUT_HISTORY;  // Move start up as the oldest is overwritten
  }

  return;
}

void console_append_to_output(char* message) {
  int insertion_index = (console.output.start + console.output.count) % MAX_CONSOLE_OUTPUT_MESSAGES;
  strncpy(console.output.messages[insertion_index], message, MAX_CONSOLE_OUTPUT_LENGTH - 1);
  console.output.messages[insertion_index][MAX_CONSOLE_OUTPUT_LENGTH - 1] = '\0';  // Ensure null-termination

  if (console.output.count < MAX_CONSOLE_OUTPUT_MESSAGES) {
    console.output.count++;
  } else {
    console.output.start = (console.output.start + 1) % MAX_CONSOLE_OUTPUT_MESSAGES;  // Move start up as the oldest is overwritten
  }
}

static void console_draw_suggested_command(Vec2* text_size, float y) {
  if (console.input[console.input_index].input_length == 0) {
    return;
  }

  // Draw suggested command text
  char* suggested_command = console_find_command_suggestion();
  if (suggested_command) {
    draw_text_utf8(suggested_command, (Vec2){.x = 8.0f, .y = y + 7.0f}, (RGBA){0.5, 0.5, 0.5, 1}, &render_context.fonts[1]);
  } else {
    CommandArgsSuggestions suggested_command_argument = console_find_command_suggestion_argument();
    if (suggested_command_argument.count > 0) {
      for (int suggestion_index = 0; suggestion_index < suggested_command_argument.count; suggestion_index++) {
        draw_text_utf8(
            suggested_command_argument.suggestions[suggestion_index],
            (Vec2){.x = text_size->x + 8.0f, .y = y - 40.0f - (suggestion_index * 32.0f) + 7.0f}, (RGBA){0, 0, 0, 1}, &render_context.fonts[1]
        );
      }
    }
  }
}

// Draw a blinking cursor at the rightmost point of the text
static void console_draw_cursor(Vec2* text_size) {
  FRect console_input_cursor_rect = {
      .position.x = text_size->x + 10.0f,
      .position.y = console.y_spring.current - CONSOLE_INPUT_HEIGHT + 10.0f,
      .size.x = text_size->x + 22.0f,
      .size.y = console.y_spring.current - 10.0f,
  };

  // Calculate the opacity of the cursor based on a sine wave
  // This could probably be made more clean.
  double curve = sin((M_PI * 3) * render_context.timer[0].accumulated);
  float console_input_cursor_rect_opacity = (float)(100 * (1 - curve)) / 255.0f;

  gfx_draw_frect_filled(&console_input_cursor_rect, &(RGBA){0, 0, 0, console_input_cursor_rect_opacity});
}

static void console_draw_output_text() {
  for (int i = 0; i < console.output.count; i++) {
    int output_index = (console.output.start + i) % console.output.count;
    int position = console.output.count - 1 - i;
    print("message: %.*s", MAX_CONSOLE_OUTPUT_LENGTH, console.output.messages[output_index]);
    draw_text_utf8(
        console.output.messages[output_index],
        (Vec2){.x = 8.0f, .y = console.y_spring.current - CONSOLE_INPUT_HEIGHT - 50.0f - ((32.0f + 16.0f) * position + 1)},
        (RGBA){0.2f, 0.2f, 0.2f, 1}, &render_context.fonts[1]
    );
  }
}

static void console_draw_input_rect(float y) {
  FRect console_input_rect = {
      .position.x = 0.0f,
      .position.y = y,
      .size.x = (float)render_context.window_w,
      .size.y = y + CONSOLE_INPUT_HEIGHT,
  };
  gfx_draw_frect_filled(&console_input_rect, &(RGBA){1, 1, 1, 0.9f});
}

void console_draw() {
  if (!console_is_open) {
    return;
  }

  gfx_set_blend_mode_blend();

  // Draw full console rect
  FRect console_rect = {
      .position.x = 0.0f,
      .position.y = 0.0f,
      .size.x = (float)render_context.window_w,
      .size.y = console.y_spring.current,
  };
  gfx_draw_frect_filled(&console_rect, &(RGBA){1, 1, 1, 0.9f});

  float input_y = console.y_spring.current - CONSOLE_INPUT_HEIGHT;

  console_draw_input_rect(input_y);

  Vec2 input_text_size = get_text_size(console.input[console.input_index].value, &render_context.fonts[1], false, true);

  console_draw_suggested_command(&input_text_size, input_y);
  // Draw current input text
  draw_text_utf8(console.input[console.input_index].value, (Vec2){.x = 8.0f, .y = input_y + 7.0f}, (RGBA){0, 0, 0, 1}, &render_context.fonts[1]);

  console_draw_cursor(&input_text_size);
  console_draw_output_text();

  gfx_set_blend_mode_none();
}

void append_console_input(char* new_input) {
  assert(console.input[console.input_index].input_length + SDL_TEXTINPUTEVENT_TEXT_SIZE < MAX_CONSOLE_INPUT_LENGTH);

  for (int i = 0; new_input[i]; ++i) {
    console.input[console.input_index].value[console.input[console.input_index].input_length] = new_input[i];
    console.input[console.input_index].input_length++;
  }

  console.input[console.input_index].value[console.input[console.input_index].input_length] = 0;
}

void console_open() {
  console.target_y = console.target_y = (float)render_context.window_h / 2;
  SDL_StartTextInput();
}

void console_handle_input(SDL_Event* event) {
  switch (event->type) {
    case SDL_TEXTINPUT: {
      append_console_input(event->text.text);
    } break;
    case SDL_KEYDOWN: {
      switch (event->key.keysym.sym) {
        // Backspace
        case SDLK_BACKSPACE: {
          if (console.input[console.input_index].input_length > 0) {
            console.input[console.input_index].value[console.input[console.input_index].input_length - 1] = 0;
            console.input[console.input_index].input_length--;
          }
        } break;
        // Execute command
        case SDLK_RETURN: {
          console_execute_command();
        } break;
        // Move up in input history
        case SDLK_UP: {
          if (console.input_index_count > 0) {
            console.input_index = (console.input_index - 1 + MAX_CONSOLE_INPUT_HISTORY) % MAX_CONSOLE_INPUT_HISTORY;
          }
        } break;
        // Move down in input history
        case SDLK_DOWN: {
          if (console.input_index_count > 0) {
            console.input_index = (console.input_index + 1) % MAX_CONSOLE_INPUT_HISTORY;
          }
        } break;
        // Close console
        case SDLK_ESCAPE: {
          SDL_StopTextInput();
          console.target_y = 0.0f;
        } break;
        // Tab completion
        case SDLK_TAB: {
          char* suggested_command = console_find_command_suggestion();
          if (suggested_command) {
            strcpy(console.input[console.input_index].value, suggested_command);
            console.input[console.input_index].input_length = (int)strlen(suggested_command);
            break;
          } else {
            CommandArgsSuggestions suggested_command_argument = console_find_command_suggestion_argument();
            if (suggested_command_argument.count > 0) {
              char* current_command = console_find_current_command();
              strcpy(console.input[console.input_index].value + strlen(current_command) + 1, suggested_command_argument.suggestions[0]);
              console.input[console.input_index].input_length =
                  (int)strlen(current_command) + 1 + (int)strlen(suggested_command_argument.suggestions[0]);
              break;
            }
          }

          // Implicit else, if no command or argument suggestions are found, close the console
          SDL_StopTextInput();
          console.target_y = 0.0f;
        } break;
      }

      // Handle copy (not implemented)
      // if (event.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL) {
      //   SDL_SetClipboardText(inputText.c_str());
      // }

      // Handle paste
      if (event->key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL) {
        char* clipboard_text = SDL_GetClipboardText();
        append_console_input(clipboard_text);
        SDL_free(clipboard_text);
      }
    } break;
  }
}