#pragma once

#include "heap.h"

#include <functional>
#include <type_traits>

extern int printf(const char *format, ...);

namespace mbz {
namespace utils {
namespace heap {

uint32_t hasher();
uint32_t hasher(uint32_t hash, const char *s);
uint32_t hasher(uint32_t hash, const void *value, int size);

template<typename T>
uint32_t hasher(uint32_t hash, const T &value) {
  return hasher(hash, reinterpret_cast<const void*>(&value), sizeof(T));
}


struct Hashable {
 public:
  virtual ~Hashable() = default;
  virtual uint32_t getHashOf() const = 0;
};

template<typename T>
struct Hashmap {
  struct E {
    uint32_t hash;
    Heap::Tile *data = nullptr;
    Heap::Tile *next = nullptr;
    int size() const {
      if (!data)
        return 0;
      if (!next)
        return 1;
      return 1 + next->p<E>()->size();
    }

    T* get() {
      if (data) {
        T *p = data->p<T>();
        return p;
      }
      return nullptr;
    }

    const T* kget() const {
      if (data) {
        return data->kp<T>();
      }
      return nullptr;
    }

    E* nextE() {
      if (next) {
        return next->p<E>();
      }
      return nullptr;
    }

    const E* knextE() const {
      if (next) {
        return next->kp<E>();
      }
      return nullptr;
    }

    void createNext(const T &v, uint32_t hash, std::shared_ptr<Heap> heap) {
      next = heap->reserve(sizeof(E));
      E *e = nextE();
      e->data = heap->reserve(sizeof(T));
      *e->get() = v;
      e->next = nullptr;
      e->hash = hash;
    }

    void removeNext(Heap &heap) {
      E *e = nextE();
      if (e) {
        Heap::Tile *nextNext = e->next;
        heap.release(e->data);
        heap.release(next);
        next = nextNext;
      }
    }

    T* find(uint32_t hash) {
      if (!data)
        return nullptr;
      if (this->hash == hash)
        return get();
      E *e = nextE();
      if (e) {
        return e->find(hash);
      }
      return nullptr;
    }

    const T* kfind(uint32_t hash) const {
      if (!data)
        return nullptr;
      if (this->hash == hash)
        return kget();
      const E *e = knextE();
      if (e) {
        return e->kfind(hash);
      }
      return nullptr;
    }
    T* insertIf(const T &v, uint32_t hash, std::shared_ptr<Heap> heap, uint32_t &count) {
      if (!data) {
        count++;
        this->hash = hash;
        next = nullptr;
        data = heap->reserve(sizeof(T));
        *get() = v;
        return get();
      }
      if (this->hash == hash) {
        return get();
      }
      if (next) {
        E *e = next->p<E>();
        return e->insertIf(v, hash, heap, count);
      }
      createNext(v, hash, heap);
      count++;
      return next->p<E>()->get();
    }
  };

  uint32_t tableSize, tableGrow;
  uint32_t numResizes = 0;
  std::function<uint32_t(const T&)> hashFunc;
  std::shared_ptr<Heap> heap = nullptr;
  Heap::Tile *pool = nullptr;
  uint32_t inserts = 0;

  int size() const {
    int sum = 0;
    const E *table = pool->kp<E>();
    for (uint32_t i = 0; i < tableSize; i++) {
      sum += table[i].size();
    }
    return sum;
  }

  // Constructor definition
  Hashmap(uint32_t tableSize, std::function<uint32_t(const T &v)> hashFunc, std::shared_ptr<Heap> heap)
      :
      tableSize(tableSize),
      tableGrow(tableSize),
      hashFunc(hashFunc),
      heap(heap) {
    pool = this->heap->reserve(sizeof(E) * tableSize);
    E *table = pool->p<E>();
    for (uint32_t i = 0; i < tableSize; i++) {
      table[i].data = table[i].next = nullptr;
      table[i].hash = 0;
    }
  }

  // Constructor definition
  Hashmap(uint32_t tableSize, std::shared_ptr<Heap> heap)
      :
      tableSize(tableSize),
      tableGrow(tableSize),
      hashFunc([](const T &v) -> uint32_t {
        return v.getHashOf();
      })
      ,
      heap(heap) {
    static_assert(std::is_base_of<Hashable, T>::value);
    pool = heap->reserve(sizeof(E) * tableSize);
    E *table = pool->p<E>();
    for (uint32_t i = 0; i < tableSize; i++) {
      table[i].data = table[i].next = nullptr;
      table[i].hash = 0;
    }

  }

  T* contains(const T &value) {
    int index = int(hashFunc(value) % tableSize);
    E *table = pool->p<E>();
    return table[index].find(value);
  }

  T* contains(uint32_t hash) {
    int index = int(hash % tableSize);
    E *table = pool->p<E>();
    return table[index].find(hash);
  }

  const T* kcontains(uint32_t hash) const {
    int index = int(hash % tableSize);
    const E *table = pool->kp<E>();
    return table[index].kfind(hash);
  }

  T* insertIf(const T &value) {
    int index = int(hashFunc(value) % tableSize);
    E *table = pool->p<E>();
    return table[index].insertIf(value, hashFunc(value), heap, inserts);
  }

  void print() {
    static_assert(std::is_same<T, int>::value, "print only works with int rn");

    E *table = pool->p<E>();
    for (uint32_t i = 0; i < tableSize; i++) {
      if (!table[i].data)
        continue;
      printf("%d:\n", i);
      printf("[ ");
      printf("%d ", *table[i].get());
      Heap::Tile *tile = table[i].next;
      while (tile) {
        E *e = tile->p<E>();
        printf("%d ", *e->get());
        tile = e->next;
      }
      printf("]\n");
    }
  }

  struct Container {
    uint32_t hash;
    Heap::Tile *data;
    T* get() {
      return data->p<T>();
    }
    T *operator()(){
      return data->p<T>();
    }
  };

  Heap::Tile* getList(int &count) {
    E *table = pool->p<E>();
    Heap::Tile *r = heap->reserve(size() * sizeof(Container));

    Container *list = r->p<Container>();
    count = 0;
    for (uint32_t i = 0; i < tableSize; i++) {
      E *e = &table[i];
      if (!e->data)
        continue;
      list[count].data = e->data;
      list[count].hash = e->hash;
      count++;
      Heap::Tile *tile = e->next;
      while (tile) {
        E *e = tile->p<E>();
        list[count].data = e->data;
        list[count].hash = e->hash;
        count++;
        tile = e->next;
      }
    }
    return r;
  }

  void resize() {
    printf("resizing...\n");
    E *table = pool->p<E>();

    Heap::Tile *r = heap->reserve(size() * sizeof(Container));
    Container *vs = r->p<Container>();
    int count = 0;
    for (uint32_t i = 0; i < tableSize; i++) {
      E *e = &table[i];
      if (!e->data)
        continue;
      vs[count].data = e->data;
      vs[count].hash = e->hash;
      count++;
      Heap::Tile *tile = e->next;
      while (tile) {
        E *e = tile->p<E>();
        vs[count].data = e->data;
        vs[count].hash = e->hash;
        count++;
        heap->release(tile);
        tile = e->next;
      }
    }
    for (int i = 0; i < count; i++) {
      Heap::Tile *t = vs[i].data;
      int v = *t->p<int>();
      printf("%u %d\n", vs[i].hash, v);
    }

    heap->release(pool);
    {
      uint32_t s = tableSize;
      tableSize = tableSize + tableGrow;
      tableGrow = s;
    }

    pool = heap->reserve(sizeof(E) * tableSize);
    table = pool->p<E>();
    for (uint32_t i = 0; i < tableSize; i++) {
      table[i].data = table[i].next = nullptr;
      table[i].hash = 0;
    }

    for (int i = 0; i < count; i++) {
      uint32_t index = vs[i].hash % tableSize;
      if (!table[index].data) {
        table[index].data = vs[i].data;
        table[index].hash = vs[i].hash;
        continue;
      }
      E *e = &table[index];
      while (e->next) {
        Heap::Tile *t = e->next;
        e = t->p<E>();
      }
      e->next = heap->reserve(sizeof(E));
      Heap::Tile *t = e->next;
      e = t->p<E>();
      e->data = vs[i].data;
      e->hash = vs[i].hash;
      e->next = nullptr;
    }
    heap->release(r);
  }
};

}
}
}
