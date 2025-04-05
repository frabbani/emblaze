#pragma once

#include "math/vector.h"

#include "math/bpcd/grid.h"
#include "utils/workers.h"
#include "utils/image.h"
#include "rasterizer/rasterizer.h"

#include <array>
#include <vector>
#include <string_view>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>

namespace mbz {

constexpr int numWorkers = 8;
class AOSolver : public utils::multithread::Workers<numWorkers> {
 public:
  struct Vertex {
    math::Vector3 p;
    math::Vector3 n;
    math::Vector2 uv1;
    math::Vector2 uv2;
  };

  std::shared_ptr<utils::heap::Heap> heap = nullptr;
  std::shared_ptr<math::bpcd::Grid> grid = nullptr;
  std::vector<int> textures;
  std::shared_ptr<rasterizer::Canvas> canvas = nullptr;
  std::shared_ptr<rasterizer::Scanner> scanner = nullptr;

  utils::heap::Array<Vertex> vertices;
  utils::heap::Array<std::array<int, 4>> triangles;
  utils::img::Image result;

  struct Task : public utils::multithread::Task {
   public:
    int x, y;
    std::shared_ptr<const math::bpcd::Grid> grid;
    math::Vector3 p;
    math::Vector3 n;
    math::Vector3 result;
    virtual void perform() override;
  };

  AOSolver(std::shared_ptr<utils::heap::Heap> heap)
      :
      utils::multithread::Workers<numWorkers>(heap, 512 * 512, 4096),
      heap(heap),
      vertices(heap, 12),
      triangles(heap, 4) {
  }
  ~AOSolver() {
    if (vertices.size)
      vertices.free();
    if (triangles.size)
      triangles.free();
  }
  bool create(std::string_view fbxName, uint32_t rasterWidth, uint32_t rasterHeight);
  void save();
};

}

