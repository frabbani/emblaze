#include "builder.h"
#include "../thirdparty/openfbx/ofbx.h"
#include "../thirdparty/lodepng/lodepng.h"
#include "../utils/file.h"
#include "../utils/log.h"

namespace mbz {
namespace lightmap {

bool LightmapBuilder::buildFromFBX(std::string_view fbxName, float cellScale) {
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

  lightmap->textures.clear();
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
    lightmap->textures.push_back(-1);
    rasterizer::loadTexture(image, name, lightmap->textures.back());
  }

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

  Vector3 minExt, maxExt;
  minExt = maxExt = vertices.kp()[0].p;
  for (int i = 0; i < vertices.size; i++) {
    for (int j = 0; j < 3; j++) {
      minExt.xyz[j] = std::min(minExt.xyz[j], vertices.kp()[i].p.xyz[j]);
      maxExt.xyz[j] = std::max(maxExt.xyz[j], vertices.kp()[i].p.xyz[j]);
    }
  }
  Vector3 cellSize = cellScale * (maxExt - minExt);
  float length = (cellSize.x + cellSize.y + cellSize.z) / 3.0f;
  LOGINFO("LightmapBuilder::buildFromFBX", "min/max: {%f,%f,%f} / {%f,%f,%f}", minExt.x, minExt.y, minExt.z, maxExt.x, maxExt.y, maxExt.z);
  LOGINFO("LightmapBuilder::buildFromFBX", "cell size: {%f,%f,%f}", length, length, length);

  grid = std::make_shared<math::bpcd::Grid>(heap);
  std::vector<std::array<Vector3, 3>> tris;

  for (int i = 0; i < triangles.size; i++) {
    Vector3 vs[3];
    for (int j = 0; j < 3; j++)
      vs[j] = vertices.kp()[triangles.kp()[i][j]].p;
    tris.push_back( { vs[0], vs[1], vs[2] });
  }

  grid->build(tris, Vector3(length, length, length));

  Lightmap::Tri ltri = lightmap->getTri();

  auto tri_area = [](Vector2 p, Vector2 p2, Vector2 p3) {
    Vector2 u = p.point(p2);
    Vector2 v = p.point(p3);
    return 0.5f * (u.x * v.y - v.x * u.y);
  };

  math::Matrix2 M;
  M.identity();
  M.e00 = float(lightmap->canvas.w - 1);
  M.e11 = float(lightmap->canvas.h - 1);
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
    int texHandle = lightmap->textures[tri[3]];
    int level = utils::img::computeMipmapLevel(tri_area(t1, t2, t3), tri_area(s1, s2, s3)) - 1;

    ltri.getPoint(0).p = M * s1;
    ltri.plot<0>(0).v = id;
    ltri.plot<1>(0).v = p1;
    ltri.plot<2>(0).v = n1;
    ltri.plot<3>(0).set(t1, texHandle, level);

    ltri.getPoint(1).p = M * s2;
    ltri.plot<0>(1).v = id;
    ltri.plot<1>(1).v = p2;
    ltri.plot<2>(1).v = n2;
    ltri.plot<3>(1).set(t2, texHandle, level);

    ltri.getPoint(2).p = M * s3;
    ltri.plot<0>(2).v = id;
    ltri.plot<1>(2).v = p3;
    ltri.plot<2>(2).v = n3;
    ltri.plot<3>(2).set(t3, texHandle, level);
    ltri.render();
  }

  /*
  utils::img::Image image;
  image.w = int(lightmap->canvas.w);
  image.h = int(lightmap->canvas.h);
  image.pixels.reserve(image.w * image.h);

  auto &maskLayer = lightmap->getLayer<0>();
  auto &positionLayer = lightmap->getLayer<1>();
  auto &normalLayer = lightmap->getLayer<2>();
  auto &albedoLayer = lightmap->getLayer<3>();

  image.pixels.clear();
  for (int i = 0; i < maskLayer.size; i++) {
    uint8_t mask = maskLayer[i].v > 0.0f ? 255 : 0;
    image.pixels.push_back(Color(mask, mask, mask));
  }
  utils::img::writeImageToBMPFile(image, "mask");

  image.pixels.clear();
  for (int i = 0; i < positionLayer.size; i++) {
    auto p = positionLayer[i].v - grid->o;
    p.x /= grid->bigBox.size().x;
    p.y /= grid->bigBox.size().y;
    p.z /= grid->bigBox.size().z;
    p = 255.0f * p;
    image.pixels.push_back(Color(p.x, p.y, p.z));
  }
  utils::img::writeImageToBMPFile(image, "position");

  image.pixels.clear();
  for (int i = 0; i < normalLayer.size; i++) {
    Vector3 normal = normalLayer[i].v;
    Vector3 c = 255.0f * (0.5f * normal + Vector3(0.5f, 0.5f, 0.5f));
    image.pixels.push_back(Color(c.x, c.y, c.z));
  }
  utils::img::writeImageToBMPFile(image, "normal");

  image.pixels.clear();
  for (int i = 0; i < albedoLayer.size; i++) {
    rasterizer::Texel texel = albedoLayer[i].v;
    image.pixels.push_back(texel.sample());
  }
  utils::img::writeImageToBMPFile(image, "albedo");
  */

  return true;
}

}
}
