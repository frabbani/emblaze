#pragma once

#include <functional>
#include <string_view>

namespace mbz {
namespace utils {
namespace logger {

void logFunc(std::function<void(const char*)>);
void logOut(const char *format, ...);
void debugging(bool enable = false);
void logSingle(std::string_view format, ...);
//void logInfo(std::string_view tag, std::string_view format, ...);
//void logWarn(std::string_view tag, std::string_view format, ...);
//void logError(std::string_view tag, std::string_view format, ...);
//void logDebug(std::string_view tag, std::string_view format, ...);
void logType(int type, std::string_view tag, std::string_view format, ...);

}
}
}

#define LOGINFO(tag, format, ...) mbz::utils::logger::logType(1, tag, format, ##__VA_ARGS__)
#define LOGWARN(tag, format, ...) mbz::utils::logger::logType(2, tag, format, ##__VA_ARGS__)
#define LOGERROR(tag, format, ...) mbz::utils::logger::logType(3, tag, format, ##__VA_ARGS__)
#define LOGDEBUG(tag, format, ...) mbz::utils::logger::logType(0, tag, format, ##__VA_ARGS__)

