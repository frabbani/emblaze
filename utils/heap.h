#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace mbz {
namespace utils {
namespace heap{

struct Heap : public std::enable_shared_from_this<Heap> {
  static constexpr int tableSize = 1024;

  struct Tile {
    friend class Heap;
   protected:
    struct Block {
      int size;
      void *data;
    };
    uint32_t hash;
    bool used = false;
    int pos;
    Block block;
    Tile *next = nullptr;
   public:

    template<class T>
    int size() const {
      return block.size / sizeof(T);
    }

    template<class T>
    T* operator()() {
      if (0 == size<T>())
        return nullptr;
      return reinterpret_cast<T*>(block.data);
    }

    template<class T>
    const T *kp() const {
      if (0 == size<T>())
        return nullptr;
      return reinterpret_cast<const T*>(block.data);
    }

    template<class T>
    T *p(){
      if (0 == size<T>())
        return nullptr;
      return reinterpret_cast<T*>(block.data);
    }
  };

  std::shared_ptr<Heap> getShared() {
      return shared_from_this();
  }
 protected:

  uint32_t getHashOf(uint32_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x);
    return (x % tableSize);
  }

  int fetch(int size){
    pos += size;
    return (pos - size);
  }

  Tile *table[tableSize];
  std::vector<uint8_t> hunk;
  int pos = 0;

 public:
  int recycles = 0;
  int reservations = 0;

  Heap(int hunkSize = 16 * 1024 * 1024);
  int remaining() const {
    return total() - pos;
  }
  int total() const {
    return (int) hunk.size();
  }
  Tile *reserve(int size);
  void release(Tile *tile) {
    if( tile )
      tile->used = false;
  }
};

}
}
}
