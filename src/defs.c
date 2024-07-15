#include "headers.h"

bool floats_equal(float a, float b) {
  return (a >= b - ath_epsilon && a <= b + ath_epsilon);
}

float frect_width(FRect *rect) {
  return rect->size.x - rect->position.x;
}

float frect_height(FRect *rect) {
  return rect->size.y - rect->position.y;
}

#ifdef _WIN32
#define string_compare_insensitive _stricmp
#define strnicmp _strnicmp
#elif __linux__
#define string_compare_insensitive strcasecmp
#define strnicmp strncasecmp
#endif
