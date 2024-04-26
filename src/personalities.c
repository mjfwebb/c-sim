#include "headers.h"

#define X(name) [Personalities__##name] = #name,
static const char* Personality__Strings[] = {PERSONALITY_LIST};
#undef X
