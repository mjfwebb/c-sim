#define SDL_MAIN_HANDLED

#include "headers.h"

#include "fonts.c"
#include "personalities.c"
#include "seed.c"

#define VA_ARGS(...) , ##__VA_ARGS__  // For variadic macros
#define entity_loop(index_name) for (int index_name = 0; index_name < game_context.entity_count; index_name++)
#define reverse_entity_loop(index_name) for (int index_name = game_context.entity_count - 1; index_name >= 0; index_name--)
#define mouse_primary_pressed(mouse_state) \
  (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_PRESSED)

#define INVALID_ENTITY (-100000000)
#define NUM_OF_FONTS 8
#define MAX_ENTITIES 1024
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define array_count(static_array) (sizeof(static_array) / sizeof((static_array)[0]))
#define print(format, ...)            \
  printf(format "\n", ##__VA_ARGS__); \
  fflush(stdout)

typedef struct Spring {
  float target;
  float current;
  float velocity;
  float acceleration;
  float friction;
} Spring;

typedef struct {
  FPoint current;
  FPoint target;
  float target_zoom;
  float zoom;
  Spring zoom_spring;
  Spring pan_spring_x;
  Spring pan_spring_y;
} Camera;

typedef struct {
  FPoint position;
  FPoint target;
  Spring spring_x;
  Spring spring_y;
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
  Font fonts[NUM_OF_FONTS];
  float fps;
  Selection selection;
  const u8 *keyboard_state;
  Image *images;
} RenderContext;

typedef struct {
  int health[MAX_ENTITIES];
  char names[MAX_ENTITIES][128];
  bool selected[MAX_ENTITIES];
  bool hovered[MAX_ENTITIES];
  FRect rect[MAX_ENTITIES];
  int image[MAX_ENTITIES];
  FPoint direction[MAX_ENTITIES];
  int personalities[MAX_ENTITIES][Personality_Count];
  int entity_count;
} GameContext;

typedef struct {
  int prev_state;
  int state;
  int button;
  FPoint position;
  FPoint prev_position;
  int clicks;
} MouseState;

GameContext game_context = {0};
RenderContext render_context = {0};

int random_int_between(int min, int max) {
  return min + (rand() % (max - min));
}

int Entity__get_personality_count(int entity_index) {
  int result = 0;
  for (int i = 0; i < Personality_Count; i++) {
    if (game_context.personalities[i] > 0) {
      result += 1;
    }
  }

  return result;
}

bool Entity__has_personality(int entity_index, Personality personality) {
  return game_context.personalities[entity_index][personality] > 0;
}

void create_entities() {
  float entity_width = 100.0f;

  char entity_names[][32] = {
      "pushqrdx",
      "Athano",
      "AshenHobs",
      "adrian_learns",
      "RVerite",
      "Orshy",
      "ruggs888",
      "Xent12",
      "nuke_bird",
      "Kasper_573",
      "SturdyPose",
      "coffee_lava",
      "goudacheeseburgers",
      "ikiwixz",
      "NixAurvandil",
      "smilingbig",
      "tk_dev",
      "realSuperku",
      "Hoby2000",
      "CuteMath",
      "forodor",
      "Azenris",
      "collector_of_stuff",
      "EvanMMO",
      "thechaosbean",
      "Lutf1sk",
      "BauBas9883",
      "physbuzz",
      "rizoma0x00",
      "Tkap1",
      "GavinsAwfulStream",
      "Resist_0",
      "b1k4sh",
      "nhancodes",
      "qcircuit1",
      "fruloo",
      "programmer_jeff",
      "BluePinStudio",
      "Pierito95RsNg",
      "jumpylionnn",
      "Aruseus",
      "lastmiles",
      "soulfoam",
      "AQtun81",
      "jess_forrealz",
      "RAFi18",
      "Delvoid",
      "Lolboy_30"
  };

  for (int name_index = 0; name_index < array_count(entity_names); name_index++) {
    int image_id = random_int_between(0, 3);
    float scale = entity_width / render_context.images[image_id].w;
    float entity_height = (float)(render_context.images[image_id].h * scale);

    game_context.health[game_context.entity_count] = 100;
    strcpy(game_context.names[game_context.entity_count], entity_names[name_index]);  // FIXME: Use the safe version strcpy_s. PRs welcome
    game_context.selected[game_context.entity_count] = false;
    game_context.hovered[game_context.entity_count] = false;
    game_context.rect[game_context.entity_count] = (FRect){
        .h = entity_height,
        .w = entity_width,
        .x = (float)random_int_between(-1000, 1000),
        .y = (float)random_int_between(-1000, 1000),
    };
    game_context.direction[game_context.entity_count] = (FPoint){
        .x = ((float)(rand() % 200) - 100) / 100,
        .y = ((float)(rand() % 200) - 100) / 100,
    };
    game_context.image[game_context.entity_count] = image_id;

    int random_amount_of_personalities = random_int_between(5, 10);
    for (int i = 0; i < random_amount_of_personalities; i++) {
      int personality = random_int_between(0, Personality_Count);
      game_context.personalities[game_context.entity_count][personality] = random_int_between(0, 100);
    }

    game_context.entity_count++;
  }
}

SDL_FRect create_screen_to_world_rect(FRect *screen_rect) {
  SDL_FRect world_rect = {
      .w = screen_rect->w / render_context.camera.zoom,
      .h = screen_rect->h / render_context.camera.zoom,
      .x = (screen_rect->x - render_context.window_w / 2) / render_context.camera.zoom + render_context.camera.current.x,
      .y = (screen_rect->y - render_context.window_h / 2) / render_context.camera.zoom + render_context.camera.current.y,
  };

  return world_rect;
}

SDL_FRect create_world_to_screen_rect(FRect *world_rect) {
  SDL_FRect screen_rect = {
      .w = world_rect->w * render_context.camera.zoom,
      .h = world_rect->h * render_context.camera.zoom,
      .x = (world_rect->x - render_context.camera.current.x) * render_context.camera.zoom + render_context.window_w / 2,
      .y = (world_rect->y - render_context.camera.current.y) * render_context.camera.zoom + render_context.window_h / 2,
  };

  return screen_rect;
}

void draw_texture(int image_id, SDL_FRect *rendering_rect) {
  int copy_result = SDL_RenderCopyF(render_context.renderer, render_context.images[image_id].texture, NULL, rendering_rect);
  if (copy_result != 0) {
    printf("Failed to render copy: %s\n", SDL_GetError());
    return;
  }
}

void draw_entity_name(int entity_id) {
  Font *font = &render_context.fonts[0];
  RGBA color = (RGBA){1, 1, 1, 1};
  float y = (game_context.rect[entity_id].y - render_context.camera.current.y - (45.0f / render_context.camera.zoom)) * render_context.camera.zoom +
            render_context.window_h / 2;

  if (game_context.hovered[entity_id]) {
    y -= 10.0f;  // move the text up a little when using the bigger font
    color = (RGBA){1, 1, 0, 1};
    font = &render_context.fonts[1];
  }

  FPoint text_size = get_text_size(game_context.names[entity_id], font, false, true);
  float diff = ((game_context.rect[entity_id].w * render_context.camera.zoom) - text_size.x) / 2;
  float x = (((game_context.rect[entity_id].x - render_context.camera.current.x) * render_context.camera.zoom) + diff) + render_context.window_w / 2;

  draw_text_outlined_utf8(game_context.names[entity_id], (FPoint){x, y}, color, (RGBA){0, 0, 0, 1}, font);
}

void draw_debug_text(int index, char *str, ...) {
  char text_buffer[128];
  va_list args;
  va_start(args, str);
  int chars_written = vsnprintf(text_buffer, sizeof(text_buffer), str, args);
  assert(chars_written > 0);
  va_end(args);

  draw_text_outlined_utf8(text_buffer, (FPoint){10.0f, (32.0f * index)}, (RGBA){1, 1, 1, 1}, (RGBA){0, 0, 0, 1}, &render_context.fonts[0]);
}

FRect get_selection_rect(MouseState *mouse_state) {
  return (FRect){
      .x = min(mouse_state->position.x, render_context.selection.position.x),
      .y = min(mouse_state->position.y, render_context.selection.position.y),
      .w = SDL_fabsf(mouse_state->position.x - render_context.selection.position.x),
      .h = SDL_fabsf(mouse_state->position.y - render_context.selection.position.y),
  };
}

void render_debug_info(MouseState *mouse_state) {
  int index = 0;
  draw_debug_text(index++, "fps: %.2f", render_context.fps);
  draw_debug_text(index++, "mouse state: %d, button: %d, clicks: %d", mouse_state->state, mouse_state->button, mouse_state->clicks);
  draw_debug_text(index++, "prev mouse state: %d", mouse_state->prev_state);
  draw_debug_text(index++, "camera zoom: %.1f", render_context.camera.target_zoom);
  draw_debug_text(index++, "game speed: %.1f", render_context.speed);
  draw_debug_text(
      index++, "camera: current x,y: %.2f,%.2f target x,y: %.2f,%.2f", render_context.camera.current.x, render_context.camera.current.y,
      render_context.camera.target.x, render_context.camera.target.y
  );
  FRect selection_rect = get_selection_rect(mouse_state);
  draw_debug_text(
      index++, "selection: current x,y: %.2f,%.2f target x,y: %.2f,%.2f", selection_rect.x, selection_rect.y, render_context.selection.target.x,
      render_context.selection.target.y
  );
}

void draw_selection_box(MouseState *mouse_state) {
  FRect selection_rect = get_selection_rect(mouse_state);

  if (selection_rect.w < 3.0f) {
    return;
  }

  SDL_FRect selection_rect_f = {
      .x = selection_rect.x,
      .y = selection_rect.y,
      .w = selection_rect.w,
      .h = selection_rect.h,
  };
  SDL_SetRenderDrawColor(render_context.renderer, 255, 255, 255, 255);
  int result = SDL_RenderDrawRectF(render_context.renderer, &selection_rect_f);
  assert(result == 0);
}

float Spring__update(Spring *spring, float target) {
  spring->target = target;
  spring->velocity += (target - spring->current) * spring->acceleration;
  spring->velocity *= spring->friction;
  return spring->current += spring->velocity;
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

    SDL_SetRenderDrawColor(render_context.renderer, 255, 255, 255, 255);
    SDL_FRect rect = create_world_to_screen_rect(&borders[i]);
    SDL_RenderFillRectF(render_context.renderer, &rect);
  }
}

void update_entity(int entity_id) {
  game_context.rect[entity_id].x += game_context.direction[entity_id].x * (render_context.delta_time * render_context.speed);
  game_context.rect[entity_id].y += game_context.direction[entity_id].y * (render_context.delta_time * render_context.speed);
}

void render_entity(int entity_id) {
  SDL_FRect rendering_rect = create_world_to_screen_rect(&game_context.rect[entity_id]);

  draw_texture(game_context.image[entity_id], &rendering_rect);

  if (game_context.selected[entity_id]) {
    draw_border(game_context.rect[entity_id], 5.0f / render_context.camera.zoom, 4.0f / render_context.camera.zoom);
  }
}

Image Image__load(const char *texture_file_path) {
  SDL_Surface *surface = IMG_Load(texture_file_path);
  assert(surface);

  SDL_Texture *texture = SDL_CreateTextureFromSurface(render_context.renderer, surface);
  assert(texture);

  Image image = (Image){
      .h = surface->h,
      .w = surface->w,
      .texture = texture,
  };

  return image;
}

void draw_grid() {
  // Draw it blended
  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(render_context.renderer, 0, 0, 0, 25);
  float grid_size = 100.0f;
  float window_w = (float)render_context.window_w;
  float window_h = (float)render_context.window_h;

  SDL_FRect grid_to_screen = create_world_to_screen_rect(&(FRect){
      .w = grid_size,
      .h = grid_size,
      .x = 0,
      .y = 0,
  });

  float x_start = grid_to_screen.x - floorf(grid_to_screen.x / grid_to_screen.w) * grid_to_screen.w;
  for (float x = x_start; x < window_w; x += grid_to_screen.w) {
    SDL_RenderDrawLineF(render_context.renderer, x, 0, x, window_h);
  }

  float y_start = grid_to_screen.y - floorf(grid_to_screen.y / grid_to_screen.h) * grid_to_screen.h;
  for (float y = y_start; y < window_h; y += grid_to_screen.h) {
    SDL_RenderDrawLineF(render_context.renderer, 0, y, window_w, y);
  }

  // Reset the blend mode
  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_NONE);
}

void clear_screen() {
  SDL_SetRenderDrawColor(
      render_context.renderer, render_context.background_color.r, render_context.background_color.g, render_context.background_color.b,
      render_context.background_color.a
  );
  SDL_RenderClear(render_context.renderer);
}

void mouse_control_camera(MouseState *mouse_state) {
  if (mouse_state->button == SDL_BUTTON_RIGHT && mouse_state->state == SDL_PRESSED) {
    if (mouse_state->prev_position.x != mouse_state->position.x || mouse_state->prev_position.y != mouse_state->position.y) {
      float delta_x = mouse_state->position.x - mouse_state->prev_position.x;
      float delta_y = mouse_state->position.y - mouse_state->prev_position.y;
      mouse_state->prev_position.x = mouse_state->position.x;
      mouse_state->prev_position.y = mouse_state->position.y;

      render_context.camera.target.x -= delta_x / render_context.camera.zoom;
      render_context.camera.target.y -= delta_y / render_context.camera.zoom;
    }
  }
}

// Camera movement and selection rect movement
void keyboard_control_camera() {
  float camera_keyboard_movement_speed = 5.0f;
  if (render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_w)]) {
    render_context.camera.target.y -= camera_keyboard_movement_speed / render_context.camera.zoom;
    render_context.selection.target.y += camera_keyboard_movement_speed;
  }
  if (render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_s)]) {
    render_context.camera.target.y += camera_keyboard_movement_speed / render_context.camera.zoom;
    render_context.selection.target.y -= camera_keyboard_movement_speed;
  }
  if (render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_a)]) {
    render_context.camera.target.x -= camera_keyboard_movement_speed / render_context.camera.zoom;
    render_context.selection.target.x += camera_keyboard_movement_speed;
  }
  if (render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_d)]) {
    render_context.camera.target.x += camera_keyboard_movement_speed / render_context.camera.zoom;
    render_context.selection.target.x -= camera_keyboard_movement_speed;
  }
}

int get_entity_to_follow() {
  int result = INVALID_ENTITY;
  int selected_count = 0;
  entity_loop(entity_i) {
    if (game_context.selected[entity_i]) {
      selected_count += 1;
      result = entity_i;
    }
  }
  return selected_count == 1 ? result : INVALID_ENTITY;
}

// Set the camera to follow an entity, if only one entity is selected
void camera_follow_entity() {
  int to_follow = get_entity_to_follow();
  if (to_follow != INVALID_ENTITY) {
    render_context.camera.target.x = game_context.rect[to_follow].x + game_context.rect[to_follow].w / 2;
    render_context.camera.target.y = game_context.rect[to_follow].y + game_context.rect[to_follow].h / 2;
  }
}

// Set selected on any entity within the selection_rect
void select_entities_within_selection_rect(MouseState *mouse_state) {
  entity_loop(entity_i) {
    SDL_FRect rect = create_world_to_screen_rect(&game_context.rect[entity_i]);
    SDL_FPoint point_top_left = {
        .x = rect.x,
        .y = rect.y,
    };
    SDL_FPoint point_bottom_right = {
        .x = rect.x + rect.w,
        .y = rect.y + rect.h,
    };

    // If the selection rect is bigger than 3 pixels, select the entity if it's within the selection rect
    FRect selection_rect = get_selection_rect(mouse_state);
    SDL_FRect sdl_frect = {
        .x = selection_rect.x,
        .y = selection_rect.y,
        .w = selection_rect.w,
        .h = selection_rect.h,
    };
    if (selection_rect.w > 3.0f) {
      if (SDL_PointInFRect(&point_top_left, &sdl_frect) && SDL_PointInFRect(&point_bottom_right, &sdl_frect)) {
        game_context.selected[entity_i] = true;
      } else {
        if (!render_context.keyboard_state[SDL_GetScancodeFromKey(SDLK_LSHIFT)]) {
          game_context.selected[entity_i] = false;
        }
      }
    }
  }
}

bool entity_collides_rect(int entity_id, FRect *rect) {
  SDL_FRect rect_to_sdl_frect = {
      .w = rect->w,
      .y = rect->y,
      .x = rect->x,
      .h = rect->h,
  };
  SDL_FRect screen_entity = create_world_to_screen_rect(&game_context.rect[entity_id]);

  return SDL_HasIntersectionF(&rect_to_sdl_frect, &screen_entity);
}

bool entity_under_mouse(int entity_id, MouseState *mouse_state) {
  SDL_FRect rect = create_world_to_screen_rect(&game_context.rect[entity_id]);

  return SDL_PointInFRect(
      &(SDL_FPoint){
          .x = mouse_state->position.x,
          .y = mouse_state->position.y,
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

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
  SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
}

void log_entity_personalities(int entity_id) {
  for (int personality_i = 0; personality_i < Personality_Count; personality_i++) {
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

  SDL_Window *window = SDL_CreateWindow(
      "Cultivation Sim", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
  );
  if (!window) {
    fprintf(stderr, "could not create window: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (!renderer) {
    fprintf(stderr, "could not create renderer: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  render_context.renderer = renderer;
  render_context.animated_time = 0;
  render_context.speed = 200.0f;
  render_context.delta_time = 0;
  render_context.background_color = (SDL_Color){35, 127, 178, 255};
  render_context.camera = (Camera){
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
  };
  render_context.selection =
      (Selection){
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
  render_context.images = (Image[]){
      Image__load("assets/stone.bmp"),
      Image__load("assets/fish.bmp"),
      Image__load("assets/lamb.bmp"),
      Image__load("assets/lamb2.bmp"),
  };

  init_japanese_character_sets(HIRAGANA_BIT | KATAKANA_BIT);

  init_latin_character_sets(BASIC_LATIN_BIT | LATIN_ONE_SUPPLEMENT_BIT);

  FontLoadParams font_parameters = {0};
  font_parameters.size = 24;
  font_parameters.renderer = render_context.renderer;
  font_parameters.character_sets = BASIC_LATIN_BIT | LATIN_ONE_SUPPLEMENT_BIT;
  font_parameters.outline_size = 1;

  render_context.fonts[0] = load_font("assets/OpenSans-Regular.ttf", font_parameters);
  font_parameters.size = 32;
  render_context.fonts[1] = load_font("assets/OpenSans-Regular.ttf", font_parameters);

  MouseState mouse_state = {0};

  create_entities();

  int game_is_still_running = 1;
  u32 start_ticks = SDL_GetTicks();
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
      if (event.type == SDL_MOUSEWHEEL) {
        if (event.wheel.y > 0) {
          // zoom in
          render_context.camera.target_zoom = min(render_context.camera.target_zoom + 0.1f, 2.0f);
        } else if (event.wheel.y < 0) {
          // zoom out
          render_context.camera.target_zoom = max(render_context.camera.target_zoom - 0.1f, 0.1f);
        }
      }
      if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            bool was_something_selected = false;

            reverse_entity_loop(entity_i) {
              if (game_context.selected[entity_i]) {
                was_something_selected = true;
                game_context.selected[entity_i] = false;
              }
            }
            if (!was_something_selected) {
              game_is_still_running = 0;
            }
            // Maybe the following process:
            // 1. If anything is selected, then deselect it and break.
            // 2. If nothing was deselected, then open the pause menu.
            // 3. If in the pause menu, then close the pause menu.
            break;

          case SDLK_UP:
            render_context.speed += 100.0f;
            break;

          case SDLK_DOWN:
            render_context.speed = max(render_context.speed - 100.0f, 0);
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
      reverse_entity_loop(entity_i) {
        game_context.hovered[entity_i] = entity_under_mouse(entity_i, &mouse_state);
      }

      reverse_entity_loop(entity_i) {
        if (entity_under_mouse(entity_i, &mouse_state)) {
          if (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_RELEASED) {
            game_context.selected[entity_i] = !game_context.selected[entity_i];
            log_entity_personalities(entity_i);
            break;
          }
        }
      }
    }

    mouse_control_camera(&mouse_state);

    keyboard_control_camera();

    if (mouse_primary_pressed(mouse_state)) {
      select_entities_within_selection_rect(&mouse_state);
    } else {
      camera_follow_entity();
    }

    // Spring the selection box
    render_context.selection.position.x = Spring__update(&render_context.selection.spring_x, render_context.selection.target.x);
    render_context.selection.position.y = Spring__update(&render_context.selection.spring_y, render_context.selection.target.y);

    // Spring the camera position
    render_context.camera.current.x = Spring__update(&render_context.camera.pan_spring_x, render_context.camera.target.x);
    render_context.camera.current.y = Spring__update(&render_context.camera.pan_spring_y, render_context.camera.target.y);

    clear_screen();

    draw_grid();

    if (render_context.speed > 0.0f) {
      entity_loop(entity_i) {
        update_entity(entity_i);
      }
    }

    FRect camera_rect = (FRect){.w = (float)render_context.window_w, .h = (float)render_context.window_h, .x = 0, .y = 0};
    entity_loop(entity_i) {
      if (entity_collides_rect(entity_i, &camera_rect)) {
        render_entity(entity_i);
      }
    }

    if (render_context.camera.zoom > 0.5f) {
      entity_loop(entity_i) {
        if (entity_collides_rect(entity_i, &camera_rect)) {
          draw_entity_name(entity_i);
        }
      }
    }

    if (mouse_primary_pressed(mouse_state)) {
      // Draw the selection box
      draw_selection_box(&mouse_state);
    }

    render_debug_info(&mouse_state);

    SDL_RenderPresent(render_context.renderer);
  }

  SDL_DestroyRenderer(render_context.renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return EXIT_SUCCESS;
}
