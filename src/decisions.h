#pragma once

#include "headers.h"

#define Decisions_LIST \
  X(Wander)            \
  X(Cultivate)         \
  X(Find_Tree)         \
  X(Find_Rock)         \
  X(Find_Human)        \
  X(Chop_Tree)         \
  X(Mine_Rock)         \
  X(Attack_Human)      \
  X(Flee)              \
  X(Wait)

#define X(name) Decisions__##name,
typedef enum { Decisions_LIST Decisions_Count } Decisions;
#undef X

#define X(name) [Decisions__##name] = #name,
static const char* Decisions__Strings[] = {Decisions_LIST};
#undef X

void make_decision(int entity_id);