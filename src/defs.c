#include "headers.h"

// FIXME: This can only produce numbers up to RAND_MAX which is at least 32767
int random_int_between(int min, int max) {
  return min + (rand() % (max - min));
}

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
#define string_compare_insensitive_n _strnicmp
#elif __linux__
#define string_compare_insensitive strcasecmp
#define string_compare_insensitive_n strncasecmp
#endif
