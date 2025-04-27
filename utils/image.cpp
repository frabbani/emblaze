#include <cstdio>
#include <cstring>
#include <cmath>
#include "image.h"
#include "log.h"

#include "../thirdparty/lodepng/lodepng.h"

namespace mbz {
namespace utils {
namespace img {

enum Compression {
  RGB = 0,
  RLE8,
  RLE4,
  BITFIELDS,
  JPEG,
  PNG,
};

#pragma pack(push, 1)

//2 bytes
struct FileMagic {
  uint8_t num0, num1;
};

//12 bytes
struct FileHeader {
  uint32_t fileSize;
  uint16_t creators[2];
  uint32_t dataOffset;
};

//40 bytes, all windows versions since 3.0
struct DibHeader {
  uint32_t headerSize;
  int32_t width, height;
  uint16_t numPlanes, bitsPerPixel;
  uint32_t compression;
  uint32_t dataSize;
  int32_t hPixelsPer, vPixelsPer;  //horizontal and vertical pixels-per-meter
  uint32_t numPalColors, numImportantColors;
};

#pragma pack(pop)

Color Image::sample(float x, float y, bool clamp) const {

  float xStart = floorf(x);
  float xStop = ceilf(x);

  float yStart = floorf(y);
  float yStop = ceilf(y);

  float u = x - xStart;
  float v = y - yStart;

  Color colors[2][2];
  colors[0][0] = get(xStart, yStart, clamp);
  colors[0][1] = get(xStop, yStart, clamp);
  colors[1][0] = get(xStart, yStop, clamp);
  colors[1][1] = get(xStop, yStop, clamp);

  auto color = colors[0][0].lerp(colors[0][1], u);
  auto color2 = colors[1][0].lerp(colors[1][1], u);

  return color.lerp(color2, v);

}

Color Image::sampleBox(int x, int y, int w, int h, bool clamp) const {
  uint32_t r = 0, g = 0, b = 0;
  for (int i = 0; i < h; i++)
    for (int j = 0; j < w; j++) {
      auto c = get(x + j, y + i, clamp);
      r += c.r;
      g += c.g;
      b += c.b;
    }
  float s = 1.0f / float(w * h);
  r = (uint32_t(float(r) * s) & 0xff);
  g = (uint32_t(float(g) * s) & 0xff);
  b = (uint32_t(float(b) * s) & 0xff);
  return Color(r, g, b);
}

Color Image::sampleMipmap(float x, float y, int level, bool clamp) const {
  if (level <= 0 || !mip)
    return sample(x, y, clamp);
  const Image *ptr = this;
  for (int i = 1; i <= level; i++) {
    if (!ptr->mip)
      break;
    ptr = ptr->mip.get();
  }
  return ptr->sample(x, y, clamp);
}

void Image::createMips() {
  if (w < 4 || h < 4)
    return;
  if ((w & 0x01) || (h & 0x01))
    return;

  mip = std::make_unique<Image>();
  mip->w = w >> 1;
  mip->h = h >> 1;
  mip->pixels = std::vector<Color>(mip->w * mip->h);
  for (int y = 0; y < mip->h; y++)
    for (int x = 0; x < mip->w; x++) {
      mip->pixels[y * mip->w + x] = sampleBox(x * 2, y * 2, 2, 2);
    }
  mip->createMips();
}

void Image::fromBMP(std::vector<uint8_t> rawData, std::string_view source) {
  const char *tag = "Image::fromBMP()";
  if (!rawData.size())
    return;

  size_t streamPos = 0;

  const FileMagic *magic = (const FileMagic*) &rawData[streamPos];
  if (magic->num0 != 'B' && magic->num1 != 'M') {
    LOGERROR(tag, "'%s' is not a valid bitmap image", source.data());
    return;
  }
  streamPos += sizeof(FileMagic);

  const FileHeader *header = (const FileHeader*) &rawData[streamPos];
  streamPos += sizeof(FileHeader);

  const DibHeader *dib = (const DibHeader*) &rawData[streamPos];
  if (dib->compression != Compression::RGB || dib->bitsPerPixel != 24) {
    LOGERROR(tag, "'%s' is not a 24 bit bitmap image", source.data());
  }

  w = dib->width;
  h = dib->height;
  pixels = std::vector<Color>(w * h);

  int bypp = dib->bitsPerPixel / 8;
  int rem = 0;
  if ((w * bypp) & 0x03) {
    rem = 4 - ((w * bypp) & 0x03);
  }

  streamPos = header->dataOffset;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      pixels[y * w + x].b = rawData[streamPos++];
      pixels[y * w + x].g = rawData[streamPos++];
      pixels[y * w + x].r = rawData[streamPos++];
    }
    for (int i = 0; i < rem; i++)
      streamPos++;
  }
  this->source = std::string(source);
  LOGINFO(tag, "image dimensions: %d x %d\n", w, h);

}

Image::Image(const Image &from) {
  w = from.w;
  h = from.h;
  pixels = from.pixels;
  if (from.mip) {
    mip = std::make_unique<Image>(*from.mip);
  }
}

int computeMipmapLevel(float sourceArea, float targetArea) {
  // the area of the next level is a quarter of the size of the original
  // to solve for level L, solve 4^L = sourceArea/targetArea
  // since 4 = 2^2, we can use base 2

  double logRatio = log2(sourceArea / targetArea);
  return std::max(int(round(logRatio / 2)), 0);
}

bool writeImageToBMPFile(const Image &image, std::string_view name) {
  char file[256];

  if (!image.w || !image.h || !image.pixels.size()) {
    return false;
  }

  strcpy(file, name.data());
  int ext = 0;
  for (const char *p = name.data(); *p != '\0'; p++) {
    if ('.' == p[0] && 'b' == p[1] && 'm' == p[2] && 'p' == p[3] && '\0' == p[4]) {
      ext++;
      break;
    }
  }
  if (!ext)
    strcat(file, ".bmp");

  FILE *fp = fopen(file, "wb");

  FileMagic magic;
  magic.num0 = 'B';
  magic.num1 = 'M';
  fwrite(&magic, 2, 1, fp);  //hard coding 2 bytes, (our structure isn't packed).

  FileHeader fileHeader;
  fileHeader.fileSize = image.w * image.h * sizeof(int) + 54;
  fileHeader.creators[0] = fileHeader.creators[1] = 0;
  fileHeader.dataOffset = 54;
  fwrite(&fileHeader, sizeof(fileHeader), 1, fp);

  DibHeader dibHeader;
  dibHeader.headerSize = 40;
  dibHeader.width = image.w;
  dibHeader.height = image.h;
  dibHeader.numPlanes = 1;
  dibHeader.bitsPerPixel = 24;
  dibHeader.compression = Compression::RGB;
  dibHeader.dataSize = 0;
  dibHeader.hPixelsPer = dibHeader.vPixelsPer = 1000;
  dibHeader.numPalColors = dibHeader.numImportantColors = 0;
  fwrite(&dibHeader, sizeof(DibHeader), 1, fp);

  int rem = 0;
  if ((image.w * 3) & 0x03)
    rem = 4 - ((image.w * 3) & 0x03);

  for (int y = 0; y < image.h; y++) {
    for (int x = 0; x < image.w; x++) {
      Color pixel = image.pixels[y * image.w + x];
      fputc(pixel.b, fp);
      fputc(pixel.g, fp);
      fputc(pixel.r, fp);
    }
    for (int i = 0; i < rem; i++)
      fputc(0xff, fp);
  }

  fclose(fp);
  LOGINFO("writeImageToBMPFile", "image exported to '%s", file);

  return true;
}

bool writeImageToPNGFile(const Image &image, std::string_view name) {
  char file[256];
  strcpy(file, name.data());
  int ext = 0;
  for (const char *p = name.data(); *p != '\0'; p++) {
    if ('.' == p[0] && 'p' == p[1] && 'n' == p[2] && 'g' == p[3] && '\0' == p[4]) {
      ext++;
      break;
    }
  }
  if (!ext)
    strcat(file, ".png");
  std::vector<uint8_t> data;
  data.resize(image.w * image.h * 4);
  int index = 0;
  for (int y = image.h - 1; y >= 0; y--)
    for (int x = 0; x < image.w; x++) {
      data[index++] = image.pixels[y * image.w + x].r;
      data[index++] = image.pixels[y * image.w + x].g;
      data[index++] = image.pixels[y * image.w + x].b;
      data[index++] = 255;
    }

  std::vector<unsigned char> png;
  unsigned error = lodepng::encode(png, data.data(), image.w, image.h);
  if (error) {
    LOGERROR("writeImageToPNGFile", "failed to encode image '%s' - %s", file, lodepng_error_text(error));
    return false;
  }
  lodepng::save_file(png, file);
  LOGINFO("writeImageToPNGFile", "image exported to '%s", file);

  return true;
}

}
}
}

