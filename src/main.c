#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "seed.c"

#define VA_ARGS(...) , ##__VA_ARGS__
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

typedef struct Spring
{
  float target;
  float current;
  float velocity;
  float acceleration;
  float friction;
} Spring;

typedef struct
{
  SDL_Texture *texture;
  rect_fields;
  point_fields;
  float direction_x;
  float direction_y;
  bool selected;
  char *name;
} Entity;

typedef struct
{
  SDL_Texture *texture;
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

typedef struct
{
  point_fields;
  target_point_fields;
  Spring spring_x;
  Spring spring_y;
  SDL_FRect rect;
} Selection;

typedef struct
{
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
} RenderContext;

typedef struct
{
  int prev_state;
  int state;
  int button;
  point_fields;
  float prev_x;
  float prev_y;
  int clicks;
} MouseState;

void render_entity(RenderContext *render_context, Entity *entity);

SDL_FRect camera_relative_rect(RenderContext *render_context, SDL_FRect *source_rect)
{
  SDL_FRect rect = {
      .w = source_rect->w * render_context->camera.zoom,
      .h = source_rect->h * render_context->camera.zoom,
      .x = ((source_rect->x - render_context->camera.x) * render_context->camera.zoom + render_context->window_w / 2),
      .y = ((source_rect->y - render_context->camera.y) * render_context->camera.zoom + render_context->window_h / 2),
  };

  return rect;
};

Entity Entity__create(RenderContext *render_context, char *image_path, char *name)
{
  SDL_Surface *image = SDL_LoadBMP(image_path);
  assert(image);

  SDL_Texture *texture = SDL_CreateTextureFromSurface(render_context->renderer, image);
  assert(texture);

  float width = 100.0f;
  float scale = width / image->w;
  float height = (float)(image->h * scale);

  Entity entity = {
      .direction_x = ((float)(rand() % 200) - 100) / 100,
      .direction_y = ((float)(rand() % 200) - 100) / 100,
      .x = (float)(rand() % 2000) - 1000,
      .y = (float)(rand() % 2000) - 1000,
      .texture = texture,
      .h = height,
      .w = width,
      .selected = false,
      .name = name,
  };

  // Surface no longer needed after the texture is created
  SDL_FreeSurface(image);

  return entity;
}

void draw_texture(RenderContext *render_context, Entity *entity, SDL_FRect *rendering_rect)
{
  int copy_result = SDL_RenderCopyF(render_context->renderer, entity->texture, NULL, rendering_rect);
  if (copy_result != 0)
  {
    printf("Failed to render copy: %s\n", SDL_GetError());
    return;
  }
}

void draw_entity_name(RenderContext *render_context, Entity *entity)
{
  TTF_Font *font = render_context->fonts[1];
  SDL_Color White = {255, 255, 255};
  SDL_Surface *text_surface = TTF_RenderText_Blended(font, entity->name, White);
  if (!text_surface)
  {
    fprintf(stderr, "could not create text surface: %s\n", SDL_GetError());
  }

  SDL_Texture *text_texture = SDL_CreateTextureFromSurface(render_context->renderer, text_surface);
  if (!text_texture)
  {
    fprintf(stderr, "could not create text texture: %s\n", SDL_GetError());
  }

  float diff = ((entity->w * render_context->camera.zoom) - text_surface->w) / 2;
  float x = (((entity->x - render_context->camera.x) * render_context->camera.zoom) + diff) + render_context->window_w / 2;

  SDL_FRect text_rect = {
      .w = (float)text_surface->w,
      .h = (float)text_surface->h,
      .x = x,
      .y = (entity->y - render_context->camera.y - (45.0f / render_context->camera.zoom)) * render_context->camera.zoom + render_context->window_h / 2,
  };

  SDL_RenderCopyF(render_context->renderer, text_texture, NULL, &text_rect);
  SDL_FreeSurface(text_surface);
  SDL_DestroyTexture(text_texture);
}

void draw_debug_text(RenderContext *render_context, int index, char *str, ...)
{
  char text_buffer[128];
  va_list args;
  va_start(args, str);
  int chars_written = vsnprintf(text_buffer, sizeof(text_buffer), str, args);
  assert(chars_written > 0);
  va_end(args);

  TTF_Font *font = render_context->fonts[0];
  SDL_Color White = {255, 255, 255};
  SDL_Surface *text_surface = TTF_RenderText_Blended(font, text_buffer, White);
  if (!text_surface)
  {
    fprintf(stderr, "could not create text surface: %s\n", SDL_GetError());
  }

  SDL_FRect text_rect = {
      .w = (float)text_surface->w,
      .h = (float)text_surface->h,
      .x = 10.0f,
      .y = 10.0f + (32.0f * index),
  };

  SDL_Texture *text_texture = SDL_CreateTextureFromSurface(render_context->renderer, text_surface);
  if (!text_texture)
  {
    fprintf(stderr, "could not create text texture: %s\n", SDL_GetError());
  }

  SDL_RenderCopyF(render_context->renderer, text_texture, NULL, &text_rect);
  SDL_FreeSurface(text_surface);
  SDL_DestroyTexture(text_texture);
}

void render_debug_info(RenderContext *render_context, MouseState *mouse_state)
{
  int index = 0;
  draw_debug_text(render_context, index++, "fps: %.2f", render_context->fps);
  draw_debug_text(render_context, index++, "mouse state: %d, button: %d, clicks: %d", mouse_state->state, mouse_state->button, mouse_state->clicks);
  draw_debug_text(render_context, index++, "prev mouse state: %d", mouse_state->prev_state);
  draw_debug_text(render_context, index++, "camera zoom: %.1f", render_context->camera.target_zoom);
  draw_debug_text(render_context, index++, "game speed: %.1f", render_context->speed);
  draw_debug_text(render_context, index++, "camera: current x,y: %.2f,%.2f target x,y: %.2f,%.2f", render_context->camera.x, render_context->camera.y, render_context->camera.target_x, render_context->camera.target_y);
  draw_debug_text(render_context, index++, "selection: current x,y: %.2f,%.2f target x,y: %.2f,%.2f", render_context->selection.x, render_context->selection.y, render_context->selection.target_x, render_context->selection.target_y);
}

void render_selection_box(RenderContext *render_context)
{
  SDL_SetRenderDrawColor(render_context->renderer, 255, 255, 255, 255);
  int result = SDL_RenderDrawRectF(render_context->renderer, &render_context->selection.rect);
  assert(result == 0);
}

float Spring__update(Spring *spring, float target)
{
  spring->target = target;
  spring->velocity += (target - spring->current) * spring->acceleration;
  spring->velocity *= spring->friction;
  return spring->current += spring->velocity;
}

void draw_border(RenderContext *render_context, SDL_FRect around, float gap_width, float border_width)
{
  SDL_FRect borders[4];

  //         1
  //   |-----------|
  //   |           |
  // 0 |           | 2
  //   |           |
  //   |-----------|
  //         3
  for (int i = 0; i < 4; i++)
  {
    borders[i] = around;

    if (i % 2 == 0)
    { // Left (0) and right (2)
      borders[i].w = border_width;
      borders[i].h += (gap_width + border_width) * 2;
      borders[i].x += (i == 2 ? around.w + gap_width : -(gap_width + border_width));
      borders[i].y -= gap_width + border_width;
    }
    else
    { // Top (1) and bottom (3)
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

void update_entity(RenderContext *render_context, Entity *entity)
{
  entity->x += entity->direction_x * (render_context->delta_time * render_context->speed);
  entity->y += entity->direction_y * (render_context->delta_time * render_context->speed);
}

void render_entity(RenderContext *render_context, Entity *entity)
{
  SDL_FRect rendering_rect = camera_relative_rect(render_context, &(SDL_FRect){
                                                                      .w = entity->w,
                                                                      .h = entity->h,
                                                                      .x = entity->x,
                                                                      .y = entity->y,
                                                                  });

  draw_texture(render_context, entity, &rendering_rect);

  if (entity->selected)
  {
    draw_border(
        render_context,
        (SDL_FRect){
            .h = entity->h,
            .w = entity->w,
            .x = entity->x,
            .y = entity->y,
        },
        5.0f / render_context->camera.zoom,
        4.0f / render_context->camera.zoom);
  }

  draw_entity_name(render_context, entity);
}

TTF_Font *Font__load(const char *font_file_path, int font_size)
{
  TTF_Font *font = TTF_OpenFont(font_file_path, font_size);
  assert(font);

  return font;
}

void clear_screen(RenderContext *render_context)
{

  SDL_SetRenderDrawColor(render_context->renderer, render_context->background_color.r, render_context->background_color.g, render_context->background_color.b, render_context->background_color.a);
  SDL_RenderClear(render_context->renderer);
}

void mouse_control_camera(RenderContext *render_context, MouseState *mouse_state)
{
  if (mouse_state->button == SDL_BUTTON_RIGHT && mouse_state->state == SDL_PRESSED)
  {
    if (mouse_state->prev_x != mouse_state->x || mouse_state->prev_y != mouse_state->y)
    {
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
void keyboard_control_camera(RenderContext *render_context)
{
  float camera_keyboard_movement_speed = 5.0f;
  if (render_context->keyboard_state[SDL_GetScancodeFromKey(SDLK_w)])
  {
    render_context->camera.target_y -= camera_keyboard_movement_speed / render_context->camera.zoom;
    render_context->selection.target_y += camera_keyboard_movement_speed;
  }
  if (render_context->keyboard_state[SDL_GetScancodeFromKey(SDLK_s)])
  {
    render_context->camera.target_y += camera_keyboard_movement_speed / render_context->camera.zoom;
    render_context->selection.target_y -= camera_keyboard_movement_speed;
  }
  if (render_context->keyboard_state[SDL_GetScancodeFromKey(SDLK_a)])
  {
    render_context->camera.target_x -= camera_keyboard_movement_speed / render_context->camera.zoom;
    render_context->selection.target_x += camera_keyboard_movement_speed;
  }
  if (render_context->keyboard_state[SDL_GetScancodeFromKey(SDLK_d)])
  {
    render_context->camera.target_x += camera_keyboard_movement_speed / render_context->camera.zoom;
    render_context->selection.target_x -= camera_keyboard_movement_speed;
  }
}

// Set the camera to follow an entity, if only one entity is selected
void camera_follow_entity(RenderContext *render_context, Entity *entities)
{
  if (render_context->camera.following_entity > -1)
  {
    // TODO: Make it center on the entity
    render_context->camera.target_x = entities[render_context->camera.following_entity].x;
    render_context->camera.target_y = entities[render_context->camera.following_entity].y;
  }
}

// Set selected on any entity within the selection_rect
void select_entities_within_selection_rect(RenderContext *render_context, Entity *entities, int entities_count)
{
  for (int entity_i = 0; entity_i < entities_count; entity_i++)
  {
    SDL_FRect rect = camera_relative_rect(render_context, &(SDL_FRect){
                                                              .w = entities[entity_i].w,
                                                              .h = entities[entity_i].h,
                                                              .x = entities[entity_i].x,
                                                              .y = entities[entity_i].y,
                                                          });
    SDL_FPoint point_top_left = {
        .x = rect.x,
        .y = rect.y,
    };
    SDL_FPoint point_bottom_right = {
        .x = rect.x + rect.w,
        .y = rect.y + rect.h,
    };
    if (render_context->selection.rect.w > 3.0f)
    {
      if (SDL_PointInFRect(&point_top_left, &render_context->selection.rect) && SDL_PointInFRect(&point_bottom_right, &render_context->selection.rect))
      {
        entities[entity_i].selected = true;
      }
      else
      {
        if (!render_context->keyboard_state[SDL_GetScancodeFromKey(SDLK_LSHIFT)])
        {
          entities[entity_i].selected = false;
        }
      }
    }
  }
}

bool entity_under_mouse(RenderContext *render_context, Entity *entity, MouseState *mouse_state)
{
  SDL_FRect source_rect = {
      .w = entity->w,
      .h = entity->h,
      .x = entity->x,
      .y = entity->y,
  };
  SDL_FRect rect = camera_relative_rect(render_context, &source_rect);
  SDL_FPoint point = {
      .x = mouse_state->x,
      .y = mouse_state->y,
  };

  return SDL_PointInFRect(&point, &rect);
}

int init()
{
  srand(create_seed("ATHANO_LOVES_CHAT_OWO"));

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
    return 1;
  }

  if (TTF_Init() == -1)
  {
    fprintf(stderr, "could not initialize ttf: %s\n", TTF_GetError());
    return 1;
  }

  return 0;
}

int main(int argc, char *args[])
{
  if (init() > 0)
  {
    return 1;
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

  SDL_Window *window = SDL_CreateWindow(
      "Cultivation Sim",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      SCREEN_WIDTH, SCREEN_HEIGHT,
      SDL_WINDOW_SHOWN);
  if (!window)
  {
    fprintf(stderr, "could not create window: %s\n", SDL_GetError());
    return 1;
  }

  RenderContext render_context = {
      .renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
      .animated_time = 0,
      .speed = 200.0f,
      .delta_time = 0,
      .background_color = {45, 125, 2, 255},
      .camera = {
          .x = 0,
          .y = 0,
          .target_zoom = 1.0f,
          .pan_spring_x = {
              .target = 1.0f,
              .current = 1.0f,
              .velocity = 0.0f,
              .acceleration = 0.5f,
              .friction = 0.1f,
          },
          .pan_spring_y = {
              .target = 1.0f,
              .current = 1.0f,
              .velocity = 0.0f,
              .acceleration = 0.5f,
              .friction = 0.1f,
          },
          .zoom_spring = {
              .target = 1.0f,
              .current = 1.0f,
              .velocity = 0.0f,
              .acceleration = 0.4f,
              .friction = 0.1f,
          },
          .following_entity = -1,
      },
      .fonts = (TTF_Font *[]){
          Font__load("assets/OpenSans-Regular.ttf", 32),
          Font__load("assets/OpenSans-Regular.ttf", 24),
          Font__load("assets/OpenSans-Regular.ttf", 16),
      },
      .selection = {
          .spring_x = {
              .target = 1.0f,
              .current = 1.0f,
              .velocity = 0.0f,
              .acceleration = 0.5f,
              .friction = 0.1f,
          },
          .spring_y = {
              .target = 1.0f,
              .current = 1.0f,
              .velocity = 0.0f,
              .acceleration = 0.5f,
              .friction = 0.1f,
          },
      }};

  if (!render_context.renderer)
  {
    fprintf(stderr, "could not create renderer: %s\n", SDL_GetError());
    return 1;
  }

  Entity entities[] = {
      Entity__create(&render_context, "assets/lamb2.bmp", "pushqrdx"),
      Entity__create(&render_context, "assets/stone.bmp", "Athano"),
      Entity__create(&render_context, "assets/lamb.bmp", "AshenHobs"),
      Entity__create(&render_context, "assets/stone.bmp", "adrian_learns"),
      Entity__create(&render_context, "assets/lamb.bmp", "RVerite"),
      Entity__create(&render_context, "assets/stone.bmp", "Orshy"),
      Entity__create(&render_context, "assets/lamb2.bmp", "ruggs888"),
      Entity__create(&render_context, "assets/fish.bmp", "Xent12"),
      Entity__create(&render_context, "assets/fish.bmp", "nuke_bird"),
      Entity__create(&render_context, "assets/stone.bmp", "Kasper_573"),
      Entity__create(&render_context, "assets/fish.bmp", "SturdyPose"),
      Entity__create(&render_context, "assets/stone.bmp", "coffee_lava"),
      Entity__create(&render_context, "assets/stone.bmp", "goudacheeseburgers"),
      Entity__create(&render_context, "assets/stone.bmp", "ikiwixz"),
      Entity__create(&render_context, "assets/lamb2.bmp", "NixAurvandil"),
      Entity__create(&render_context, "assets/lamb2.bmp", "smilingbig"),
      Entity__create(&render_context, "assets/lamb.bmp", "tk_dev"),
      Entity__create(&render_context, "assets/lamb2.bmp", "realSuperku"),
      Entity__create(&render_context, "assets/stone.bmp", "Hoby2000"),
      Entity__create(&render_context, "assets/stone.bmp", "CuteMath"),
      Entity__create(&render_context, "assets/stone.bmp", "forodor"),
      Entity__create(&render_context, "assets/stone.bmp", "Azenris"),
      Entity__create(&render_context, "assets/stone.bmp", "collector_of_stuff"),
      Entity__create(&render_context, "assets/lamb2.bmp", "EvanMMO"),
      Entity__create(&render_context, "assets/stone.bmp", "thechaosbean"),
      Entity__create(&render_context, "assets/stone.bmp", "Lutf1sk"),
      Entity__create(&render_context, "assets/lamb2.bmp", "BauBas9883"),
      Entity__create(&render_context, "assets/stone.bmp", "physbuzz"),
      Entity__create(&render_context, "assets/lamb2.bmp", "rizoma0x00"),
      Entity__create(&render_context, "assets/stone.bmp", "Tkap1"),
      Entity__create(&render_context, "assets/lamb2.bmp", "GavinsAwfulStream"),
      Entity__create(&render_context, "assets/lamb2.bmp", "Resist_0"),
      Entity__create(&render_context, "assets/stone.bmp", "b1k4sh"),
      Entity__create(&render_context, "assets/lamb.bmp", "nhancodes"),
      Entity__create(&render_context, "assets/stone.bmp", "qcircuit1"),
      Entity__create(&render_context, "assets/stone.bmp", "fruloo"),
      Entity__create(&render_context, "assets/stone.bmp", "programmer_jeff"),
      Entity__create(&render_context, "assets/stone.bmp", "BluePinStudio"),
  };

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

  while (game_is_still_running)
  {
    frame_count++;
    if (SDL_GetTicks() - start_ticks >= 1000)
    {
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
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
      {
        mouse_state.prev_state = mouse_state.state;
        mouse_state.state = event.button.state;
        mouse_state.button = event.button.button;
        mouse_state.clicks = event.button.clicks;
        if (mouse_state.prev_state != SDL_PRESSED)
        {
          render_context.selection.target_x = mouse_state.x;
          render_context.selection.target_y = mouse_state.y;
        }
      }
      if (event.type == SDL_MOUSEMOTION)
      {
        mouse_state.prev_state = mouse_state.state;
        mouse_state.prev_x = mouse_state.x;
        mouse_state.prev_y = mouse_state.y;
        mouse_state.x = (float)event.motion.x;
        mouse_state.y = (float)event.motion.y;
      }
      if (event.type == SDL_MOUSEWHEEL)
      {
        if (event.wheel.y > 0)
        {
          // zoom in
          render_context.camera.target_zoom = SDL_min(render_context.camera.target_zoom + 0.1f, 2.0f);
        }
        else if (event.wheel.y < 0)
        {
          // zoom out
          render_context.camera.target_zoom = SDL_max(render_context.camera.target_zoom - 0.1f, 0.1f);
        }
      }
      if (event.type == SDL_KEYDOWN)
      {
        switch (event.key.keysym.sym)
        {
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
          if (render_context.prev_speed > 0)
          {
            render_context.speed = render_context.prev_speed;
            render_context.prev_speed = 0;
          }
          else
          {
            render_context.prev_speed = render_context.speed;
            render_context.speed = 0;
          }
        case SDLK_LCTRL:
          // Deliberately inside the poll event loop
          if (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.clicks == 1)
          {
            for (int entity_i = array_count(entities) - 1; entity_i >= 0; entity_i--)
            {
              if (entity_under_mouse(&render_context, &entities[entity_i], &mouse_state))
              {
                render_context.camera.following_entity = entities[entity_i].selected ? entity_i : -1;
                break;
              }
            }
          }
          break;

        default:
          break;
        }
      }
      else if (event.type == SDL_QUIT)
      {
        game_is_still_running = 0;
      }

      // Deliberately inside the poll event loop
      if (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.clicks == 1)
      {
        for (int entity_i = array_count(entities) - 1; entity_i >= 0; entity_i--)
        {
          if (entity_under_mouse(&render_context, &entities[entity_i], &mouse_state))
          {
            entities[entity_i].selected = !entities[entity_i].selected;
            break;
          }
        }
      }
    }

    // {
    //   if (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_RELEASED && mouse_state.prev_state != SDL_RELEASED && render_context.selection_rect.w == 0)
    //   {
    //     for (int entity_i = 0; entity_i < array_count(entities); entity_i++)
    //     {
    //       entities[entity_i].selected = false;
    //     }
    //   }
    // }

    mouse_control_camera(&render_context, &mouse_state);

    keyboard_control_camera(&render_context);

    { // Selection rect creation
      if (mouse_state.button == SDL_BUTTON_LEFT && mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_PRESSED)
      {
        render_context.selection.rect = (SDL_FRect){
            .x = SDL_min(mouse_state.x, render_context.selection.x),
            .y = SDL_min(mouse_state.y, render_context.selection.y),
            .w = SDL_fabsf(mouse_state.x - render_context.selection.x),
            .h = SDL_fabsf(mouse_state.y - render_context.selection.y),
        };
      }
    }

    camera_follow_entity(&render_context, entities);

    select_entities_within_selection_rect(&render_context, entities, array_count(entities));

    { // Spring the selection box
      render_context.selection.x = Spring__update(&render_context.selection.spring_x, render_context.selection.target_x);
      render_context.selection.y = Spring__update(&render_context.selection.spring_y, render_context.selection.target_y);
    }

    { // Spring the camera position
      render_context.camera.x = Spring__update(&render_context.camera.pan_spring_x, render_context.camera.target_x);
      render_context.camera.y = Spring__update(&render_context.camera.pan_spring_y, render_context.camera.target_y);
    }

    clear_screen(&render_context);

    for (int entity_i = 0; entity_i < array_count(entities); entity_i++)
    {
      if (entities[entity_i].texture)
      {
        update_entity(&render_context, &entities[entity_i]);
        render_entity(&render_context, &entities[entity_i]);
      }
    }

    render_selection_box(&render_context);

    render_debug_info(&render_context, &mouse_state);

    SDL_RenderPresent(render_context.renderer);

    // Clear the selection rect
    render_context.selection.rect = (SDL_FRect){0};
  }

  SDL_DestroyWindow(window);
  return 0;
}
