#pragma once

#include "../bcs.h"
#include "../../utils/hash.h"
#include <array>
#include <vector>
#include <algorithm>
#include <tuple>
#include "../../utils/array.h"

using namespace mbz::math;
using namespace mbz::utils;
using namespace mbz::utils::heap;

namespace mbz {
namespace math {
namespace bpcd {

struct Grid {
  static uint32_t getHashOf(int l, int r, int c);

  struct Cell : public utils::heap::Hashable {
    int l, r, c;
    Aabb aabb;
    Array<int> triIndices;

    Cell& operator =(const Cell &rhs) {
      l = rhs.l;
      r = rhs.r;
      c = rhs.c;
      aabb = rhs.aabb;
      triIndices.free();
      triIndices.init(rhs.triIndices.heap, 8, rhs.triIndices.growth);
      for (int i = 0; i < rhs.triIndices.size; i++)
        triIndices.append(rhs.triIndices.kp()[i]);
      return *this;
    }

    virtual uint32_t getHashOf() const override {
      return Grid::getHashOf(l, r, c);
    }

    Cell(int l_, int r_, int c_, Aabb aabb_, std::shared_ptr<Heap> heap)
        :
        l(l_),
        r(r_),
        c(c_),
        aabb(aabb_),
        triIndices(heap, 8) {
    }

  };

  struct Trace {
    RaySeg raySeg;
    int index;
    std::optional<BcsCoord> bcsCoord;
    std::optional<Vector3> point;
    Trace(const RaySeg& raySeg)
        :
        raySeg(raySeg),
        index(-1),
        bcsCoord(std::nullopt),
        point(std::nullopt) {
    }
    Trace(Ray ray, float dist)
        :
        raySeg(ray, dist),
        index(-1),
        bcsCoord(std::nullopt),
        point(std::nullopt) {
    }
    bool operator()() const {
      return bcsCoord.has_value() && bcsCoord.value().inside();
    }
  };

  std::shared_ptr<Heap> heap;
  Array<Bcs3> tris;
  Hashmap<Cell> cells;

  Vector3 o;
  Vector3 cellSize;
  Aabb bigBox;

  Aabb cellBox(int l, int r, int c) const {
    Vector3 p = o;
    p.x = o.x + (float(c) + 0.5f) * cellSize.x;
    p.y = o.y + (float(r) + 0.5f) * cellSize.y;
    p.z = o.z + (float(l) + 0.5f) * cellSize.z;
    return Aabb(p, 0.5f * cellSize);
  }


  void getCellForPoint(Vector3 p, int &l, int &r, int &c) const {
    p -= o;
    l = int(floorf(p.z / cellSize.z));
    r = int(floorf(p.y / cellSize.y));
    c = int(floorf(p.x / cellSize.x));
  }

  Grid(std::shared_ptr<Heap> heap)
      :
      heap(heap),
      tris(heap, 1024),
      cells(1024, heap) {
  }
  bool build(const std::vector<std::array<Vector3, 3>> &trisPoints, Vector3 cellSize);
  void getBoxes(std::vector<Aabb> &boxes);

  bool traceRay(RaySeg raySeg, Trace &trace, std::optional<std::reference_wrapper<std::vector<std::array<int, 3>>>> indices = std::nullopt) const;
};

}
}
}
