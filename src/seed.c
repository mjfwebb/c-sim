#include <stddef.h>
#include <string.h>

unsigned int hash(unsigned int x)
{
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = (x >> 16) ^ x;
  return x;
}

int create_seed(char *seed_string)
{
  int seed = 0;
  size_t length_of_seed = strlen(seed_string);
  for (int i = 0; i < length_of_seed; i++)
  {
    char character = seed_string[i];
    seed ^= character;
  }

  return hash(seed);
}