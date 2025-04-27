#include "grid.h"
#include "../../utils/log.h"

using namespace mbz::math;
using namespace mbz::math::bpcd;
using namespace mbz::utils::logger;
using namespace mbz::utils::heap;

uint32_t Grid::getHashOf(int l, int r, int c) {
  uint32_t hash = hasher<int>(hasher(), l);
  hash = hasher<int>(hash, r);
  hash = hasher<int>(hash, c);
  return hash;
}

void Grid::getBoxes(std::vector<Aabb> &boxes) {
  boxes.clear();
  int n;
  Heap::Tile *tile = cells.getList(n);
  Hashmap<Cell>::Container *list = tile->p<Hashmap<Cell>::Container>();
  for (int i = 0; i < n; i++) {
    boxes.push_back((list[i])()->aabb);
  }
  heap->release(tile);
}

bool Grid::build(const std::vector<std::array<Vector3, 3>> &trisPoints, Vector3 cellSize) {
  int n = int(trisPoints.size());
  Vector3 min = trisPoints[0][0];
  Vector3 max = trisPoints[0][0];
  for (int i = 0; i < n; i++) {
    Bcs3 bcs;
    bcs.init(trisPoints[i][0], trisPoints[i][1], trisPoints[i][2]);
    tris.append(bcs);
    for (int j = 0; j < 3; j++) {
      min.x = std::min(min.x, trisPoints[i][j].x);
      max.x = std::max(max.x, trisPoints[i][j].x);

      min.y = std::min(min.y, trisPoints[i][j].y);
      max.y = std::max(max.y, trisPoints[i][j].y);

      min.z = std::min(min.z, trisPoints[i][j].z);
      max.z = std::max(max.z, trisPoints[i][j].z);
    }
  }
  bigBox.fromExtents(min, max);
  o = bigBox.minExtent();
  LOGINFO("Grid::build()", "Extents: {%.4f, %.4f, %.4f} / {%.4f, %.4f, %.4f}", min.x, min.y, min.z, max.x, max.y, max.z);
  LOGINFO("Grid::build()", "Origin.: {%.4f, %.4f, %.4f}", o.x, o.y, o.z);
  this->cellSize = cellSize;

  int index = 0;
  for (auto &triPoints : trisPoints) {
    int l1, l2, l3, lRange[2];
    int r1, r2, r3, rRange[2];
    int c1, c2, c3, cRange[2];
    getCellForPoint(triPoints[0], l1, r1, c1);
    getCellForPoint(triPoints[1], l2, r2, c2);
    getCellForPoint(triPoints[2], l3, r3, c3);
    lRange[0] = std::min(l1, std::min(l2, l3)) - 1;
    rRange[0] = std::min(r1, std::min(r2, r3)) - 1;
    cRange[0] = std::min(c1, std::min(c2, c3)) - 1;
    lRange[1] = std::max(l1, std::max(l2, l3)) + 1;
    rRange[1] = std::max(r1, std::max(r2, r3)) + 1;
    cRange[1] = std::max(c1, std::max(c2, c3)) + 1;

    //printf("levels.: %d - %d\n", lRange[0], lRange[1]);
    //printf("rows...: %d - %d\n", rRange[0], rRange[1]);
    //printf("columns: %d - %d\n", cRange[0], cRange[1]);

    for (int l = lRange[0]; l <= lRange[1]; l++) {
      for (int r = rRange[0]; r <= rRange[1]; r++) {
        for (int c = cRange[0]; c <= cRange[1]; c++) {
          Aabb aabb = cellBox(l, r, c);
          Sphere sphere(aabb.p, aabb.halfSize.lengthSq());
          Sphere sphere2(triPoints);
          if (!sphere.touches(sphere2))
            continue;
          if(!aabb.intersects(triPoints))
            continue;

          //if (!aabb.collidesWith(triPoints))
          //  continue;
          Cell *cell = cells.insertIf(Cell(l, r, c, aabb, heap));
          cell->triIndices.append(index);
        }
      }
    }
    index++;
  }

//  Hashmap<Cell>::E *elems = cells.pool->p<Hashmap<Cell>::E>();
//  for (uint32_t i = 0; i < cells.tableSize; i++) {
//    if (!elems[i].data)
//      continue;
//
//  }

  return true;
}

bool Grid::traceRay(RaySeg raySeg, Trace &trace, std::optional<std::reference_wrapper<std::vector<std::array<int, 3>>>> indices) const {

  trace.raySeg = raySeg;
  trace.bcsCoord = std::nullopt;
  trace.point = std::nullopt;
  for (int i = 0; i < 3; i++) {
    if (fabsf(raySeg.d.xyz[i]) < math::tol)
      raySeg.d.xyz[i] = 0.0f;
  }
  if (indices.has_value())
    indices->get().clear();

  int l, r, c;
  Vector3 o = raySeg.p;
  Vector3 p = o;
  getCellForPoint(raySeg.p, l, r, c);
  auto d = raySeg.d;

  int dl = d.z < -math::tol ? -1 : d.z > math::tol ? +1 : 0;
  int dr = d.y < -math::tol ? -1 : d.y > math::tol ? +1 : 0;
  int dc = d.x < -math::tol ? -1 : d.x > math::tol ? +1 : 0;

  auto march = [&](float &distLeft) {
    if (indices.has_value())
      indices->get().push_back( { l, r, c });
    auto box = cellBox(l, r, c);
    Vector3 min = box.minExtent();
    Vector3 max = box.maxExtent();

    Vector3 dist;
    for (int i = 0; i < 3; i++) {
      if (d.xyz[i] >= math::tol) {
        dist.xyz[i] = (max.xyz[i] - p.xyz[i]) / d.xyz[i];
      } else if (d.xyz[i] <= math::tol) {
        dist.xyz[i] = (min.xyz[i] - p.xyz[i]) / d.xyz[i];
      } else
        dist.xyz[i] = INFINITY;
    }
    float shortest = std::min(dist.x, std::min(dist.y, dist.z));
    shortest = std::min(shortest, distLeft);
    Vector3 p2 = p + shortest * d;
    auto cell = cells.kcontains(getHashOf(l, r, c));
    if (cell) {
      bool hit = false;
      Vector3 e = p2;
      for (int i = 0; i < cell->triIndices.size; i++) {
        RaySeg seg(o, e);
        int j = cell->triIndices.kp()[i];
        const auto &bcs = tris.kp()[j];
        auto coord = bcs.project(seg);
        if (coord.has_value()) {
          trace.index = j;
          trace.bcsCoord = coord;
          trace.point = bcs.o + coord->x * bcs.u + coord->y * bcs.v;
          trace.raySeg = seg;
          hit = true;
          e = trace.point.value();
        }
      }
      if (hit) {
        return false;
      }
    }
    distLeft -= shortest;
    if (distLeft == 0.0f)
      return false;

    p = p2;
    if (dist.x == shortest)
      c += dc;
    if (dist.y == shortest)
      r += dr;
    if (dist.z == shortest)
      l += dl;
    return true;
  };

  Vector3 u = raySeg.end() - raySeg.p;
  if (fabsf(u.x) < math::tol && fabsf(u.y) < math::tol && fabsf(u.z) < math::tol)
    return false;

  float dist = raySeg.dist;
  for (int i = 0; i < 500; i++)
    if (!march(dist))
      break;

  return trace();
  /*
  bool hit = false;
  for (int i = 0; i < tris.size; i++) {
    auto bcs = tris.kp()[i];
    auto coord = bcs.project(raySeg);
    if (coord.has_value()) {
      trace.index = i;
      trace.bcsCoord = coord;
      trace.point = bcs.o + coord->x * bcs.u + coord->y * bcs.v;
      raySeg.dist = raySeg.p.point(*trace.point).dot(raySeg.d);
      hit = true;
    }
  }
  return hit;
  */
}
