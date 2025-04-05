#include <cstdio>
#include <mutex>

#include "log.h"

namespace mbz {
namespace utils {
namespace logger {

static std::function<void(const char*)> printFunc = nullptr;
static bool debugPrint = false;

void logFunc(std::function<void(const char*)> print) {
  printFunc = print;
}

void logOut(const char *format, ...) {
  if (!printFunc)
    return;
  char str[16 * 1024], out[16 * 1024];
  va_list args;
  __builtin_va_start(args, format);
  vsprintf(str, format, args);
  __builtin_va_end(args);
  sprintf(out, "[MBZ] %s\n", str);
  printFunc(out);
}

void logSingle(std::string_view format, ...) {
  if (!printFunc)
    return;
  char str[16 * 1024], out[16 * 1024];
  va_list args;
  __builtin_va_start(args, format);
  vsprintf(str, format.data(), args);
  __builtin_va_end(args);
  sprintf(out, "[MBZ] %s", str);
  printFunc(out);
}

void logType(int type, std::string_view tag, std::string_view format, ...) {
  if (!printFunc)
    return;
  char str[16 * 1024], out[16 * 1024];
  va_list args;
  __builtin_va_start(args, format);
  vsnprintf(str, sizeof(str), format.data(), args);
  __builtin_va_end(args);
  switch (type) {
    case 0:
      sprintf(out, "[MBZ][DEBUG].: %s: %s\n", tag.data(), str);
      break;
    case 1:
      sprintf(out, "[MBZ][INFO]..: %s: %s\n", tag.data(), str);
      break;
    case 2:
      sprintf(out, "[MBZ][WARN]..: %s: %s\n", tag.data(), str);
      break;
    default:
      sprintf(out, "[MBZ][ERROR].: %s: %s\n", tag.data(), str);
      break;
  }
  printFunc(out);
}

void logInfo(std::string_view tag, std::string_view format, ...) {
  va_list args;
  __builtin_va_start(args, format);
  logType(1, tag, format, args);
  __builtin_va_end(args);
}

void logInfo2(const char *tag, const char* format, ...) {
  va_list args;
  __builtin_va_start(args, format);
  logType(1, tag, format, args);
  __builtin_va_end(args);
}

void logWarn(std::string_view tag, std::string_view format, ...) {
  va_list args;
  __builtin_va_start(args, format);
  logType(2, tag, format, args);
  __builtin_va_end(args);
}

void logError(std::string_view tag, std::string_view format, ...) {
  va_list args;
  __builtin_va_start(args, format);
  logType(3, tag, format, args);
  __builtin_va_end(args);
}

void logDebug(std::string_view tag, std::string_view format, ...) {
  if (!debugPrint)
    return;
  va_list args;
  __builtin_va_start(args, format);
  logType(0, tag, format, args);
  __builtin_va_end(args);
}

void debugging(bool enable) {
  debugPrint = enable;
}

}
}
}
