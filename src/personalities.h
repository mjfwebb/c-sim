#pragma once

#include "headers.h"

#define PERSONALITY_LIST \
  X(Adventurous)         \
  X(Aggressive)          \
  X(Aloof)               \
  X(Altruistic)          \
  X(Ambitious)           \
  X(Analytical)          \
  X(Artistic)            \
  X(Assertive)           \
  X(Brave)               \
  X(Carefree)            \
  X(Cautious)            \
  X(Charismatic)         \
  X(Compassionate)       \
  X(Confident)           \
  X(Courageous)          \
  X(Creative)            \
  X(Cruel)               \
  X(Curious)             \
  X(Cynical)             \
  X(Deceptive)           \
  X(Detached)            \
  X(Dreamy)              \
  X(Earnest)             \
  X(Emotional)           \
  X(Empathetic)          \
  X(Extroverted)         \
  X(Flexible)            \
  X(Follower)            \
  X(Friendly)            \
  X(Generous)            \
  X(Greedy)              \
  X(Hopeful)             \
  X(Hostile)             \
  X(Humorous)            \
  X(Impulsive)           \
  X(Indifferent)         \
  X(Innocent)            \
  X(Innovative)          \
  X(Insecure)            \
  X(Introverted)         \
  X(Jaded)               \
  X(Kind)                \
  X(Lazy)                \
  X(Leader)              \
  X(Logical)             \
  X(Loyal)               \
  X(Meticulous)          \
  X(Mysterious)          \
  X(Naive)               \
  X(Neutral)             \
  X(Obedient)            \
  X(Openbook)            \
  X(Optimistic)          \
  X(Passionate)          \
  X(Peaceful)            \
  X(Pessimistic)         \
  X(Planner)             \
  X(Playful)             \
  X(Pragmatic)           \
  X(Realistic)           \
  X(Rebellious)          \
  X(Responsible)         \
  X(Romantic)            \
  X(Selfish)             \
  X(Seriousminded)       \
  X(Serious)             \
  X(Shortsighted)        \
  X(Shy)                 \
  X(Skeptical)           \
  X(Sloppy)              \
  X(Spontaneous)         \
  X(Stingy)              \
  X(Stoic)               \
  X(Stubborn)            \
  X(Submissive)          \
  X(Timid)               \
  X(Traditional)         \
  X(Trustworthy)         \
  X(Unreliable)          \
  X(Visionary)           \
  X(Wisecracking)        \
  X(Wise)                \
  X(Worldly)

#define X(name) Personalities__##name,
typedef enum { PERSONALITY_LIST Personality_Count } Personality;
#undef X

#define X(name) [Personalities__##name] = #name,
static const char* Personality__Strings[] = {PERSONALITY_LIST};
#undef X
