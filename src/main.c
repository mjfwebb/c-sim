#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define array_count(static_array) (sizeof(static_array) / sizeof((static_array)[0]))
#define rect_fields \
  float w;          \
  float h
#define point_fields \
  float x;           \
  float y

typedef struct
{
  SDL_Texture *texture;
  rect_fields;
  point_fields;
  float directionX;
  float directionY;
  float speed;
  int layer;
  bool dead;
} Entity;

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
  Entity camera;
  TTF_Font **fonts;
  int fonts_length;
  float fps;
} RenderContext;

typedef struct
{
  int state;
  int button;
  point_fields;
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
      .directionX = 0.4f,
      .directionY = 0.5f,
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
  SDL_FRect message_rect = {
      .w = entity->w,
      .h = entity->h,
      .x = entity->x,
      .y = entity->y,
  };

  int copy_result = SDL_RenderCopyF(render_context->renderer, entity->texture, NULL, &message_rect);
  if (copy_result != 0)
  {
    printf("Failed to render copy: %s\n", SDL_GetError());
    return;
  }
}

void detect_window_edge_collision(Entity *entity, Rect window)
{
  if (entity->x <= 0)
  {
    entity->x = 0.1f;
    entity->directionX *= -1;
  }
  if ((entity->x + entity->w) >= window.w)
  {
    entity->x = (float)(window.w - entity->w);
    entity->directionX *= -1;
  }
  if (entity->y <= 0)
  {
    entity->y = 0.1f;
    entity->directionY *= -1;
  }
  if ((entity->y + entity->h) >= window.h)
  {
    entity->y = (float)(window.h - entity->h);
    entity->directionY *= -1;
  }
}

void render_camera(RenderContext *render_context)
{
  SDL_FRect camera_rect = {
      .h = render_context->camera.h,
      .w = render_context->camera.w,
      .x = render_context->camera.x,
      .y = render_context->camera.y,
  };

  SDL_SetRenderDrawColor(render_context->renderer, 255, 255, 255, 100);
  SDL_SetRenderDrawBlendMode(render_context->renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderDrawRectF(render_context->renderer, &camera_rect);
  SDL_RenderFillRectF(render_context->renderer, &camera_rect);
}

bool detect_entity_collision(Entity *entityA, Entity *entityB)
{
  if ((entityA->x <= entityB->x + entityB->w && (entityA->x + entityA->w) >= entityB->x) && (entityA->y <= entityB->y + entityB->h && (entityA->y + entityA->h) >= entityB->y))
  {
    return true;
  }

  return false;
}

void render_debug_info(RenderContext *render_context)
{
  SDL_Color White = {255, 255, 255};
  TTF_Font *font = render_context->fonts[0];
  char fps_text[32];
  sprintf(fps_text, "fps: %.2f", render_context->fps);

  SDL_Surface *text_surface = TTF_RenderText_Solid(font, fps_text, White);
  if (!text_surface)
  {
    fprintf(stderr, "could not create surface: %s\n", SDL_GetError());
  }

  SDL_Texture *text_texture = SDL_CreateTextureFromSurface(render_context->renderer, text_surface);
  if (!text_texture)
  {
    fprintf(stderr, "could not create surface: %s\n", SDL_GetError());
  }

  SDL_FRect text_rect = {
      .w = (float)text_surface->w,
      .h = (float)text_surface->h,
      .x = 10,
      .y = 10,
  };

  SDL_RenderCopyF(render_context->renderer, text_texture, NULL, &text_rect);
}

void render(RenderContext *render_context, Entity *entities, int entities_count, int *entities_render_order, int entities_render_order_count)
{
  SDL_SetRenderDrawColor(render_context->renderer,
                         render_context->background_color.r, render_context->background_color.g, render_context->background_color.b, 255);

  int result = SDL_RenderFillRect(render_context->renderer, NULL);
  assert(result == 0);

  int window_w;
  int window_h;
  SDL_GetWindowSizeInPixels(render_context->window, &window_w, &window_h);

  for (int i = 0; i < entities_count; i++)
  {
    if (entities[entities_render_order[i]].texture && entities[entities_render_order[i]].dead == false)
    {
      detect_window_edge_collision(&entities[entities_render_order[i]], (Rect){.w = window_w, .h = window_h});

      for (int j = 0; j < entities_count; j++)
      {
        if (entities[entities_render_order[j]].texture)
        {
          if (i != j)
          {
            bool collision = detect_entity_collision(&entities[entities_render_order[i]], &entities[entities_render_order[j]]);
            if (collision)
            {
              // printf("entity %d collided with entity %d\n", i, j);
            }
          }
        }
      }

      render_entity(render_context, &entities[entities_render_order[i]]);
    }
  }

  render_camera(render_context);
  render_debug_info(render_context);

  SDL_RenderPresent(render_context->renderer);
}

void render_entity(RenderContext *render_context, Entity *entity)
{
  entity->x += entity->directionX * (render_context->delta_time * render_context->speed);
  entity->y += entity->directionY * (render_context->delta_time * render_context->speed);

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
      make_entity("assets/lamb2.bmp", renderer),
      make_entity("assets/stone.bmp", renderer),
      make_entity("assets/lamb2.bmp", renderer),
      make_entity("assets/stone.bmp", renderer),
      make_entity("assets/lamb2.bmp", renderer),
      make_entity("assets/fish.bmp", renderer),
      make_entity("assets/fish.bmp", renderer),
      make_entity("assets/fish.bmp", renderer),
      make_entity("assets/fish.bmp", renderer),
      make_entity("assets/stone.bmp", renderer),
  };

  TTF_Font *fonts[] = {
      load_font("assets/OpenSans-Regular.ttf", 18),
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
          .h = 500.0f,
          .w = 500.0f,
          .x = 0,
          .y = 0,
      },
      .fonts = fonts,
  };

  int game_is_still_running = 1;
  unsigned int start_ticks = SDL_GetTicks();
  int frame_count = 0;

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

    MouseState mouse_state = {
        .state = -1,
        .button = 0,
        .clicks = 0,
        .x = 0,
        .y = 0,
    };

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
      {
        mouse_state.state = event.button.state;
        mouse_state.button = event.button.button;
        mouse_state.clicks = event.button.clicks;
        mouse_state.x = (float)event.button.x;
        mouse_state.y = (float)event.button.y;
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

        case SDLK_w:
          render_context.camera.y -= 1000.0f * render_context.delta_time;
          break;

        case SDLK_s:
          render_context.camera.y += 1000.0f * render_context.delta_time;
          break;

        case SDLK_a:
          render_context.camera.x -= 1000.0f * render_context.delta_time;
          break;

        case SDLK_d:
          render_context.camera.x += 1000.0f * render_context.delta_time;
          break;

        default:
          break;
        }
      }
      else if (event.type == SDL_QUIT)
      {
        game_is_still_running = 0;
      }
    }

    render_context.animated_time = fmodf(render_context.animated_time + render_context.delta_time * 0.5f, 1);

    if (mouse_state.state == SDL_PRESSED)
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

    render(&render_context, entities, array_count(entities), entities_render_order, array_count(entities));
  }

  SDL_DestroyWindow(window);
  return 0;
}