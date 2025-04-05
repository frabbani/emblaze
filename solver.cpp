#include "solver.h"

#include "utils/log.h"
#include "utils/file.h"
#include "math/noise.h"
#include "math/geometry.h"

#include "thirdparty/lodepng/lodepng.h"
#include "thirdparty/openfbx/ofbx.h"

#include <string>
#include <sstream>
#include <memory>

using namespace mbz;
using namespace mbz::math;
/*
 mbz::AOSolver::AOSolver() {
 auto work = [&]() {
 std::unique_lock<std::mutex> lock(this->mutex);
 this->condition.wait(lock, [&] {
 return this->start;
 });

 std::string id;
 std::stringstream ss;
 ss << std::this_thread::get_id();
 ss >> id;

 {
 std::lock_guard<std::mutex> lg(this->taskMutex);
 if (this->tasks.size()) {
 LOGINFO(id.c_str(), "Working on element %zu\n", this->tasks.size());
 this->tasks.pop_back();
 }
 }
 LOGINFO(id.c_str(), "Done!");
 };

 for (auto &worker : workers)
 worker = std::thread(work);
 }

 mbz::AOSolver::~AOSolver() {
 for (std::thread &worker : workers)
 if (worker.joinable())
 worker.join();
 }

 void mbz::AOSolver::begin() {
 start = true;
 condition.notify_all();
 }
 */

bool AOSolver::create(std::string_view fbxName, uint32_t rasterWidth, uint32_t rasterHeight) {
  utils::FileData data(std::string("assets/") + std::string(fbxName) + std::string(".fbx"));
  ofbx::LoadFlags f =
  //    ofbx::LoadFlags::IGNORE_MODELS |
      ofbx::LoadFlags::IGNORE_BLEND_SHAPES | ofbx::LoadFlags::IGNORE_CAMERAS | ofbx::LoadFlags::IGNORE_LIGHTS |
      //    ofbx::LoadFlags::IGNORE_TEXTURES |
          ofbx::LoadFlags::IGNORE_SKIN | ofbx::LoadFlags::IGNORE_BONES | ofbx::LoadFlags::IGNORE_PIVOTS |
          //    ofbx::LoadFlags::IGNORE_MATERIALS |
          ofbx::LoadFlags::IGNORE_POSES | ofbx::LoadFlags::IGNORE_VIDEOS | ofbx::LoadFlags::IGNORE_LIMBS |
          //    ofbx::LoadFlags::IGNORE_MESHES |
          ofbx::LoadFlags::IGNORE_ANIMATIONS;

  auto scene = ofbx::load(data.data.data(), data.data.size(), ofbx::u16(f));
  if (!scene->getMeshCount()) {
    LOGERROR(__FUNCTION__, "scene has no mesh\n");
    return false;
  }
  auto mesh = scene->getMesh(0);

  textures.clear();
  for (int i = 0; i < mesh->getMaterialCount(); i++) {
    std::string name(mesh->getMaterial(i)->name);
    std::string file = "assets/" + name + ".png";
    LOGINFO(__FUNCTION__, "loading texture '%s'", file.c_str());
    uint32_t w, h;
    std::vector<uint8_t> pixels;
    auto error = lodepng::decode(pixels, w, h, file);
    if (error) {
      LOGERROR(__FUNCTION__, "PNG load failed: %s", lodepng_error_text(error));
      continue;
    }
    LOGINFO(__FUNCTION__, " - texture size %d x %d", w, h);
    utils::img::Image image;
    image.w = w;
    image.h = h;
    image.pixels = std::vector<Color>(image.w * image.h);
    for (int j = 0; j < int(w * h); j++) {
      uint8_t r = pixels.data()[j * 4];
      uint8_t g = pixels.data()[j * 4 + 1];
      uint8_t b = pixels.data()[j * 4 + 2];
      image.pixels.data()[j] = Color(r, g, b);
    }
    image.createMips();
    textures.push_back(-1);
    rasterizer::loadTexture(image, name, textures.back());
  }

  std::vector<rasterizer::VariableType> types;
  types.push_back(rasterizer::VariableType::ScalarType);  //  id
  types.push_back(rasterizer::VariableType::Vector3Type);  //  position
  types.push_back(rasterizer::VariableType::Vector3Type);  //  normal
  types.push_back(rasterizer::VariableType::TexelType);   //  color

  canvas = std::make_shared<rasterizer::Canvas>(heap, rasterWidth, rasterHeight, types);
  scanner = std::make_shared<rasterizer::Scanner>(*canvas);

  const auto &geom = mesh->getGeometry()->getGeometryData();

  auto positions = geom.getPositions();
  auto normals = geom.getNormals();
  auto uv1s = geom.getUVs(0);
  auto uv2s = geom.getUVs(1);

  //vertices.init(heap, positions.count);
  // triangles.init(heap, positions.count * 3);

  auto push_vertex = [&](int index) {
    auto p = positions.get(index);
    auto n = normals.get(index);
    auto uv1 = uv1s.get(index);
    auto uv2 = uv2s.get(index);
    Vertex vert;
    vert.p = Vector3(p.x, p.y, p.z);
    vert.n = Vector3(n.x, n.y, n.z);
    vert.uv1 = Vector2(uv1.x, uv1.y);
    vert.uv2 = Vector2(uv2.x, uv2.y);
    vertices.append(vert);
    return int(vertices.size - 1);
  };

  for (int i = 0; i < geom.getPartitionCount(); i++) {
    const auto &par = geom.getPartition(i);
    for (int j = 0; j < par.polygon_count; j++) {
      auto poly = par.polygons[j];
      int a = push_vertex(poly.from_vertex + 0);
      int b = push_vertex(poly.from_vertex + 1);
      int c = push_vertex(poly.from_vertex + 2);
      triangles.append( { a, b, c, i });
    }
  }

  grid = std::make_shared<math::bpcd::Grid>(heap);
  std::vector<std::array<Vector3, 3>> tris;
  for (int i = 0; i < triangles.size; i++) {
    Vector3 vs[3];
    for (int j = 0; j < 3; j++)
      vs[j] = vertices.kp()[triangles.kp()[i][j]].p;
    tris.push_back( { vs[0], vs[1], vs[2] });
  }

  grid->build(tris, Vector3(0.5f, 0.5f, 0.5f));

  rasterizer::Point pts[3];
  pts[0] = canvas->createPoint();
  pts[1] = canvas->createPoint();
  pts[2] = canvas->createPoint();

  auto tri_area = [](Vector2 p, Vector2 p2, Vector2 p3) {
    Vector2 u = p.point(p2);
    Vector2 v = p.point(p3);
    return 0.5f * (u.x * v.y - v.x * u.y);
  };

  math::Matrix2 M;
  M.identity();
  M.e00 = float(canvas->w - 1);
  M.e11 = float(canvas->h - 1);
  for (int i = 0; i < triangles.size; i++) {
    auto tri = triangles.kp()[i];
    Vector2 s1 = vertices.kp()[tri[0]].uv1;
    Vector2 s2 = vertices.kp()[tri[1]].uv1;
    Vector2 s3 = vertices.kp()[tri[2]].uv1;

    Vector2 t1 = vertices.kp()[tri[0]].uv2;
    Vector2 t2 = vertices.kp()[tri[1]].uv2;
    Vector2 t3 = vertices.kp()[tri[2]].uv2;

    Vector3 p1 = vertices.kp()[tri[0]].p;
    Vector3 p2 = vertices.kp()[tri[1]].p;
    Vector3 p3 = vertices.kp()[tri[2]].p;

    Vector3 n1 = vertices.kp()[tri[0]].n;
    Vector3 n2 = vertices.kp()[tri[1]].n;
    Vector3 n3 = vertices.kp()[tri[2]].n;

    float id = float(i + 1);
    int texHandle = textures[tri[3]];
    int level = utils::img::computeMipmapLevel(tri_area(t1, t2, t3), tri_area(s1, s2, s3)) - 1;

    pts[0].p = M * s1;
    std::get<rasterizer::ScalarVariable>(pts[0].plots[0]).v = id;
    std::get<rasterizer::Vector3Variable>(pts[0].plots[1]).v = p1;
    std::get<rasterizer::Vector3Variable>(pts[0].plots[2]).v = n1;
    std::get<rasterizer::TexelVariable>(pts[0].plots[3]).set(t1, texHandle, level);

    pts[1].p = M * s2;
    std::get<rasterizer::ScalarVariable>(pts[1].plots[0]).v = id;
    std::get<rasterizer::Vector3Variable>(pts[1].plots[1]).v = p2;
    std::get<rasterizer::Vector3Variable>(pts[1].plots[2]).v = n2;
    std::get<rasterizer::TexelVariable>(pts[1].plots[3]).set(t2, texHandle, level);

    pts[2].p = M * s3;
    std::get<rasterizer::ScalarVariable>(pts[2].plots[0]).v = id;
    std::get<rasterizer::Vector3Variable>(pts[2].plots[1]).v = p3;
    std::get<rasterizer::Vector3Variable>(pts[2].plots[2]).v = n3;
    std::get<rasterizer::TexelVariable>(pts[2].plots[3]).set(t3, texHandle, level);

    scanner->buildEdge(pts[0], pts[1]);
    scanner->buildEdge(pts[0], pts[2]);
    scanner->buildEdge(pts[1], pts[2]);
    scanner->scanReset();
  }

  utils::img::Image image;
  image.w = int(canvas->w);
  image.h = int(canvas->h);
  image.pixels.reserve(image.w * image.h);

  auto &layer = std::get<rasterizer::Vector3Variables>(canvas->layers[1]);
  image.pixels.clear();
  for (int i = 0; i < layer.size; i++) {
    auto p = layer[i].v - grid->o;
    p.x /= grid->bigBox.size().x;
    p.y /= grid->bigBox.size().y;
    p.z /= grid->bigBox.size().z;
    p = 255.0f * p;
    image.pixels.push_back(Color(p.x, p.y, p.z));
  }
  utils::img::writeImageToBMPFile(image, "position");

  auto &layer2 = std::get<rasterizer::Vector3Variables>(canvas->layers[2]);
  image.pixels.clear();
  for (int i = 0; i < layer2.size; i++) {
    Vector3 normal = layer2[i].v;
    Vector3 c = 255.0f * (0.5f * normal + Vector3(0.5f, 0.5f, 0.5f));
    image.pixels.push_back(Color(c.x, c.y, c.z));
  }
  utils::img::writeImageToBMPFile(image, "normal");

  auto &layer3 = std::get<rasterizer::TexelVariables>(canvas->layers[3]);
  image.pixels.clear();
  for (int i = 0; i < layer3.size; i++) {
    rasterizer::Texel texel = layer3[i].v;
    image.pixels.push_back(texel.sample());
  }
  utils::img::writeImageToBMPFile(image, "albedo");

  auto &maskLayer = std::get<rasterizer::ScalarVariables>(canvas->layers[0]);
  auto &positionLayer = std::get<rasterizer::Vector3Variables>(canvas->layers[1]);
  auto &normalLayer = std::get<rasterizer::Vector3Variables>(canvas->layers[2]);

  int w = int(canvas->w);
  int h = int(canvas->h);
  {
    std::lock_guard<std::mutex> lg(todoMutex);
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++) {
        int i = y * w + x;
        if (0.0f == maskLayer[i].v)
          continue;
        std::unique_ptr<Task> task = std::make_unique<Task>();
        task->x = x;
        task->y = y;
        task->grid = grid;
        task->p = positionLayer[i].v;
        task->n = normalLayer[i].v;
        todo.append_move(std::move(task));
      }

  }
  mbz::math::mc::seedRand(5489);
  return true;
}

void AOSolver::save() {
  int w(canvas->w), h(canvas->h);
  result.w = w;
  result.h = h;
  result.pixels = std::vector<Color>(w * h);

  {
    std::lock_guard<std::mutex> lg(completedMutex);
    printf("completed size: %d\n", completed.size);
    for (int i = 0; i < completed.size; i++) {
      auto t = static_cast<Task*>(completed.p()[i].release());
      result.pixels[t->y * w + t->x] = Color(t->result.x, t->result.y, t->result.z);
      delete t;
    }
  }
  utils::img::writeImageToBMPFile(result, "ao");
  LOGINFO("AOSolver::save()", "saved result as 'ao.bmp' to disk.");

}

void AOSolver::Task::perform() {
  int N = 32;

  /*
   Vector3 v = p - grid->o;
   v.x /= grid->bigBox.size().x;
   v.y /= grid->bigBox.size().y;
   v.z /= grid->bigBox.size().z;
   result = 255.0f * v;
   //result = 255.0f * (Vector3(0.5f, 0.5f, 0.5f) + 0.5f * n);
   return;
   */

  /*
   Vector3 bases[3];
   bases[2] = n;
   if (fabs(n.dot(Vector3(1.0f, 0.0f, 0.0f)) < cosf(75.0f * math::Pi / 180.0f))) {
   bases[1] = bases[2].cross(Vector3(1.0f, 0.0f, 0.0f)).normalized();
   bases[0] = bases[1].cross(bases[2]).normalized();
   } else {
   bases[1] = bases[2].cross(Vector3(0.0f, 1.0f, 0.0f)).normalized();
   bases[0] = bases[1].cross(bases[2]).normalized();
   }

   float alpha = 255.0f;
   float delta = 255.0f / float(N);

   for (int i = 0; i < N; i++) {
   auto d = math::mc::randomPointOnHemisphere();
   d = bases[0] * d.x + bases[1] * d.y + bases[2] * d.z;
   Ray ray(p + 0.01 * n, d);
   RaySeg raySeg(ray, 10.0f);
   bpcd::Grid::Trace trace(raySeg);
   grid->traceRay(raySeg, trace);
   if (trace.point.has_value()) {
   alpha -= delta * d.dot(n);
   }
   result = Vector3(alpha, alpha, alpha);
   }
   */

  int total = 0;
  float sum = 0.0f;
  while (total < N) {
    auto d = math::mc::randomPointOnSphere();
    if (d.dot(n) <= 0.0f)
      continue;
    total++;
    Ray ray(p + 0.001 * n, d);
    RaySeg raySeg(ray, 10.0f);
    bpcd::Grid::Trace trace(raySeg);
    grid->traceRay(raySeg, trace);
    if (!trace.point.has_value())
      sum += d.dot(n);
  }
  sum *= 255.0f * 2.0f / float(N);
  sum = std::max(0.0f, std::min(sum, 255.0f));
  result = Vector3(sum, sum, sum);
}
