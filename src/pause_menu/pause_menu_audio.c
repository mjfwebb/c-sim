#include "headers.h"

void pause_menu_draw_audio(int number_of_elements, int element_id_start, float element_height) {
  number_of_elements = 3;
  int element_count = 0;
  float element_heights[] = {64.0f, 150.0f, 150.0f, 150.0f};
  FRect container = get_container(element_heights, array_count(element_heights));

  pause_menu_title((PauseMenuTitle){
      .id = 0,
      .text = "Audio",
      .rect = (FRect){.left = container.left, .top = 150, .right = container.right, .bottom = container.top + 64.0f},
  });

  element_count++;
  float current_element_y = current_element_position_y(element_heights, element_count);
  PauseMenuSliderResult master_volume_slider = draw_pause_menu_slider((PauseMenuSlider){
      .id = element_id_start + element_count,
      .rect =
          {.left = container.left,
           .top = container.top + current_element_y,
           .right = container.right,
           .bottom = container.top + current_element_y + element_height},
      .min = 0,
      .max = MAX_VOLUME,
      .value = audio_context.master_volume,
      .text = "Master Volume",
  });
  if (master_volume_slider.changed) {
    int new_value_actual = clamp(master_volume_slider.new_value, 0, MAX_VOLUME);
    audio_set_master_volume(new_value_actual);
  }

  element_count++;
  current_element_y = current_element_position_y(element_heights, element_count);
  PauseMenuSliderResult music_volume_slider = draw_pause_menu_slider((PauseMenuSlider){
      .id = element_id_start + element_count,
      .rect =
          {.left = container.left,
           .top = container.top + current_element_y,
           .right = container.right,
           .bottom = container.top + current_element_y + element_height},
      .min = 0,
      .max = MAX_VOLUME,
      .value = audio_context.music_volume,
      .text = "Music Volume",
  });
  if (music_volume_slider.changed) {
    int new_value_actual = clamp(music_volume_slider.new_value, 0, MAX_VOLUME);
    audio_set_music_volume(new_value_actual);
  }

  element_count++;
  current_element_y = current_element_position_y(element_heights, element_count);
  PauseMenuSliderResult sound_volume_slider = draw_pause_menu_slider((PauseMenuSlider){
      .id = element_id_start + element_count,
      .rect =
          {.left = container.left,
           .top = container.top + current_element_y,
           .right = container.right,
           .bottom = container.top + current_element_y + element_height},
      .min = 0,
      .max = MAX_VOLUME,
      .value = audio_context.sound_volume,
      .text = "Sound Effect Volume",
  });
  if (sound_volume_slider.changed) {
    int new_value_actual = clamp(sound_volume_slider.new_value, 0, MAX_VOLUME);
    audio_set_sound_volume(new_value_actual);
  }
}
