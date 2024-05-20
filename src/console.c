#include "headers.h"

Console console = {0};

static bool display_command_names(char* _unused);

bool part_in_range(ConsoleCommandPart arg, int index) {
  return index >= arg.start && index <= arg.end;
}

int part_index(ConsoleCommandParts args, int index) {
  for (int i = 0; i < args.length; i++) {
    if (part_in_range(args.part[i], index)) {
      return i;
    }
  }
  return args.length - 1;
}

ConsoleCommandParts parse_command(char* source) {
  ConsoleCommandParts console_command_parts = {0};

  if (source == NULL) {
    return console_command_parts;
  }

  int current_command_part = 0;

  console_command_parts.part[current_command_part].start = 0;
  console_command_parts.part[current_command_part].end = 0;

  int current_char = 0;
  int current_word_char = 0;
  while (true) {
    if (source[current_char] == ' ' || source[current_char] == '\0') {
      console_command_parts.part[current_command_part].end = current_char;
      console_command_parts.part[current_command_part].value[current_word_char] = 0;
      current_command_part++;
      current_word_char = 0;

      if (source[current_char] == '\0') {
        break;
      }

      console_command_parts.part[current_command_part].start = current_char;
    } else {
      console_command_parts.part[current_command_part].value[current_word_char] = source[current_char];
      current_word_char++;
    }
    current_char++;
  }

  console_command_parts.length = current_command_part;

  return console_command_parts;
}

// TODO: This should be in a geometry/math lib thing?
float vector_length(Vec2 v) {
  return sqrtf((v.x * v.x) + (v.y * v.y));
}

// TODO: This should be in a geometry/math lib thing?
float distance(Vec2 a, Vec2 b) {
  Vec2 difference = {
      .x = b.x - a.x,
      .y = b.y - a.y,
  };

  return vector_length(difference);
}

CommandArgsSuggestions get_entity_names(char* text) {
  CommandArgsSuggestions suggestions = {0};

  ConsoleCommandParts console_command_parts = parse_command(console.input[console.input_index].value);

  int current_part_index =
      part_index(console_command_parts, console.input[console.input_index].input_length);  // TODO: Implement cursor index and use it here

  if (console_command_parts.part[current_part_index].value) {
    loop(game_context.entity_count, entity_id) {
      if (_strnicmp(
              game_context.names[entity_id], console_command_parts.part[current_part_index].value,
              strlen(console_command_parts.part[current_part_index].value)
          ) == 0) {
        strcpy(suggestions.suggestions[suggestions.count], game_context.names[entity_id]);
        suggestions.count++;

        if (suggestions.count == MAX_ARGS_SUGGESTIONS) {
          break;
        }
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

static bool calculate_distance_between_entities(char* arguments_text) {
  ConsoleCommandParts console_command_parts = parse_command(console.input[console.input_index].value);

  if (strcmpi(console_command_parts.part[1].value, console_command_parts.part[2].value) == 0) {
    console_append_to_output("They're the same name you idiot");
    return false;
  }

  int found_entity_a = -1;
  int found_entity_b = -1;
  loop(game_context.entity_count, entity_id) {
    if (found_entity_a == -1 && _strcmpi(game_context.names[entity_id], console_command_parts.part[1].value) == 0) {
      found_entity_a = entity_id;
    }
    if (found_entity_b == -1 && _strcmpi(game_context.names[entity_id], console_command_parts.part[2].value) == 0) {
      found_entity_b = entity_id;
    }
  }

  char output_message[128];
  if (found_entity_a < 0) {
    sprintf(output_message, "No entity found with the name %s", console_command_parts.part[1].value);
    console_append_to_output(output_message);
    return false;
  }

  if (found_entity_b < 0) {
    sprintf(output_message, "No entity found with the name %s", console_command_parts.part[2].value);
    console_append_to_output(output_message);
    return false;
  }

  Vec2 entity_a_position = game_context.positions[found_entity_a].current_position;
  Vec2 entity_b_position = game_context.positions[found_entity_b].current_position;

  sprintf(
      output_message, "The distance between %s and %s, is %.2f", console_command_parts.part[1].value, console_command_parts.part[2].value,
      distance(entity_a_position, entity_b_position)
  );
  console_append_to_output(output_message);
  return true;
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
        .args[0].suggestion_callback = get_entity_names,
    },
    {
        .name = "heal",
        .callback = heal_entity,
        .close_console_on_success = true,
        .args[0].suggestion_callback = get_entity_names,
    },
    {
        .name = "distance",
        .callback = calculate_distance_between_entities,
        .close_console_on_success = false,
        .args[0].suggestion_callback = get_entity_names,
        .args[1].suggestion_callback = get_entity_names,
    },
    {
        .name = "commands",
        .callback = display_command_names,
        .close_console_on_success = false,
    }
};

static bool display_command_names(char* _unused) {
  char output_message[128];
  for (int i = 0; i < array_count(console_commands); i++) {
    if (strcmp(console_commands[i].name, "commands") == 0) {
      continue;
    }

    sprintf(output_message, "%s", console_commands[i].name);
    console_append_to_output(output_message);
  }
  return true;
}

static CommandArgsSuggestions console_find_command_suggestion_argument(void) {
  ConsoleCommandParts console_command_parts = parse_command(console.input[console.input_index].value);
  int current_part_index =
      part_index(console_command_parts, console.input[console.input_index].input_length);  // TODO: Implement cursor index and use it here

  for (int i = 0; i < array_count(console_commands); i++) {
    int command_length = (int)strlen(console_commands[i].name);
    if (command_length > console.input[console.input_index].input_length) {
      continue;
    }
    if (current_part_index > 0 && console_commands[i].args[current_part_index - 1].suggestion_callback &&
        strncmp(console_commands[i].name, console.input[console.input_index].value, command_length) == 0) {
      return console_commands[i].args[current_part_index - 1].suggestion_callback(console.input[console.input_index].value + command_length + 1);
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

static ConsoleCommand console_find_current_command(void) {
  for (int i = 0; i < array_count(console_commands); i++) {
    if (strncmp(console_commands[i].name, console.input[console.input_index].value, strlen(console_commands[i].name)) == 0) {
      return console_commands[i];
    }
  }

  return (ConsoleCommand){0};
}

static ConsoleCommandPart console_get_current_command(void) {
  ConsoleCommandParts console_command_parts = parse_command(console.input[console.input_index].value);
  int current_part_index = part_index(console_command_parts, console.input[console.input_index].input_length);
  ConsoleCommandPart console_command_part = console_command_parts.part[current_part_index];
  return console_command_part;
};

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
      char* input = console.input[console.input_index].value + command_length + 1;

      bool result = console_commands[i].callback(input);
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

static void console_draw_suggestion_text(Vec2* text_size, float y) {
  if (console.input[console.input_index].input_length == 0) {
    return;
  }

  ConsoleCommand current_command = console_find_current_command();

  ConsoleCommandParts console_command_parts = parse_command(console.input[console.input_index].value);
  int current_part_index =
      part_index(console_command_parts, console.input[console.input_index].input_length);  // TODO: Implement cursor index and use it here

  CommandArgsSuggestionCallback suggestion_callback = current_command.args[current_part_index - 1].suggestion_callback;

  if (current_command.name && suggestion_callback) {
    CommandArgsSuggestions suggested_command_argument = console_find_command_suggestion_argument();
    if (suggested_command_argument.count > 0) {
      // First check if the current input is the same as one of the suggested arguments.
      // If so, don't draw anything:
      for (int suggestion_index = 0; suggestion_index < suggested_command_argument.count; suggestion_index++) {
        if (strcmp(console_command_parts.part[current_part_index].value, suggested_command_argument.suggestions[suggestion_index]) == 0) {
          return;
        }
      }

      for (int suggestion_index = 0; suggestion_index < suggested_command_argument.count; suggestion_index++) {
        draw_text_utf8(
            suggested_command_argument.suggestions[suggestion_index],
            (Vec2){.x = text_size->x + 8.0f, .y = y - 40.0f - (suggestion_index * 32.0f) + 7.0f}, (RGBA){0, 0, 0, 1}, &render_context.fonts[1]
        );
      }
    }
  } else {
    // Draw suggested command text
    char* suggested_command = console_find_command_suggestion();
    if (suggested_command) {
      draw_text_utf8(suggested_command, (Vec2){.x = 8.0f, .y = y + 7.0f}, (RGBA){0.5, 0.5, 0.5, 1}, &render_context.fonts[1]);
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

  console_draw_suggestion_text(&input_text_size, input_y);
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
      bool text_to_add_is_space = event->text.text[0] == ' ';
      bool is_last_character_space = console.input[console.input_index].value[console.input[console.input_index].input_length - 1] == ' ';

      if (text_to_add_is_space && is_last_character_space) {
        break;
      }

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
          if (console.input[console.input_index].input_length == 0) {
            break;
          }
          char* suggested_command = console_find_command_suggestion();
          if (suggested_command) {
            strcpy(console.input[console.input_index].value, suggested_command);
            console.input[console.input_index].input_length = (int)strlen(suggested_command);
            break;
          } else {
            CommandArgsSuggestions suggested_command_argument = console_find_command_suggestion_argument();
            if (suggested_command_argument.count > 0) {
              // ConsoleCommandPart current_command_part = console_get_current_command();
              ConsoleCommandParts console_command_parts = parse_command(console.input[console.input_index].value);
              int current_part_index = part_index(console_command_parts, console.input[console.input_index].input_length);

              strcpy(
                  console.input[console.input_index].value + console_command_parts.part[current_part_index - 1].end + 1,
                  suggested_command_argument.suggestions[0]
              );
              console.input[console.input_index].input_length =
                  console_command_parts.part[current_part_index - 1].end + 1 + (int)strlen(suggested_command_argument.suggestions[0]);
              break;
            }
          }
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