#include "texture.h"
#include "../utils/log.h"

#include <cstdlib>
#include <cstdio>
#include <map>
#include <algorithm>
#include <memory>

using namespace mbz::utils;
using namespace mbz::math;

namespace mbz{
namespace rasterizer{

static std::map<int, std::shared_ptr<img::Image>> images;
static std::map<int, std::string> imageTags;

bool loadTexture(const img::Image &image, std::string_view tag, int &handle) {
  static int uniqueId = 0;
  handle = -1;
  if (!image.isValid()) {
    LOGERROR(__FUNCTION__, "invalid image");
    return false;
  }
  auto newImage = std::make_shared<img::Image>(image);
  handle = getTextureHandle(tag);
  if(handle == -1)
    handle = ++uniqueId;

  images[handle] = std::move(newImage);
  imageTags[handle] = tag;
  return true;
}

Color sampleTexture(int handle, Vector2 uv, int mipLevel) {
  const auto it = images.find(handle);
  if (it == images.end())
    return Color();

  auto image = it->second;

  float xScale = float(image->w) - 1.0f;
  float yScale = float(image->h) - 1.0f;

  float xOffset = 0.5f / float(image->w);
  float yOffset = 0.5f / float(image->h);

  uv.x = (uv.x - xOffset) * xScale;
  uv.y = (uv.y - yOffset) * yScale;
  return image->sampleMipmap(uv.x, uv.y, mipLevel, false);
}

int getTextureHandle(std::string_view tag){
  for(auto& [k,v] : imageTags)
    if(tag == v)
      return k;
  return -1;
}

Color Texel::sample() const {
  return sampleTexture(textureHandle, uv, mipLevel);
}

}
}

