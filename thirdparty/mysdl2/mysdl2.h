#pragma once

#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <vector>

namespace sdl2 {

#pragma pack( push, 1 )
struct Pixel24 {
  Uint8 b, g, r;
  Pixel24(Uint8 r = 255, Uint8 g = 255, Uint8 b = 255)
      :
      b(b),
      g(g),
      r(r) {
  }
  Pixel24& operator =(const Pixel24 &rhs) {
    memcpy(this, &rhs, sizeof(Pixel24));
    return *this;
  }
};
#pragma pack( pop )

struct Pixel32 {
  Uint8 b, g, r, a;
  Pixel32(Uint8 r = 0, Uint8 g = 0, Uint8 b = 0, Uint8 a = 0)
      :
      b(b),
      g(g),
      r(r),
      a(a) {
  }
  Pixel32(const Pixel24 &pixel, Uint8 a = 0)
      :
      b(pixel.b),
      g(pixel.g),
      r(pixel.r),
      a(a) {
  }
  Pixel32& operator =(const Pixel24 &rhs) {
    r = rhs.r;
    g = rhs.g;
    b = rhs.b;
    return *this;
  }
};

struct Pixels {
  struct Coord {
    int x, y;
    Coord(int x = 0, int y = 0)
        :
        x(x),
        y(y) {
    }
    Coord lerp(const Coord &next, float alpha) const;
  };

  Uint8 *data = nullptr;
  int w = 0, h = 0, p = 0, bpp = 0;
  bool inverted = false;

  void plot(int x, int y, const Pixel32 &pixel);
  void plot(int x, int y, const Pixel24 &pixel);

  inline void plot(int x, int y, Uint8 r, Uint8 g, Uint8 b) {
    plot(x, y, Pixel24(r, g, b));
  }
  inline void plot(Coord p, Uint8 r, Uint8 g, Uint8 b) {
    plot(p.x, p.y, Pixel24(r, g, b));
  }

  inline void plot(int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    plot(x, y, Pixel32(r, g, b, a));
  }
  inline void plot(Coord p, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    plot(p.x, p.y, Pixel32(r, g, b, a));
  }

  Pixel32 sample(float u, float v, bool clamped = true);

  bool hasData() const {
    return w > 0 && h > 0 && data != nullptr;
  }

  Pixel32* get32(int x, int y);
  Pixel24* get24(int x, int y);
  void clear(const Pixel24 &color);
  void clear(const Pixel32 &color);
  void flip();
};

struct Rect : public SDL_Rect {
  Rect(int x = 0, int y = 0, int w = 0, int h = 0) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
  }
};

struct Bitmap {
  SDL_Surface *surf = nullptr;
  std::string source;
  Pixels pixels;
  Bitmap(int w, int h, int d, std::string name = "Bitmap");
  Bitmap(const char *bmpFile);
  Bitmap(const std::vector<Uint8> &bmpData, std::string name = "Bitmap");
  ~Bitmap();
  int width();
  int height();
  int depth();
  bool locked();
  void lock();
  void unlock();
  void save(std::string_view bmpFile);
  void savePNG(std::string_view pngFile);
  void blit(Bitmap &dstBmp, const Rect *srcRect = nullptr, Rect *dstRect = nullptr);
};

struct Font {
  TTF_Font *font = nullptr;
  std::vector<SDL_Surface*> glyphs;
  Font(std::string_view path, int size);
  void render(SDL_Surface *dest, int x, int y, std::string_view text, Pixel24 color, bool upsideDown = false);
  void render(Pixels &bg, int x, int y, std::string_view text, Pixel24 color);
  ~Font();
};

struct FontAtlas {
  SDL_Surface *glyphAtlas = nullptr;
  std::vector<Rect> glyphRects;
  void text(std::string_view str, std::vector<Rect> &rects);
  bool create(const Font &font);
  ~FontAtlas();
};

struct SDL {

  bool inited = false;
  SDL_Window *win = nullptr;
  SDL_Surface *surf = nullptr;
  SDL_GLContext ctx = nullptr;
  const Uint8 *keys = nullptr;
  Sint32 numKeys = 0;
  Uint32 keyCounters[SDL_NUM_SCANCODES];
  Sint32 mouseX, mouseY;
  bool warpSnap = false;
  Uint32 mouseCounters[8];
  Uint32 screenshot = 0;
  volatile uint64_t startCounter = 0;

  bool init(Uint32 w, Uint32 h, bool borderless = true, std::string_view title = "demo", bool withOpenGL = false);
  void pump();
  void swap();
  void term();
  Uint32 getTicks();
  Uint64 getPerfCounter();
  double getPerfFreq();
  Pixels lock();
  bool keyDown(Sint32 key);
  bool keyPress(Sint32 key);
  bool mouseKeyDown(Uint8 key);
  bool mouseKeyPress(Uint8 key);
  void warpMouse(Sint32 x, Sint32 y);
  void takeScreenshot();
};

}
