#pragma once

#include <array>
#include <optional>
#include <vector>
#include <cmath>

#include "vector.h"

namespace mbz {
namespace math {

struct Ray {
  Vector3 p, d;
  Ray() = default;
  Ray(Vector3 p_, Vector3 d_)
      :
      p(p_),
      d(d_.normalized()) {
  }

  Vector3 dir() const {
    return d;
  }
  Vector3 dir(Vector3 d) {
    this->d = d.normalized();
    return d;
  }

};

struct RaySeg : public Ray {
  float dist;
  Vector3 end() const {
    return p + dist * d;
  }
  RaySeg(const Vector3 &p, const Vector3 &p2) {
    this->p = p;
    d = (p2 - p);
    dist = d.lengthSq();
    if (dist > math::tolSq) {
      dist = sqrtf(dist);
      this->d *= (1.0f / dist);
    } else{
      dist = 0.0f;
      this->d = Vector3(0.0f, 0.0f, 0.0f);
    }
  }

  RaySeg(const Ray &ray, float dist)
      :
      Ray(ray),
      dist(dist) {
  }

  bool isPoint() {
    return 0.0f == dist;
  }
};

struct Plane {
  Vector3 n;
  float dist = 0.0f;

  Plane() = default;
  Plane(const Vector3 &n_, const Vector3 &p) {
    init(n_, p);
  }

  Plane(const Vector3 &n_, float dist_)
      :
      n(n_),
      dist(dist_) {
  }

  void init(const Vector3 &n_, const Vector3 &p) {
    n = n_.normalized();
    dist = n.dot(p);
  }

  void calculate(const Vector3 &p, const Vector3 &p2, const Vector3 &p3) {
    Vector3 u = p.point(p2);
    Vector3 v = p.point(p3);
    n = u.cross(v).normalized();
    dist = p.dot(n);
  }

  float solve(const Vector3 &p) const {
    return n.dot(p) - dist;
  }

  int getSide(const Vector3 &p) const {
    float pro = p.dot(n);
    if (pro > dist + tol)
      return 1;
    if (pro < dist - tol)
      return -1;
    return 0;
  }

  float rayDist(const Ray &ray, std::optional<float> ddotn = std::nullopt) const {
    return (dist - ray.p.dot(n)) / ddotn.value_or(ray.dir().dot(n));
  }

  bool crosses(std::vector<Vector3> points) const {
    int side = getSide(points[0]);
    bool crosses = false;

    for (size_t i = 1; i < points.size(); i++)
      if (getSide(points[i]) != side) {
        crosses = true;
        break;
      }
    return crosses;
  }
};

struct Sat {
  Vector3 v;
  float mins[2], maxs[2];
  Sat() {
    for (int i = 0; i < 2; i++) {
      mins[i] = maxs[i] = 0.0f;
    }
  }

  void create(const Vector3 &v) {
    this->v = v;
    for (int i = 0; i < 2; i++) {
      mins[i] = maxs[i] = 0.0f;
    }
  }

  void set(const std::vector<Vector3> &points, bool first) {
    int index = first ? 0 : 1;
    mins[index] = maxs[index] = points[0].dot(v);
    for (size_t i = 1; i < points.size(); i++) {
      float dist = points[i].dot(v);
      mins[index] = std::min(mins[index], dist);
      maxs[index] = std::max(maxs[index], dist);
    }
  }

  void append(const std::vector<Vector3> &points, bool first) {
    int index = first ? 0 : 1;
    for (auto p : points) {
      float dist = p.dot(v);
      mins[index] = std::min(mins[index], dist);
      maxs[index] = std::max(maxs[index], dist);
    }
  }

  void set(const Vector3 &point, bool first) {
    int index = first ? 0 : 1;
    mins[index] = maxs[index] = point.dot(v);
  }

  void append(const Vector3 &point, bool first) {
    int index = first ? 0 : 1;
    float dist = point.dot(v);
    mins[index] = std::min(mins[index], dist);
    maxs[index] = std::max(maxs[index], dist);
  }

  bool operator()() const {
    return maxs[0] >= mins[1] && maxs[1] >= mins[0];
  }
};

[[maybe_unused]] static float areaOfTriangle(const Vector3 &p, const Vector3 &p1, const Vector3 &p2) {
  return 0.5f * Vector3(p.point(p1)).cross(Vector3(p.point(p2))).length();
}

struct Rect {
  Vector3 p;
  Vector3 axes[2];
  Vector2 halfSize;
  Plane plane;

  void calculate(const Vector3 &p, const Vector3 &n, const Vector3 u, float halfW, float halfH) {
    this->p = p;
    plane.init(n, p);
    axes[0] = u.normalized();
    axes[1] = n.cross(u).normalized();
    halfSize.x = fabsf(halfW);
    halfSize.y = fabsf(halfH);

  }

  std::optional<RaySeg> intersects(const Ray &ray, bool doubleSided = false);

};

struct Aabb {
  Vector3 p;
  Vector3 halfSize;

  Aabb() = default;

  Vector3 minExtent() const {
    return p - halfSize;
  }
  Vector3 maxExtent() const {
    return p + halfSize;
  }

  Vector3 size() const {
    return 2.0f * halfSize;
  }

  Aabb(const Vector3 &p_, const Vector3 &halfSize_)
      :
      p(p_),
      halfSize(halfSize_) {
  }

  Aabb(const Aabb &rhs)
      :
      p(rhs.p),
      halfSize(rhs.halfSize) {
  }

  void fromExtents(const Vector3 &min, const Vector3 &max) {
    this->p = p;
    for (int i = 0; i < 3; i++) {
      this->halfSize.xyz[i] = fabsf(max.xyz[i] - min.xyz[i]) * 0.5f;
      this->p.xyz[i] = 0.5f * fabsf(min.xyz[i] + max.xyz[i]);
    }
  }

  std::optional<RaySeg> clip(const Ray &ray);

  bool collidesWith(const std::array<Vector3, 3> &triPoints);
};

}
}
