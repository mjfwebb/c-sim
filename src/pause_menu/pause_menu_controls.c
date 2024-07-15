#include "headers.h"

void pause_menu_draw_controls(int number_of_elements, int element_id_start, float element_height) {
  number_of_elements = 3;
  int element_count = 0;
  float element_heights[] = {64.0f, element_height};
  FRect container = get_container(element_heights, array_count(element_heights));

  pause_menu_title((PauseMenuTitle){
      .id = 0,
      .text = "Controls",
      .rect = (FRect
      ){.position.x = container.position.x, .position.y = container.position.y, .size.x = container.size.x, .size.y = container.position.y + 64.0f},
  });

  element_count++;
  float current_element_y = current_element_position_y(element_heights, element_count);
  if (draw_pause_menu_dropdown((PauseMenuDropdown){
          .id = element_id_start + element_count,
          .rect =
              {.position.x = container.position.x,
               .position.y = container.position.y + current_element_y,
               .size.x = container.size.x,
               .size.y = container.position.y + current_element_y + element_height},
          .text = "Keyboard",
      })) {
    // Then we have clicked continue
    toggle_pause_menu();
  }

  element_count++;
  current_element_y = current_element_position_y(element_heights, element_count);
  if (draw_pause_menu_dropdown((PauseMenuDropdown){
          .id = element_id_start + element_count,
          .rect =
              {.position.x = container.position.x,
               .position.y = container.position.y + current_element_y,
               .size.x = container.size.x,
               .size.y = container.position.y + current_element_y + element_height},
          .text = "Mouse",
      })) {
    // Then we have clicked continue
    toggle_pause_menu();
  }

  element_count++;
  current_element_y = current_element_position_y(element_heights, element_count);
  if (draw_pause_menu_dropdown((PauseMenuDropdown){
          .id = element_id_start + element_count,
          .rect =
              {.position.x = container.position.x,
               .position.y = container.position.y + current_element_y,
               .size.x = container.size.x,
               .size.y = container.position.y + current_element_y + element_height},
          .text = "Mouse",
      })) {
    // Then we have clicked continue
    toggle_pause_menu();
  }
}
