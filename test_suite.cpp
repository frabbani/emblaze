#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <set>
#include <map>
#include <string>

#include "utils/log.h"
#include "utils/file.h"
#include "utils/heap.h"
#include "utils/image.h"
#include "utils/hash.h"
#include "utils/workers.h"

#include "math/vector.h"
#include "math/geometry.h"
#include "math/bcs.h"
#include "math/noise.h"
#include "math/bpcd/grid.h"

#include "rasterizer/rasterizer.h"
#include "thirdparty/mtwister/mtwister.h"
#include "thirdparty/openfbx/ofbx.h"
#include "thirdparty/lodepng/lodepng.h"

using namespace mbz;
using namespace mbz::math;

void testHeap() {
  utils::heap::Heap heap(64 * 1024 * 1024);
  MTRandWrapper mt(1337);

  LOGINFO(__FUNCTION__, "%f out of %f mbs reserved for use", (float ) (heap.total() - heap.remaining()) / (1024.0f * 1024.0f), float(heap.total()) / (1024.0f * 1024.0f));
  utils::heap::Heap::Tile *allocs[200] = { NULL };
  int requested = 0;
  for (int i = 0; i < 5000; i++) {
    int n = mt.randomLong() % 200;
    if (mt.randomBool() && allocs[n]) {
      heap.release(allocs[n]);
      allocs[n] = nullptr;
    }
    int size = mt.randomLong() % (16 * 1024) + 1;  // allocate random sized memory between 1 and 16k bytes
    allocs[n] = heap.reserve(size);
    requested += size;
  }

  float recycle_rate = 100.0f * (float) heap.recycles / (float) (heap.reservations + heap.recycles);
  LOGINFO(__FUNCTION__, "reservations v. recycles: %d v %d (recycle rate ~%.2f%%)", heap.reservations, heap.recycles, recycle_rate);
  LOGINFO(__FUNCTION__, "%f out of %f mbs reserved for use", ((float ) heap.total() - heap.remaining()) / (1024.0f * 1024.0f), (float ) heap.total() / (1024.0f * 1024.0f));
  LOGINFO(__FUNCTION__, "%f mbs actually requested", (float ) requested / (1024.0f * 1024.0f));
}

void testBuffer() {
  std::shared_ptr<utils::heap::Heap> heap = std::make_shared<utils::heap::Heap>(4096);
  utils::heap::Array<int> buffer( heap, 8);

  for(int i = 0; i < 128; i++){
    buffer.append(i);
    LOGINFO(__FUNCTION__, " %d (%u)", buffer[i], buffer.tile->size<int>());
  }

  float recycle_rate = 100.0f * (float) heap->recycles / (float) (heap->reservations + heap->recycles);
  LOGINFO(__FUNCTION__, "reservations v. recycles: %d v %d (recycle rate ~%.2f%%)", heap->reservations, heap->recycles, recycle_rate);
  LOGINFO(__FUNCTION__, "%f out of %f mbs reserved for use", ((float ) heap->total() - heap->remaining()) / (1024.0f * 1024.0f), (float ) heap->total() / (1024.0f * 1024.0f));
}


void testAABB() {
  std::array<Vector3, 3> tri = { Vector3(1.0f, 1.0f, 0.0f),  // Point 1
  Vector3(3.0f, 1.0f, 0.0f),  // Point 2
  Vector3(2.0f, 3.0f, 0.0f)   // Point 3
      };
  Aabb aabb(Vector3(2.0f, 2.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f));
  LOGINFO(__FUNCTION__, "collision with triangle: %s", aabb.collidesWith(tri) ? "yes" : "no");

  aabb.p = Vector3();
  Ray ray(Vector3(-5.0f, -5.0f, -4.0f), Vector3(1, 1, 1));  // A ray starting at (0, 0, -5) going in the +Z direction

// Perform the intersection test
  auto result = aabb.clip(ray);

  if (result) {
    // If there's an intersection, print the entry and exit points
    LOGINFO(__FUNCTION__, "ray intersect:");
    LOGINFO(__FUNCTION__, " + enter: {%f, %f, %f}", result->p.x, result->p.y, result->p.z);
    LOGINFO(__FUNCTION__, " + exit.: {%f, %f, %f}", result->end().x, result->end().y, result->end().z);
  }

}

void testIntersect() {
  utils::img::Image texImage;
  utils::img::Image renderImage;
  utils::FileData data("assets/grass.bmp");
  texImage.fromBMP(data.data, "grass");
  renderImage.w = renderImage.h = 256;
  renderImage.pixels = std::vector<Color>(renderImage.w * renderImage.h);
  printf("texture: %d x %d\n", texImage.w, texImage.h);
  printf("render area: %d x %d\n", renderImage.w, renderImage.h);

  struct Vert {
    Vector3 p;
    Vector2 uv;
    Color c;
  };

  Vert triVerts[3];
  triVerts[0].p = Vector3(-0.5f, -0.5f, 0.01f);
  triVerts[0].uv = Vector2(0.0f, 0.0f);
  triVerts[0].c = Color(255, 0, 0);

  triVerts[1].p = Vector3(0.5f, -0.25f, 0.02f);
  triVerts[1].uv = Vector2(1.0f, 0.0f);
  triVerts[1].c = Color(0, 255, 0);

  triVerts[2].p = Vector3(0.2f, 0.35f, 0.03f);
  triVerts[2].uv = Vector2(0.5f, 1.0f);
  triVerts[2].c = Color(0, 0, 255);

  Bcs3 bcs;
  bcs.init(triVerts[0].p, triVerts[1].p, triVerts[2].p);

  Vector3 o(0.0f, 0.0f, -2.0f);
  for (int y = 0; y < renderImage.h; y++) {
    for (int x = 0; x < renderImage.w; x++) {
      Vector3 d;
      d.x = float(x) / float(renderImage.w - 1) - 0.5f;
      d.y = float(y) / float(renderImage.h - 1) - 0.5f;
      d.z = 1.0f;
      Ray ray(o, d);
      auto coord = bcs.project(ray);
      if (!coord.has_value())
        continue;
      if (!coord->inside())
        continue;

      Color c = triVerts[0].c.lerp(triVerts[1].c, coord->x);
      c = c.lerp(triVerts[2].c, coord->y);
      Vector2 uv = triVerts[0].uv + coord->x * (triVerts[1].uv - triVerts[0].uv);
      uv += coord->y * (triVerts[2].uv - triVerts[0].uv);
      uv.x *= renderImage.w;
      uv.y *= renderImage.h;
      renderImage.put(x, y, texImage.sample(uv.x, uv.y));
    }
  }
  utils::img::writeImageToBMPFile(renderImage, "result.bmp");
}

void testFbx() {
  utils::FileData data("assets/demo_scene.fbx");
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
    printf("scene has no mesh\n");
    return;
  }
  auto mesh = scene->getMesh(0);
  printf(" * mesh: %s\n", mesh->name);
  for (int i = 0; i < mesh->getMaterialCount(); i++) {
    printf("    + material: '%s'\n", mesh->getMaterial(i)->name);
  }
  const auto &geom = mesh->getGeometry()->getGeometryData();
  for (int i = 0; i < geom.getPartitionCount(); i++) {
    const auto &par = geom.getPartition(i);
    printf("    + partition: %d polygons\n", par.polygon_count);
//    for (int j = 0; j < par.polygon_count; j++) {
//      auto poly = par.polygons[j];
//      printf("       - [ %d, %d ]\n", poly.from_vertex, poly.vertex_count);
//    }

  }
  auto positions = geom.getPositions();
  auto normals = geom.getNormals();
  auto uvs = geom.getUVs(0);
  auto uv2s = geom.getUVs(1);
  printf("    * %d positions\n", positions.count);
  printf("    * %d normals\n", normals.count);
  printf("    * %d texcoords (first)\n", uvs.count);
  printf("    * %d texcoords (second)\n", uv2s.count);
//  for(int i = 0; i < positions.count; i++){
//    auto v = positions.get(i);
//    printf("        - { %f, %f, %f }\n", v.x, v.y, v.z);
//  }

  auto export_partition = [&](ofbx::GeometryPartition par, ofbx::Vec2Attributes uv0s) {
    FILE *fp = fopen("dump.obj", "w");
    for (int i = 0; i < par.polygon_count; i++) {
      auto poly = par.polygons[i];
      auto p = positions.get(poly.from_vertex);
      fprintf(fp, "v %f %f %f\n", p.x, p.y, p.z);
      p = positions.get(poly.from_vertex + 1);
      fprintf(fp, "v %f %f %f\n", p.x, p.y, p.z);
      p = positions.get(poly.from_vertex + 2);
      fprintf(fp, "v %f %f %f\n", p.x, p.y, p.z);
    }
    for (int i = 0; i < par.polygon_count; i++) {
      auto poly = par.polygons[i];
      auto t = uv0s.get(poly.from_vertex);
      fprintf(fp, "vt %f %f\n", t.x, t.y);
      t = uv0s.get(poly.from_vertex + 1);
      fprintf(fp, "vt %f %f\n", t.x, t.y);
      t = uv0s.get(poly.from_vertex + 2);
      fprintf(fp, "vt %f %f\n", t.x, t.y);
    }
    int c = 1;
    for (int i = 0; i < par.polygon_count; i++) {
      fprintf(fp, "f %d/%d %d/%d %d/%d\n", c, c, c + 1, c + 1, c + 2, c + 2);
      c += 3;
    }
    fclose(fp);
  };

  export_partition(geom.getPartition(0), uv2s);

}

void testMipmap() {
  utils::FileData data("assets/grass.bmp");
  utils::img::Image image;
  image.fromBMP(data.data, "grass");
  image.createMips();
  int level = 1;
  utils::img::Image *ptr = &image;
  while (ptr->mip) {
    std::string bmp = "mip" + std::to_string(level);
    utils::img::writeImageToBMPFile(*ptr, bmp);
    ptr = ptr->mip.get();
    level++;
  }
}

void testRasterization() {
  /*
   1. create canvas
   2. load fbx file
   3. load textures forthe materials
   4. iterate over all the material faces and render to canvas
   5. export canvas to image
   */
  std::shared_ptr<utils::heap::Heap> heap = std::make_shared<utils::heap::Heap>(64 * 1024 * 1024);
  utils::FileData data("assets/demo_scene.fbx");
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
    printf("scene has no mesh\n");
    return;
  }
  auto mesh = scene->getMesh(0);
  const auto &geom = mesh->getGeometry()->getGeometryData();

  std::vector<int> textures(mesh->getMaterialCount());

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
    rasterizer::loadTexture(image, name, textures[i]);
  }

  auto uvs = geom.getUVs(0);
  auto uv2s = geom.getUVs(1);

  std::vector<rasterizer::VariableType> types;
  types.push_back(rasterizer::VariableType::TexelType);
  rasterizer::Canvas canvas(heap, 1024, 1024, types);
  rasterizer::Scanner scanner(canvas);
  rasterizer::Point pts[3];
  pts[0] = canvas.createPoint();
  pts[1] = canvas.createPoint();
  pts[2] = canvas.createPoint();

  auto tov = [](ofbx::Vec2 v) {
    return Vector2(v.x, v.y);
  };

  auto tri_area = [](Vector2 p, Vector2 p2, Vector2 p3) {
    Vector2 u = p.point(p2);
    Vector2 v = p.point(p3);
    return 0.5f * (u.x * v.y - v.x * u.y);
  };

  math::Matrix2 M;
  M.identity();
  M.e00 = float(canvas.w - 1);
  M.e11 = float(canvas.h - 1);
  for (int i = 0; i < geom.getPartitionCount(); i++) {
    int texHandle = textures[i];
    auto par = geom.getPartition(i);
    for (int j = 0; j < par.polygon_count; j++) {
      auto poly = par.polygons[j];
      Vector2 s1 = tov(uvs.get(poly.from_vertex));
      Vector2 s2 = tov(uvs.get(poly.from_vertex + 1));
      Vector2 s3 = tov(uvs.get(poly.from_vertex + 2));

      Vector2 t1 = tov(uv2s.get(poly.from_vertex));
      Vector2 t2 = tov(uv2s.get(poly.from_vertex + 1));
      Vector2 t3 = tov(uv2s.get(poly.from_vertex + 2));

      int level = utils::img::computeMipmapLevel(tri_area(t1, t2, t3), tri_area(s1, s2, s3)) - 1;

      pts[0].p = M * s1;
      std::get<rasterizer::TexelVariable>(pts[0].plots[0]).set(t1, texHandle, level);

      pts[1].p = M * s2;
      std::get<rasterizer::TexelVariable>(pts[1].plots[0]).set(t2, texHandle, level);

      pts[2].p = M * s3;
      std::get<rasterizer::TexelVariable>(pts[2].plots[0]).set(t3, texHandle, level);

      scanner.buildEdge(pts[0], pts[1]);
      scanner.buildEdge(pts[0], pts[2]);
      scanner.buildEdge(pts[1], pts[2]);
      scanner.scan();
      scanner.reset();
    }
  }

  auto &layer = std::get<rasterizer::TexelVariables>(canvas.layers[0]);
  std::vector<Color> buffer;
  buffer.reserve(layer.size);
  for(int i = 0; i < layer.size; i++){
    buffer.push_back(layer[i].v.sample());
  }

  utils::img::Image image;
  image.w = int(canvas.w);
  image.h = int(canvas.h);
  image.pixels = std::move(buffer);
  utils::img::writeImageToBMPFile(image, "rasterized");

}


void testHash() {
  printf("*** TEST ***\n");
  std::shared_ptr<utils::heap::Heap> heap = std::make_shared<utils::heap::Heap>(64 * 1024);
  srand(123);
  utils::heap::Hashmap<int> hashmap(16, [](const int &v) {
    return utils::heap::hasher<int>(utils::heap::hasher(), v);
  }
                                    , heap);
  for (int i = 0; i < 64; i++) {
    printf("inserting %d...\n", i);
    hashmap.insertIf(i);
  }
  hashmap.print();
  printf("*****\n");
  hashmap.resize();
  hashmap.print();

  int n;
  utils::heap::Heap::Tile *tile = hashmap.getList(n);
  utils::heap::Hashmap<int>::Container *list = tile->p<utils::heap::Hashmap<int>::Container>();

  printf("COUNT: %d\n", n);
  for (int i = 0; i < n; i++)
    printf("%d\n", *list[i]());
  heap->release(tile);

  printf("%d elements (%d insertions)\n", hashmap.size(), hashmap.inserts);
  float recycle_rate = 100.0f * (float) heap->recycles / (float) (heap->reservations + heap->recycles);
  printf("reservations v. recycles: %d v %d (recycle rate ~%.2f%%)\n", heap->reservations, heap->recycles, recycle_rate);
  printf("%f out of %f mbs reserved for use\n", ((float) heap->total() - heap->remaining()) / (1024.0f * 1024.0f), (float) heap->total() / (1024.0f * 1024.0f));
}

void testMultithread(){
  auto heap = std::make_shared<utils::heap::Heap>(4 * 1024);
   utils::multithread::Workers<8> workers(heap, 50);
   struct Task : public utils::multithread::Task{
     int x;
     virtual void perform(utils::multithread::Toolbox *toolbox) override {
       printf("x = %d\n", x);
     }
   };
   for(int i = 0; i < 25; i++){
     auto task = std::make_unique<Task>();
     task->x = 123 + i;
     workers.todo.append_move(std::move(task));
   }
   workers.begin();
   workers.join();
   for(int i = 0; i < workers.completed.size; i++){
     Task *done = static_cast<Task*>(workers.completed.p()[i].release());
     printf("done x = %d\n", done->x);
     delete done;
   }
}
