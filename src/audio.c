#include "headers.h"

#define MAX_SOUNDS 10
#define MAX_MUSIC 5
#define MAX_VOLUME 128

// Enums for sound effects and music tracks
enum SoundEffect {
  SOUND_HIT_WOOD_1,
  SOUND_HIT_WOOD_2,
  SOUND_HIT_WOOD_3,
  KILL_WOOD_1,
  SOUND_HIT_ROCK_1,
  SOUND_HIT_ROCK_2,
  SOUND_HIT_ROCK_3,
  KILL_ROCK_1,
  SOUND_HIT_ORGANIC_1,
  SOUND_HIT_ORGANIC_2,
  SOUND_HIT_ORGANIC_3,
  KILL_ORGANIC_1,
  SOUND_COUNT
};

enum MusicTrack { MUSIC_MAIN_THEME, MUSIC_MENU_THEME, MUSIC_COUNT };

typedef struct {
  Mix_Chunk* sounds[MAX_SOUNDS];
  Mix_Music* music[MAX_MUSIC];
  int sound_count;
  int music_count;
  int music_volume;
  int sound_volume;
  int master_volume;
} AudioContext;

AudioContext audio_context = {0};

void audio_set_music_volume(int volume) {
  audio_context.music_volume = volume;

  int music_volume = (audio_context.music_volume * audio_context.master_volume) / MAX_VOLUME;

  Mix_VolumeMusic(music_volume);
}

void audio_set_sound_volume(int volume) {
  audio_context.sound_volume = volume;

  int sound_volume = ((int)(audio_context.sound_volume * render_context.camera.zoom) * audio_context.master_volume) / MAX_VOLUME;

  Mix_Volume(-1, sound_volume);  // -1 means all channels
}

void audio_set_master_volume(int volume) {
  audio_context.master_volume = volume;

  audio_set_music_volume(audio_context.music_volume);
  audio_set_sound_volume(audio_context.sound_volume);
}

void audio_load_sound(enum SoundEffect sound, const char* filename) {
  if (sound < SOUND_COUNT && audio_context.sound_count < MAX_SOUNDS) {
    audio_context.sounds[sound] = Mix_LoadWAV(filename);
    if (audio_context.sounds[sound] == NULL) {
      print("Failed to load sound effect: %s\n", Mix_GetError());
    } else {
      audio_context.sound_count++;
    }
  }
}

void audio_load_music(enum MusicTrack track, const char* filename) {
  if (track < MUSIC_COUNT && audio_context.music_count < MAX_MUSIC) {
    audio_context.music[track] = Mix_LoadMUS(filename);
    if (audio_context.music[track] == NULL) {
      print("Failed to load music: %s\n", Mix_GetError());
    } else {
      audio_context.music_count++;
    }
  }
}

void audio_play_sound(enum SoundEffect sound) {
  if (sound < SOUND_COUNT && audio_context.sounds[sound] != NULL) {
    Mix_PlayChannel(-1, audio_context.sounds[sound], 0);
  }
}

void audio_play_music(enum MusicTrack track) {
  if (track < MUSIC_COUNT && audio_context.music[track] != NULL) {
    Mix_PlayMusic(audio_context.music[track], -1);  // -1 for infinite loop
  }
}

void audio_stop_music(void) {
  Mix_HaltMusic();
}

void audio_cleanup(void) {
  for (int i = 0; i < SOUND_COUNT; i++) {
    if (audio_context.sounds[i] != NULL) {
      Mix_FreeChunk(audio_context.sounds[i]);
    }
  }

  for (int i = 0; i < MUSIC_COUNT; i++) {
    if (audio_context.music[i] != NULL) {
      Mix_FreeMusic(audio_context.music[i]);
    }
  }

  Mix_CloseAudio();
  SDL_Quit();
}

int audio_init(void) {
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    print("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
    return EXIT_FAILURE;
  }

  // audio_context.master_volume = 16;
  // audio_context.music_volume = 16;
  // audio_context.sound_volume = 16;
  audio_set_music_volume(16);
  audio_set_master_volume(128);
  audio_set_sound_volume(16);

  return EXIT_SUCCESS;
}

int audio_load_sounds(void) {
  // Load sound effects
  audio_load_sound(SOUND_HIT_WOOD_1, "assets/audio/sounds/hit_wood_1.mp3");
  audio_load_sound(SOUND_HIT_WOOD_2, "assets/audio/sounds/hit_wood_2.mp3");
  audio_load_sound(SOUND_HIT_WOOD_3, "assets/audio/sounds/hit_wood_3.mp3");
  audio_load_sound(KILL_WOOD_1, "assets/audio/sounds/kill_wood_1.mp3");
  audio_load_sound(SOUND_HIT_ROCK_1, "assets/audio/sounds/hit_rock_1.mp3");
  audio_load_sound(SOUND_HIT_ROCK_2, "assets/audio/sounds/hit_rock_2.mp3");
  audio_load_sound(SOUND_HIT_ROCK_3, "assets/audio/sounds/hit_rock_3.mp3");
  audio_load_sound(KILL_ROCK_1, "assets/audio/sounds/kill_rock_1.mp3");
  audio_load_sound(SOUND_HIT_ORGANIC_1, "assets/audio/sounds/hit_organic_1.mp3");
  audio_load_sound(SOUND_HIT_ORGANIC_2, "assets/audio/sounds/hit_organic_2.mp3");
  audio_load_sound(SOUND_HIT_ORGANIC_3, "assets/audio/sounds/hit_organic_3.mp3");
  audio_load_sound(KILL_ORGANIC_1, "assets/audio/sounds/kill_organic_1.mp3");

  // Load music tracks
  audio_load_music(MUSIC_MAIN_THEME, "assets/audio/music/main_theme.mp3");
  audio_load_music(MUSIC_MENU_THEME, "assets/audio/music/menu_theme.mp3");

  return 0;
}