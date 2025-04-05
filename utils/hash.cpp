#include "hash.h"

namespace mbz {
namespace utils {
namespace heap {

uint32_t hasher() {
  return 2166136261u;
}

uint32_t hasher(uint32_t hash, const char *s) {
  int i = 0;
  while (s[i] != '\0') {
    hash ^= (uint8_t) (s[i]);
    hash *= 16777619;
    i++;
  }
  return hash;
}

uint32_t hasher(uint32_t hash, const void *value, int size) {
  const uint8_t *bytes = (const uint8_t*) value;
  for (int i = 0; i < size; i++) {
    hash ^= bytes[i];
    hash *= 16777619;
  }
  return hash;
}


}
}
}
