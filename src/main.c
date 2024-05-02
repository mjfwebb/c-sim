#define SDL_MAIN_HANDLED

#include "headers.h"

#include "defs.c"
#include "fonts.c"
#include "render_batcher.c"
#include "personalities.c"
#include "seed.c"
#include "console.c"

#define VA_ARGS(...) , ##__VA_ARGS__  // For variadic macros
#define mouse_primary_pressed(mouse_state) \
  (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_PRESSED)

typedef struct {
  int prev_state;
  int state;
  int button;
  Vec2 position;
  Vec2 prev_position;
  int clicks;
} MouseState;

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

RenderBatcher render_batcher = {0};
MouseState mouse_state = {0};
int game_is_still_running = 1;

bool entity_has_personality(int entity_index, Personality personality) {
  return game_context.personalities[entity_index][personality] > 0;
}

Vec2 vec2_world_to_screen(Vec2 point) {
  Vec2 translated_point;
  translated_point.x = (point.x - render_context.camera.current.x) * render_context.camera.zoom + render_context.window_w * 0.5f;
  translated_point.y = (point.y - render_context.camera.current.y) * render_context.camera.zoom + render_context.window_h * 0.5f;
  return translated_point;
}

Vec2 vec2_screen_to_world(Vec2 point) {
  Vec2 translated_point;
  translated_point.x = (point.x - render_context.window_w * 0.5f) / render_context.camera.zoom + render_context.camera.current.x;
  translated_point.y = (point.y - render_context.window_h * 0.5f) / render_context.camera.zoom + render_context.camera.current.y;
  return translated_point;
}

FRect frect_world_to_screen(FRect rect) {
  FRect translated_frect;
  translated_frect.position = vec2_world_to_screen(rect.position);
  translated_frect.size = vec2_world_to_screen(rect.size);
  return translated_frect;
}

FRect frect_screen_to_world(FRect rect) {
  FRect translated_frect;
  translated_frect.position = vec2_screen_to_world(rect.position);
  translated_frect.size = vec2_screen_to_world(rect.size);
  return translated_frect;
}

SDL_FRect frect_to_sdl_frect(FRect *rect) {
  SDL_FRect sdl_frect = {
      .x = rect->position.x,
      .y = rect->position.y,
      .w = frect_width(rect),
      .h = frect_width(rect),
  };

  return sdl_frect;
}

bool frect_intersects_frect(FRect *rect_a, FRect *rect_b) {
  SDL_FRect sdl_frect_a = frect_to_sdl_frect(rect_a);
  SDL_FRect sdl_frect_b = frect_to_sdl_frect(rect_b);

  return SDL_HasIntersectionF(&sdl_frect_a, &sdl_frect_b);
}

void load_textures() {
  // Eventually you could consider dynamically loading files from the assets folder. Implementing this is, however,
  // unfortunately platform dependent so it would also require some type of platform abstraction to be viable.
  char texture_paths[][128] = {"assets/stone.bmp", "assets/fish.bmp", "assets/lamb.bmp", "assets/lamb2.bmp"};

  for (u32 i = 0; i < array_count(texture_paths); i++) {
    SDL_Surface *surface = IMG_Load(texture_paths[i]);
    render_context.texture_atlas.size[i].x = (float)surface->w;
    render_context.texture_atlas.size[i].y = (float)surface->h;
    render_context.texture_atlas.textures[i] = SDL_CreateTextureFromSurface(render_context.renderer, surface);
    assert(render_context.texture_atlas.textures[i]);
    ++render_context.texture_atlas.count;
    SDL_FreeSurface(surface);
  }
}

void load_fonts() {
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
}

void create_entity(char *name) {
  float entity_width = 100.0f;
  int texture_id = random_int_between(0, render_context.texture_atlas.count);
  game_context.textures[game_context.entity_count] = (TextureComponent){.texture_id = texture_id, .size = {.x = entity_width}};

  float scale = entity_width / render_context.texture_atlas.size[texture_id].x;
  game_context.textures[game_context.entity_count].size.y = (float)(render_context.texture_atlas.size[texture_id].y * scale);

  game_context.health[game_context.entity_count] = random_int_between(10, 100);
  strcpy(game_context.names[game_context.entity_count], name);  // FIXME: Use the safe version strcpy_s. PRs welcome

  game_context.selected[game_context.entity_count] = false;
  game_context.hovered[game_context.entity_count] = false;
  game_context.positions[game_context.entity_count] = (PositionComponent){
      .current_position =
          {
              .x = (float)random_int_between(-1000, 1000),
              .y = (float)random_int_between(-1000, 1000),
          },
  };
  game_context.positions[game_context.entity_count].previous_position = game_context.positions[game_context.entity_count].current_position;
  game_context.speeds[game_context.entity_count] = (SpeedComponent){
      .current_direction.x = (((float)(rand() % 400) - 200) / 100),
      .current_direction.y = (((float)(rand() % 400) - 200) / 100),
      .current_velocity = (float)random_int_between(40, 55),
  };

  int random_amount_of_personalities = random_int_between(5, 10);
  for (int i = 0; i < random_amount_of_personalities; i++) {
    int personality = random_int_between(0, Personality_Count);
    game_context.personalities[game_context.entity_count][personality] = random_int_between(0, 100);
  }

  game_context.entity_count++;
}

void create_entities() {
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
      "kasper_573",
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
      "Lolboy_30",
      "VevenVelour",
      "Kisamius",
      "tobias_bms",
      "spectral_ray1",
      "Toasty",  // AKA CarbonCollins
      "Roilisi",
      "MickyMaven",
      "Katsuida",
      "YogiEisbar",
      "WaryOfDairy",
      "BauBas9883",
      "Kataemoi",
      "AgentulSRI"
  };

  for (int name_index = 0; name_index < array_count(entity_names); name_index++) {
    create_entity(entity_names[name_index]);
  }
}

SDL_FRect create_world_to_screen_rect(FRect *world_rect) {
  float width = frect_width(world_rect);
  float height = frect_height(world_rect);

  SDL_FRect rect = {
      .x = (world_rect->position.x - render_context.camera.current.x) * render_context.camera.zoom + render_context.window_w / 2,
      .y = (world_rect->position.y - render_context.camera.current.y) * render_context.camera.zoom + render_context.window_h / 2,
      .w = width * render_context.camera.zoom,
      .h = height * render_context.camera.zoom,
  };

  return rect;
}

FRect get_camera_rect() {
  FRect camera_rect = {
      .position =
          {
              .x = -300.0f,
              .y = -300.0f,
          },
      .size =
          {
              .x = (float)render_context.window_w,
              .y = (float)render_context.window_h,
          },
  };

  return camera_rect;
}

FRect get_entity_texture_rect(int entity_id) {
  FRect texture_rect = {
      .position =
          {.x = game_context.positions[entity_id].current_position.x * (float)physics_context.alpha +
                game_context.positions[entity_id].previous_position.x * (float)(1.0 - physics_context.alpha),
           .y = game_context.positions[entity_id].current_position.y * (float)physics_context.alpha +
                game_context.positions[entity_id].previous_position.y * (float)(1.0 - physics_context.alpha)}
  };
  texture_rect.size.x = texture_rect.position.x + game_context.textures[entity_id].size.x;
  texture_rect.size.y = texture_rect.position.y + game_context.textures[entity_id].size.y;

  return texture_rect;
}

void draw_entity_name_batched(int entity_id, RenderBatcher *batcher) {
  Font *font = &render_context.fonts[0];
  RGBA color = (RGBA){1, 1, 1, 1};
  FRect entity_texture_rect = get_entity_texture_rect(entity_id);
  FRect entity_screen_rect = frect_world_to_screen(entity_texture_rect);

  float y = (entity_screen_rect.position.y - (45.0f));

  if (game_context.hovered[entity_id]) {
    y -= 10.0f;  // move the text up a little when using the bigger font
    color = (RGBA){1, 1, 0, 1};
    font = &render_context.fonts[1];
  }

  Vec2 text_size = get_text_size(game_context.names[entity_id], font, false, true);

  float diff = ((entity_screen_rect.size.x - entity_screen_rect.position.x) / 2) - (text_size.x / 2);
  float x = entity_screen_rect.position.x + diff;

  draw_text_outlined_utf8_batched(game_context.names[entity_id], (Vec2){x, y}, color, (RGBA){0, 0, 0, 1}, font, batcher);
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

FRect get_selection_rect() {
  return (FRect
  ){.position =
        {
            .x = min(mouse_state.position.x, render_context.selection.position.x),
            .y = min(mouse_state.position.y, render_context.selection.position.y),
        },
    .size = {
        .x = SDL_fabsf(mouse_state.position.x - render_context.selection.position.x),
        .y = SDL_fabsf(mouse_state.position.y - render_context.selection.position.y),
    }};
}

void render_debug_info() {
  int index = 0;
  draw_debug_text(index++, "fps: %.2f", render_context.fps);
  draw_debug_text(index++, "mouse state: %d, button: %d, clicks: %d", mouse_state.state, mouse_state.button, mouse_state.clicks);
  draw_debug_text(index++, "prev mouse state: %d", mouse_state.prev_state);
  draw_debug_text(index++, "camera zoom: %.1f", render_context.camera.target_zoom);
  draw_debug_text(index++, "game speed: %.1f", physics_context.simulation_speed);
  draw_debug_text(
      index++, "camera: current x,y: %.2f,%.2f target x,y: %.2f,%.2f", render_context.camera.current.x, render_context.camera.current.y,
      render_context.camera.target.x, render_context.camera.target.y
  );
  FRect selection_rect = get_selection_rect();
  draw_debug_text(
      index++, "selection: current x,y: %.2f,%.2f target x,y: %.2f,%.2f", selection_rect.position.x, selection_rect.position.y,
      render_context.selection.target.x, render_context.selection.target.y
  );
}

void draw_selection_box() {
  FRect selection_rect = get_selection_rect();

  SDL_FRect selection_rect_f = {
      .x = selection_rect.position.x,
      .y = selection_rect.position.y,
      .w = selection_rect.size.x,
      .h = selection_rect.size.y,
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

void draw_health_bar(int entity_id, FRect entity_rect) {
  int health = game_context.health[entity_id];

  const float y = (entity_rect.position.y - 15.0f * min(render_context.camera.zoom, 1.0f));
  const float h = (10.0f * min(render_context.camera.zoom, 1.0f));

  SDL_SetRenderDrawColor(render_context.renderer, 0, 0, 0, 255);
  SDL_FRect total_health_rect = {
      .x = entity_rect.position.x,
      .y = y,
      .w = frect_width(&entity_rect),
      .h = h,
  };

  SDL_RenderFillRectF(render_context.renderer, &total_health_rect);

  SDL_SetRenderDrawColor(render_context.renderer, 200, 0, 0, 255);
  SDL_FRect current_health_rect = {
      .x = entity_rect.position.x,
      .y = y,
      .w = (frect_width(&entity_rect) / 100) * health,
      .h = h,
  };

  SDL_RenderFillRectF(render_context.renderer, &current_health_rect);
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
      borders[i].size.x = border_width;
      borders[i].size.y = height + (gap_width + border_width) * 2;
      borders[i].position.x += -(gap_width + border_width);
      borders[i].position.y -= gap_width + border_width;
    } else if (i == 2) {  // right (2)
      borders[i].size.x = border_width;
      borders[i].size.y = height + (gap_width + border_width) * 2;
      borders[i].position.x = around.position.x + width + gap_width;
      borders[i].position.y -= gap_width + border_width;
    } else if (i == 1) {  // Top (1)
      borders[i].size.x = width + (gap_width + border_width) * 2;
      borders[i].size.y = border_width;
      borders[i].position.x -= gap_width + border_width;
      borders[i].position.y += -(gap_width + border_width);
    } else {  // bottom (3)
      borders[i].size.x = width + (gap_width + border_width) * 2;
      borders[i].size.y = border_width;
      borders[i].position.x -= gap_width + border_width;
      borders[i].position.y = around.position.y + height + gap_width;
    }

    SDL_SetRenderDrawColor(render_context.renderer, 255, 255, 255, 255);
    SDL_FRect rect = {
        .x = borders[i].position.x,
        .y = borders[i].position.y,
        .w = borders[i].size.x,
        .h = borders[i].size.y,
    };

    SDL_RenderFillRectF(render_context.renderer, &rect);
  }
}

void move_entity(int entity_id) {
  game_context.positions[entity_id].previous_position = game_context.positions[entity_id].current_position;
  game_context.speeds[entity_id].previous_direction = game_context.speeds[entity_id].current_direction;
  game_context.speeds[entity_id].previous_velocity = game_context.speeds[entity_id].current_velocity;
  game_context.positions[entity_id].current_position.x += game_context.speeds[entity_id].current_direction.x *
                                                          game_context.speeds[entity_id].current_velocity *
                                                          (float)(physics_context.delta_time * physics_context.simulation_speed);
  game_context.positions[entity_id].current_position.y += game_context.speeds[entity_id].current_direction.y *
                                                          game_context.speeds[entity_id].current_velocity *
                                                          (float)(physics_context.delta_time * physics_context.simulation_speed);
}

void render_entity_batched(int entity_id, RenderBatcher *batcher) {
  FRect entity_texture_rect = get_entity_texture_rect(entity_id);
  FRect entity_screen_rect = frect_world_to_screen(entity_texture_rect);

  RGBA color = {1, 1, 1, 1};
  render_batcher_copy_texture_quad(
      batcher, render_context.texture_atlas.textures[game_context.textures[entity_id].texture_id], &color, &entity_screen_rect, NULL
  );

  draw_health_bar(entity_id, entity_screen_rect);

  // FIXME: Make this faster using the render batcher
  if (game_context.selected[entity_id]) {
    draw_border(entity_screen_rect, 5.0f, 4.0f);
  }
}

void draw_grid() {
  // Draw it blended
  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(render_context.renderer, 0, 0, 0, 25);
  float grid_size = 100.0f;
  float window_w = (float)render_context.window_w;
  float window_h = (float)render_context.window_h;

  SDL_FRect grid_to_screen = create_world_to_screen_rect(&(FRect
  ){.position =
        {
            .x = 0,
            .y = 0,
        },
    .size = {
        .x = grid_size,
        .y = grid_size,
    }});

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

void mouse_control_camera() {
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

// Camera movement and selection rect movement
void keyboard_control_camera() {
  float camera_keyboard_movement_speed = 15.0f;
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
    render_context.camera.target.x = game_context.positions[to_follow].current_position.x + (game_context.textures[to_follow].size.x / 2);
    render_context.camera.target.y = game_context.positions[to_follow].current_position.y + (game_context.textures[to_follow].size.y / 2);
  }
}

// Set selected on any entity within the selection_rect
void select_entities_within_selection_rect() {
  entity_loop(entity_id) {
    FRect entity_texture_rect = get_entity_texture_rect(entity_id);
    FRect entity_screen_rect = frect_world_to_screen(entity_texture_rect);

    SDL_FPoint point_top_left = {
        .x = entity_screen_rect.position.x,
        .y = entity_screen_rect.position.y,
    };
    SDL_FPoint point_bottom_right = {
        .x = entity_screen_rect.size.x,
        .y = entity_screen_rect.size.y,
    };

    FRect selection_rect = get_selection_rect();
    SDL_FRect selection_sdl_frect = {
        .x = selection_rect.position.x,
        .y = selection_rect.position.y,
        .w = selection_rect.size.x,
        .h = selection_rect.size.y,
    };

    if (selection_rect.size.x > 3.0f) {
      if (SDL_PointInFRect(&point_top_left, &selection_sdl_frect) && SDL_PointInFRect(&point_bottom_right, &selection_sdl_frect)) {
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
  FRect entity_texture_rect = get_entity_texture_rect(entity_id);
  SDL_FRect rect = create_world_to_screen_rect(&entity_texture_rect);

  return SDL_PointInFRect(
      &(SDL_FPoint){
          .x = mouse_state.position.x,
          .y = mouse_state.position.y,
      },
      &rect
  );
}

void init() {
  srand(create_seed("ATHANO_THINKS_CHAT_IS_KINDA_CUTE"));

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
    if (entity_has_personality(entity_id, personality_i)) {
      print(
          "Entity %s has personality %s with value %d", game_context.names[entity_id], Personality__Strings[personality_i],
          game_context.personalities[entity_id][personality_i]
      );
    }
  }
}

void update() {
  // Spring the camera zoom
  render_context.camera.zoom = Spring__update(&render_context.camera.zoom_spring, render_context.camera.target_zoom);

  mouse_control_camera(&mouse_state);

  if (!console_is_open) {
    keyboard_control_camera();
  }

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

  // Spring the console position
  console.y = Spring__update(&console.y_spring, console.target_y);

  if (physics_context.simulation_speed > 0.0) {
    entity_loop(entity_i) {
      move_entity(entity_i);
    }
  }
}

void handle_input() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_TEXTINPUT && console_is_open) {
      append_console_input(event.text.text);
    }

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
          if (console_is_open) {
            SDL_StopTextInput();
            console.target_y = 0.0f;
            break;
          }

          bool entity_had_selection = false;

          reverse_entity_loop(entity_i) {
            if (game_context.selected[entity_i]) {
              entity_had_selection = true;
              game_context.selected[entity_i] = false;
            }
          }

          if (entity_had_selection) {
            break;
          }

          game_context.game_is_still_running = 0;

          // Maybe the following process:
          // 1. If anything is selected, then deselect it and break.
          // 2. If nothing was deselected, then open the pause menu.
          // 3. If in the pause menu, then close the pause menu.
          break;
        case SDLK_UP:
          if (console_is_open) {
            int new_index = (console.input_index - 1);
            if (new_index < 0) {
              new_index = MAX_CONSOLE_INPUT_HISTORY - 1;
            }

            bool current_index_has_value = console.input[console.input_index].input_length > 0;
            bool next_index_has_value = console.input[new_index].input_length > 0;

            if (!current_index_has_value && !next_index_has_value) {
              break;
            }

            console.input_index = new_index;
          } else {
            physics_context.simulation_speed += 0.5;
            physics_context.simulation_speed = min(physics_context.simulation_speed, 10.0);
          }
          break;
        case SDLK_DOWN:
          if (console_is_open) {
            int new_index = console.input_index + 1;
            if (new_index == MAX_CONSOLE_INPUT_HISTORY) {
              new_index = 0;
            }

            bool current_index_has_value = console.input[console.input_index].input_length > 0;
            bool next_index_has_value = console.input[new_index].input_length > 0;

            if (!current_index_has_value && !next_index_has_value) {
              break;
            }

            console.input_index = new_index;
          } else {
            physics_context.simulation_speed -= 0.5;
            physics_context.simulation_speed = max(physics_context.simulation_speed, 0.0);
          }
          break;
        case SDLK_SPACE:
          if (console_is_open) {
            break;
          }
          if (physics_context.prev_simulation_speed > 0) {
            physics_context.simulation_speed = physics_context.prev_simulation_speed;
            physics_context.prev_simulation_speed = 0;
          } else {
            physics_context.prev_simulation_speed = physics_context.simulation_speed;
            physics_context.simulation_speed = 0;
          }
          break;
        case SDLK_TAB:
          if (console_is_open) {
            if (console.input[console.input_index].input_length > 0) {
              char *suggested_command = find_command_suggestion();
              if (suggested_command) {
                strcpy(console.input[console.input_index].value, suggested_command);
                console.input[console.input_index].input_length = (int)strlen(suggested_command);
                break;
              }

              CommandArgsSuggestions suggested_command_argument = find_command_suggestion_argument();
              // Figure out what the current argument is (we can assume 1st for now)
              // Concat the existing input (removing any arg values) with the accepted suggestion

              if (suggested_command_argument.count > 0) {
                char *current_command = find_current_command();
                // print("%s", current_command);

                // print("%s", suggested_command_argument.suggestions[0]);

                strcpy(console.input[console.input_index].value + strlen(current_command) + 1, suggested_command_argument.suggestions[0]);
                console.input[console.input_index].input_length =
                    (int)strlen(current_command) + 1 + (int)strlen(suggested_command_argument.suggestions[0]);
              }
              break;
            }

            SDL_StopTextInput();
            console.target_y = 0.0f;
          } else {
            SDL_StartTextInput();
            console.target_y = (float)render_context.window_h / 2;
          }
          break;
        case SDLK_RETURN: {
          if (console_is_open) {
            handle_console_input();
          };
          break;
        }
        case SDLK_BACKSPACE: {
          if (console_is_open) {
            if (console.input[console.input_index].input_length > 0) {
              console.input[console.input_index].input_length--;
              console.input[console.input_index].value[console.input[console.input_index].input_length] = 0;
            }
          };
          break;
        }
        default:
          break;
      }

      // Handle copy
      // if (event.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL) {
      //   SDL_SetClipboardText(inputText.c_str());
      // }
      // Handle paste
      if (event.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL) {
        char *clipboard_text = SDL_GetClipboardText();
        append_console_input(clipboard_text);
        SDL_free(clipboard_text);
      }

    } else if (event.type == SDL_QUIT) {
      game_context.game_is_still_running = 0;
    }

    // Two loops needed so we can have a case where multiple entities can be hovered over, but only one can be selected
    reverse_entity_loop(entity_i) {
      game_context.hovered[entity_i] = is_entity_under_mouse(entity_i);
    }

    reverse_entity_loop(entity_i) {
      if (is_entity_under_mouse(entity_i)) {
        if (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_RELEASED) {
          game_context.selected[entity_i] = !game_context.selected[entity_i];
          log_entity_personalities(entity_i);
          break;
        }
      }
    }
  }
}

void render() {
  clear_screen();

  draw_grid();

  FRect camera_rect = get_camera_rect();
  FRect translated_rect = frect_screen_to_world(camera_rect);

  entity_loop(entity_id) {
    FRect entity_texture_rect = get_entity_texture_rect(entity_id);
    if (frect_intersects_frect(&entity_texture_rect, &translated_rect)) {
      render_entity_batched(entity_id, &render_batcher);
    }
  }

  if (render_context.camera.zoom > 0.5f) {
    entity_loop(entity_id) {
      FRect entity_texture_rect = get_entity_texture_rect(entity_id);
      if (frect_intersects_frect(&entity_texture_rect, &translated_rect)) {
        draw_entity_name_batched(entity_id, &render_batcher);
      }
    }
  }

  flush_render_batcher(&render_batcher);

  if (mouse_primary_pressed(mouse_state)) {
    // Draw the selection box
    draw_selection_box(&mouse_state);
  }

  render_debug_info(&mouse_state);

  draw_console();
}

void update_timer(Timer *timer, double frame_time) {
  timer->accumulated += frame_time;
  if (timer->accumulated >= timer->interval) {
    timer->accumulated -= timer->interval;
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

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  if (!renderer) {
    fprintf(stderr, "could not create renderer: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  render_context.renderer = renderer;
  // render_context.animated_time = 0;
  // render_context.speed = 200.0f;
  // render_context.delta_time = 0;
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

  load_textures();

  load_fonts();

  console.y_spring = (Spring){
      .target = 0.0f,
      .current = 0.0f,
      .velocity = 0.1f,
      .acceleration = 0.5f,
      .friction = 0.1f,
  };

  game_context.game_is_still_running = 1;

  create_entities();

  render_batcher = new_render_batcher(1000000, render_context.renderer);

  physics_context = (PhysicsContext){.delta_time = 0.01, .simulation_speed = 1.0};

  u32 start_ticks = SDL_GetTicks();
  // int current_time = 0;
  int frame_count = 0;
  // int last_update_time = 0;

  // Here we keep the frame_time within a reasonable bound. If a frame_time exceeds 250ms, we "give up" and drop simulation frames
  // This is necessary as if our frame_time were to become too large, we would effectively lock ourselves in an update cycle
  // and the simulation would fall completely out of sync with the physics being rendered
  float max_frame_time_threshold = 0.25;
  double accumulator = 0.0;
  double current_time = SDL_GetTicks64() / 1000.0;

  render_context.timer[0] = (Timer){.interval = 100};  // Second timer
  render_context.timer[1] = (Timer){.interval = 60000};  // Minute timer

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

    // current_time = SDL_GetTicks();
    // last_update_time = current_time;
    // render_context.animated_time = fmodf(render_context.animated_time + (float)(current_time - last_update_time) / 1000 * 0.5f, 1);

    SDL_GetWindowSizeInPixels(window, &render_context.window_w, &render_context.window_h);
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

    physics_context.alpha = fmin(accumulator / physics_context.delta_time, 1.0);

    render();

    SDL_RenderPresent(render_context.renderer);
  }

  SDL_DestroyRenderer(render_context.renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return EXIT_SUCCESS;
}
