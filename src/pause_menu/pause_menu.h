#pragma once

#include "headers.h"

typedef enum {
  PAUSE_MENU_MAIN,
  PAUSE_MENU_VIDEO,
  PAUSE_MENU_AUDIO,
  PAUSE_MENU_CONTROLS,
} PauseMenuScreen;

typedef enum {
  PAUSE_MENU_MOUSE_MODE,
  PAUSE_MENU_KEYBOARD_MODE,
} PauseMenuInputMode;

typedef struct {
  int id;
  FRect rect;
  char text[32];
} PauseMenuDropdown;

typedef struct {
  int id;
  FRect rect;
  char text[32];
} PauseMenuButton;

typedef struct {
  int id;
  FRect rect;
  char text[32];
} PauseMenuTitle;

typedef struct {
  int hovered_id;
  int focused_id;
  double hover_start_time;
  PauseMenuInputMode input_mode;
  PauseMenuScreen current_screen;
} PauseMenu;

FRect get_container(float* element_sizes, int element_count);

void pause_menu_handle_input(SDL_Event*);

void pause_menu_draw_video(int number_of_elements, int element_id_start, float element_height);

// void pause_menu_draw_audio(int number_of_elements, int element_id_start, float element_height);

void pause_menu_draw_controls(int number_of_elements, int element_id_start, float element_height);

void pause_menu_draw_main(void);

bool pause_menu_title(PauseMenuTitle title);

float current_element_position_y(float* element_heights, int current_element);

bool draw_pause_menu_dropdown(PauseMenuDropdown dropdown);

void toggle_pause_menu(void);