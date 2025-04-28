#pragma once

#include "math/vector.h"
#include "math/noise.h"
#include "math/bpcd/grid.h"
#include "utils/workers.h"
#include "utils/image.h"
#include "solvers/lightmap.h"
#include "thirdparty/mtwister/mtwister.h"

#include <array>
#include <vector>
#include <string_view>
#include <memory>

namespace mbz {

constexpr int numWorkers = 14;
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

  uint32_t seed = 345;
  struct Task : public utils::multithread::Task {
   public:
    int x, y;
    std::shared_ptr<const math::bpcd::Grid> grid;
    math::Vector3 p;
    math::Vector3 n;
    math::Vector3 result;
    virtual void perform(utils::multithread::Toolbox *toolbox) override;
  };

  struct Toolbox : public utils::multithread::Toolbox{
    MTRandWrapper mtRand;
    Toolbox(uint32_t seed){
      mtRand.seed(seed);
    }
    Vector3 randomPointOnSphere(){
      return math::mc::randomPointOnSphere(mtRand);
    }
  };

  virtual std::unique_ptr<utils::multithread::Toolbox> divyToolbox(int workerId) override {
    seed += seed;
    return std::make_unique<Toolbox>(seed);
  }

  AOSolver(std::shared_ptr<utils::heap::Heap> heap)
      :
      utils::multithread::Workers<numWorkers>(heap, 512 * 512, 4096),
      heap(heap),
      vertices(heap, 12),
      triangles(heap, 4) {
  }
  ~AOSolver() {
    if (vertices.size)
      vertices.release();
    if (triangles.size)
      triangles.release();
  }
  bool create(std::string_view fbxName, uint32_t rasterWidth, uint32_t rasterHeight);
  void save();
};

class LightSolver : public utils::multithread::Workers<numWorkers> {
 public:
  struct Vertex {
    math::Vector3 p;
    math::Vector3 n;
    math::Vector2 uv1;
    math::Vector2 uv2;
  };

  struct Lighting{
    struct Direct{
      Color color;
      float intensity;
      Vector3 direction;
      float radius;
    };

    struct Point{
      Sphere sphere;
      Color color;
      float intensity;
    };

    struct Area{
      Color color;
      Rect rect;
      float intensity;
    };

    struct Cone{
      Vector3 origin;
      Vector3 direction;
      float angle; //in degrees
      float intensity;
    };

    struct Dome{
      float angle;  //in degrees
      Vector3 direction;
      Color color;
      float intensity;
    };

    std::vector<Dome> domeLights;
    std::vector<Direct> directLights;
    std::vector<Point> pointLights;
    std::vector<Area> areaLights;

	  Color skyColor;
	  Color sunColor;
	  Vector3 sunDirection;
	  //TODO: add additional point/area lights
  };

  std::shared_ptr<utils::heap::Heap> heap = nullptr;
  std::shared_ptr<math::bpcd::Grid> grid = nullptr;
  std::vector<int> textures;
  std::shared_ptr<rasterizer::Canvas> canvas = nullptr;
  std::shared_ptr<rasterizer::Scanner> scanner = nullptr;

  utils::heap::Array<Vertex> vertices;
  utils::heap::Array<std::array<int, 4>> triangles;
  utils::img::Image result;
  uint32_t seed = 345;

  struct Task : public utils::multithread::Task {
   public:
    int x, y;
    Lighting lighting;

    std::shared_ptr<const math::bpcd::Grid> grid;
    math::Vector3 p;
    math::Vector3 n;
    Color c;
    math::Vector3 result;
    virtual void perform(utils::multithread::Toolbox *toolbox) override;
  };

  struct Toolbox : public utils::multithread::Toolbox{
    MTRandWrapper mtRand;
    Toolbox(uint32_t seed){
      mtRand.seed(seed);
    }
    Vector3 randomPointOnSphere(){
      return math::mc::randomPointOnSphere(mtRand);
    }
  };

  virtual std::unique_ptr<utils::multithread::Toolbox> divyToolbox(int workerId) override {
    seed += seed;
    return std::make_unique<Toolbox>(seed);
  }


  LightSolver(std::shared_ptr<utils::heap::Heap> heap)
      :
      utils::multithread::Workers<numWorkers>(heap, 512 * 512, 4096),
      heap(heap),
      vertices(heap, 12),
      triangles(heap, 4) {
  }
  ~LightSolver() {
    if (vertices.size)
      vertices.release();
    if (triangles.size)
      triangles.release();
  }
  bool create(std::string_view fbxName, uint32_t rasterWidth, uint32_t rasterHeight, Lighting lighting);
  void save();
};

}

