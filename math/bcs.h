#pragma once
#include <type_traits>
#include "vector.h"
#include "geometry.h"


    extern int printf(const char *format, ...);

namespace mbz {
namespace math {

//template <typename T>
//struct IsBcsType : std::disjunction<std::is_same<T, Vector2>, std::is_same<T, Vector3>> {};
struct BcsCoord : public Vector2 {
  BcsCoord() = default;
  BcsCoord(const Vector2 &rhs)
      :
      Vector2(rhs) {
  }
  bool inside() const {
    //return x >= 0.0f && x <= 1.0f && y >= 0.0f && y <= 1.0f && (x + y) >= 0.0f && (x + y) <= 1.0f;
    return x >= 0.0f && y >= 0.0f && (x + y) <= 1.0f;
  }
};

template<typename T>
struct Bcs {
  static_assert( std::is_same<T, Vector2>::value || std::is_same<T, Vector3>::value, "Bcs: invalid type." );

  Matrix2 M;
  T o, u, v;
  bool valid;

  void init(const T &p, const T &p2, const T &p3) {
    o = p;
    u = p.point(p2);
    v = p.point(p3);
    Matrix2 A;
    A.e00 = u.dot(u);
    A.e01 = A.e10 = u.dot(v);
    A.e11 = v.dot(v);
    float det = A.determinant();
    valid = det < 0.0f || det > 0.0f;
    if (valid)
      M = A.inverted();
  }

  BcsCoord project(const T &p) const {
    T r;
    r = o.point(p);
    //Au + Bv = a
    //A(u.u) + B(v.u) = a . u
    //A(u.v) + B(v.v) = a . v
    //   | A |  =  |  u.u,  v.u  | ^-1  * | a . u |
    //   | B |     |  u.v,  v.v  |        | a . v |
    //0 <= A <= 1,  0 <= B <= 1, 0 <= A+B <= 1

    return M * Vector2(r.dot(u), r.dot(v));

  }

};

struct Bcs2 : public Bcs<Vector2> {
  Matrix2 A;

  void init(const Vector2 &p, const Vector2 &p2, const Vector2 &p3);
  void getOrthoNormalBasis(const Vector3 &p, const Vector3 &p2, const Vector3 &p3, Vector3 bases[3], bool orthoNormalize = true) const;
};

struct Bcs3 : public Bcs<Vector3> {
  Plane plane;
  void init(const Vector3 &p, const Vector3 &p2, const Vector3 &p3);
  std::optional<BcsCoord> project(const Ray &ray) const;
  std::optional<BcsCoord> project(const RaySeg &raySeg) const;
};

}
}
