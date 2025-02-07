#pragma once

#include "headers.h"

// These should be defined in order of click precedence
#define SPECIES_LIST \
  X(Human)           \
  X(Rock)            \
  X(Quarried_Rock)   \
  X(Lumber)          \
  X(Tree)

#define X(name) Species__##name,
typedef enum { SPECIES_LIST Species_Count } Species;
#undef X

#define X(name) [Species__##name] = #name,
static const char* Species__Strings[] = {SPECIES_LIST};
#undef X
