#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "personalities.c"
#include "seed.c"

#define VA_ARGS(...) , ##__VA_ARGS__  // For variadic macros
#define MAX_ENTITIES 1024
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define array_count(static_array) (sizeof(static_array) / sizeof((static_array)[0]))
#define rect_fields \
  float w;          \
  float h
#define point_fields \
  float x;           \
  float y
#define target_point_fields \
  float target_x;           \
  float target_y

#define print(format, ...)            \
  printf(format "\n", ##__VA_ARGS__); \
  fflush(stdout)

typedef struct FRect {
  float x;
  float y;
  float w;
  float h;
} FRect;

typedef struct FPoint {
  float x;
  float y;
} FPoint;

typedef struct Spring {
  float target;
  float current;
  float velocity;
  float acceleration;
  float friction;
} Spring;

typedef struct {
  rect_fields;
  point_fields;
  target_point_fields;
  float target_zoom;
  float zoom;
  Spring zoom_spring;
  Spring pan_spring_x;
  Spring pan_spring_y;
  int following_entity;
} Camera;

typedef struct {
  point_fields;
  target_point_fields;
  Spring spring_x;
  Spring spring_y;
  SDL_FRect rect;
} Selection;

typedef struct {
  int w;
  int h;
  SDL_Texture *texture;
} Image;

typedef struct {
  float speed;
  float prev_speed;
  float delta_time;
  float animated_time;
  SDL_Renderer *renderer;
  int window_w;
  int window_h;
  SDL_Color background_color;
  Camera camera;
  TTF_Font **fonts;
  float fps;
  Selection selection;
  const Uint8 *keyboard_state;
  Image *images;
} RenderContext;

typedef struct {
  int health[MAX_ENTITIES];
  char *names[MAX_ENTITIES];
  bool selected[MAX_ENTITIES];
  bool hovered[MAX_ENTITIES];
  FRect rect[MAX_ENTITIES];
  int image[MAX_ENTITIES];
  FPoint direction[MAX_ENTITIES];
  int personalities[MAX_ENTITIES][array_count(Personality__Strings)];
} GameContext;

typedef struct {
  int prev_state;
  int state;
  int button;
  point_fields;
  float prev_x;
  float prev_y;
  int clicks;
} MouseState;

GameContext game_context = {0};
int num_of_entities = 0;

int random_int_between(int min, int max) {
  return min + (rand() % (max - min));
}

int Entity__get_personality_count(int entity_id) {
  int result = 0;
  for (int i = 0; i < array_count(Personality__Strings); i++) {
    if (game_context.personalities[i] > 0) {
      result += 1;
    }
  }

  return result;
}

bool Entity__has_personality(int entity_id, Personality personality) {
  return game_context.personalities[entity_id][personality] > 0;
}

SDL_FRect camera_relative_rect(RenderContext *render_context, SDL_FRect *source_rect) {
  SDL_FRect rect = {
      .w = source_rect->w * render_context->camera.zoom,
      .h = source_rect->h * render_context->camera.zoom,
      .x = ((source_rect->x - render_context->camera.x) * render_context->camera.zoom + render_context->window_w / 2),
      .y = ((source_rect->y - render_context->camera.y) * render_context->camera.zoom + render_context->window_h / 2),
  };

  return rect;
}

void Entity__create(RenderContext *render_context, char *name) {
  int image_id = random_int_between(0, 3);
  float width = 100.0f;
  float scale = width / render_context->images[image_id].w;
  float height = (float)(render_context->images[image_id].h * scale);

  game_context.health[num_of_entities] = 100;
  game_context.names[num_of_entities] = name;
  game_context.selected[num_of_entities] = false;
  game_context.hovered[num_of_entities] = false;
  game_context.rect[num_of_entities] = (FRect){
      .h = height,
      .w = width,
      .x = (float)(rand() % 2000) - 1000,
      .y = (float)(rand() % 2000) - 1000,
  };
  game_context.direction[num_of_entities] = (FPoint){
      .x = ((float)(rand() % 200) - 100) / 100,
      .y = ((float)(rand() % 200) - 100) / 100,
  };
  game_context.image[num_of_entities] = image_id;

  int random_amount_of_personalities = random_int_between(5, 10);
  for (int i = 0; i < random_amount_of_personalities; i++) {
    int personality = random_int_between(0, array_count(Personality__Strings));
    game_context.personalities[num_of_entities][personality] = random_int_between(0, 100);
  }

  // Now increment num_of_entities so the next one has a higher index;
  num_of_entities++;
}

void draw_texture(RenderContext *render_context, int image_id, SDL_FRect *rendering_rect) {
  int copy_result = SDL_RenderCopyF(render_context->renderer, render_context->images[image_id].texture, NULL, rendering_rect);
  if (copy_result != 0) {
    printf("Failed to render copy: %s\n", SDL_GetError());
    return;
  }
}

void draw_entity_name(RenderContext *render_context, int entity_id) {
  TTF_Font *font = NULL;
  SDL_Surface *text_surface = NULL;
  float y = (game_context.rect[entity_id].y - render_context->camera.y - (45.0f / render_context->camera.zoom)) * render_context->camera.zoom +
            render_context->window_h / 2;

  if (game_context.hovered[entity_id]) {
    SDL_Color Yellow = {255, 255, 0};
    font = render_context->fonts[0];
    text_surface = TTF_RenderText_Blended(font, game_context.names[entity_id], Yellow);
    y -= 10.0f;  // move the text up a little when using the bigger font
  } else {
    SDL_Color Black = {0, 0, 0};
    font = render_context->fonts[1];
    text_surface = TTF_RenderText_Blended(font, game_context.names[entity_id], Black);
  }

  assert(font);
  assert(text_surface);

  SDL_Texture *text_texture = SDL_CreateTextureFromSurface(render_context->renderer, text_surface);
  if (!text_texture) {
    fprintf(stderr, "could not create text texture: %s\n", SDL_GetError());
  }

  float diff = ((game_context.rect[entity_id].w * render_context->camera.zoom) - text_surface->w) / 2;
  float x = (((game_context.rect[entity_id].x - render_context->camera.x) * render_context->camera.zoom) + diff) + render_context->window_w / 2;

  SDL_FRect text_rect = {.w = (float)text_surface->w, .h = (float)text_surface->h, .x = x, .y = y};

  SDL_RenderCopyF(render_context->renderer, text_texture, NULL, &text_rect);
  SDL_FreeSurface(text_surface);
  SDL_DestroyTexture(text_texture);
}

void draw_debug_text(RenderContext *render_context, int index, char *str, ...) {
  char text_buffer[128];
  va_list args;
  va_start(args, str);
  int chars_written = vsnprintf(text_buffer, sizeof(text_buffer), str, args);
  assert(chars_written > 0);
  va_end(args);

  TTF_Font *font = render_context->fonts[0];
  SDL_Color White = {255, 255, 255};
  SDL_Surface *text_surface = TTF_RenderText_Blended(font, text_buffer, White);
  if (!text_surface) {
    fprintf(stderr, "could not create text surface: %s\n", SDL_GetError());
  }

  SDL_FRect text_rect = {
      .w = (float)text_surface->w,
      .h = (float)text_surface->h,
      .x = 10.0f,
      .y = (32.0f * index),
  };

  SDL_Texture *text_texture = SDL_CreateTextureFromSurface(render_context->renderer, text_surface);
  if (!text_texture) {
    fprintf(stderr, "could not create text texture: %s\n", SDL_GetError());
  }

  SDL_RenderCopyF(render_context->renderer, text_texture, NULL, &text_rect);
  SDL_FreeSurface(text_surface);
  SDL_DestroyTexture(text_texture);
}

void render_debug_info(RenderContext *render_context, MouseState *mouse_state) {
  int index = 0;
  draw_debug_text(render_context, index++, "fps: %.2f", render_context->fps);
  draw_debug_text(render_context, index++, "mouse state: %d, button: %d, clicks: %d", mouse_state->state, mouse_state->button, mouse_state->clicks);
  draw_debug_text(render_context, index++, "prev mouse state: %d", mouse_state->prev_state);
  draw_debug_text(render_context, index++, "camera zoom: %.1f", render_context->camera.target_zoom);
  draw_debug_text(render_context, index++, "game speed: %.1f", render_context->speed);
  draw_debug_text(
      render_context, index++, "camera: current x,y: %.2f,%.2f target x,y: %.2f,%.2f", render_context->camera.x, render_context->camera.y,
      render_context->camera.target_x, render_context->camera.target_y
  );
  draw_debug_text(
      render_context, index++, "selection: current x,y: %.2f,%.2f target x,y: %.2f,%.2f", render_context->selection.x, render_context->selection.y,
      render_context->selection.target_x, render_context->selection.target_y
  );
}

void render_selection_box(RenderContext *render_context) {
  SDL_SetRenderDrawColor(render_context->renderer, 255, 255, 255, 255);
  int result = SDL_RenderDrawRectF(render_context->renderer, &render_context->selection.rect);
  assert(result == 0);
}

float Spring__update(Spring *spring, float target) {
  spring->target = target;
  spring->velocity += (target - spring->current) * spring->acceleration;
  spring->velocity *= spring->friction;
  return spring->current += spring->velocity;
}

void draw_border(RenderContext *render_context, SDL_FRect around, float gap_width, float border_width) {
  SDL_FRect borders[4];

  //         1
  //   |-----------|
  //   |           |
  // 0 |           | 2
  //   |           |
  //   |-----------|
  //         3
  for (int i = 0; i < 4; i++) {
    borders[i] = around;

    if (i % 2 == 0) {  // Left (0) and right (2)
      borders[i].w = border_width;
      borders[i].h += (gap_width + border_width) * 2;
      borders[i].x += (i == 2 ? around.w + gap_width : -(gap_width + border_width));
      borders[i].y -= gap_width + border_width;
    } else {  // Top (1) and bottom (3)
      borders[i].w += (gap_width + border_width) * 2;
      borders[i].h = border_width;
      borders[i].x -= gap_width + border_width;
      borders[i].y += (i == 3 ? (around.h + gap_width) : -(gap_width + border_width));
    }

    SDL_SetRenderDrawColor(render_context->renderer, 255, 255, 255, 255);
    SDL_FRect rect = camera_relative_rect(render_context, &borders[i]);
    SDL_RenderFillRectF(render_context->renderer, &rect);
  }
}

void update_entity(RenderContext *render_context, int entity_id) {
  game_context.rect[entity_id].x += game_context.direction[entity_id].x * (render_context->delta_time * render_context->speed);
  game_context.rect[entity_id].y += game_context.direction[entity_id].y * (render_context->delta_time * render_context->speed);
}

void render_entity(RenderContext *render_context, int entity_id) {
  SDL_FRect rendering_rect = camera_relative_rect(
      render_context,
      &(SDL_FRect){
          .w = game_context.rect[entity_id].w,
          .h = game_context.rect[entity_id].h,
          .x = game_context.rect[entity_id].x,
          .y = game_context.rect[entity_id].y,
      }
  );

  draw_texture(render_context, game_context.image[entity_id], &rendering_rect);

  if (game_context.selected[entity_id]) {
    draw_border(
        render_context,
        (SDL_FRect){
            .h = game_context.rect[entity_id].h,
            .w = game_context.rect[entity_id].w,
            .x = game_context.rect[entity_id].x,
            .y = game_context.rect[entity_id].y,
        },
        5.0f / render_context->camera.zoom, 4.0f / render_context->camera.zoom
    );
  }
}

Image Image__load(RenderContext *render_context, const char *texture_file_path) {
  SDL_Surface *surface = IMG_Load(texture_file_path);
  assert(surface);

  SDL_Texture *texture = SDL_CreateTextureFromSurface(render_context->renderer, surface);
  assert(texture);

  Image image = (Image){
      .h = surface->h,
      .w = surface->w,
      .texture = texture,
  };

  return image;
}

TTF_Font *Font__load(const char *font_file_path, int font_size) {
  TTF_Font *font = TTF_OpenFont(font_file_path, font_size);
  assert(font);

  return font;
}

SDL_FRect screen_to_world(RenderContext *render_context, SDL_FRect screen_rect) {
  SDL_FRect world_rect = {
      .w = screen_rect.w / render_context->camera.zoom,
      .h = screen_rect.h / render_context->camera.zoom,
      .x = (screen_rect.x - render_context->window_w / 2) / render_context->camera.zoom + render_context->camera.x,
      .y = (screen_rect.y - render_context->window_h / 2) / render_context->camera.zoom + render_context->camera.y,
  };

  return world_rect;
}

SDL_FRect world_to_screen(RenderContext *render_context, SDL_FRect world_rect) {
  SDL_FRect screen_rect = {
      .w = world_rect.w * render_context->camera.zoom,
      .h = world_rect.h * render_context->camera.zoom,
      .x = (world_rect.x - render_context->camera.x) * render_context->camera.zoom + render_context->window_w / 2,
      .y = (world_rect.y - render_context->camera.y) * render_context->camera.zoom + render_context->window_h / 2,
  };

  return screen_rect;
}

void draw_grid(RenderContext *render_context) {
  // Draw it blended
  SDL_SetRenderDrawBlendMode(render_context->renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(render_context->renderer, 0, 0, 0, 25);
  float grid_size = 100.0f;
  float window_w = (float)render_context->window_w;
  float window_h = (float)render_context->window_h;

  SDL_FRect grid_to_screen = world_to_screen(
      render_context,
      (SDL_FRect){
          .w = grid_size,
          .h = grid_size,
          .x = 0,
          .y = 0,
      }
  );

  float x_start = grid_to_screen.x - floorf(grid_to_screen.x / grid_to_screen.w) * grid_to_screen.w;
  for (float x = x_start; x < window_w; x += grid_to_screen.w) {
    SDL_RenderDrawLineF(render_context->renderer, x, 0, x, window_h);
  }

  float y_start = grid_to_screen.y - floorf(grid_to_screen.y / grid_to_screen.h) * grid_to_screen.h;
  for (float y = y_start; y < window_h; y += grid_to_screen.h) {
    SDL_RenderDrawLineF(render_context->renderer, 0, y, window_w, y);
  }

  // Reset the blend mode
  SDL_SetRenderDrawBlendMode(render_context->renderer, SDL_BLENDMODE_NONE);
}

void clear_screen(RenderContext *render_context) {
  SDL_SetRenderDrawColor(
      render_context->renderer, render_context->background_color.r, render_context->background_color.g, render_context->background_color.b,
      render_context->background_color.a
  );
  SDL_RenderClear(render_context->renderer);
}

void mouse_control_camera(RenderContext *render_context, MouseState *mouse_state) {
  if (mouse_state->button == SDL_BUTTON_RIGHT && mouse_state->state == SDL_PRESSED) {
    if (mouse_state->prev_x != mouse_state->x || mouse_state->prev_y != mouse_state->y) {
      float delta_x = mouse_state->x - mouse_state->prev_x;
      float delta_y = mouse_state->y - mouse_state->prev_y;
      mouse_state->prev_x = mouse_state->x;
      mouse_state->prev_y = mouse_state->y;

      render_context->camera.target_x -= delta_x / render_context->camera.zoom;
      render_context->camera.target_y -= delta_y / render_context->camera.zoom;
    }
  }
}

// Camera movement and selection rect movement
void keyboard_control_camera(RenderContext *render_context) {
  float camera_keyboard_movement_speed = 5.0f;
  if (render_context->keyboard_state[SDL_GetScancodeFromKey(SDLK_w)]) {
    render_context->camera.target_y -= camera_keyboard_movement_speed / render_context->camera.zoom;
    render_context->selection.target_y += camera_keyboard_movement_speed;
  }
  if (render_context->keyboard_state[SDL_GetScancodeFromKey(SDLK_s)]) {
    render_context->camera.target_y += camera_keyboard_movement_speed / render_context->camera.zoom;
    render_context->selection.target_y -= camera_keyboard_movement_speed;
  }
  if (render_context->keyboard_state[SDL_GetScancodeFromKey(SDLK_a)]) {
    render_context->camera.target_x -= camera_keyboard_movement_speed / render_context->camera.zoom;
    render_context->selection.target_x += camera_keyboard_movement_speed;
  }
  if (render_context->keyboard_state[SDL_GetScancodeFromKey(SDLK_d)]) {
    render_context->camera.target_x += camera_keyboard_movement_speed / render_context->camera.zoom;
    render_context->selection.target_x -= camera_keyboard_movement_speed;
  }
}

// Set the camera to follow an entity, if only one entity is selected
void camera_follow_entity(RenderContext *render_context) {
  if (render_context->camera.following_entity > -1) {
    // TODO: Make it center on the entity
    render_context->camera.target_x = game_context.rect[render_context->camera.following_entity].x;
    render_context->camera.target_y = game_context.rect[render_context->camera.following_entity].y;
  }
}

// Set selected on any entity within the selection_rect
void select_entities_within_selection_rect(RenderContext *render_context) {
  for (int entity_i = 0; entity_i < num_of_entities; entity_i++) {
    SDL_FRect rect = camera_relative_rect(
        render_context,
        &(SDL_FRect){
            .w = game_context.rect[entity_i].w,
            .h = game_context.rect[entity_i].h,
            .x = game_context.rect[entity_i].x,
            .y = game_context.rect[entity_i].y,
        }
    );
    SDL_FPoint point_top_left = {
        .x = rect.x,
        .y = rect.y,
    };
    SDL_FPoint point_bottom_right = {
        .x = rect.x + rect.w,
        .y = rect.y + rect.h,
    };

    // If the selection rect is bigger than 3 pixels, select the entity if it's within the selection rect
    if (render_context->selection.rect.w > 3.0f) {
      if (SDL_PointInFRect(&point_top_left, &render_context->selection.rect) &&
          SDL_PointInFRect(&point_bottom_right, &render_context->selection.rect)) {
        game_context.selected[entity_i] = true;
      } else {
        if (!render_context->keyboard_state[SDL_GetScancodeFromKey(SDLK_LSHIFT)]) {
          game_context.selected[entity_i] = false;
        }
      }
    }
  }
}

bool entity_under_mouse(RenderContext *render_context, int entity_id, MouseState *mouse_state) {
  SDL_FRect source_rect = {
      .w = game_context.rect[entity_id].w,
      .h = game_context.rect[entity_id].h,
      .x = game_context.rect[entity_id].x,
      .y = game_context.rect[entity_id].y,
  };
  SDL_FRect rect = camera_relative_rect(render_context, &source_rect);

  return SDL_PointInFRect(
      &(SDL_FPoint){
          .x = mouse_state->x,
          .y = mouse_state->y,
      },
      &rect
  );
}

void init() {
  srand(create_seed("ATHANO_LOVES_CHAT_OWO"));

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
    exit(1);
  }

  if (TTF_Init() == -1) {
    fprintf(stderr, "could not initialize ttf: %s\n", TTF_GetError());
    exit(1);
  }
}

void log_entity_personalities(int entity_id) {
  for (int personality_i = 0; personality_i < array_count(game_context.personalities[entity_id]); personality_i++) {
    if (Entity__has_personality(entity_id, personality_i)) {
      print(
          "Entity %s has personality %s with value %d", game_context.names[entity_id], Personality__Strings[personality_i],
          game_context.personalities[entity_id][personality_i]
      );
    }
  }
}

int main(int argc, char *args[]) {
  init();

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

  SDL_Window *window =
      SDL_CreateWindow("Cultivation Sim", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (!window) {
    fprintf(stderr, "could not create window: %s\n", SDL_GetError());
    return 1;
  }

  RenderContext render_context = {
      .renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
      .animated_time = 0,
      .speed = 200.0f,
      .delta_time = 0,
      .background_color = {35, 127, 178, 255},
      .camera =
          {
              .x = 0,
              .y = 0,
              .target_zoom = 1.0f,
              .pan_spring_x =
                  {
                      .target = 1.0f,
                      .current = 1.0f,
                      .velocity = 0.0f,
                      .acceleration = 0.5f,
                      .friction = 0.1f,
                  },
              .pan_spring_y =
                  {
                      .target = 1.0f,
                      .current = 1.0f,
                      .velocity = 0.0f,
                      .acceleration = 0.5f,
                      .friction = 0.1f,
                  },
              .zoom_spring =
                  {
                      .target = 1.0f,
                      .current = 1.0f,
                      .velocity = 0.0f,
                      .acceleration = 0.4f,
                      .friction = 0.1f,
                  },
              .following_entity = -1,
          },
      .fonts =
          (TTF_Font *[]){
              Font__load("assets/OpenSans-Regular.ttf", 32),
              Font__load("assets/OpenSans-Regular.ttf", 24),
              Font__load("assets/OpenSans-Regular.ttf", 16),
          },
      .selection =
          {
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
      .images =
          (Image[]){
              Image__load(&render_context, "assets/stone.bmp"),
              Image__load(&render_context, "assets/fish.bmp"),
              Image__load(&render_context, "assets/lamb.bmp"),
              Image__load(&render_context, "assets/lamb2.bmp"),
          }
  };

  if (!render_context.renderer) {
    fprintf(stderr, "could not create renderer: %s\n", SDL_GetError());
    return 1;
  }

  Entity__create(&render_context, "pushqrdx");
  Entity__create(&render_context, "Athano");
  Entity__create(&render_context, "AshenHobs");
  Entity__create(&render_context, "adrian_learns");
  Entity__create(&render_context, "RVerite");
  Entity__create(&render_context, "Orshy");
  Entity__create(&render_context, "ruggs888");
  Entity__create(&render_context, "Xent12");
  Entity__create(&render_context, "nuke_bird");
  Entity__create(&render_context, "Kasper_573");
  Entity__create(&render_context, "SturdyPose");
  Entity__create(&render_context, "coffee_lava");
  Entity__create(&render_context, "goudacheeseburgers");
  Entity__create(&render_context, "ikiwixz");
  Entity__create(&render_context, "NixAurvandil");
  Entity__create(&render_context, "smilingbig");
  Entity__create(&render_context, "tk_dev");
  Entity__create(&render_context, "realSuperku");
  Entity__create(&render_context, "Hoby2000");
  Entity__create(&render_context, "CuteMath");
  Entity__create(&render_context, "forodor");
  Entity__create(&render_context, "Azenris");
  Entity__create(&render_context, "collector_of_stuff");
  Entity__create(&render_context, "EvanMMO");
  Entity__create(&render_context, "thechaosbean");
  Entity__create(&render_context, "Lutf1sk");
  Entity__create(&render_context, "BauBas9883");
  Entity__create(&render_context, "physbuzz");
  Entity__create(&render_context, "rizoma0x00");
  Entity__create(&render_context, "Tkap1");
  Entity__create(&render_context, "GavinsAwfulStream");
  Entity__create(&render_context, "Resist_0");
  Entity__create(&render_context, "b1k4sh");
  Entity__create(&render_context, "nhancodes");
  Entity__create(&render_context, "qcircuit1");
  Entity__create(&render_context, "fruloo");
  Entity__create(&render_context, "programmer_jeff");
  Entity__create(&render_context, "BluePinStudio");
  Entity__create(&render_context, "Pierito95RsNg");
  Entity__create(&render_context, "jumpylionnn");
  Entity__create(&render_context, "Aruseus");
  Entity__create(&render_context, "lastmiles");
  Entity__create(&render_context, "soulfoam");

  MouseState mouse_state = {
      .prev_state = 0,
      .state = 0,
      .button = 0,
      .clicks = 0,
      .prev_x = 0,
      .prev_y = 0,
      .x = 0,
      .y = 0,
  };

  int game_is_still_running = 1;
  unsigned int start_ticks = SDL_GetTicks();
  int current_time = 0;
  int frame_count = 0;
  int last_update_time = 0;

  while (game_is_still_running) {
    frame_count++;
    if (SDL_GetTicks() - start_ticks >= 1000) {
      render_context.fps = (float)frame_count;
      frame_count = 0;
      start_ticks = SDL_GetTicks();
    }

    current_time = SDL_GetTicks();
    render_context.delta_time = (float)(current_time - last_update_time) / 1000;
    last_update_time = current_time;
    render_context.animated_time = fmodf(render_context.animated_time + render_context.delta_time * 0.5f, 1);
    render_context.camera.zoom = Spring__update(&render_context.camera.zoom_spring, render_context.camera.target_zoom);
    SDL_GetWindowSizeInPixels(window, &render_context.window_w, &render_context.window_h);
    render_context.keyboard_state = SDL_GetKeyboardState(NULL);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
        mouse_state.prev_state = mouse_state.state;
        mouse_state.state = event.button.state;
        mouse_state.button = event.button.button;
        mouse_state.clicks = event.button.clicks;
        if (mouse_state.prev_state != SDL_PRESSED) {
          // Set selection target to the current mouse position
          render_context.selection.target_x = mouse_state.x;
          render_context.selection.target_y = mouse_state.y;
          // Reset selection spring so it doesn't spring between the old selection and the new one
          render_context.selection.spring_x.current = render_context.selection.target_x;
          render_context.selection.spring_y.current = render_context.selection.target_y;
        }
      }
      if (event.type == SDL_MOUSEMOTION) {
        mouse_state.prev_state = mouse_state.state;
        mouse_state.prev_x = mouse_state.x;
        mouse_state.prev_y = mouse_state.y;
        mouse_state.x = (float)event.motion.x;
        mouse_state.y = (float)event.motion.y;
      }
      if (event.type == SDL_MOUSEWHEEL) {
        if (event.wheel.y > 0) {
          // zoom in
          render_context.camera.target_zoom = SDL_min(render_context.camera.target_zoom + 0.1f, 2.0f);
        } else if (event.wheel.y < 0) {
          // zoom out
          render_context.camera.target_zoom = SDL_max(render_context.camera.target_zoom - 0.1f, 0.1f);
        }
      }
      if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            game_is_still_running = 0;
            break;

          case SDLK_UP:
            render_context.speed += 100.0f;
            break;

          case SDLK_DOWN:
            render_context.speed = SDL_max(render_context.speed - 100.0f, 0);
            break;

          case SDLK_SPACE:
            if (render_context.prev_speed > 0) {
              render_context.speed = render_context.prev_speed;
              render_context.prev_speed = 0;
            } else {
              render_context.prev_speed = render_context.speed;
              render_context.speed = 0;
            }
          default:
            break;
        }
      } else if (event.type == SDL_QUIT) {
        game_is_still_running = 0;
      }

      // Two loops needed so we can have a case where multiple entities can be hovered over, but only one can be selected
      for (int entity_id = num_of_entities - 1; entity_id >= 0; entity_id--) {
        game_context.hovered[entity_id] = entity_under_mouse(&render_context, entity_id, &mouse_state);
      }

      for (int entity_id = num_of_entities - 1; entity_id >= 0; entity_id--) {
        if (entity_under_mouse(&render_context, entity_id, &mouse_state)) {
          if (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_RELEASED &&
              render_context.selection.rect.w == 0) {
            game_context.selected[entity_id] = !game_context.selected[entity_id];
            log_entity_personalities(entity_id);

            if (game_context.selected[entity_id]) {
              render_context.camera.following_entity = entity_id;
            } else {
              render_context.camera.following_entity = -1;
            }
            break;
          }
        }
      }
    }

    mouse_control_camera(&render_context, &mouse_state);

    keyboard_control_camera(&render_context);

    {  // Selection rect creation
      if (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_PRESSED) {
        render_context.selection.rect = (SDL_FRect){
            .x = SDL_min(mouse_state.x, render_context.selection.x),
            .y = SDL_min(mouse_state.y, render_context.selection.y),
            .w = SDL_fabsf(mouse_state.x - render_context.selection.x),
            .h = SDL_fabsf(mouse_state.y - render_context.selection.y),
        };
      }
    }

    camera_follow_entity(&render_context);

    select_entities_within_selection_rect(&render_context);

    {  // Spring the selection box
      render_context.selection.x = Spring__update(&render_context.selection.spring_x, render_context.selection.target_x);
      render_context.selection.y = Spring__update(&render_context.selection.spring_y, render_context.selection.target_y);
    }

    {  // Spring the camera position
      render_context.camera.x = Spring__update(&render_context.camera.pan_spring_x, render_context.camera.target_x);
      render_context.camera.y = Spring__update(&render_context.camera.pan_spring_y, render_context.camera.target_y);
    }

    clear_screen(&render_context);

    draw_grid(&render_context);

    for (int entity_i = 0; entity_i < num_of_entities; entity_i++) {
      update_entity(&render_context, entity_i);
      render_entity(&render_context, entity_i);
    }

    if (render_context.camera.zoom > 0.5f) {
      for (int entity_id = 0; entity_id < num_of_entities; entity_id++) {
        draw_entity_name(&render_context, entity_id);
      }
    }

    render_selection_box(&render_context);

    render_debug_info(&render_context, &mouse_state);

    SDL_RenderPresent(render_context.renderer);

    // Clear the selection rect
    render_context.selection.rect = (SDL_FRect){0};
  }

  SDL_DestroyRenderer(render_context.renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
