#include <cmath>

#include "bcs.h"

namespace mbz {
namespace math {

void Bcs2::init(const Vector2 &p, const Vector2 &p2, const Vector2 &p3) {
  Bcs::init(p, p2, p3);
  Matrix2 M;
  M.e00 = u.x;
  M.e01 = u.y;
  M.e10 = v.x;
  M.e11 = v.y;
  A = M.inverted();
}

void Bcs2::getOrthoNormalBasis(const Vector3 &p, const Vector3 &p2, const Vector3 &p3, Vector3 bases[3], bool orthoNormalize) const {
  Vector3 z;
  bases[0] = z;
  bases[1] = z;
  bases[2] = z;
  if (!valid)
    return;

  Vector3 a(p.point(p2));
  Vector3 b(p.point(p3));

  bases[0] = A.e00 * a + A.e01 * b;
  bases[2] = a.cross(b);  // planar normal

  if (orthoNormalize) {
    bases[1] = bases[2].cross(bases[1]);
    bases[0].normalize();
    bases[1].normalize();
    bases[2].normalize();
  } else {
    bases[1] = A.e10 * a + A.e11 * b;
  }
}

void Bcs3::init(const Vector3 &p, const Vector3 &p2, const Vector3 &p3) {
  Bcs::init(p, p2, p3);
  plane.init(u.cross(v), o);
}

std::optional<BcsCoord> Bcs3::project(const Ray &ray) const {
  if (!valid)
    return std::nullopt;
  float ddotn = ray.dir().dot(plane.n);
  if (ddotn <= 0.0f)
    return std::nullopt;
  float dist = plane.rayDist(ray, ddotn);
  if (dist <= 0.0f)
    return std::nullopt;
  auto co = Bcs::project(ray.p + dist * ray.dir());
  if (co.inside())
    return co;
  return std::nullopt;
}

std::optional<BcsCoord> Bcs3::project(const RaySeg &raySeg) const {
  if (!valid)
    return std::nullopt;
  Ray ray = static_cast<Ray>(raySeg);
  float ddotn = ray.dir().dot(plane.n);
  if (ddotn >= 0.0f)
    return std::nullopt;
  float dist = plane.rayDist(ray, ddotn);
  if (dist <= 0.0f || dist > raySeg.dist)
    return std::nullopt;
  auto co = Bcs::project(ray.p + dist * ray.dir());
  if (co.inside())
    return co;
  return std::nullopt;

}

}
}
