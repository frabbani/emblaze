#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <set>
#include <map>

#include "../color.h"
#include "../math/vector.h"

namespace mbz {
namespace utils {
namespace wavefront {


struct OBJ {
  struct Material {
    Color albedo;
    Color emission;
    std::string name;
  };

  struct Vertex {
    mbz::math::Vector3 p;
    mbz::math::Vector3 n;
    mbz::math::Vector2 t;

    bool operator <(const Vertex &rhs) const;
    bool operator >(const Vertex &rhs) const;
    bool operator ==(const Vertex &rhs) const;
  };

  struct Face {
    struct Vertex {
      int p;
      int n;
      int uv;
    };
    union{
      struct{
        Vertex v, v2, v3;
      };
      Vertex vs[3];
    };
    int m;
    OBJ::Vertex verts[3];
    int ids[3];
  };


  std::string fileName;
  std::map<std::string, Material> materials;
  std::map<std::string, std::vector<uint32_t>> materialFaces;
  std::vector<mbz::math::Vector3> pts;
  std::vector<mbz::math::Vector3> nos;
  std::vector<mbz::math::Vector2> uvs;
  std::vector<std::string> mats;
  std::vector<Face> faces;
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  OBJ() = default;
  bool load(std::string_view fileName);
  bool loadMaterials(std::string_view fileName);
};

void exportObj( std::string_view fileName, const OBJ& obj );
void exportObj(std::string_view fileName, OBJ &obj, std::string material );

}
}
}
