#pragma once

#include "../utils/log.h"
#include "../math/vector.h"
#include "texture.h"

#include <string>
#include <variant>
#include <functional>
#include <optional>
#include <type_traits>
#include "../utils/array.h"

using namespace mbz::math;

namespace mbz {
namespace rasterizer {

enum VariableType {
  ScalarType,
  Vector2Type,
  Vector3Type,
  ColorType,
  TexelType,
};

constexpr size_t maxLayers = 8;

template<class T>
struct VariableBase {
  static_assert(std::is_same<T, float>::value || std::is_same<T, Vector2>::value || std::is_same<T, Vector3>::value || std::is_same<T, Color>::value || std::is_same<T, Texel>::value, "variable type not supported" );

  static constexpr VariableType type = []() {
    if (std::is_same<T, float>::value)
      return ScalarType;
    else if (std::is_same<T, Vector2>::value)
      return Vector2Type;
    else if (std::is_same<T, Vector3>::value)
      return Vector3Type;
    else if (std::is_same<T, Color>::value)
      return ColorType;
    return TexelType;
  }();
  T v;
  virtual T vary(const T &next, float alpha) const {
    return alpha >= 0.5f ? next : v;
  }
  std::string getType() {
    switch (type) {
      case ScalarType:
        return "ScalarType";
        break;
      case Vector2Type:
        return "Vector2Type";
        break;
      case Vector3Type:
        return "Vector3Type";
        break;
      case ColorType:
        return "ColorType";
        break;
      case TexelType:
        return "TexelType";
        break;
    }
    return "the hell???";
  }

  VariableBase() = default;
  VariableBase(T v)
      :
      v(v) {
  }
  virtual ~VariableBase() = default;
}
;

extern int scalarConDes;
struct ScalarVariable : public VariableBase<float> {
  float vary(const float &next, float alpha) const override {
    return v + alpha * (next - v);
  }
  ScalarVariable()
      :
      VariableBase<float>() {
    scalarConDes++;
  }
  ~ScalarVariable() {
    v = 0.0f;
    //scalarConDes--;
  }

};

struct Vector2Variable : public VariableBase<Vector2> {
  Vector2 vary(const Vector2 &next, float alpha) const override {
    return v + alpha * (next - v);
  }
  Vector2Variable()
      :
      VariableBase<Vector2>() {
  }
};

struct Vector3Variable : public VariableBase<Vector3> {
  Vector3 vary(const Vector3 &next, float alpha) const override {
    return v + alpha * (next - v);
  }
  Vector3Variable()
      :
      VariableBase<Vector3>() {
  }
};

struct ColorVariable : public VariableBase<Color> {
  Color vary(const Color &next, float alpha) const override {
    return v.lerp(next, alpha);
  }
  ColorVariable()
      :
      VariableBase<Color>() {
  }
};

struct TexelVariable : public VariableBase<Texel> {
  Texel vary(const Texel &next, float alpha) const override {
    if (v.textureHandle >= 0) {
      Texel t;
      if (v.textureHandle == next.textureHandle) {
        t.textureHandle = v.textureHandle;
        t.uv = v.uv + alpha * (next.uv - v.uv);
        t.mipLevel = v.mipLevel;
      } else {
        t.textureHandle = alpha >= 0.5f ? next.textureHandle : v.textureHandle;
        t.uv = alpha >= 0.5f ? next.uv : v.uv;
        t.mipLevel = alpha >= 0.5f ? next.mipLevel : v.mipLevel;
      }
      return t;
    }
    return v;
  }
  void set(Vector2 uv, int textureHandle, int mipLevel = 0) {
    v.textureHandle = textureHandle;
    v.mipLevel = mipLevel;
    v.uv = uv;
  }

  TexelVariable()
      :
      VariableBase<Texel>() {
  }
};

using ScalarVariables = utils::heap::Array<ScalarVariable>;
using Vector2Variables = utils::heap::Array<Vector2Variable>;
using Vector3Variables = utils::heap::Array<Vector3Variable>;
using ColorVariables = utils::heap::Array<ColorVariable>;
using TexelVariables = utils::heap::Array<TexelVariable>;

using Variable = std::variant<ScalarVariable, Vector2Variable, Vector3Variable, ColorVariable, TexelVariable>;
using Variables = std::variant<ScalarVariables, Vector2Variables, Vector3Variables, ColorVariables, TexelVariables >;
using Plot = std::vector<Variable>;
using Layers = std::vector<Variables>;

struct Point : public utils::heap::Destructable {
  Vector2 p;
  Plot plot;
  static Point blendPoints(const Point &begin, const Point &end, float alpha);
  virtual void destruct() override {
    plot.clear();
    LOGDEBUG("Point::release", "plot cleared");
  }
};

struct Canvas {
  uint32_t w = 0, h = 0;
  std::shared_ptr<utils::heap::Heap> heap = nullptr;
  Layers layers;

  ~Canvas() {
    printf("~Canvas\n");
    while (!layers.empty()) {
      layers.pop_back();
      printf(" + layer cleared...\n");
    }
  }

  Canvas(std::shared_ptr<utils::heap::Heap> heap, uint32_t w, uint32_t h, const std::vector<VariableType> &types);

  size_t xy(int32_t x, int32_t y) const {
    x = std::clamp(x, 0, int32_t(w - 1));
    y = std::clamp(y, 0, int32_t(h - 1));
    return size_t(y * w + x);
  }

  /*
   template<class T> void fill(size_t layerNo) {
   static_assert( IsContainedIn<T, Variable>::value, "Canvas::fill error");
   if (layerNo < layers.size()) {
   auto &layer = std::get<std::vector<T>>(layers[layerNo]);
   layer.clear();
   for (size_t i = 0; i < w * h; i++) {
   layer.emplace_back();
   }
   }
   }
   */

  Point createPoint(int x = 0, int y = 0) const;
  void plotPoint(const Point &pt, int x, int y);
};

struct Scanner {
 protected:
  struct Pair {
    std::optional<int> y = std::nullopt;
    Point pt0, pt1;
    void push(int y, const Point &pt);
    void reset() {
      y = std::nullopt;
    }
  };

  int yTop = -1;
  int yBottom = INT_MAX;
  Canvas &canvas;
  utils::heap::Array<Pair> pairs;

  void pushPoint(const Point &pt);
  void plotPoint(const Point &pt, int x, int y);
  void scanLine(const Point &pt0, const Point &pt1, int y);

 public:
  Scanner(Canvas &canvas_)
      :
      canvas(canvas_),
      pairs(canvas.heap, canvas.h, utils::heap::Growth::Fixed) {
    for (uint32_t i = 0; i < canvas.h; i++) {
      pairs.append(Pair());
    }
  }

  void reset() {
    yTop = -1;
    yBottom = INT_MAX;
    for (int i = 0; i < pairs.size; i++)
      pairs.p()[i].reset();
  }

  void buildEdge(const Point &pt0, const Point &pt1);
  void scan();
  void scanReset() {
    scan();
    reset();
  }
};

}
}
