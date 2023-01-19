#include "log.h"

#include <pthread.h>
#include <stdarg.h>

/* XXX: wayyyy faster stdio */
#undef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO 0

#include <stdio.h>
#include <time.h>

bool g_log_colored = true;

#ifndef DEBUG_NO_FILE
void log_print(const char *file, int line, const char *fn, enum log_level level, const char *fmt, ...)
#else
void log_print(enum log_level level, const char *fmt, ...)
#endif
{
	va_list ap;
	va_start(ap, fmt);
#ifndef DEBUG_NO_FILE
	log_printv(file, line, fn, level, fmt, ap);
#else
	log_printv(level, fmt, ap);
#endif
}

#ifndef DEBUG_NO_FILE
void log_printv(const char *file, int line, const char *fn, enum log_level level, const char *fmt, va_list ap)
#else
void log_printv(enum log_level level, const char *fmt, va_list ap)
#endif
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	char buffer[4096];
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	static const char *colors[] =
	{
		"\e[1;32m",
		"\e[1;96m",
		"\e[1;93m",
		"\e[1;31m",
	};
	static const char *levels[] =
	{
		"DEBUG",
		"INFO ",
		"WARN ",
		"ERROR",
	};
	FILE *files[] =
	{
		stdout,
		stdout,
		stderr,
		stderr,
	};
	const char *color = g_log_colored ? colors[level] : "";
	const char *color_bold = g_log_colored ? "\e[1m" : "";
	const char *color_reset = g_log_colored ? "\e[0m" : "";
	char date_buf[256];
	struct tm tm;
	time_t t = time(NULL);
#ifdef _WIN32
	localtime_s(&tm, &t);
#else
	localtime_r(&t, &tm);
#endif
	strftime(date_buf, sizeof(date_buf), "%c", &tm);
	pthread_mutex_lock(&mutex);
#ifndef DEBUG_NO_FILE
	fprintf(files[level], "[%s] %s[%s%s%s] %s%s%s (%s@%s:%d)\n", date_buf, color_reset, color, levels[level], color_reset, color_bold, buffer, color_reset, fn, file, line);
#else
	fprintf(files[level], "[%s] %s[%s%s%s] %s%s%s\n"           , date_buf, color_reset, color, levels[level], color_reset, color_bold, buffer, color_reset);
#endif
	fflush(files[level]);
	pthread_mutex_unlock(&mutex);
}
