#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdarg.h>

#ifndef LOG_NO_FILE
# define LOG_PRINT(level, ...) log_print(__FILE__, __LINE__, __func__, level, __VA_ARGS__)
# define LOG_PRINTV(level, ...) log_printv(__FILE__, __LINE__, __func__, level, __VA_ARGS__)
#else
# define LOG_PRINT(level, ...) log_print(level, __VA_ARGS__)
# define LOG_PRINTV(level, ...) log_printv(level, __VA_ARGS__)
#endif

#ifndef LOG_LEVEL_MIN
# define LOG_LEVEL_MIN 4
#endif

#if !defined(NDEBUG) && LOG_LEVEL_MIN >= 1
# define LOG_DEBUG(...) LOG_PRINT(LOG_LEVEL_DEBUG, __VA_ARGS__)
# define LOG_DEBUGV(...) LOG_PRINTV(LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
# define LOG_DEBUG(...) do {} while (0)
# define LOG_DEBUGV(...) do {} while (0)
#endif

#if LOG_LEVEL_MIN >= 2
# define LOG_INFO(...) LOG_PRINT(LOG_LEVEL_INFO, __VA_ARGS__)
# define LOG_INFOV(...) LOG_PRINTV(LOG_LEVEL_INFO, __VA_ARGS__)
#else
# define LOG_INFO(...) do {} while (0)
# define LOG_INFOV(...) do {} while (0)
#endif

#if LOG_LEVEL_MIN >= 3
# define LOG_WARN(...) LOG_PRINT(LOG_LEVEL_WARN, __VA_ARGS__)
# define LOG_WARNV(...) LOG_PRINTV(LOG_LEVEL_WARN, __VA_ARGS__)
#else
# define LOG_WARN(...) do {} while (0)
# define LOG_WARNV(...) do {} while (0)
#endif

#if LOG_LEVEL_MIN >= 4
# define LOG_ERROR(...) LOG_PRINT(LOG_LEVEL_ERROR, __VA_ARGS__)
# define LOG_ERRORV(...) LOG_PRINTV(LOG_LEVEL_ERROR, __VA_ARGS__)
#else
# define LOG_ERROR(...) do {} while (0)
# define LOG_ERRORV(...) do {} while (0)
#endif

enum log_level
{
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR,
};

#ifndef LOG_NO_FILE
void log_print(const char *file, int line, const char *fn, enum log_level level, const char *fmt, ...) __attribute__((format(printf, 5, 6)));
void log_printv(const char *file, int line, const char *fn, enum log_level level, const char *fmt, va_list ap);
#else
void log_print(enum log_level level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void log_printv(enum log_level level, const char *fmt, va_list ap);
#endif

extern bool g_log_colored;

#endif
