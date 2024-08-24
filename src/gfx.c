#include "headers.h"

#define MAX_TEXTURES 128

// Function to read shader source from a file
char *read_shader_source(const char *file_path) {
  FILE *file = fopen(file_path, "r");
  if (!file) {
    printf("Could not open file %s\n", file_path);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *source = (char *)malloc(fileSize + 1);
  fread(source, 1, fileSize, file);
  source[fileSize] = '\0';

  fclose(file);
  return source;
}

// Initialize the graphics context, including SDL and SDL_image, and create a window and renderer
int gfx_init(void) {
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Could not initialize sdl2: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  if (TTF_Init() == -1) {
    fprintf(stderr, "Could not initialize ttf: %s\n", TTF_GetError());
    return EXIT_FAILURE;
  }

  GPU_Target *target = GPU_Init(SCREEN_WIDTH, SCREEN_HEIGHT, GPU_DEFAULT_INIT_FLAGS);
  if (!target) {
    printf("GPU_Init failed: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }
  render_context.target = target;

  // Load shaders
  char *vertex_shader_source = read_shader_source("shaders/vertex_shader.glsl");
  char *fragment_shader_source = read_shader_source("shaders/fragment_shader.glsl");

  if (!vertex_shader_source || !fragment_shader_source) {
    printf("Failed to load shaders.\n");
    return EXIT_FAILURE;
  }

  // Compile shaders
  Uint32 vertex_shader = GPU_CompileShader(GPU_VERTEX_SHADER, vertex_shader_source);
  Uint32 fragment_shader = GPU_CompileShader(GPU_FRAGMENT_SHADER, fragment_shader_source);

  free(vertex_shader_source);
  free(fragment_shader_source);

  if (!vertex_shader || !fragment_shader) {
    printf("Shader compilation failed!\n");
    return EXIT_FAILURE;
  }

  // Link shaders into a shader program
  Uint32 shader_program = GPU_LinkShaders(vertex_shader, fragment_shader);
  if (!shader_program) {
    printf("Shader linking failed: %s\n", GPU_PopErrorCode().details);
    return EXIT_FAILURE;
  }
  render_context.shader_program = shader_program;

  // Activate shader program
  GPU_ShaderBlock block = GPU_LoadShaderBlock(shader_program, "position", "texCoord", NULL, "textureSampler");
  GPU_ActivateShaderProgram(shader_program, &block);

  // TODO: Check if this is needed, probably not :)
  // Set scale quality to best
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
  // Set render batching
  SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");

  return EXIT_SUCCESS;
}

// Update the screen with the current render context since the last call to gfx_render_present
void gfx_render_present(void) {
  // SDL_RenderPresent(render_context);
  GPU_Flip(render_context.target);
}

// Get the window size and store it in out_w and out_h
void gfx_get_window_size(int *out_w, int *out_h) {
  if (render_context.target != NULL) {
    *out_w = render_context.target->w;
    *out_h = render_context.target->h;
  } else {
    *out_w = 0;
    *out_h = 0;
  }
}

// Internal function to convert a rect to an SDL_Rect
static SDL_FRect frect_to_sdl_frect(FRect *rect) {
  return (SDL_FRect){
      .x = rect->left,
      .y = rect->top,
      .w = frect_width(rect),
      .h = frect_height(rect),
  };
}

// Draw a rectangle with the given color
void gfx_draw_frect(FRect *rect, RGBA *color) {
  // SDL_SetRenderDrawColor(render_context.renderer, (u8)(255 * color->r), (u8)(255 * color->g), (u8)(255 * color->b), (u8)(255 * color->a));

  // int result = SDL_RenderDrawRectF(render_context.renderer, &sdl_frect);
  // assert(result == 0);

  // Draw the rectangle using GPU_Rectangle
  GPU_Rectangle(
      render_context.target, rect->left, rect->top, rect->right, rect->bottom,
      GPU_MakeColor((Uint8)(255 * color->r), (Uint8)(255 * color->g), (Uint8)(255 * color->b), (Uint8)(255 * color->a))
  );
}

// Draw a filled rectangle with the given color
void gfx_draw_frect_filled(FRect *rect, RGBA *color) {
  // SDL_FRect sdl_frect = frect_to_sdl_frect(rect);
  // SDL_SetRenderDrawColor(render_context.renderer, (u8)(255 * color->r), (u8)(255 * color->g), (u8)(255 * color->b), (u8)(255 * color->a));

  // int result = SDL_RenderFillRectF(render_context.renderer, &sdl_frect);
  // assert(result == 0);

  // Draw the filled rectangle using GPU_RectangleFilled
  GPU_RectangleFilled(
      render_context.target, rect->left, rect->top, rect->right, rect->bottom,
      GPU_MakeColor((Uint8)(255 * color->r), (Uint8)(255 * color->g), (Uint8)(255 * color->b), (Uint8)(255 * color->a))
  );
};

void gfx_draw_texture(int texture_id, float x, float y) {
  GPU_Blit(render_context.texture_atlas.textures[texture_id], NULL, render_context.target, x, y);
}

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
int gfx_load_textures(void) {
  char texture_paths[][MAX_TEXTURES] = {
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
      "assets/rock.png",
      "assets/rock-smashed.png",
      "assets/tombstone.png",
      "assets/tree/1.png",
      "assets/tree/2.png",
      "assets/tree/3.png",
      "assets/tree/4.png",
      "assets/tree/5.png",
      "assets/tree/6.png",
      "assets/tree/1-stump.png",
      "assets/tree/2-stump.png",
      "assets/tree/3-stump.png",
      "assets/tree/4-stump.png",
      "assets/tree/5-stump.png",
      "assets/tree/6-stump.png",
  };

  for (u32 i = 0; i < array_count(texture_paths); i++) {
    GPU_Image *texture = GPU_LoadImage(texture_paths[i]);
    if (!texture) {
      printf("Failed to load texture: %s\n", SDL_GetError());
      return EXIT_FAILURE;
    }

    render_context.texture_atlas.textures[i] = texture;
  }

  return EXIT_SUCCESS;
}

// TODO: Fix this
// Sets the blend mode to blend. This means that the alpha channel of the texture will be used to blend
// void gfx_set_blend_mode_blend(void) {
//   // SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_BLEND);
//   GPU_SetBlendMode(render_context.target, GPU_BLEND_PREMULTIPLIED_ALPHA);
// }

// TODO: Fix this
// Sets the blend mode to none. This means that the alpha channel of the texture will be ignored
// void gfx_set_blend_mode_none(void) {
//   // SDL_SetRenderDrawBlendMode(render_context.renderer, SDL_BLENDMODE_NONE);
//   GPU_SetBlendMode(render_context.target, GPU_BLEND_NORMAL);
// }

// Clean up all resources
void gfx_destroy(void) {
  for (u32 i = 0; i < render_context.texture_atlas.count; i++) {
    GPU_FreeImage(render_context.texture_atlas.textures[i]);
  }

  // TODO:
  GPU_FreeShaderProgram(render_context.shader_program);
  GPU_Quit();
  TTF_Quit();
  SDL_Quit();
}

// Draw a line from (x1, y1) to (x2, y2) with the given color
void gfx_draw_line(float x1, float y1, float x2, float y2, RGBA *color) {
  // Set the drawing color and draw the line
  GPU_Line(
      render_context.target,  // The rendering target (e.g., the screen)
      x1, y1,  // Starting point of the line
      x2, y2,  // Ending point of the line
      GPU_MakeColor(  // Line color
          (Uint8)(255 * color->r), (Uint8)(255 * color->g), (Uint8)(255 * color->b), (Uint8)(255 * color->a)
      )
  );
}

// Clear the screen with the background color from the render context
void gfx_clear_screen(void) {
  // SDL_SetRenderDrawColor(
  //     render_context.renderer, render_context.background_color.r, render_context.background_color.g, render_context.background_color.b,
  //     render_context.background_color.a
  // );
  // SDL_RenderClear(render_context.renderer);
  GPU_Clear(render_context.target);
}

// Check if two rectangles intersect
bool gfx_frect_intersects_frect(FRect *a, FRect *b) {
  return (a->left <= b->right && b->left <= a->right && a->top <= b->bottom && b->top <= a->bottom);
}

// Check if a rectangle contains a point
bool gfx_frect_contains_point(FRect *rect, Vec2 *point) {
  return ((point->x >= rect->left) && (point->x < (rect->right)) && (point->y >= rect->top) && (point->y < (rect->bottom)));
}

HSV rgb_to_hsv(RGBA rgb) {
  HSV color = {0};
  float rd = rgb.r;
  float gd = rgb.g;
  float bd = rgb.b;
  float tmax = max(rd, max(gd, bd));
  float tmin = min(rd, min(gd, bd));
  float dt = tmax - tmin;

  if (floats_equal(dt, 0)) {
    color.h = 0;
  } else if (floats_equal(tmax, rd)) {
    color.h = fmodf((gd - bd) / dt, 6);
  } else if (floats_equal(tmax, gd)) {
    color.h = (bd - rd) / dt + 2;
  } else {
    color.h = (rd - gd) / dt + 4;
  }

  color.h *= 60;
  if (color.h < 0) {
    color.h += 360;
  }

  color.s = (floats_equal(tmax, 0)) ? 0 : dt / tmax;
  color.v = tmax;
  return color;
}

RGBA hsv_to_rgb(HSV hsv) {
  RGBA color;

  // Red channel
  float k = fmodf((5.0f + hsv.h / 60.0f), 6);
  float t = 4.0f - k;
  k = (t < k) ? t : k;
  k = (k < 1) ? k : 1;
  k = (k > 0) ? k : 0;
  color.r = (hsv.v - hsv.v * hsv.s * k);

  // Green channel
  k = fmodf((3.0f + hsv.h / 60.0f), 6);
  t = 4.0f - k;
  k = (t < k) ? t : k;
  k = (k < 1) ? k : 1;
  k = (k > 0) ? k : 0;
  color.g = (hsv.v - hsv.v * hsv.s * k);

  // Blue channel
  k = fmodf((1.0f + hsv.h / 60.0f), 6);
  t = 4.0f - k;
  k = (t < k) ? t : k;
  k = (k < 1) ? k : 1;
  k = (k > 0) ? k : 0;
  color.b = (hsv.v - hsv.v * hsv.s * k);
  color.a = 1;

  return color;
}