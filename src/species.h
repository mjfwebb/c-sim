#pragma once

#include "headers.h"

#define SPECIES_LIST \
  X(Human)           \
  X(Rock)            \
  X(Tree)

#define X(name) Species__##name,
typedef enum { SPECIES_LIST Species_Count } Species;
#undef X

#define X(name) [Species__##name] = #name,
static const char* Species__Strings[] = {SPECIES_LIST};
#undef X
