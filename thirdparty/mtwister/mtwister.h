#ifndef __MTWISTER_H
#define __MTWISTER_H

// https://github.com/ESultanik/mtwister

#define STATE_VECTOR_LENGTH 624
#define STATE_VECTOR_M      397 /* changes to STATE_VECTOR_LENGTH also require changes to this */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagMTRand {
  unsigned long mt[STATE_VECTOR_LENGTH];
  int index;
} MTRand;

MTRand seedRand(unsigned long seed);
unsigned long genRandLong(MTRand *rand);
double genRand(MTRand *rand);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <inttypes.h>

class MTRandWrapper {
 public:
  MTRandWrapper(uint32_t seed = 123) {
    mtRand = seedRand(seed);
  }

  void seed(uint32_t seed) {
    mtRand = seedRand(seed);
  }

  uint32_t randomLong() {
    return genRandLong(&mtRand);
  }

  double random() {
    return genRand(&mtRand);
  }

  bool randomBool() {
    return (randomLong() & 0x01) > 0;
  }
 private:
  MTRand mtRand;
};

#endif

#endif /* #ifndef __MTWISTER_H */
