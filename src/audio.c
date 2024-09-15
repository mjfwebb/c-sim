#include "headers.h"

void audio_set_music_volume(int volume) {
  audio_context.music_volume = volume;

  int music_volume = (audio_context.music_volume * audio_context.master_volume) / MAX_VOLUME;

  // Now convert to 0 - 128 because SDL2_Audio uses a maximum of 128 instead of 100
  int converted_volume = music_volume * 128 / 100;

  Mix_VolumeMusic(converted_volume);
}

void audio_set_sound_volume(int volume) {
  audio_context.sound_volume = volume;

  int sound_volume = ((int)(audio_context.sound_volume * render_context.camera.target_zoom) * audio_context.master_volume) / MAX_VOLUME;
  print("setting sound volume to %d", sound_volume);

  // Now convert to 0 - 128 because SDL2_Audio uses a maximum of 128 instead of 100
  int converted_volume = sound_volume * 128 / 100;

  Mix_Volume(-1, converted_volume);  // -1 means all channels
}

void audio_set_master_volume(int volume) {
  audio_context.master_volume = volume;

  audio_set_music_volume(audio_context.music_volume);
  audio_set_sound_volume(audio_context.sound_volume);
}

void audio_load_sound(enum SoundEffect sound, const char* filename) {
  // TODO: Add check that file actually exists, because apparently SDL doesn't do that Q_Q
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
  audio_set_master_volume(64);
  audio_set_sound_volume(16);

  return EXIT_SUCCESS;
}

int audio_load_sounds(void) {
  // Load sound effects
  audio_load_sound(SOUND_HIT_WOOD_1, "assets/audio/sounds/hit_wood_1.mp3");
  audio_load_sound(SOUND_HIT_WOOD_2, "assets/audio/sounds/hit_wood_2.mp3");
  audio_load_sound(SOUND_HIT_WOOD_3, "assets/audio/sounds/hit_wood_3.mp3");
  audio_load_sound(SOUND_KILL_WOOD_1, "assets/audio/sounds/kill_wood_1.mp3");
  audio_load_sound(SOUND_HIT_ROCK_1, "assets/audio/sounds/hit_rock_1.mp3");
  audio_load_sound(SOUND_HIT_ROCK_2, "assets/audio/sounds/hit_rock_2.mp3");
  audio_load_sound(SOUND_KILL_ROCK_1, "assets/audio/sounds/kill_rock_1.mp3");
  audio_load_sound(SOUND_HIT_ORGANIC_1, "assets/audio/sounds/hit_organic_1.mp3");
  audio_load_sound(SOUND_HIT_ORGANIC_2, "assets/audio/sounds/hit_organic_2.mp3");
  audio_load_sound(SOUND_HIT_ORGANIC_3, "assets/audio/sounds/hit_organic_3.mp3");
  audio_load_sound(SOUND_KILL_ORGANIC_1, "assets/audio/sounds/kill_organic_1.mp3");
  audio_load_sound(SOUND_CULTIVATE_1, "assets/audio/sounds/cultivate_1.mp3");
  audio_load_sound(SOUND_HEAL_1, "assets/audio/sounds/heal_1.mp3");

  // Load music tracks
  audio_load_music(MUSIC_MAIN_THEME, "assets/audio/music/main_theme.mp3");
  audio_load_music(MUSIC_MENU_THEME, "assets/audio/music/menu_theme.mp3");

  return 0;
}