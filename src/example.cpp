#include <SDL.h>
#include <SDL_gpu.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_TEXTURES 64

// Define the structures
typedef struct {
  Uint32 count;  // Number of textures loaded into the atlas
  GPU_Image* textures[MAX_TEXTURES];  // Array of texture pointers
} TextureAtlas;

typedef struct {
  TextureAtlas texture_atlas;  // Collection of textures
  GPU_Target* screen;  // Screen target for rendering
} RenderContext;

// Function to read shader source from a file
char* readShaderSource(const char* filePath) {
  FILE* file = fopen(filePath, "r");
  if (!file) {
    printf("Could not open file %s\n", filePath);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char* source = (char*)malloc(fileSize + 1);
  fread(source, 1, fileSize, file);
  source[fileSize] = '\0';

  fclose(file);
  return source;
}

// Function to initialize the texture atlas
int initTextureAtlas(TextureAtlas* atlas) {
  atlas->count = 0;
  return 0;
}

// Function to load a texture into the atlas
int loadTextureToAtlas(TextureAtlas* atlas, const char* filepath) {
  if (atlas->count >= MAX_TEXTURES) {
    printf("Texture atlas is full!\n");
    return -1;
  }

  GPU_Image* texture = GPU_LoadImage(filepath);
  if (!texture) {
    printf("Failed to load texture: %s\n", filepath);
    return -1;
  }

  atlas->textures[atlas->count] = texture;
  atlas->count++;

  return 0;
}

// Function to clean up the texture atlas
void freeTextureAtlas(TextureAtlas* atlas) {
  for (Uint32 i = 0; i < atlas->count; ++i) {
    GPU_FreeImage(atlas->textures[i]);
  }
  atlas->count = 0;
}

int main(int argc, char* argv[]) {
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return -1;
  }

  // Create RenderContext
  RenderContext context;

  // Initialize SDL2_GPU
  context.screen = GPU_Init(800, 600, GPU_DEFAULT_INIT_FLAGS);
  if (context.screen == NULL) {
    printf("GPU_Init failed: %s\n", GPU_PopErrorCode().details);
    SDL_Quit();
    return -1;
  }

  // Initialize texture atlas
  initTextureAtlas(&context.texture_atlas);

  // Load textures into the atlas
  if (loadTextureToAtlas(&context.texture_atlas, "texture1.png") != 0 || loadTextureToAtlas(&context.texture_atlas, "texture2.png") != 0) {
    printf("Error loading textures.\n");
    freeTextureAtlas(&context.texture_atlas);
    GPU_Quit();
    SDL_Quit();
    return -1;
  }

  // Load shaders
  char* vertexShaderSource = readShaderSource("vertex_shader.glsl");
  char* fragmentShaderSource = readShaderSource("fragment_shader.glsl");

  if (!vertexShaderSource || !fragmentShaderSource) {
    printf("Failed to load shaders.\n");
    freeTextureAtlas(&context.texture_atlas);
    GPU_Quit();
    SDL_Quit();
    return -1;
  }

  // Compile shaders
  Uint32 vertexShader = GPU_CompileShader(GPU_VERTEX_SHADER, vertexShaderSource);
  Uint32 fragmentShader = GPU_CompileShader(GPU_FRAGMENT_SHADER, fragmentShaderSource);

  free(vertexShaderSource);
  free(fragmentShaderSource);

  if (!vertexShader || !fragmentShader) {
    printf("Shader compilation failed!\n");
    freeTextureAtlas(&context.texture_atlas);
    GPU_Quit();
    SDL_Quit();
    return -1;
  }

  // Link shaders into a shader program
  Uint32 shaderProgram = GPU_LinkShaders(vertexShader, fragmentShader);
  if (!shaderProgram) {
    printf("Shader linking failed: %s\n", GPU_PopErrorCode().details);
    freeTextureAtlas(&context.texture_atlas);
    GPU_Quit();
    SDL_Quit();
    return -1;
  }

  // Activate shader program
  GPU_ShaderBlock block = GPU_LoadShaderBlock(shaderProgram, "position", "texCoord", NULL, "textureSampler");
  GPU_ActivateShaderProgram(shaderProgram, &block);

  // Main loop
  int running = 1;
  SDL_Event event;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = 0;
      }
    }

    // Clear the screen
    GPU_Clear(context.screen);

    // Render all textures in the atlas
    for (Uint32 i = 0; i < context.texture_atlas.count; ++i) {
      // Render each texture at different positions
      GPU_Blit(context.texture_atlas.textures[i], NULL, context.screen, 200 + i * 150, 300);
    }

    // Flip to the screen
    GPU_Flip(context.screen);
  }

  // Clean up
  freeTextureAtlas(&context.texture_atlas);
  GPU_FreeShaderProgram(shaderProgram);
  GPU_Quit();
  SDL_Quit();

  return 0;
}
