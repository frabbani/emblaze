#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <set>
#include <map>
#include <string>

#include <mygl.h>
#include <vecdefs.h>

#include "utils/log.h"
#include "utils/file.h"
#include "utils/heap.h"
#include "utils/image.h"
#include "utils/workers.h"

#include "math/vector.h"
#include "math/geometry.h"
#include "math/bcs.h"
//#include "math/noise.h"
#include "math/bpcd/grid.h"

#include "rasterizer/rasterizer.h"
#include "thirdparty/openfbx/ofbx.h"
#include "thirdparty/lodepng/lodepng.h"
#include "thirdparty/mysdl2/mysdl2.h"

#include "graphics/text.h"
#include "graphics/camera.h"
#include "graphics/shaders.h"
#include "utils/hash.h"

#include "solver.h"

using namespace sdl2;
using namespace mbz;
using namespace mbz::math;

MyGL_Vec3 myglv3(Vector3 v) {
  MyGL_Vec3 r;
  r.x = v.x;
  r.y = v.y;
  r.z = v.z;
  return r;
}

enum MouseLock {
  Unlocked,
  Locking,
  Locked
};
bool lockMouse = false;
MouseLock mouseLock = Unlocked;

SDL sdl;
MyGL *mygl = nullptr;
Camera cam;
MyGL_Vec3 rayPoints[2];
std::vector<std::array<int, 3>> traceCells;
std::vector<std::array<MyGL_Vec3, 3>> dtris;
std::shared_ptr<utils::heap::Heap> myHeap = nullptr;
std::unique_ptr<mbz::LightSolver> solver = nullptr;
std::shared_ptr<bpcd::Grid> grid = nullptr;
bool wireFrame = false;

constexpr int FPS = 40;
constexpr float DT_SECS = (1.0f / float(FPS));
constexpr float DT_MS = (1000.0f / float(FPS));

#define DISP_W  1280
#define DISP_H  720

std::mutex prMut;
void pr(const char *data) {
  std::lock_guard<std::mutex> lg(prMut);
  printf("%s", data);
  //printf("[%lf]: %s", double(sdl.getPerfCounter() - sdl.startCounter) / sdl.getPerfFreq(), data);
}

void resetMyGL() {
  MyGL_resetCull();
  MyGL_resetDepth();
  MyGL_resetBlend();
  MyGL_resetStencil();
  MyGL_resetColorMask();
}

struct DrawElement {
  std::string texName;
  std::string iboName;
  std::string vboName;
  int numPrimitives;
  void draw(MyGL &mygl) {
    mygl.W_matrix = MyGL_mat4Identity;
    mygl.samplers[0] = MyGL_str64(texName.c_str());
    MyGL_bindSampler(0);
    MyGL_drawIndexedVbo(vboName.c_str(), iboName.c_str(), MYGL_TRIANGLES, numPrimitives * 3);
  }
};

std::vector<DrawElement> drawElements;

void loadMap(const bpcd::Grid &grid) {
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
    LOGERROR(__FUNCTION__, "scene has no mesh\n");
    return;
  }
  auto mesh = scene->getMesh(0);

  printf(" * mesh: %s\n", mesh->name);
  for (int i = 0; i < mesh->getMaterialCount(); i++) {
    std::string name(mesh->getMaterial(i)->name);
    std::string file = "assets/" + name + ".png";
    LOGINFO(__FUNCTION__, "    - loading texture '%s'", file.c_str());
    FileData fileData(file);
    MyGL_Image image = MyGL_imageFromPNGData(fileData.data.data(), fileData.data.size(), mesh->getMaterial(i)->name);
    LOGINFO(__FUNCTION__, "       + texture size %u x %u", image.w, image.h);
    auto roImage = (MyGL_ROImage ) { image.w, image.h, image.pixels };
    MyGL_createTexture2D(name.c_str(), roImage, "rgb10a2", GL_TRUE, GL_TRUE, GL_TRUE);
    DrawElement e;
    e.texName = name;
    drawElements.push_back(e);
  }

  const auto &geom = mesh->getGeometry()->getGeometryData();
  for (int i = 0; i < geom.getPartitionCount(); i++) {
    const auto &par = geom.getPartition(i);
    printf("    + partition: %d polygons\n", par.polygon_count);
    auto &elem = drawElements[i];
    elem.numPrimitives = par.polygon_count;
    elem.iboName = elem.texName + " indices";
    MyGL_createIbo(elem.iboName.c_str(), par.polygon_count * 3);
    MyGL_IboStream stream = MyGL_iboStream(elem.iboName.c_str());
    int index = 0;
    for (int j = 0; j < par.polygon_count; j++) {
      auto poly = par.polygons[j];
      stream.data[index++] = poly.from_vertex;
      stream.data[index++] = poly.from_vertex + 1;
      stream.data[index++] = poly.from_vertex + 2;
    }
    MyGL_iboPush(elem.iboName.c_str());
  }

  auto positions = geom.getPositions();
  auto normals = geom.getNormals();
  auto uvs = geom.getUVs(0);
  auto uv2s = geom.getUVs(1);
  printf("    * %d positions\n", positions.count);
  printf("    * %d normals\n", normals.count);
  printf("    * %d texcoords (first)\n", uvs.count);
  printf("    * %d texcoords (second)\n", uv2s.count);

  //MyGL_VertexAttrib attribs[] = { { MYGL_VERTEX_FLOAT, MYGL_XYZW, GL_FALSE }, { MYGL_VERTEX_FLOAT, MYGL_XYZ, GL_FALSE }, { MYGL_VERTEX_FLOAT, MYGL_XY, GL_FALSE }, { MYGL_VERTEX_FLOAT, MYGL_XY, GL_FALSE }, };
  //MyGL_createVbo("Map", positions.count, attribs, 4);
  MyGL_VertexAttrib attribs[] = { { MYGL_VERTEX_FLOAT, MYGL_XYZW, GL_FALSE }, { MYGL_VERTEX_FLOAT, MYGL_XYZW, GL_FALSE } };
  MyGL_createVbo("Map", positions.count, attribs, 2);
  MyGL_VboStream stream = MyGL_vboStream("Map");
  struct V {
    MyGL_Vec4 p;
    //MyGL_Vec3 n;
    MyGL_Vec4 t;
    //MyGL_Vec2 t2;
  };
  V *verts = (V*) stream.data;
  for (int i = 0; i < positions.count; i++) {
    auto p = positions.get(i);
    verts[i].p = MyGL_vec4(p.x, p.y, p.z, 1.0f);
    //auto n = normals.get(i);
    //verts[i].n = MyGL_vec3(n.x, n.y, n.z);
    auto t = uvs.get(i);
    auto t2 = uv2s.get(i);
    verts[i].t = MyGL_vec4(t2.x, t2.y, t.x, t.y);
    //verts[i].t2 = MyGL_vec2(t2.x, t2.y);
  }

  MyGL_vboPush("Map");
  for (auto &e : drawElements)
    e.vboName = std::string("Map");

  /*
   std::vector<std::array<Vector3, 3>> tris;
   for (int i = 0; i < geom.getPartitionCount(); i++) {
   const auto &par = geom.getPartition(i);
   for (int j = 0; j < par.polygon_count; j++) {
   auto poly = par.polygons[j];
   auto p = positions.get(poly.from_vertex);
   auto p2 = positions.get(poly.from_vertex + 1);
   auto p3 = positions.get(poly.from_vertex + 2);
   Vector3 u(p.x, p.y, p.z);
   Vector3 v(p2.x, p2.y, p2.z);
   Vector3 w(p3.x, p3.y, p3.z);
   tris.push_back( { u, v, w });
   }
   }
   grid.build(tris, Vector3(0.25f, 0.25f, 0.25f));
   */

  dtris.clear();
  for (int i = 0; i < grid.tris.size; i++) {
    const Bcs3 &bcs = grid.tris.kp()[i];
    dtris.push_back( { myglv3(bcs.o), myglv3(bcs.o + bcs.u), myglv3(bcs.o + bcs.v) });
  }

}

void init() {

  printf("*** INIT ***\n");

  utils::logger::logFunc(pr);

  mygl = MyGL_initialize(pr, 1, 0);
  mygl->cull.on = GL_FALSE;
  mygl->depth.on = GL_TRUE;
  mygl->blend.on = GL_FALSE;
  mygl->stencil.on = GL_FALSE;
  mygl->clearColor = { 0.0f, 0.0f, 0.16f, 1.0f };
  mygl->clearDepth = 1.0f;
  resetMyGL();

  auto loadFont = [=](const char *name) {
    AsciiCharSet charSet(name);
    MyGL_loadAsciiCharSet(static_cast<MyGL_AsciiCharSet*>(&charSet), 1, 1);
  };
  loadFont("lemonmilk");
  loadShaderLibraries();

  myHeap = std::make_shared<utils::heap::Heap>(32 * 1024 * 1024);
  solver = std::make_unique<mbz::LightSolver>(myHeap);
  LightSolver::Lighting lighting;
  lighting.skyColor = Color(157, 169, 207);
  lighting.sunColor = Color(240, 238, 185);
  lighting.sunDirection = Vector3(1.0f, 1.0f, 2.0f).normalized();
  solver->create("demo_scene", 512, 512, lighting);
  grid = solver->grid;
  solver->begin();

  //std::vector<std::array<Vector3, 3>> tris;
  //tris.push_back(tri);
  //grid->build(tris, Vector3(1.0f, 1.0f, 1.0f));

  cam.position = { 0.0f, -0.25f, 0.5f };
  cam.upSpeed = 0.0625 / DT_SECS;
  cam.forwardSpeed = 0.25f / DT_SECS;
  cam.strafeSpeed = 0.125f / DT_SECS;
  cam.yawSpeed = 1.5f / DT_SECS;
  cam.pitchSpeed = 1.5f / DT_SECS;

  traceCells.clear();
  rayPoints[0] = cam.position;
  rayPoints[1] = MyGL_vec3Add(rayPoints[0], MyGL_vec3Scale(cam.lookVector(), 0.2f));

  //ray = RaySeg(p, p2);

  MyGL_Debug_setChatty(GL_TRUE);
  loadMap(*grid);
  MyGL_Debug_setChatty(GL_FALSE);

  printf("************\n");

}

#include <vecdefs.h>

void look() {
  if (mouseLock != MouseLock::Locked)
    return;
  float dx = -(float) (sdl.mouseX - DISP_W / 2) * 2.0f / (float) DISP_W;
  float dy = -(float) (sdl.mouseY - DISP_H / 2) * 2.0f / (float) DISP_H;

  cam.yawSpeed = 0.125f * dx / DT_SECS * 360.0f;
  cam.pitchSpeed = 0.125f * dy / DT_SECS * 360.0f;
  cam.look(1, 1, DT_SECS);
}

void step() {
  static uint32_t cpp = 0;
  cpp++;

  static bool pause = false;
  if (sdl.keyPress('p'))
    pause = !pause;
  if (pause)
    return;

  lockMouse = sdl.mouseKeyDown(2);
  //if (sdl.keyPress(SDLK_TAB))
  //  lockMouse = !lockMouse;

  if (sdl.keyPress('1'))
    wireFrame = !wireFrame;

  int yaw = 0;
  int pitch = 0;
  if (sdl.keyDown(SDLK_DOWN))
    pitch = +1;
  if (sdl.keyDown(SDLK_UP))
    pitch = -1;
  if (sdl.keyDown(SDLK_LEFT))
    yaw = +1;
  if (sdl.keyDown(SDLK_RIGHT))
    yaw = -1;

  int forward = 0;
  if (sdl.keyDown('w'))
    forward++;
  if (sdl.keyDown('s'))
    forward--;
  int side = 0;
  if (sdl.keyDown('a'))
    side--;
  if (sdl.keyDown('d'))
    side++;

  int up = 0;
  if (sdl.keyDown(' '))
    up++;
  if (sdl.keyDown('c'))
    up--;
  cam.look(yaw, pitch, DT_SECS);
  look();
  cam.move(forward, side, up, DT_SECS);

  if (sdl.keyDown('f') || sdl.mouseKeyDown(0)) {
    traceCells.clear();
    auto W = cam.worldMatrix();
    auto r = MyGL_vec4Transf(W, MyGL_vec4(0.1f, 0.0f, -0.1f, 1.0f));
    auto r2 = MyGL_vec4Transf(W, MyGL_vec4(0.1f, 10.0f, -0.1f, 1.0f));
    rayPoints[0] = MyGL_vec3(r.x, r.y, r.z);
    rayPoints[1] = MyGL_vec3(r2.x, r2.y, r2.z);

    Vector3 o = Vector3(rayPoints[0].x, rayPoints[0].y, rayPoints[0].z);
    Vector3 e = Vector3(rayPoints[1].x, rayPoints[1].y, rayPoints[1].z);
    RaySeg raySeg(o, e);
//    dtris.clear();
//  for (uint32_t i = 0; i < grid->tris.size; i++) {
//    const Bcs3 &bcs = grid->tris.get()[i];
//    dtris.push_back( { myglv3(bcs.o), myglv3(bcs.o + bcs.u), myglv3(bcs.o + bcs.v) });
//    auto coord = bcs.project(raySeg);
//    if (coord.has_value()) {
//      e = bcs.o + coord->x * bcs.u + coord->y * bcs.v;
//      raySeg.dist = o.point(e).dot(raySeg.d);
//    }
//  }
//  rayPoints[1] = myglv3(raySeg.end());
    bpcd::Grid::Trace trace(raySeg);
    grid->traceRay(raySeg, trace, std::ref(traceCells));
    if (trace.point.has_value()) {
      rayPoints[1] = myglv3(trace.point.value());
    }
  }
}

void drawLine(MyGL_Vec3 p, MyGL_Vec3 p2, MyGL_Vec4 color) {

  mygl->W_matrix = MyGL_mat4Identity;

  MyGL_VertexAttributeStream vs = MyGL_vertexAttributeStream("Position");
  MyGL_VertexAttributeStream cs = MyGL_vertexAttributeStream("Color");

  vs.arr.vec4s[0] = MyGL_vec4(p.x, p.y, p.z, 1.0f);
  vs.arr.vec4s[1] = MyGL_vec4(p2.x, p2.y, p2.z, 1.0f);

  cs.arr.vec4s[0] = MyGL_vec4Scale(color, 0.75f);
  cs.arr.vec4s[1] = color;

  mygl->primitive = MYGL_LINES;
  mygl->numPrimitives = 1;
  MyGL_drawStreaming("Position, Color");
}

void drawTris() {

  mygl->W_matrix = MyGL_mat4Identity;

  MyGL_VertexAttributeStream vs = MyGL_vertexAttributeStream("Position");
  MyGL_VertexAttributeStream cs = MyGL_vertexAttributeStream("Color");

  auto up = [](MyGL_Vec3 p) {
    MyGL_Vec4 v;
    v.x = p.x;
    v.y = p.y;
    v.z = p.z;
    v.w = 1.0f;
    return v;
  };

//  int i = 0;
//  for (auto &tri : dtris) {
//    vs.arr.vec4s[i + 0] = up(tri[0]);
//    vs.arr.vec4s[i + 1] = up(tri[1]);
//
//    vs.arr.vec4s[i + 2] = up(tri[1]);
//    vs.arr.vec4s[i + 3] = up(tri[2]);
//
//    vs.arr.vec4s[i + 4] = up(tri[2]);
//    vs.arr.vec4s[i + 5] = up(tri[0]);
//
//    cs.arr.vec4s[i + 0] = MyGL_vec4(1.0f, 0.0f, 0.0f, 1.0f);
//    cs.arr.vec4s[i + 1] = MyGL_vec4(0.0f, 1.0f, 0.0f, 1.0f);
//    cs.arr.vec4s[i + 2] = MyGL_vec4(0.0f, 0.0f, 1.0f, 1.0f);
//    cs.arr.vec4s[i + 3] = MyGL_vec4(1.0f, 0.0f, 0.0f, 1.0f);
//    cs.arr.vec4s[i + 4] = MyGL_vec4(0.0f, 1.0f, 0.0f, 1.0f);
//    cs.arr.vec4s[i + 5] = MyGL_vec4(0.0f, 0.0f, 1.0f, 1.0f);
//    i += 6;
//  }
//  mygl->primitive = MYGL_LINES;
//  mygl->numPrimitives = int(dtris.size()) * 3;
//  MyGL_drawStreaming("Position, Color");

  int i = 0;
  for (auto &tri : dtris) {
    vs.arr.vec4s[i + 0] = up(tri[0]);
    vs.arr.vec4s[i + 1] = up(tri[1]);
    vs.arr.vec4s[i + 2] = up(tri[2]);
    cs.arr.vec4s[i + 0] = MyGL_vec4(1.0f, 0.0f, 0.0f, 1.0f);
    cs.arr.vec4s[i + 1] = MyGL_vec4(0.0f, 1.0f, 0.0f, 1.0f);
    cs.arr.vec4s[i + 2] = MyGL_vec4(0.0f, 0.0f, 1.0f, 1.0f);
    i += 3;
  }
  mygl->primitive = MYGL_TRIANGLES;
  mygl->numPrimitives = int(dtris.size());
  MyGL_drawStreaming("Position, Color");
}

void drawBox(MyGL_Vec3 p, MyGL_Vec3 s, bool green = false) {
  MyGL_Vec4 corners[8];
  corners[0] = MyGL_vec4(-1.0f, -1.0f, -1.0f, +1.0f);
  corners[1] = MyGL_vec4(+1.0f, -1.0f, -1.0f, +1.0f);
  corners[2] = MyGL_vec4(+1.0f, +1.0f, -1.0f, +1.0f);
  corners[3] = MyGL_vec4(-1.0f, +1.0f, -1.0f, +1.0f);

  corners[4] = MyGL_vec4(-1.0f, -1.0f, +1.0f, +1.0f);
  corners[5] = MyGL_vec4(+1.0f, -1.0f, +1.0f, +1.0f);
  corners[6] = MyGL_vec4(+1.0f, +1.0f, +1.0f, +1.0f);
  corners[7] = MyGL_vec4(-1.0f, +1.0f, +1.0f, +1.0f);

  mygl->W_matrix = MyGL_mat4Scale(p, s.x, s.y, s.z);

  MyGL_VertexAttributeStream vs = MyGL_vertexAttributeStream("Position");
  MyGL_VertexAttributeStream cs = MyGL_vertexAttributeStream("Color");

  vs.arr.vec4s[0] = corners[0];
  vs.arr.vec4s[1] = corners[1];
  vs.arr.vec4s[2] = corners[1];
  vs.arr.vec4s[3] = corners[2];
  vs.arr.vec4s[4] = corners[2];
  vs.arr.vec4s[5] = corners[3];
  vs.arr.vec4s[6] = corners[3];
  vs.arr.vec4s[7] = corners[0];

  vs.arr.vec4s[8] = corners[4];
  vs.arr.vec4s[9] = corners[5];
  vs.arr.vec4s[10] = corners[5];
  vs.arr.vec4s[11] = corners[6];
  vs.arr.vec4s[12] = corners[6];
  vs.arr.vec4s[13] = corners[7];
  vs.arr.vec4s[14] = corners[7];
  vs.arr.vec4s[15] = corners[4];

  vs.arr.vec4s[16] = corners[0];
  vs.arr.vec4s[17] = corners[4];
  vs.arr.vec4s[18] = corners[1];
  vs.arr.vec4s[19] = corners[5];
  vs.arr.vec4s[20] = corners[2];
  vs.arr.vec4s[21] = corners[6];
  vs.arr.vec4s[22] = corners[3];
  vs.arr.vec4s[23] = corners[7];

  for (int i = 0; i < 24; i++)
    cs.arr.vec4s[i] = green ? MyGL_vec4(0.0f, 1.0f, 0.0f, 1.0f) : MyGL_vec4(0.0f, 0.0f, 0.5f, 1.0f);

  mygl->primitive = MYGL_LINES;
  mygl->numPrimitives = 12;
  MyGL_drawStreaming("Position, Color");
}

void draw() {
  resetMyGL();
  MyGL_clear(GL_TRUE, GL_TRUE, GL_TRUE);

  mygl->material = MyGL_str64("Vertex Position and Color");
  mygl->W_matrix = MyGL_mat4Identity;
//  mygl->V_matrix = MyGL_mat4Identity;
//  mygl->P_matrix = MyGL_mat4Perspective((float) DISP_W / (float) DISP_H, 75.0f * 3.14159265f / 180.0f, 0.01f, 1000.0f);
  mygl->V_matrix = cam.viewMatrix();
  mygl->P_matrix = cam.projectionMatrix(float(DISP_W) / float(DISP_H));

  if (wireFrame)
    drawTris();
//  std::vector<Aabb> boxes;
//  grid->getBoxes(boxes);
//  for (auto box : boxes) {
//    drawBox(MyGL_vec3(box.p.x, box.p.y, box.p.z), MyGL_vec3(box.halfSize.x, box.halfSize.y, box.halfSize.z), box.collidesWith(tri));
//  }

  drawLine(rayPoints[0], rayPoints[1], MyGL_vec4(1.0f, 0.0f, 0.0f, 1.0f));
  for (auto &lrc : traceCells) {
    Aabb box = grid->cellBox(lrc[0], lrc[1], lrc[2]);
    drawBox(MyGL_vec3(box.p.x, box.p.y, box.p.z), MyGL_vec3(box.halfSize.x, box.halfSize.y, box.halfSize.z), false);
  }

  drawBox(rayPoints[1], MyGL_vec3(0.1f, 0.1f, 0.1f), true);

  if (!wireFrame) {
    mygl->material = MyGL_str64("Vertex Position and Texture");
    for (auto &e : drawElements)
      e.draw(*mygl);
  }
}

void term() {
  printf("*** TERM ***\n");
  solver->join();
  solver->save();
  float recycle_rate = 100.0f * (float) myHeap->recycles / (float) (myHeap->reservations + myHeap->recycles);
  LOGINFO(__FUNCTION__, "reservations v. recycles: %d v %d (recycle rate ~%.2f%%)", myHeap->reservations, myHeap->recycles, recycle_rate);
  LOGINFO(__FUNCTION__, "%f out of %f mbs reserved for use", ((float ) myHeap->total() - myHeap->remaining()) / (1024.0f * 1024.0f), (float ) myHeap->total() / (1024.0f * 1024.0f));

  printf("************\n");
}

int main(int argc, char *args[]) {
  setbuf( stdout, NULL);
  if (!sdl.init( DISP_W, DISP_H, false, "EMBLAZE Lighting Demo", true))
    return 0;

  setbuf( stdout, NULL);

  double drawTimeInSecs = 0.0;
  Uint32 drawCount = 0.0;
  double invFreq = 1.0 / sdl.getPerfFreq();

  auto start = sdl.getTicks();
  auto last = start;
  init();
  while (true) {
    auto now = sdl.getTicks();
    auto elapsed = now - last;

    if (elapsed >= DT_MS) {
      sdl.pump();
      if (sdl.keyDown(SDLK_ESCAPE))
        break;

      //printf("%d -> %d (%d)\n", last, now, elapsed);
      step();  // Running at 60fps because of swap()
      last = (now / DT_MS) * DT_MS;
      if (lockMouse) {
        SDL_ShowCursor(SDL_FALSE);
        sdl.warpMouse(DISP_W / 2, DISP_H / 2);
        mouseLock = mouseLock == MouseLock::Locked ? MouseLock::Locking : MouseLock::Locked;

      } else {
        mouseLock = MouseLock::Unlocked;
        SDL_ShowCursor(SDL_TRUE);
      }
    }

    Uint64 beginCount = sdl.getPerfCounter();
    draw();
    drawCount++;
    sdl.swap();
    drawTimeInSecs += (double) (sdl.getPerfCounter() - beginCount) * invFreq;
  }
  term();
  if (drawCount)
    printf("Average draw time: %f ms\n", (float) ((drawTimeInSecs * 1e3) / (double) drawCount));

  sdl.term();
  printf("goodbye!\n");
  return 0;
}

