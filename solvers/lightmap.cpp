#include "lightmap.h"

#include "../utils/image.h"

namespace mbz{
namespace lightmap{

void Lightmap::exportPNGs(){
  utils::img::Image image;
  image.w = int(canvas.w);
  image.h = int(canvas.h);
  image.pixels.reserve(image.w * image.h);

  auto &maskLayer = getLayer<0>();
  auto &positionLayer = getLayer<1>();
  auto &normalLayer = getLayer<2>();
  auto &albedoLayer = getLayer<3>();

  image.pixels.clear();
  for (int i = 0; i < maskLayer.size; i++) {
    uint8_t mask = maskLayer[i].v > 0.0f ? 255 : 0;
    image.pixels.push_back(Color(mask, mask, mask));
  }
  utils::img::writeImageToPNGFile(image, "mask");

  /*
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
  */

  image.pixels.clear();
  for (int i = 0; i < normalLayer.size; i++) {
    Vector3 normal = normalLayer[i].v;
    Vector3 c = 255.0f * (0.5f * normal + Vector3(0.5f, 0.5f, 0.5f));
    image.pixels.push_back(Color(c.x, c.y, c.z));
  }
  utils::img::writeImageToPNGFile(image, "normal");

  image.pixels.clear();
  for (int i = 0; i < albedoLayer.size; i++) {
    rasterizer::Texel texel = albedoLayer[i].v;
    image.pixels.push_back(texel.sample());
  }
  utils::img::writeImageToPNGFile(image, "albedo");
}

}
}
