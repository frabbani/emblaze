#include "rasterizer.h"

#include <cmath>

template<typename T, typename Variant>
struct IsContainedIn;

template<typename T, typename ... Ts>
struct IsContainedIn<T, std::variant<Ts...>> : std::disjunction<std::is_same<T, Ts>...> {
};

namespace mbz {
namespace rasterizer {

std::string getTypeName(Variable var) {
  if (std::holds_alternative<ScalarVariable>(var)) {
    return "Scalar";
  } else if (std::holds_alternative<Vector2Variable>(var)) {
    return "Vector2";
  } else if (std::holds_alternative<Vector3Variable>(var)) {
    return "Vector3";
  } else if (std::holds_alternative<ColorVariable>(var)) {
    return "Color";
  } else if (std::holds_alternative<TexelVariable>(var)) {
    return "Texel";
  }
  return "???";
}

//template<typename T>
//std::string getTypeName() {
//  if constexpr (std::is_same_v<T, ScalarType>) {
//    return "ScalarType";
//  } else if constexpr (std::is_same_v<T, Vector2Type>) {
//    return "Vector2Type";
//  } else if constexpr (std::is_same_v<T, Vector3Type>) {
//    return "Vector3Type";
//  } else if constexpr (std::is_same_v<T, ColorType>) {
//    return "ColorType";
//  } else if constexpr (std::is_same_v<T, TexelType>) {
//    return "TexelType";
//  }
//  return "Unknown Type";
//}

template<class T> struct VaryOp {
  static_assert( IsContainedIn<T, Variable>::value, "invalid varying");

  const T begin;
  const T end;
  VaryOp(const Variable &begin, const Variable &end)
      :
      begin(std::get<T>(begin)),
      end(std::get<T>(end)) {
  }

  void operate(T &out, float alpha) {
    out.v = begin.vary(end.v, alpha);
  }

};

struct Vary {
  const Variable begin;
  const Variable end;
  float alpha;
  Vary(const Variable begin, const Variable end, float alpha)
      :
      begin(std::move(begin)),
      end(std::move(end)),
      alpha(alpha) {
  }
  void operator()(ScalarVariable &out) {
    VaryOp<ScalarVariable> v(begin, end);
    v.operate(out, alpha);
  }
  void operator()(Vector2Variable &out) {
    VaryOp<Vector2Variable> v(begin, end);
    v.operate(out, alpha);
  }
  void operator()(Vector3Variable &out) {
    VaryOp<Vector3Variable> v(begin, end);
    v.operate(out, alpha);
  }
  void operator()(ColorVariable &out) {
    VaryOp<ColorVariable> v(begin, end);
    v.operate(out, alpha);
  }
  void operator()(TexelVariable &out) {
    VaryOp<TexelVariable> v(begin, end);
    v.operate(out, alpha);
  }

};

Point Canvas::createPoint(int x, int y) {
  struct V {
    Variable &var;
    size_t index = 0;
    V(Variable &v, size_t i)
        :
        var(v),
        index(i) {
    }
    void operator()(const ScalarVariables &from) {
      var = from.kp()[index];
    }
    void operator()(const Vector2Variables &from) {
      var = from.kp()[index];
    }
    void operator()(const Vector3Variables &from) {
      var = from.kp()[index];
    }
    void operator()(const ColorVariables &from) {
      var = from.kp()[index];
    }
    void operator()(const TexelVariables &from) {
      var = from.kp()[index];
    }
  };
  Point pt;
  size_t i = 0;
  size_t j = xy(x, y);
  pt.plots = std::vector<Variable>(layers.size());
  for (const auto &layer : layers) {
    V visitor(pt.plots[i++], j);
    std::visit(visitor, layer);
  }
  return pt;
}

Canvas::Canvas(std::shared_ptr<utils::heap::Heap> heap, uint32_t w, uint32_t h, const std::vector<VariableType> &types) {
  this->heap = heap;
  this->w = w;
  this->h = h;

  size_t n = std::min(maxLayers, types.size());
  int m = int(w * h);
  for (size_t i = 0; i < n; i++) {
    switch (types[i]) {
      case Vector2Type: {
        Vector2Variables layer(heap, m, utils::heap::Growth::Fixed);
        for (int i = 0; i < m; i++)
          layer.append(Vector2Variable());
        layers.push_back(std::move(layer));
        break;
      }
      case Vector3Type: {
        Vector3Variables layer(heap, m, utils::heap::Growth::Fixed);
        for (int i = 0; i < m; i++)
          layer.append(Vector3Variable());
        layers.push_back(std::move(layer));
        break;
      }
      case ColorType: {
        ColorVariables layer(heap, m, utils::heap::Growth::Fixed);
        for (int i = 0; i < m; i++)
          layer.append(ColorVariable());
        layers.push_back(std::move(layer));
        break;
      }
      case TexelType: {
        TexelVariables layer(heap, m, utils::heap::Growth::Fixed);
        for (int i = 0; i < m; i++)
          layer.append(TexelVariable());
        layers.push_back(std::move(layer));
        break;
      }
      case ScalarType:
      default: {
        ScalarVariables layer(heap, m, utils::heap::Growth::Fixed);
        for (int i = 0; i < m; i++)
          layer.append(ScalarVariable());
        layers.push_back(std::move(layer));
        break;
      }
    }
  }
}

/*
 Canvas::Canvas(std::shared_ptr<utils::heap::Heap> heap, uint32_t w, uint32_t h, const std::vector<VariableType> &types)
 :
 this->heap(heap) {
 this->w = w;
 this->h = h;
 for (auto type : types) {
 if (layers.size == maxLayers)
 break;
 switch (type) {
 case ScalarType: {
 utils::heap::Buffer layer;
 auto layer = std::vector<ScalarVariable>(w * h);
 layers.push_back(std::move(layer));
 break;
 }
 case Vector2Type: {
 auto layer = std::vector<Vector2Variable>(w * h);
 layers.push_back(std::move(layer));
 break;
 }
 case Vector3Type: {
 auto layer = std::vector<Vector3Variable>(w * h);
 layers.push_back(std::move(layer));
 break;
 }
 case ColorType: {
 auto layer = std::vector<ColorVariable>(w * h);
 layers.push_back(std::move(layer));
 break;
 }
 case TexelType: {
 auto layer = std::vector<TexelVariable>(w * h);
 layers.push_back(std::move(layer));
 break;
 }
 }
 }
 }
 */

void Canvas::plotPoint(const Point &pt, int x, int y) {

  struct V {
    const Variable &var;
    size_t index;
    V(const Variable &v, size_t i)
        :
        var(v),
        index(i) {
    }
    void operator()(ScalarVariables &to) {
      to.p()[index] = std::get<ScalarVariable>(var);
    }
    void operator()(Vector2Variables &to) {
      to.p()[index] = std::get<Vector2Variable>(var);
    }
    void operator()(Vector3Variables &to) {
      to.p()[index] = std::get<Vector3Variable>(var);
    }
    void operator()(ColorVariables &to) {
      to.p()[index] = std::get<ColorVariable>(var);
    }
    void operator()(TexelVariables &to) {
      to.p()[index] = std::get<TexelVariable>(var);
    }
  };

  size_t index = xy(x, y);
  size_t i = 0;
  for (auto &layer : layers) {
    V visitor(pt.plots[i++], index);
    std::visit(visitor, layer);
  }
}

Point blendPoints(const Point &begin, const Point &end, float alpha) {
  Point pt;
  pt.p = begin.p + alpha * (end.p - begin.p);
//do i need to check variants hold same datatype?
  for (size_t i = 0; i < begin.plots.size(); i++) {
    auto beginVar = begin.plots[i];
    auto endVar = end.plots[i];
    Variable result = beginVar;
    Vary vary(beginVar, endVar, alpha);
    std::visit(vary, result);
    pt.plots.push_back(std::move(result));
  }
  return pt;
}

void Scanner::Pair::push(int y, const Point &pt) {
  if (!this->y.has_value()) {
    this->y = y;
    pt0 = pt1 = pt;
  } else {
    if (pt.p.x < pt0.p.x)
      pt0 = pt;
    if (pt.p.x > pt1.p.x)
      pt1 = pt;
  }
}

void Scanner::pushPoint(const Point &pt) {
  int yBottom = int(floorf(pt.p.y));
  int yTop = int(ceilf(pt.p.y));
  yBottom = std::clamp(yBottom, 0, int(canvas.h - 1));
  yTop = std::clamp(yTop, 0, int(canvas.h - 1));
  if (yBottom < this->yBottom)
    this->yBottom = yBottom;
  if (yTop > this->yTop)
    this->yTop = yTop;

  for (int y = yBottom; y <= yTop; y++) {
    pairs[y].push(y, pt);
  }
}

void Scanner::plotPoint(const Point &pt, int x, int y) {

}

void Scanner::buildEdge(const Point &pt0, const Point &pt1) {
  const Point *bottom = &pt0;
  const Point *top = &pt1;
  if (bottom->p.y > top->p.y) {
    std::swap(top, bottom);
  }

  int yBottom = int(floorf(bottom->p.y));
  int yTop = int(ceilf(top->p.y));
  if (yBottom == yTop) {
    //pushPoint(blendPoints(*bottom, *top, 0.5f));
    return;
  }
  for (int y = yBottom; y <= yTop; y++) {
    float alpha = float(y - yBottom) / float(yTop - yBottom);
    pushPoint(blendPoints(*bottom, *top, alpha));
  }
}

void Scanner::scanLine(const Point &pt0, const Point &pt1, int y) {
  const Point *left = &pt0;
  const Point *right = &pt1;
  if (left->p.x > right->p.x) {
    std::swap(left, right);
  }

  int xLeft = int(floorf(left->p.x));
  int xRight = int(ceilf(right->p.x));
  if (xLeft == xRight) {
    canvas.plotPoint(blendPoints(*left, *right, 0.5f), xLeft, y);
    return;
  }
  for (int x = xLeft; x <= xRight; x++) {
    float alpha = float(x - xLeft) / float(xRight - xLeft);
    canvas.plotPoint(blendPoints(*left, *right, alpha), x, y);
  }
}

void Scanner::scan() {
  for (int y = yBottom; y <= yTop; y++) {
    scanLine(pairs.kp()[y].pt0, pairs.kp()[y].pt1, y);
    //canvas.plotPoint(pairs[y].pt0, int(floorf(pairs[y].pt0.p.x)), y);
    //canvas.plotPoint(pairs[y].pt1, int(ceilf(pairs[y].pt1.p.x)), y);
  }
}

}
}
