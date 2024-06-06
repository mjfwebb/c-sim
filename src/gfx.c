#include "headers.h"

// Initialize the graphics context, including SDL and SDL_image, and create a window and renderer
int gfx_init() {
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Could not initialize sdl2: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  // Initialize SDL_image
  if (TTF_Init() == -1) {
    fprintf(stderr, "Could not initialize ttf: %s\n", TTF_GetError());
    return EXIT_FAILURE;
  }

  // Set scale quality to best
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
  // Set render batching
  SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");

  SDL_Window *window = SDL_CreateWindow(
      "Cultivation Sim", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
  );
  if (!window) {
    fprintf(stderr, "could not create window: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  render_context.window = window;

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (!renderer) {
    fprintf(stderr, "could not create renderer: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  render_context.renderer = renderer;

  return EXIT_SUCCESS;
}

// Update the screen with the current render context since the last call to gfx_render_present
void gfx_render_present() {
  SDL_RenderPresent(render_context.renderer);
}

// Get the window size and store it in out_w and out_h
void gfx_get_window_size(int *out_w, int *out_h) {
  SDL_GetWindowSize(render_context.window, out_w, out_h);
}

// Internal function to convert a rect to an SDL_Rect
static SDL_FRect frect_to_sdl_frect(FRect *rect) {
  return (SDL_FRect){
      .x = rect->position.x,
      .y = rect->position.y,
      .w = frect_width(rect),
      .h = frect_height(rect),
  };
}

// Draw a rectangle with the given color
void gfx_draw_frect(FRect *rect, RGBA *color) {
  SDL_FRect sdl_frect = frect_to_sdl_frect(rect);
  SDL_SetRenderDrawColor(render_context.renderer, (u8)(255 * color->r), (u8)(255 * color->g), (u8)(255 * color->b), (u8)(255 * color->a));

  int result = SDL_RenderDrawRectF(render_context.renderer, &sdl_frect);
  assert(result == 0);
}

// Draw a filled rectangle with the given color
void gfx_draw_frect_filled(FRect *rect, RGBA *color) {
  SDL_FRect sdl_frect = frect_to_sdl_frect(rect);
  SDL_SetRenderDrawColor(render_context.renderer, (u8)(255 * color->r), (u8)(255 * color->g), (u8)(255 * color->b), (u8)(255 * color->a));

  int result = SDL_RenderFillRectF(render_context.renderer, &sdl_frect);
  assert(result == 0);
};

typedef enum {
  SHORT_GRASS_TOP = 1,
  SHORT_GRASS_RIGHT = 2,
  SHORT_GRASS_BOTTOM = 4,
  SHORT_GRASS_LEFT = 8,
  LONG_GRASS_TOP = 16,
  LONG_GRASS_RIGHT = 32,
  LONG_GRASS_BOTTOM = 64,
  LONG_GRASS_LEFT = 128,
  OVERGROWN_GRASS_TOP = 256,
  OVERGROWN_GRASS_RIGHT = 512,
  OVERGROWN_GRASS_BOTTOM = 1024,
  OVERGROWN_GRASS_LEFT = 2048,
} GRASS_DIRECTION;

typedef enum {
  GFX_TEXTURE_COURAGE_TALISMAN,
  GFX_TEXTURE_DEATH_POTION,
  GFX_TEXTURE_HEALTH_POTION,
  GFX_TEXTURE_LOVE_POTION,
  GFX_TEXTURE_LUCK_TALISMAN,
  GFX_TEXTURE_SLEEP_POTION,
  GFX_TEXTURE_TRUTH_POTION,
  GFX_TEXTURE_WATER_BREATHING_POTION,
  GFX_TEXTURE_TREE,
  GFX_TEXTURE_SHADOW,
  GFX_TEXTURE_GRASS_LONG_CENTER,
  GFX_TEXTURE_GRASS_LONG_OVERGROWN_BOTTOM,
  GFX_TEXTURE_GRASS_LONG_OVERGROWN_LEFT,
  GFX_TEXTURE_GRASS_LONG_OVERGROWN_LEFT_BOTTOM,
  GFX_TEXTURE_GRASS_LONG_OVERGROWN_LEFT_TOP,
  GFX_TEXTURE_GRASS_LONG_OVERGROWN_RIGHT,
  GFX_TEXTURE_GRASS_LONG_OVERGROWN_RIGHT_BOTTOM,
  GFX_TEXTURE_GRASS_LONG_OVERGROWN_RIGHT_TOP,
  GFX_TEXTURE_GRASS_LONG_OVERGROWN_TOP,
  GFX_TEXTURE_GRASS_OVERGROWN_CENTER,
  GFX_TEXTURE_GRASS_SHORT_CENTER,
  GFX_TEXTURE_GRASS_SHORT_LONG_BOTTOM,
  GFX_TEXTURE_GRASS_SHORT_LONG_LEFT,
  GFX_TEXTURE_GRASS_SHORT_LONG_LEFT_BOTTOM,
  GFX_TEXTURE_GRASS_SHORT_LONG_LEFT_TOP,
  GFX_TEXTURE_GRASS_SHORT_LONG_RIGHT,
  GFX_TEXTURE_GRASS_SHORT_LONG_RIGHT_BOTTOM,
  GFX_TEXTURE_GRASS_SHORT_LONG_RIGHT_TOP,
  GFX_TEXTURE_GRASS_SHORT_LONG_TOP,
  GFX_TEXTURE_GRASS_SHORT_OVERGROWN_BOTTOM,
  GFX_TEXTURE_GRASS_SHORT_OVERGROWN_LEFT,
  GFX_TEXTURE_GRASS_SHORT_OVERGROWN_LEFT_BOTTOM,
  GFX_TEXTURE_GRASS_SHORT_OVERGROWN_LEFT_TOP,
  GFX_TEXTURE_GRASS_SHORT_OVERGROWN_RIGHT,
  GFX_TEXTURE_GRASS_SHORT_OVERGROWN_RIGHT_BOTTOM,
  GFX_TEXTURE_GRASS_SHORT_OVERGROWN_RIGHT_TOP,
  GFX_TEXTURE_GRASS_SHORT_OVERGROWN_TOP,
  GFX_TEXTURE_count,
} GFX_TEXTURE_ID;

int grass_textures[] = {
    [GFX_TEXTURE_GRASS_LONG_CENTER] = LONG_GRASS_TOP | LONG_GRASS_RIGHT | LONG_GRASS_BOTTOM | LONG_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_LONG_OVERGROWN_BOTTOM] = LONG_GRASS_TOP | LONG_GRASS_RIGHT | OVERGROWN_GRASS_BOTTOM | LONG_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_LONG_OVERGROWN_LEFT] = LONG_GRASS_TOP | LONG_GRASS_RIGHT | LONG_GRASS_BOTTOM | OVERGROWN_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_LONG_OVERGROWN_LEFT_BOTTOM] = LONG_GRASS_TOP | LONG_GRASS_RIGHT | OVERGROWN_GRASS_BOTTOM | OVERGROWN_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_LONG_OVERGROWN_LEFT_TOP] = OVERGROWN_GRASS_TOP | LONG_GRASS_RIGHT | LONG_GRASS_BOTTOM | OVERGROWN_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_LONG_OVERGROWN_RIGHT] = LONG_GRASS_TOP | OVERGROWN_GRASS_RIGHT | LONG_GRASS_BOTTOM | LONG_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_LONG_OVERGROWN_RIGHT_BOTTOM] = LONG_GRASS_TOP | OVERGROWN_GRASS_RIGHT | OVERGROWN_GRASS_BOTTOM | LONG_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_LONG_OVERGROWN_RIGHT_TOP] = OVERGROWN_GRASS_TOP | OVERGROWN_GRASS_RIGHT | LONG_GRASS_BOTTOM | LONG_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_LONG_OVERGROWN_TOP] = OVERGROWN_GRASS_TOP | LONG_GRASS_RIGHT | LONG_GRASS_BOTTOM | LONG_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_OVERGROWN_CENTER] = OVERGROWN_GRASS_TOP | OVERGROWN_GRASS_RIGHT | OVERGROWN_GRASS_BOTTOM | OVERGROWN_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_CENTER] = SHORT_GRASS_TOP | SHORT_GRASS_RIGHT | SHORT_GRASS_BOTTOM | SHORT_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_LONG_BOTTOM] = SHORT_GRASS_TOP | SHORT_GRASS_RIGHT | LONG_GRASS_BOTTOM | SHORT_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_LONG_LEFT] = SHORT_GRASS_TOP | SHORT_GRASS_RIGHT | SHORT_GRASS_BOTTOM | LONG_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_LONG_LEFT_BOTTOM] = SHORT_GRASS_TOP | SHORT_GRASS_RIGHT | LONG_GRASS_BOTTOM | LONG_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_LONG_LEFT_TOP] = LONG_GRASS_TOP | SHORT_GRASS_RIGHT | SHORT_GRASS_BOTTOM | LONG_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_LONG_RIGHT] = SHORT_GRASS_TOP | LONG_GRASS_RIGHT | SHORT_GRASS_BOTTOM | SHORT_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_LONG_RIGHT_BOTTOM] = SHORT_GRASS_TOP | LONG_GRASS_RIGHT | LONG_GRASS_BOTTOM | SHORT_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_LONG_RIGHT_TOP] = LONG_GRASS_TOP | LONG_GRASS_RIGHT | SHORT_GRASS_BOTTOM | SHORT_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_LONG_TOP] = LONG_GRASS_TOP | SHORT_GRASS_RIGHT | SHORT_GRASS_BOTTOM | SHORT_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_OVERGROWN_BOTTOM] = SHORT_GRASS_TOP | SHORT_GRASS_RIGHT | OVERGROWN_GRASS_BOTTOM | SHORT_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_OVERGROWN_LEFT] = SHORT_GRASS_TOP | SHORT_GRASS_RIGHT | SHORT_GRASS_BOTTOM | OVERGROWN_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_OVERGROWN_LEFT_BOTTOM] = SHORT_GRASS_TOP | SHORT_GRASS_RIGHT | OVERGROWN_GRASS_BOTTOM | OVERGROWN_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_OVERGROWN_LEFT_TOP] = OVERGROWN_GRASS_TOP | SHORT_GRASS_RIGHT | SHORT_GRASS_BOTTOM | OVERGROWN_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_OVERGROWN_RIGHT] = SHORT_GRASS_TOP | OVERGROWN_GRASS_RIGHT | SHORT_GRASS_BOTTOM | SHORT_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_OVERGROWN_RIGHT_BOTTOM] = SHORT_GRASS_TOP | OVERGROWN_GRASS_RIGHT | OVERGROWN_GRASS_BOTTOM | SHORT_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_OVERGROWN_RIGHT_TOP] = OVERGROWN_GRASS_TOP | OVERGROWN_GRASS_RIGHT | SHORT_GRASS_BOTTOM | SHORT_GRASS_LEFT,
    [GFX_TEXTURE_GRASS_SHORT_OVERGROWN_TOP] = OVERGROWN_GRASS_TOP | SHORT_GRASS_RIGHT | SHORT_GRASS_BOTTOM | SHORT_GRASS_LEFT,
};

// Load textures from the assets folder
void gfx_load_textures() {
  // Eventually you could consider dynamically loading files from the assets folder. Implementing this is, however,
  // unfortunately platform dependent so it would also require some type of platform abstraction to be viable.
  char texture_paths[][128] = {
      "assets/Courage_Talisman.png",
      "assets/Death_Pot.png",
      "assets/Health_Pot.png",
      "assets/Love_Pot.png",
      "assets/Luck_Talisman.png",
      "assets/Sleep_Pot.png",
      "assets/Truth_Pot.png",
      "assets/Waterbreathing_Pot.png",
      "assets/tree.png",
      "assets/shadow.png",
      "assets/grass/long-center.png",
      "assets/grass/long-overgrown-bottom.png",
      "assets/grass/long-overgrown-left.png",
      "assets/grass/long-overgrown-left-bottom.png",
      "assets/grass/long-overgrown-left-top.png",
      "assets/grass/long-overgrown-right.png",
      "assets/grass/long-overgrown-right-bottom.png",
      "assets/grass/long-overgrown-right-top.png",
      "assets/grass/long-overgrown-top.png",
      "assets/grass/overgrown-center.png",
      "assets/grass/short-center.png",
      "assets/grass/short-long-bottom.png",
      "assets/grass/short-long-left.png",
      "assets/grass/short-long-left-bottom.png",
      "assets/grass/short-long-left-top.png",
      "assets/grass/short-long-right.png",
      "assets/grass/short-long-right-bottom.png",
      "assets/grass/short-long-right-top.png",
      "assets/grass/short-long-top.png",
      "assets/grass/short-overgrown-bottom.png",
      "assets/grass/short-overgrown-left.png",
      "assets/grass/short-overgrown-left-bottom.png",
      "assets/grass/short-overgrown-left-top.png",
      "assets/grass/short-overgrown-right.png",
      "assets/grass/short-overgrown-right-bottom.png",
      "assets/grass/short-overgrown-right-top.png",
      "assets/grass/short-overgrown-top.png",
  };

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

// Sets the blend mode to blend. This means that the alpha channel of the texture will be used to blend
void gfx_set_blend_mode_blend() {
  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_BLEND);
}

// Sets the blend mode to none. This means that the alpha channel of the texture will be ignored
void gfx_set_blend_mode_none() {
  SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_NONE);
}

// Clean up all resources
void gfx_destroy() {
  for (u32 i = 0; i < render_context.texture_atlas.count; i++) {
    SDL_DestroyTexture(render_context.texture_atlas.textures[i]);
  }

  SDL_DestroyRenderer(render_context.renderer);
  SDL_DestroyWindow(render_context.window);
  TTF_Quit();
  SDL_Quit();
}

// Draw a line from (x1, y1) to (x2, y2) with the given color
void gfx_draw_line(float x1, float y1, float x2, float y2, RGBA *color) {
  SDL_SetRenderDrawColor(render_context.renderer, (u8)(255 * color->r), (u8)(255 * color->g), (u8)(255 * color->b), (u8)(255 * color->a));
  SDL_RenderDrawLineF(render_context.renderer, x1, y1, x2, y2);
}

// Clear the screen with the background color from the render context
void gfx_clear_screen() {
  SDL_SetRenderDrawColor(
      render_context.renderer, render_context.background_color.r, render_context.background_color.g, render_context.background_color.b,
      render_context.background_color.a
  );
  SDL_RenderClear(render_context.renderer);
}

// Check if two rectangles intersect
bool gfx_frect_intersects_frect(FRect *rect_a, FRect *rect_b) {
  SDL_FRect sdl_frect_a = frect_to_sdl_frect(rect_a);
  SDL_FRect sdl_frect_b = frect_to_sdl_frect(rect_b);

  return SDL_HasIntersectionF(&sdl_frect_a, &sdl_frect_b);
}

// Check if a rectangle contains a point
bool gfx_frect_contains_point(FRect *rect, Vec2 *point) {
  return ((point->x >= rect->position.x) && (point->x < (rect->size.x)) && (point->y >= rect->position.y) && (point->y < (rect->size.y))) ? true
                                                                                                                                          : false;
}
