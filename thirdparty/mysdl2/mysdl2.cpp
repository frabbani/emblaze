#include "mysdl2.h"

#include <algorithm>
#include <x86intrin.h>
#include <cmath>

using namespace sdl2;

typedef __v4sf vec4;
typedef __v4si ivec4;
typedef __v4su uvec4;

static inline vec4 toVec4(const Pixel24 &pixel, uint8_t a = 255) {
  vec4 v;
  v[0] = float(pixel.r);
  v[1] = float(pixel.g);
  v[2] = float(pixel.b);
  v[3] = float(a);
  return v;
}

static inline vec4 toVec4(const Pixel32 &pixel) {
  vec4 v;
  v[0] = float(pixel.r);
  v[1] = float(pixel.g);
  v[2] = float(pixel.b);
  v[2] = float(pixel.a);
  return v;
}

vec4 alphaBlend(const vec4 &src, const vec4 &dst) {
  vec4 _one = { 1.0, 1.0, 1.0, 1.0 };
  vec4 alpha = { src[3], src[3], src[3], src[3] };
  return src * alpha + dst * (_one - alpha) * dst;
}

Pixels::Coord Pixels::Coord::lerp(const Coord &next, float alpha) const {
  alpha = alpha < 0.0f ? 0.0f : alpha > 1.0f ? 1.0f : alpha;
  float dx = (float) (next.x - x) * alpha;
  float dy = (float) (next.y - y) * alpha;
  return Coord(x + (int) dx, y + (int) dy);
}

void Pixels::plot(int x, int y, const Pixel32 &pixel) {
  if (x < 0 || x >= w || y < 0 || y >= h)
    return;
  y = inverted ? h - 1 - y : y;
  if (bpp == 24) {
    Pixel24 *pixelOut = (Pixel24*) &data[y * p + (x * 3)];
    pixelOut->r = pixel.r;
    pixelOut->g = pixel.g;
    pixelOut->b = pixel.b;
  } else if (bpp == 32) {
    Pixel32 *pixelOut = (Pixel32*) &data[y * p + (x * 4)];
    *pixelOut = pixel;
  }
}

void Pixels::plot(int x, int y, const Pixel24 &pixel) {
  if (x < 0 || x >= w || y < 0 || y >= h)
    return;
  y = inverted ? h - 1 - y : y;
  if (bpp == 24) {
    Pixel24 *pixelOut = (Pixel24*) &data[y * p + (x * 3)];
    pixelOut->r = pixel.r;
    pixelOut->g = pixel.g;
    pixelOut->b = pixel.b;
  } else if (bpp == 32) {
    Pixel32 *pixelOut = (Pixel32*) &data[y * p + (x * 4)];
    pixelOut->r = pixel.r;
    pixelOut->g = pixel.g;
    pixelOut->b = pixel.b;
  }
}
Pixel32* Pixels::get32(int x, int y) {
  if (!data || bpp != 32)
    return nullptr;
  if (x < 0 || x >= w || y < 0 || y >= h)
    return nullptr;
  return (Pixel32*) &data[y * p + (x * 4)];
}

Pixel24* Pixels::get24(int x, int y) {
  if (!data || bpp != 24)
    return nullptr;
  if (x < 0 || x >= w || y < 0 || y >= h)
    return nullptr;
  return (Pixel24*) &data[y * p + (x * 3)];
}

void Pixels::clear(const Pixel32 &color) {
  if (bpp == 32) {
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++) {
        Pixel32 *pixel = (Pixel32*) &data[y * p + (x * 4)];
        *pixel = color;
      }
  }
  if (bpp == 24) {
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++) {
        Pixel24 *pixel = (Pixel24*) &data[y * p + (x * 3)];
        pixel->r = color.r;
        pixel->g = color.g;
        pixel->b = color.b;
      }
  }
}

void Pixels::clear(const Pixel24 &color) {
  if (bpp == 32) {
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++) {
        Pixel32 *pixel = (Pixel32*) &data[y * p + (x * 4)];
        *pixel = color;
      }
  }
  if (bpp == 24) {
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++) {
        Pixel24 *pixel = (Pixel24*) &data[y * p + (x * 3)];
        pixel->r = color.r;
        pixel->g = color.g;
        pixel->b = color.b;
      }
  }
}

Pixel32 Pixels::sample(float u, float v, bool clamped) {
  if (!data || !(bpp == 24 || bpp == 32))
    return Pixel32(0, 0, 0, 0);

  const vec4 _1 = { 1.0f, 1.0f, 1.0f, 1.0f };
  const vec4 _255 = { 255.0f, 255.0f, 255.0f, 255.0f };

  u *= float(w - 1);
  v *= float(h - 1);

  int x0, x1, y0, y1;
  if (clamped) {
    u = std::clamp(u, 0.0f, float(w - 1));
    v = std::clamp(v, 0.0f, float(h - 1));
    x0 = int(u);
    y0 = int(v);
    u = u - float(x0);
    v = v - float(y0);
    x1 = x0 + 1 >= w ? w - 1 : x0 + 1;
    y1 = y0 + 1 >= h ? h - 1 : y0 + 1;
  } else {
    x0 = int(u);
    y0 = int(v);
    u = fabsf(u - float(x0));
    v = fabsf(v - float(y0));

    if (x0 < 0 && u > 0.0f)
      x0--;
    if (y0 < 0 && v > 0.0f)
      y0--;

    x0 = x0 % w;
    if (x0 < 0)
      x0 += w;
    y0 = y0 % h;
    if (y0 < 0)
      y0 += h;
    x1 = x0 + 1 >= w ? 0 : x0 + 1;
    y1 = y0 + 1 >= h ? 0 : y0 + 1;
  }

  if (inverted) {
    y0 = h - y0 - 1;
    y1 = h - y1 - 1;
  }

  vec4 values[2][2], value0, value1, result;

  if (24 == bpp) {
    values[0][0] = toVec4(*((Pixel24*) &data[y0 * p + (x0 * 3)]));
    values[0][1] = toVec4(*((Pixel24*) &data[y0 * p + (x1 * 3)]));
    values[1][0] = toVec4(*((Pixel24*) &data[y1 * p + (x0 * 3)]));
    values[1][1] = toVec4(*((Pixel24*) &data[y1 * p + (x1 * 3)]));
  } else {
    values[0][0] = toVec4(*((Pixel32*) &data[y0 * p + (x0 * 4)]));
    values[0][1] = toVec4(*((Pixel32*) &data[y0 * p + (x1 * 4)]));
    values[1][0] = toVec4(*((Pixel32*) &data[y1 * p + (x0 * 4)]));
    values[1][1] = toVec4(*((Pixel32*) &data[y1 * p + (x1 * 4)]));
  }

  vec4 s = { u, u, u, u };
  vec4 t = { v, v, v, v };

  value0 = (_1 - s) * values[0][0] + s * values[0][1];
  value1 = (_1 - s) * values[1][0] + s * values[1][1];
  result = (_1 - t) * value0 + t * value1;
  result = result > _255 ? _255 : result;

  Pixel32 out;
  out.r = (uint8_t) result[0];
  out.g = (uint8_t) result[1];
  out.b = (uint8_t) result[2];
  out.a = (uint8_t) result[3];

  return out;
}

void Pixels::flip() {
  if (!data || h == 1)
    return;
  std::vector<uint8_t> bytes;
  int numBytes = w * bpp / 8;
  bytes.reserve(numBytes);
  for (int y = 0; y < h / 2; y++) {
    int y2 = h - y - 1;
    memcpy(bytes.data(), &data[y2 * p], numBytes);
    memcpy(&data[y2 * p], &data[y * p], numBytes);
    memcpy(&data[y * p], bytes.data(), numBytes);
  }

}

Bitmap::Bitmap(int w, int h, int d, std::string name) {
  surf = SDL_CreateRGBSurface(0, w, h, d, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
  if (surf)
    source = std::move(name);
}

Bitmap::Bitmap(const char *bmpFile) {
  surf = SDL_LoadBMP(bmpFile);
  if (surf)
    source = bmpFile;
}

Bitmap::Bitmap(const std::vector<Uint8> &bmpData, std::string name) {
  SDL_RWops *src = SDL_RWFromConstMem(bmpData.data(), bmpData.size());
  if (!src) {
    printf("Bitmap::Bitmap - error: '%s'\n", SDL_GetError());
  }
  surf = SDL_LoadBMP_RW(src, 0);
  if (!surf->w || !surf->h) {
    printf("Bitmap::Bitmap - surface error: '%s'\n", SDL_GetError());
  }
  if (surf)
    source = std::move(name);
}

void Bitmap::save(std::string_view bmpFile) {
  if (surf && bmpFile.data())
    SDL_SaveBMP(surf, bmpFile.data());
}

void Bitmap::savePNG(std::string_view pngFile) {
  if (surf && pngFile.data())
    IMG_SavePNG(surf, pngFile.data());
}

int Bitmap::width() {
  if (surf)
    return surf->w;
  return 0;
}

int Bitmap::height() {
  if (surf)
    return surf->h;
  return 0;
}

int Bitmap::depth() {
  if (surf)
    return surf->format->BitsPerPixel;
  return 0;
}

bool Bitmap::locked() {
  return surf && SDL_MUSTLOCK(surf) && surf->locked;
}

void Bitmap::lock() {
  if (!surf)
    return;

  if ( SDL_MUSTLOCK(surf) && !surf->locked)
    SDL_LockSurface(surf);
  pixels.data = (Uint8*) surf->pixels;
  pixels.bpp = surf->format->BitsPerPixel;
  ;
  pixels.w = surf->w;
  pixels.h = surf->h;
  pixels.p = surf->pitch;
}

void Bitmap::unlock() {
  if (!surf)
    return;

  if ( SDL_MUSTLOCK(surf) && surf->locked) {
    SDL_UnlockSurface(surf);
  }
  pixels.data = nullptr;
  pixels.w = pixels.h = pixels.p = 0;
}

void Bitmap::blit(Bitmap &dstBmp, const Rect *srcRect, Rect *dstRect) {
  if (surf && dstBmp.surf)
    SDL_BlitSurface(surf, static_cast<const SDL_Rect*>(srcRect), dstBmp.surf, static_cast<SDL_Rect*>(dstRect));
}

Bitmap::~Bitmap() {
  unlock();
  if (surf) {
    SDL_FreeSurface(surf);
    surf = nullptr;
  }
}

Font::Font(std::string_view path, int size) {
  font = TTF_OpenFont(path.data(), size);
  for (char i = ' '; i <= '~'; i++) {
    char token[] = { i, '\0' };
    auto surf = TTF_RenderText_Solid(font, token, SDL_Color { 255, 255, 255, 255 });
    glyphs.push_back(surf);
  }
}

void Font::render(SDL_Surface *dest, int x, int y, std::string_view text, Pixel24 color, bool upsideDown) {
  if (dest->format->BytesPerPixel != 3 && dest->format->BytesPerPixel != 4)
    return;
  bool unlock = false;
  if (SDL_MUSTLOCK(dest) && !dest->locked) {
    unlock = true;
    SDL_LockSurface(dest);
  }

  Uint8 *destData = reinterpret_cast<Uint8*>(dest->pixels);
  int bypp = dest->format->BytesPerPixel;
  int pitch = dest->pitch;
  int xPos = x;
  int yPos = y;

  for (auto c : text) {
    auto glyph = glyphs[c - ' '];
    if (!glyph)
      continue;
    Uint8 *glpyhData = reinterpret_cast<Uint8*>(glyph->pixels);

    for (int y = 0; y < glyph->h; y++) {
      int yPlot = yPos + y;
      if (yPlot < 0 || yPlot >= dest->h)
        continue;
      for (int x = 0; x < glyph->w; x++) {
        int xPlot = xPos + x;
        if (xPlot < 0 || xPlot >= dest->w)
          continue;;
        if (glpyhData[(upsideDown ? glyph->h - y - 1 : y) * glyph->pitch + x]) {
          Pixel24 *destPixel = reinterpret_cast<Pixel24*>(&destData[yPlot * pitch + xPlot * bypp]);
          *destPixel = color;
        }
      }
    }
    xPos += glyph->w;
  }
  if (unlock)
    SDL_UnlockSurface(dest);
}

void Font::render(Pixels &dest, int x, int y, std::string_view text, Pixel24 color) {
  if (dest.bpp != 24 && dest.bpp != 32)
    return;

  int xPos = x;
  int yPos = y;

  for (auto c : text) {
    auto glyph = glyphs[c - ' '];
    if (!glyph)
      continue;
    Uint8 *glpyhData = reinterpret_cast<Uint8*>(glyph->pixels);

    if (dest.inverted) {
      for (int y = 0; y < glyph->h; y++) {
        for (int x = 0; x < glyph->w; x++) {
          Uint8 mask = glpyhData[(glyph->h - y - 1) * glyph->pitch + x];
          if (mask)
            dest.plot(xPos + x, yPos + y, color);
        }
      }
    } else {
      for (int y = 0; y < glyph->h; y++) {
        for (int x = 0; x < glyph->w; x++) {
          Uint8 mask = glpyhData[y * glyph->pitch + x];
          if (mask)
            dest.plot(xPos + x, yPos + y, color);
        }
      }
    }
    xPos += glyph->w;
  }
}

Font::~Font() {
  for (auto surf : glyphs)
    SDL_FreeSurface(surf);
  if (font) {
    TTF_CloseFont(font);
    font = nullptr;
  }
}

bool FontAtlas::create(const Font &font) {
  if (!font.font || font.glyphs.empty())
    return false;

  const size_t maxGlyphs = (size_t) '~' - (size_t) ' ' + 1;

  if (glyphAtlas)
    SDL_FreeSurface(glyphAtlas);
  glyphRects.clear();

  int maxW = 0;
  int maxH = 0;
  for (auto g : font.glyphs) {
    maxW = std::max(maxW, g->w);
    maxH = std::max(maxH, g->h);
  }
  if (maxW & 0x01)
    maxW++;
  if (maxH & 0x01)
    maxH++;

  glyphAtlas = SDL_CreateRGBSurface(0, maxW, maxH * maxGlyphs, 8, 0, 0, 0, 0);
  if (!glyphAtlas)
    return false;

  int i = 0;
  for (auto g : font.glyphs) {
    int xOffset = (maxW - g->w) / 2;
    int yOffset = (maxH - g->h) / 2;
    Rect rect;
    rect.x = xOffset;
    rect.y = yOffset + i * maxH;
    rect.w = g->w;
    rect.h = g->h;
    glyphRects.push_back(rect);
    SDL_BlitSurface(g, nullptr, glyphAtlas, &rect);
    i++;
  }

  return !glyphRects.empty();
}

bool SDL::init(Uint32 w, Uint32 h, bool borderless, std::string_view title, bool withOpenGL) {
  if (SDL_Init( SDL_INIT_VIDEO) < 0) {
    printf("could not initialize SDL: %s\n", SDL_GetError());
    return false;
  } else
    printf("SDL initialized\n");

  inited = true;

  Uint32 flags = SDL_WINDOW_SHOWN | (borderless ? SDL_WINDOW_BORDERLESS : 0);
  if (withOpenGL)
    flags |= SDL_WINDOW_OPENGL;
  win = SDL_CreateWindow(title.data(),
  SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | (borderless ? SDL_WINDOW_BORDERLESS : 0));
  if (!win) {
    printf("could not create window: %s\n", SDL_GetError());
    return false;
  }
  if (withOpenGL)
    ctx = SDL_GL_CreateContext(win);

  surf = SDL_GetWindowSurface(win);
  printf("surface format: \n");
  printf(" * w x h.: %d x %d\n", surf->w, surf->h);
  printf(" * pitch.: %d\n", surf->pitch);
  printf(" * bpp...: %d\n", surf->format->BitsPerPixel);
  printf(" * format: %s\n\n", SDL_GetPixelFormatName(surf->format->format));

  keys = SDL_GetKeyboardState(&numKeys);
  for (int i = 0; i < SDL_NUM_SCANCODES; i++)
    keyCounters[i] = 0;

  startCounter = getPerfCounter();
  return true;
}

void SDL::pump() {
  SDL_PumpEvents();
  for (int i = 0; i < SDL_NUM_SCANCODES; i++)
    keyCounters[i] = keys[i] ? keyCounters[i] + 1 : 0;
  Uint32 mask = SDL_GetMouseState(&mouseX, &mouseY);
  for (int i = 0; i < 8; i++) {
    Uint32 bitMask = 1 << i;
    if (mask & bitMask)
      mouseCounters[i]++;
    else
      mouseCounters[i] = 0;
  }
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
  }
}

Uint32 SDL::getTicks() {
  return SDL_GetTicks();
}

Uint64 SDL::getPerfCounter() {
  return SDL_GetPerformanceCounter();
}

double SDL::getPerfFreq() {
  return (double) SDL_GetPerformanceFrequency();
}

void SDL::term() {

  if (win) {
    SDL_DestroyWindow(win);
    win = nullptr;
  }
  if (inited) {
    SDL_Quit();
    inited = false;
  }
}

Pixels SDL::lock() {
  Pixels pixels;
  if ( SDL_MUSTLOCK(surf) && !surf->locked)
    SDL_LockSurface(surf);

  pixels.data = (Uint8*) surf->pixels;
  pixels.bpp = surf->format->BytesPerPixel == 3 ? 24 : surf->format->BytesPerPixel == 4 ? 32 : 0;
  pixels.w = surf->w;
  pixels.h = surf->h;
  pixels.p = surf->pitch;
  return pixels;
}
void SDL::swap() {
  if (ctx) {
    SDL_GL_SwapWindow(win);
  } else {
    if ( SDL_MUSTLOCK(surf) && surf->locked)
      SDL_UnlockSurface(surf);

    SDL_UpdateWindowSurface(win);
  }
}

bool SDL::keyDown(Sint32 key) {
  return keys[SDL_GetScancodeFromKey(key)] > 0;
}

bool SDL::keyPress(Sint32 key) {
  return keyCounters[SDL_GetScancodeFromKey(key)] == 1;
}

bool SDL::mouseKeyDown(Uint8 key) {
  return mouseCounters[key & 0x07] > 0;
}
bool SDL::mouseKeyPress(Uint8 key) {
  return mouseCounters[key & 0x07] == 1;
}

void SDL::takeScreenshot() {
  std::string fileName = "screenshot" + std::to_string(screenshot) + ".png";
  IMG_SavePNG(surf, fileName.c_str());
  screenshot++;
}

void SDL::warpMouse(Sint32 x, Sint32 y) {
  SDL_WarpMouseInWindow(win, x, y);
}
