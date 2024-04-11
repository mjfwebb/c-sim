#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

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
#define velocity_fields \
  float velocity_x;     \
  float velocity_y
#define direction_fields \
  float direction_x;     \
  float direction_y
#define prev_point_fields \
  float prev_x;           \
  float prev_y
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
  prev_point_fields;
  velocity_fields;
  direction_fields;
  float speed;
  int layer;
  bool dead;
} Entity;

typedef struct
{
  SDL_Texture *texture;
  rect_fields;
  point_fields;
  prev_point_fields;
  velocity_fields;
  float zoom;
  Spring zoom_spring;
  Spring pan_spring_x;
  Spring pan_spring_y;
} Camera;

typedef struct
{
  int current_time;
  int last_update_time;
  float speed;
  float delta_time;
  float animated_time;
  SDL_Renderer *renderer;
  SDL_Surface *screen_surface;
  SDL_Surface *text_surface;
  SDL_Window *window;
  SDL_Color background_color;
  Camera camera;
  TTF_Font **fonts;
  float fps;
  float render_zoom;
  float render_camera_x;
  float render_camera_y;
} RenderContext;

typedef struct
{
  int prev_state;
  int state;
  int button;
  point_fields;
  prev_point_fields;
  int clicks;
} MouseState;

typedef struct
{
  int w;
  int h;
} Rect;

void render_entity(RenderContext *render_context, Entity *entity);

Entity make_entity(char *image_path, SDL_Renderer *renderer)
{
  SDL_Surface *image = SDL_LoadBMP(image_path);
  assert(image);

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, image);
  assert(texture);

  float width = 100.0f;
  float scale = width / image->w;
  float height = (float)(image->h * scale);

  Entity entity = {
      .direction_x = ((float)(rand() % 10)) / 10,
      .direction_y = ((float)(rand() % 10)) / 10,
      .x = (float)(rand() % 1000),
      .y = (float)(rand() % 1000),
      .texture = texture,
      .h = height,
      .w = width,
      .dead = false,
  };

  // Surface no longer needed after the texture is created
  SDL_FreeSurface(image);

  return entity;
}

void draw_texture(RenderContext *render_context, Entity *entity)
{
  int window_w;
  int window_h;
  SDL_GetWindowSizeInPixels(render_context->window, &window_w, &window_h);

  SDL_FRect texture_rect = {
      .x = (entity->x - render_context->render_camera_x) * render_context->render_zoom + window_w / 2,
      .y = (entity->y - render_context->render_camera_y) * render_context->render_zoom + window_h / 2,
      .w = entity->w * render_context->render_zoom,
      .h = entity->h * render_context->render_zoom,
  };

  int copy_result = SDL_RenderCopyF(render_context->renderer, entity->texture, NULL, &texture_rect);
  if (copy_result != 0)
  {
    printf("Failed to render copy: %s\n", SDL_GetError());
    return;
  }
}

void draw_text(RenderContext *render_context, char *text, float x, float y)
{
  TTF_Font *font = render_context->fonts[0];
  SDL_Color White = {255, 255, 255};
  SDL_Surface *text_surface = TTF_RenderText_Solid(font, text, White);
  if (!text_surface)
  {
    fprintf(stderr, "could not create text surface: %s\n", SDL_GetError());
  }

  SDL_Texture *text_texture = SDL_CreateTextureFromSurface(render_context->renderer, text_surface);
  if (!text_texture)
  {
    fprintf(stderr, "could not create text texture: %s\n", SDL_GetError());
  }

  SDL_FRect text_rect = {
      .w = (float)text_surface->w,
      .h = (float)text_surface->h,
      .x = x,
      .y = y,
  };

  SDL_RenderCopyF(render_context->renderer, text_texture, NULL, &text_rect);
  SDL_FreeSurface(text_surface);
  SDL_DestroyTexture(text_texture);
}

void render_debug_info(RenderContext *render_context, MouseState *mouse_state)
{
  char text[128];

  sprintf(text, "fps: %.2f", render_context->fps);
  draw_text(render_context, text, 10.0f, 10.0f);

  sprintf(text, "mouse state: %d, button: %d, clicks: %d", mouse_state->state, mouse_state->button, mouse_state->clicks);
  draw_text(render_context, text, 10.0f, 40.0f);

  sprintf(text, "prev mouse state: %d", mouse_state->prev_state);
  draw_text(render_context, text, 10.0f, 70.0f);

  sprintf(text, "camera zoom: %.1f", render_context->camera.zoom);
  draw_text(render_context, text, 10.0f, 100.0f);
}

float Spring__update(Spring *spring, float target)
{
  spring->target = target;
  spring->velocity += (target - spring->current) * spring->acceleration;
  spring->velocity *= spring->friction;
  return spring->current += spring->velocity;
}
void render(RenderContext *render_context, Entity *entities, int entities_count, int *entities_render_order, int entities_render_order_count)
{
  int window_w;
  int window_h;
  SDL_GetWindowSizeInPixels(render_context->window, &window_w, &window_h);

  // render_camera(render_context);

  for (int i = 0; i < entities_count; i++)
  {
    if (entities[entities_render_order[i]].texture && entities[entities_render_order[i]].dead == false)
    {
      // detect_window_edge_collision(&entities[entities_render_order[i]], (Rect){.w = window_w, .h = window_h});

      // for (int j = 0; j < entities_count; j++)
      // {
      //   if (entities[entities_render_order[j]].texture)
      //   {
      //     if (i != j)
      //     {
      //       bool collision = detect_entity_collision(&entities[entities_render_order[i]], &entities[entities_render_order[j]]);
      //       if (collision)
      //       {
      //         // printf("entity %d collided with entity %d\n", i, j);
      //       }
      //     }
      //   }
      // }

      render_entity(render_context, &entities[entities_render_order[i]]);
    }
  }

  SDL_RenderPresent(render_context->renderer);
}

void render_entity(RenderContext *render_context, Entity *entity)
{
  entity->x += entity->direction_x * (render_context->delta_time * render_context->speed);
  entity->y += entity->direction_y * (render_context->delta_time * render_context->speed);

  draw_texture(render_context, entity);
}

TTF_Font *load_font(const char *font_file_path, int font_size)
{
  TTF_Font *font = TTF_OpenFont(font_file_path, font_size);
  assert(font);

  return font;
}

int main(int argc, char *args[])
{
  srand((int)time(NULL));
  SDL_Window *window = NULL;

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

  window = SDL_CreateWindow(
      "Cultivation Sim",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      SCREEN_WIDTH, SCREEN_HEIGHT,
      SDL_WINDOW_SHOWN);
  if (!window)
  {
    fprintf(stderr, "could not create window: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer)
  {
    fprintf(stderr, "could not create renderer: %s\n", SDL_GetError());
    return 1;
  }

  Entity entities[] = {
      make_entity("assets/lamb2.bmp", renderer),
      make_entity("assets/stone.bmp", renderer),
      make_entity("assets/lamb.bmp", renderer),
      make_entity("assets/stone.bmp", renderer),
      make_entity("assets/lamb.bmp", renderer),
      make_entity("assets/stone.bmp", renderer),
      make_entity("assets/lamb2.bmp", renderer),
      make_entity("assets/fish.bmp", renderer),
      make_entity("assets/fish.bmp", renderer),
      make_entity("assets/fish.bmp", renderer),
      make_entity("assets/fish.bmp", renderer),
      make_entity("assets/stone.bmp", renderer),
  };

  TTF_Font *fonts[] = {
      load_font("assets/OpenSans-Regular.ttf", 32),
  };

  int entities_render_order[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

  RenderContext render_context = {
      .renderer = renderer,
      .current_time = 0,
      .last_update_time = 0,
      .animated_time = 0,
      .speed = 200.0f,
      .delta_time = 0,
      .window = window,
      .background_color = {45, 125, 2, 255},
      .camera = {
          .x = 0,
          .y = 0,
          .zoom = 1.0f,
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
          }},
      .fonts = fonts,
  };

  int game_is_still_running = 1;
  unsigned int start_ticks = SDL_GetTicks();
  int frame_count = 0;

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

  while (game_is_still_running)
  {
    frame_count++;
    if (SDL_GetTicks() - start_ticks >= 1000)
    {
      render_context.fps = (float)frame_count;
      frame_count = 0;
      start_ticks = SDL_GetTicks();
    }
    render_context.current_time = SDL_GetTicks();
    render_context.delta_time = (float)(render_context.current_time - render_context.last_update_time) / 1000;
    render_context.last_update_time = render_context.current_time;
    render_context.animated_time = fmodf(render_context.animated_time + render_context.delta_time * 0.5f, 1);
    render_context.render_zoom = Spring__update(&render_context.camera.zoom_spring, render_context.camera.zoom);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
      {
        mouse_state.prev_state = mouse_state.state;
        mouse_state.state = event.button.state;
        mouse_state.button = event.button.button;
        mouse_state.clicks = event.button.clicks;
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
          render_context.camera.zoom = SDL_min(render_context.camera.zoom + 0.1f, 1.5f);
        }
        else if (event.wheel.y < 0)
        {
          // zoom out
          render_context.camera.zoom = SDL_max(render_context.camera.zoom - 0.1f, 0.5f);
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

          // case SDLK_w:
          //   render_context.camera.y -= 50.0f / render_context.render_zoom;
          //   break;

          // case SDLK_s:
          //   render_context.camera.y += 50.0f / render_context.render_zoom;
          //   break;

          // case SDLK_a:
          //   render_context.camera.x -= 50.0f / render_context.render_zoom;
          //   break;

          // case SDLK_d:
          //   render_context.camera.x += 50.0f / render_context.render_zoom;
          //   break;

        default:
          break;
        }
      }
      else if (event.type == SDL_QUIT)
      {
        game_is_still_running = 0;
      }

      if (mouse_state.state == SDL_PRESSED && mouse_state.prev_state == SDL_RELEASED)
      {
        for (int entity_i = array_count(entities) - 1; entity_i >= 0; entity_i--)
        {
          if (entities[entities_render_order[entity_i]].dead)
          {
            continue;
          }

          SDL_FRect rect = {
              .x = entities[entities_render_order[entity_i]].x,
              .y = entities[entities_render_order[entity_i]].y,
              .w = entities[entities_render_order[entity_i]].w,
              .h = entities[entities_render_order[entity_i]].h,
          };

          SDL_FPoint point = {
              .x = mouse_state.x,
              .y = mouse_state.y,
          };

          if (SDL_PointInFRect(&point, &rect))
          {
            entities[entities_render_order[entity_i]].dead = true;
            break;
          }
        }
      }
    }

    if (mouse_state.state == SDL_PRESSED)
    {
      if (mouse_state.button == SDL_BUTTON_RIGHT)
      {
        if (mouse_state.prev_x != mouse_state.x || mouse_state.prev_y != mouse_state.y)
        {
          float delta_x = mouse_state.x - mouse_state.prev_x;
          float delta_y = mouse_state.y - mouse_state.prev_y;
          mouse_state.prev_x = mouse_state.x;
          mouse_state.prev_y = mouse_state.y;

          render_context.camera.x -= delta_x / render_context.render_zoom;
          render_context.camera.y -= delta_y / render_context.render_zoom;
        }
      }
    }

    float camera_movement_speed = 5.0f;
    const Uint8 *keyboard = SDL_GetKeyboardState(NULL);
    if (keyboard[SDL_GetScancodeFromKey(SDLK_w)])
    {
      render_context.camera.y -= camera_movement_speed / render_context.render_zoom;
    }
    if (keyboard[SDL_GetScancodeFromKey(SDLK_s)])
    {
      render_context.camera.y += camera_movement_speed / render_context.render_zoom;
    }
    if (keyboard[SDL_GetScancodeFromKey(SDLK_a)])
    {
      render_context.camera.x -= camera_movement_speed / render_context.render_zoom;
    }
    if (keyboard[SDL_GetScancodeFromKey(SDLK_d)])
    {
      render_context.camera.x += camera_movement_speed / render_context.render_zoom;
    }

    render_context.render_camera_x = Spring__update(&render_context.camera.pan_spring_x, render_context.camera.x);
    render_context.render_camera_y = Spring__update(&render_context.camera.pan_spring_y, render_context.camera.y);

    SDL_SetRenderDrawColor(render_context.renderer,
                           render_context.background_color.r, render_context.background_color.g, render_context.background_color.b, 255);

    // SDL_RenderSetClipRect(render_context.renderer, NULL);
    int result = SDL_RenderFillRect(render_context.renderer, NULL);
    assert(result == 0);
    // update(entities, array_count(entities));

    render_debug_info(&render_context, &mouse_state);

    render(&render_context, entities, array_count(entities), entities_render_order, array_count(entities));
  }

  SDL_DestroyWindow(window);
  return 0;
}