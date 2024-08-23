void audio_load_sound(enum SoundEffect sound, const char* filename);

void audio_load_music(enum MusicTrack track, const char* filename);

void audio_play_sound(enum SoundEffect sound);

void audio_play_music(enum MusicTrack track);

void audio_stop_music(void);

void audio_cleanup(void);

int audio_init(void);

int audio_load_sounds(void);