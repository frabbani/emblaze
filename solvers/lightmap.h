#pragma once

#include "../rasterizer/rasterizer.h"
#include <tuple>

namespace mbz {
namespace lightmap {

using Types = std::tuple<rasterizer::ScalarVariable, rasterizer::Vector3Variable, rasterizer::Vector3Variable, rasterizer::TexelVariable>;
using Layers = std::tuple<rasterizer::ScalarVariables, rasterizer::Vector3Variables, rasterizer::Vector3Variables, rasterizer::TexelVariables>;


struct Lightmap {
  struct Tri {
    friend class Lightmap;
   protected:
    std::reference_wrapper<Lightmap> lightmap;
    std::array<rasterizer::Point, 3> points;
   public:
    Tri(Lightmap &lightmap)
        :
        lightmap(lightmap) {
    }

    template<int N> typename std::tuple_element<N, Types>::type& plot(int index){
      static_assert(N >= 0 && N < std::tuple_size<Types>::value, "Lightmap::Tri::plot N oob");
      index = std::clamp(index, 0, 2);
      using T = typename std::tuple_element<N, Types>::type;
      return std::get<T>(points[index].plot[N]);
    }

    rasterizer::Point& getPoint(int index) {
      index = std::clamp(index, 0, 2);  //index < 0 ? 0 : index > 2 ? 2 : index;
      return points[index];
    }

    void render() {
      lightmap.get().scanner.buildEdge(points[0], points[1]);
      lightmap.get().scanner.buildEdge(points[0], points[2]);
      lightmap.get().scanner.buildEdge(points[1], points[2]);
      lightmap.get().scanner.scanReset();
    }
  };
  std::shared_ptr<utils::heap::Heap> heap = nullptr;
  // member declaration order matters, therefore scanner will be initialized after canvas
  rasterizer::Canvas canvas;
  rasterizer::Scanner scanner;
  std::vector<int> textures;
  Lightmap(std::shared_ptr<utils::heap::Heap> heap, uint32_t width, uint32_t height)
      :
      heap(heap),
      canvas(heap, width, height, []() {
        std::vector<rasterizer::VariableType> types;
        types.push_back(rasterizer::VariableType::ScalarType);    //  id
        types.push_back(rasterizer::VariableType::Vector3Type);   //  position
        types.push_back(rasterizer::VariableType::Vector3Type);   //  normal
        types.push_back(rasterizer::VariableType::TexelType);     //  texture
        return types;
      }()),
      scanner(canvas) {
  }

  rasterizer::Point createPoint(int x = 0, int y = 0) const {
    return canvas.createPoint(x, y);
  }

  Tri getTri(std::pair<int, int> coord = { 0, 0 }, std::pair<int, int> coord2 = { 0, 0 }, std::pair<int, int> coord3 = { 0, 0 }) {
    Tri tri(*this);
    tri.points[0] = canvas.createPoint(coord.first, coord.second);
    tri.points[1] = canvas.createPoint(coord2.first, coord2.second);
    tri.points[2] = canvas.createPoint(coord3.first, coord3.second);
    return tri;
  }

  template<int N> typename std::tuple_element<N, Layers>::type& getLayer(){
    static_assert(N >= 0 && N < std::tuple_size<Layers>::value, "Lightmap::getLayer N oob");
    using T = typename std::tuple_element<N, Layers>::type;
    return std::get<T>(canvas.layers[N]);
  }

  void exportPNGs();

};


/*
 class LightmapSolver : public utils::multithread::Workers<maxWorkers> {
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

 virtual std::unique_ptr<utils::multithread::Toolbox> createToolbox() override {
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
 vertices.free();
 if (triangles.size)
 triangles.free();
 }
 bool create(std::string_view fbxName, uint32_t rasterWidth, uint32_t rasterHeight, Lighting lighting);
 void save();
 };
 */

}
}
