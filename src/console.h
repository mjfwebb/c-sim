#pragma once

#include "headers.h"

#define console_is_open (console.y_spring.current > 0.1f)

#define CONSOLE_INPUT_HEIGHT 64.0f
#define MAX_CONSOLE_OUTPUT_LENGTH 128
#define MAX_CONSOLE_OUTPUT_MESSAGES 16
#define MAX_CONSOLE_INPUT_HISTORY 16
#define MAX_ARGS_SUGGESTIONS 8

typedef bool (*CommandCallback)(char*);

typedef struct {
  int count;
  char suggestions[MAX_ARGS_SUGGESTIONS][128];
} CommandArgsSuggestions;

typedef CommandArgsSuggestions (*CommandArgsSuggestionCallback)(char*);

typedef struct {
  CommandArgsSuggestionCallback suggestion_callback;
} ConsoleCommandArgument;

typedef struct {
  char* name;
  CommandCallback callback;
  ConsoleCommandArgument args[7];
  bool close_console_on_success;
} ConsoleCommand;

typedef struct {
  int start;
  int end;
  char value[128];
} ConsoleCommandPart;

typedef struct {
  ConsoleCommandPart part[8];
  int length;
} ConsoleCommandParts;

typedef struct {
  char messages[MAX_CONSOLE_OUTPUT_MESSAGES][MAX_CONSOLE_OUTPUT_LENGTH];
  int count;
  int start;
} ConsoleOutput;

typedef struct {
  char value[MAX_CONSOLE_INPUT_LENGTH];
  int input_length;
} ConsoleInput;

typedef struct {
  float y;
  float target_y;
  Spring y_spring;
  ConsoleInput input[MAX_CONSOLE_INPUT_HISTORY];
  int input_index;
  int input_index_count;
  int input_index_start;
  ConsoleOutput output;
} Console;

void console_append_to_output(char* message);

void console_handle_input(SDL_Event* event);

void console_open();