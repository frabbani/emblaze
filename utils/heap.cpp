#include "heap.h"

#include <cstring>
#include <climits>
#include <algorithm>

namespace mbz {
namespace utils {
namespace heap {

Heap::Heap(int hunkSize) {

  hunkSize = std::clamp(hunkSize, 4 * 1024, 256 * 1024 * 1024);
  hunk = std::vector<uint8_t>(hunkSize);
  memset(hunk.data(), 0, hunk.size());
  memset(table, 0, sizeof(table));
}

Heap::Tile *Heap::reserve(int size) {
  int rem = size & 15;
  if (rem > 0)
    size += (16 - rem);

  auto hash = getHashOf(size);

  if( table[hash] == nullptr ){
    reservations++;
   Tile** tile = &table[hash];
   int loc = fetch(sizeof(Tile) + size);
   (*tile) = (Tile*) &hunk[loc];
   (*tile)->pos = loc;
   (*tile)->block.size = size;
   (*tile)->block.data = reinterpret_cast<void*>(&hunk[loc + sizeof(Tile)]);
   (*tile)->used = true;
   (*tile)->hash = hash;
   (*tile)->next = nullptr;
   return *tile;
  }

  int lowest = INT_MAX;
  Tile *find = nullptr;
  Tile *walk = table[hash];
  // find a free tile with the best memory match
  while (walk) {
    if (!walk->used) {
      if (walk->block.size >= size && walk->block.size < lowest) {
        find = walk;
        lowest = walk->block.size;
      }
    }
    walk = walk->next;
  }
  if (find) {
    recycles++;
    find->used = true;
    return find;
  }

  reservations++;
  Tile **tile = &table[hash]->next;
  while ((*tile))
    tile = &(*tile)->next;

  int loc = fetch(sizeof(Tile) + size);
  (*tile) = (Tile*) &hunk[loc];
  (*tile)->pos = loc;
  (*tile)->block.size = size;
  (*tile)->block.data = reinterpret_cast<void*>(&hunk[loc + sizeof(Tile)]);
  (*tile)->used = true;
  (*tile)->hash = hash;
  (*tile)->next = nullptr;
  return *tile;
}

}
}
}
