#ifndef _UTILS_H_
#define _UTILS_H_

#include "generated/autoconf.h"

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_DEBUG 2
#define LOG_LEVEL_WARN 3
#define LOG_LEVEL_ERROR 4

#ifdef CONFIG_LOG_INFO
#define LOG_LEVEL_MAX LOG_LEVEL_INFO
#elif defined(CONFIG_LOG_DEBUG)
#define LOG_LEVEL_MAX LOG_LEVEL_DEBUG
#elif defined(CONFIG_LOG_WARN)
#define LOG_LEVEL_MAX LOG_LEVEL_WARN
#elif defined(CONFIG_LOG_ERROR)
#define LOG_LEVEL_MAX LOG_LEVEL_ERROR
#else
#define LOG_LEVEL_MAX LOG_LEVEL_NONE
#endif
#endif // _UTILS_H_
