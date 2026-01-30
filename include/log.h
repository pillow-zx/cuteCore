#ifndef __LOG_H__
#define __LOG_H__

#include "utils.h"

#define ANSI_FG_BLACK "\33[1;30m"
#define ANSI_FG_RED "\33[1;31m"
#define ANSI_FG_GREEN "\33[1;32m"
#define ANSI_FG_YELLOW "\33[1;33m"
#define ANSI_FG_BLUE "\33[1;34m"
#define ANSI_FG_MAGENTA "\33[1;35m"
#define ANSI_FG_CYAN "\33[1;36m"
#define ANSI_FG_WHITE "\33[1;37m"
#define ANSI_BG_BLACK "\33[1;40m"
#define ANSI_BG_RED "\33[1;41m"
#define ANSI_BG_GREEN "\33[1;42m"
#define ANSI_BG_YELLOW "\33[1;43m"
#define ANSI_BG_BLUE "\33[1;44m"
#define ANSI_BG_MAGENTA "\33[1;45m"
#define ANSI_BG_CYAN "\33[1;46m"
#define ANSI_BG_WHITE "\33[1;47m"
#define ANSI_NONE "\33[0m"

#define ANSI_FMT(str, fmt) fmt str ANSI_NONE

#define _Log(level, ...)                                                                                               \
    do {                                                                                                               \
        if (level <= LOG_LEVEL_MAX) printf(__VA_ARGS__);                                                               \
    } while (0)

#define Info(format, ...)                                                                                              \
    _Log(LOG_LEVEL_INFO, ANSI_FMT("[INFO][%s:%d %s] " format, ANSI_FG_GREEN) "\n", __FILE__, __LINE__, __func__,       \
         ##__VA_ARGS__)
#define Debug(format, ...)                                                                                             \
    _Log(LOG_LEVEL_DEBUG, ANSI_FMT("[DEBUG][%s:%d %s] " format, ANSI_FG_BLUE) "\n", __FILE__, __LINE__, __func__,      \
         ##__VA_ARGS__)
#define Warn(format, ...)                                                                                              \
    _Log(LOG_LEVEL_WARN, ANSI_FMT("[WARN][%s:%d %s] " format, ANSI_FG_YELLOW) "\n", __FILE__, __LINE__, __func__,      \
         ##__VA_ARGS__)
#define Error(format, ...)                                                                                             \
    do {                                                                                                               \
        _Log(LOG_LEVEL_ERROR, ANSI_FMT("[ERROR][%s:%d %s] " format, ANSI_FG_RED) "\n", __FILE__, __LINE__, __func__,   \
             ##__VA_ARGS__);                                                                                           \
        shutdown();                                                                                                    \
    } while (0);

#endif // __LOG_H__
