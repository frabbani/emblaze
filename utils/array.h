#pragma once

#include "log.h"
#include "heap.h"
#include <cstring>


namespace mbz {
namespace utils {
namespace heap {

enum Growth {
  Fixed,
  Fib,
  Double,
};

struct Destructable {
  virtual void destruct(){}
  virtual ~Destructable(){ destruct();}
};

template<typename T>
struct Array {
  Heap::Tile *tile = nullptr;
  std::shared_ptr<Heap> heap = nullptr;
  int grow = 0;
  int size = 0;
  Growth growth = Growth::Fib;

  //static_assert(std::is_trivially_destructible<T>::value, "heap::Array: T must be trivially destructible");

  void init(std::shared_ptr<Heap> heap, int reserveCount, Growth growth = Growth::Fib) {
    this->heap = heap;
    this->grow = reserveCount < 1 ? 1 : reserveCount;
    this->size = 0;
    this->growth = growth;
    tile = heap->reserve(reserveCount * sizeof(T));
  }

  Array(std::shared_ptr<Heap> heap, uint32_t reserveCount, Growth growth = Growth::Fib) {
    init(heap, reserveCount, growth);
  }

  ~Array() {
    //LOGDEBUG("~Array", "calling release");
    release();
  }

  void release() {
    //LOGDEBUG("Array::release", "%s", std::is_base_of_v<Releaseable, T>? "(release-able)" : "" );
    if(tile){
      if constexpr (std::is_base_of_v<Destructable, T>) {
        for (int i = 0; i < size; i++){
          tile->p<T>()[i].destruct();
        }
      }
      heap->release(tile);
    }
    grow = 0;
    size = 0;
    tile = nullptr;
  }

  Array& operator = (const Array<T>& rhs){
    release();
    heap = rhs.heap;
    grow = rhs.grow;
    size = rhs.size;
    growth = rhs.growth;
    tile = heap->reserve(rhs.size);
    for(int i = 0; i < size; i++)
      p()[i] = rhs.kp()[i];
    return *this;
  }

  int count() {
    return size;
  }

  const T* kp() const {
    return tile->kp<T>();
  }

  T* p() const {
    return tile->p<T>();
  }

  T& last() {
    return tile->kp<T>()[size - 1];
  }

  T& operator[](int index) {
    return tile->p<T>()[index];
  }

  void append(const T &v) {
    int alloc = int(tile->size<T>());

    if (size < alloc) {
      tile->p<T>()[size++] = v;
      return;
    }

    int newAlloc = alloc + grow;  // fibonacci-like growth: current allocation + previous allocation
    if (growth == Growth::Fib)
      grow = alloc;
    else if (growth == Growth::Double)
      grow = newAlloc;

    //LOGINFO("Array::append", "resizing from %d to %d", alloc, newAlloc);
    // reserve
    Heap::Tile *newTile = heap->reserve(newAlloc * sizeof(T));

    // copy
    for (int i = 0; i < alloc; i++){
      newTile->p<T>()[i] = tile->p<T>()[i];
    }

    // release
    heap->release(tile);

    // append
    tile = newTile;
    tile->p<T>()[size++] = v;
  }

  void append_move(T&& v) {
    int alloc = int(tile->size<T>());

    if (size < alloc) {
      tile->p<T>()[size++] = std::move(v);
      return;
    }

    int newAlloc = alloc + grow;  // fibonacci-like growth: current allocation + previous allocation
    if (growth == Growth::Fib)
      grow = alloc;
    else if (growth == Growth::Double)
      grow = newAlloc;

    LOGINFO("Buffer::append_move", "resizing from %d to %d", alloc, newAlloc);

    // reserve
    Heap::Tile *newTile = heap->reserve(newAlloc * sizeof(T));

    for (int i = 0; i < alloc; ++i) {
      newTile->p<T>()[i] = std::move(tile->p<T>()[i]);
    }

    // release
    heap->release(tile);

    // append
    tile = newTile;
    tile->p<T>()[size++] = std::move(v);
  }

  void remove_last() {
    if (size > 0){
      size--;
      if constexpr (std::is_base_of_v<Destructable, T>)
          tile->p<T>()[size].release();
    }
  }


  // Move constructor
  Array(Array &&other) noexcept
      :
      tile(other.tile),
      heap(std::move(other.heap)),
      grow(other.grow),
      size(other.size),
      growth(other.growth) {
    // Null out the other object to indicate it no longer owns the resources
    other.tile = nullptr;
    other.grow = 0;
    other.size = 0;
  }

  // Move assignment operator
  Array& operator=(Array &&other) noexcept {
    if (this != &other) {
      tile = nullptr;
      heap = nullptr;

      tile = other.tile;
      heap = std::move(other.heap);
      grow = other.grow;
      size = other.size;
      growth = other.growth;

      // Null out the other object to indicate it no longer owns the resources
      other.tile = nullptr;
      other.grow = 0;
      other.size = 0;
    }
    return *this;
  }

};

}
}
}
