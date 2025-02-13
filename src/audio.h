#pragma once

#include "headers.h"

#define MAX_SOUNDS 128
#define MAX_MUSIC 5
#define MAX_VOLUME 100

// Enums for sound effects and music tracks
typedef enum SoundEffect {
  SOUND_HIT_WOOD_1,
  SOUND_HIT_WOOD_2,
  SOUND_HIT_WOOD_3,
  SOUND_KILL_WOOD_1,
  SOUND_HIT_ROCK_1,
  SOUND_HIT_ROCK_2,
  SOUND_HIT_ROCK_3,
  SOUND_KILL_ROCK_1,
  SOUND_HIT_ORGANIC_1,
  SOUND_HIT_ORGANIC_2,
  SOUND_HIT_ORGANIC_3,
  SOUND_KILL_ORGANIC_1,
  SOUND_CULTIVATE_1,
  SOUND_HEAL_1,
  SOUND_COUNT
} SoundEffect;

typedef enum MusicTrack { MUSIC_MAIN_THEME, MUSIC_MENU_THEME, MUSIC_COUNT } MusicTrack;

typedef struct {
  Mix_Chunk* sounds[MAX_SOUNDS];
  Mix_Music* music[MAX_MUSIC];
  int sound_count;
  int music_count;
  int music_volume;  // 0 - 100, to be converted to 0 - 128 to be used with SDL_Audio
  int sound_volume;  // 0 - 100, to be converted to 0 - 128 to be used with SDL_Audio
  int master_volume;  // 0 - 100, to be converted to 0 - 128 to be used with SDL_Audio
} AudioContext;

void audio_set_master_volume(int volume);

void audio_set_sound_volume(int volume);

void audio_set_music_volume(int volume);

void audio_load_sound(enum SoundEffect sound, const char* filename);

void audio_load_music(enum MusicTrack track, const char* filename);

void audio_play_sound(enum SoundEffect sound);

void audio_play_music(enum MusicTrack track);

void audio_stop_music(void);

void audio_cleanup(void);

int audio_init(void);

int audio_load_sounds(void);

AudioContext audio_context = {0};