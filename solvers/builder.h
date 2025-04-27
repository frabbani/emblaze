#pragma once

#include "lightmap.h"
#include "../math/bpcd/grid.h"

#include <array>
#include <vector>
#include <string_view>
#include <memory>

namespace mbz {
namespace lightmap {

struct LightmapBuilder {
  struct Vertex {
    math::Vector3 p;
    math::Vector3 n;
    math::Vector2 uv1;
    math::Vector2 uv2;
  };

  std::shared_ptr<utils::heap::Heap> heap = nullptr;
  std::shared_ptr<Lightmap> lightmap = nullptr;
  std::shared_ptr<math::bpcd::Grid> grid = nullptr;

  utils::heap::Array<Vertex> vertices;
  utils::heap::Array<std::array<int, 4>> triangles;

  LightmapBuilder(std::shared_ptr<utils::heap::Heap> heap, std::shared_ptr<Lightmap> lightmap)
      :
      heap(heap),
      lightmap(lightmap),
      vertices(heap, 12),
      triangles(heap, 4) {
  }

  bool buildFromFBX(std::string_view fbxName, float cellScale = 0.125f);

};

}
}

