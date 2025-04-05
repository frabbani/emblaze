#include "geometry.h"

namespace mbz {
namespace math {

std::optional<RaySeg> Rect::intersects(const Ray &ray, bool doubleSided) {
  float ddotn = ray.d.dot(plane.n);
  if (fabsf(ddotn) < tol)
    return std::nullopt;
  if (!doubleSided && ddotn >= 0.0f) {
    return std::nullopt;
  }

  float dist = plane.rayDist(ray);
  if (dist < 0.0f)
    return std::nullopt;
  Vector3 r = (ray.p + dist * ray.d) - p;
  float xProj = r.dot(axes[0]);
  if (xProj < -halfSize.x || xProj > halfSize.x)
    return std::nullopt;
  float yProj = r.dot(axes[1]);
  if (yProj < -halfSize.y || yProj > halfSize.y)
    return std::nullopt;

  return RaySeg(ray, dist);
}

std::optional<RaySeg> Aabb::clip(const Ray &ray) {

  auto swap = [](float &a, float &b) {
    int t = a;
    a = b;
    b = t;
  };

  Vector3 min = minExtent();
  Vector3 max = maxExtent();

  float tMin = -INFINITY;
  float tMax = +INFINITY;

  for (int i = 0; i < 3; i++) {
    if (0.0f == ray.d.xyz[i]) {
      if (ray.p.xyz[i] < min.xyz[i] || ray.p.xyz[i] > max.xyz[i])
        return std::nullopt;
      continue;
    }

    float inv = 1.0f / ray.d.xyz[i];
    float t0 = (min.xyz[i] - ray.p.xyz[i]) * inv;
    float t1 = (max.xyz[i] - ray.p.xyz[i]) * inv;

    if (t0 > t1)
      swap(t0, t1);
    tMin = std::max(tMin, t0);
    tMax = std::min(tMax, t1);

    if (tMin > tMax)
      return std::nullopt;
  }

  if (tMax < 0.0f) {
    return std::nullopt;
  }

  tMin = std::max(tMin, 0.0f);

  Vector3 p = ray.p + tMin * ray.d;
  Vector3 p2 = ray.p + tMax * ray.d;

  return RaySeg(p, p2);
}

bool Aabb::collidesWith(const std::array<Vector3, 3> &triPoints) {
  std::array<Vector3, 3> axes = { Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) };

  auto overlap = [&](const Vector3 &axis) {
    Sat sat;
    sat.create(axis);
    sat.set(minExtent(), false);
    sat.append(maxExtent(), false);

    sat.set(triPoints[0], true);
    sat.append(triPoints[1], true);
    sat.append(triPoints[2], true);
    return sat();
  };

  Vector3 n = triPoints[0].point(triPoints[1]).cross(triPoints[0].point(triPoints[2]));
  Plane plane(n, triPoints[0]);

  for(int j = 0; j < 3; j++){
    if(!overlap(axes[j]))
      return false;
  }

  for(int i = 0; i < 3; i++){
    Vector3 edge = triPoints[(i+1) %3].point(triPoints[i]);
    for(int j = 0; j < 3; j++){
      Vector3 axis = edge.cross(axes[j]);
      if(axis.lengthSq() < tolSq)
        continue;
      if(!overlap(axis))
        return false;
    }
    if(!overlap(edge.cross(plane.n)))
      return false;
  }
  return true;
}

}
}
