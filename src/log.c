#include "BridgeEngine.h"
#include "internal/platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __XJ380_OS__

extern unsigned long long xapi_GetTime(void);
#endif

static struct {
	bapi_log_level_t min_level;
	bool			 initialized;
	bool			 use_colors;
	bool			 use_file;
	plat_mutex_t	 mutex;
	FILE			*log_file;
	const char		*log_file_path;
	const char		*level_strings[6];
	const char		*level_colors[6];
} g_log_state = {.min_level		= BAPI_LOG_LEVEL_INFO,
				 .initialized	= false,
				 .use_colors	= true,
				 .use_file		= false,
				 .mutex			= NULL,
				 .log_file		= NULL,
				 .log_file_path = NULL,
				 .level_strings = {[BAPI_LOG_LEVEL_DEBUG]	 = "DEBUG",
								   [BAPI_LOG_LEVEL_INFO]	 = "INFO",
								   [BAPI_LOG_LEVEL_WARN]	 = "WARN",
								   [BAPI_LOG_LEVEL_ERROR]	 = "ERROR",
								   [BAPI_LOG_LEVEL_CRITICAL] = "CRITICAL",
								   [BAPI_LOG_LEVEL_NONE]	 = "NONE"},
				 .level_colors	= {[BAPI_LOG_LEVEL_DEBUG]	 = "\033[37m",
								   [BAPI_LOG_LEVEL_INFO]	 = "\033[32m",
								   [BAPI_LOG_LEVEL_WARN]	 = "\033[33m",
								   [BAPI_LOG_LEVEL_ERROR]	 = "\033[31m",
								   [BAPI_LOG_LEVEL_CRITICAL] = "\033[35m",
								   [BAPI_LOG_LEVEL_NONE]	 = "\033[0m"}};

static const char *get_level_string(bapi_log_level_t level)
{
	if (level >= 0 && level <= BAPI_LOG_LEVEL_NONE) {
		return g_log_state.level_strings[level];
	}
	return "UNKNOWN";
}

static const char *get_level_color(bapi_log_level_t level)
{
	if (level >= 0 && level <= BAPI_LOG_LEVEL_NONE) {
		return g_log_state.level_colors[level];
	}
	return "\033[0m";
}

static void get_timestamp(char *buffer, size_t buffer_size)
{
#ifdef __XJ380_OS__

	UINT64 t = xapi_GetTime();
	snprintf(buffer, buffer_size, "%llu", (unsigned long long)t);
#else
	time_t	   now	   = time(NULL);
	struct tm *tm_info = localtime(&now);
	strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
#endif
}

static void log_write_to_file(const char *message)
{
	if (g_log_state.log_file != NULL) {
		fprintf(g_log_state.log_file, "%s\n", message);
		fflush(g_log_state.log_file);
	}
}

bool bapi_log_init(const bapi_log_config_t *config)
{
	if (g_log_state.initialized) {
		return true;
	}

	const plat_interface_t *plat = plat_get();

	if (config == NULL) {
		g_log_state.min_level	  = BAPI_LOG_LEVEL_INFO;
		g_log_state.use_colors	  = true;
		g_log_state.use_file	  = false;
		g_log_state.log_file_path = NULL;
	} else {
		g_log_state.min_level	  = config->min_level;
		g_log_state.use_colors	  = config->use_colors;
		g_log_state.use_file	  = config->use_file;
		g_log_state.log_file_path = config->log_file_path;
	}

	if (plat) {
		g_log_state.mutex = plat->sync.create_mutex();
		if (g_log_state.mutex == NULL) {
			return false;
		}
	}

	if (g_log_state.use_file && g_log_state.log_file_path != NULL) {
		g_log_state.log_file = fopen(g_log_state.log_file_path, "w");
		if (g_log_state.log_file == NULL) {
			g_log_state.use_file = false;
		}
	}

	g_log_state.initialized = true;

	if (plat) {
		plat->core.log_info("Logger initialized with level: %s",
							get_level_string(g_log_state.min_level));
	}

	return true;
}

void bapi_log_shutdown(void)
{
	if (!g_log_state.initialized) {
		return;
	}

	if (g_log_state.log_file != NULL) {
		fclose(g_log_state.log_file);
		g_log_state.log_file = NULL;
	}

	const plat_interface_t *plat = plat_get();
	if (plat && g_log_state.mutex != NULL) {
		plat->sync.destroy_mutex(g_log_state.mutex);
		g_log_state.mutex = NULL;
	}

	g_log_state.initialized = false;
}

void bapi_log_set_level(bapi_log_level_t level)
{
	g_log_state.min_level = level;
}

static void format_message(char *buffer, size_t buffer_size, bapi_log_level_t level,
						   const char *file, int line, const char *func, const char *format,
						   va_list args)
{
	char timestamp[32];
	get_timestamp(timestamp, sizeof(timestamp));

	const char *filename = strrchr(file, '/');
	if (filename == NULL) {
		filename = strrchr(file, '\\');
	}
	filename = (filename != NULL) ? filename + 1 : file;

	int pos = 0;

	if (g_log_state.use_colors) {
		pos += snprintf(buffer + pos, buffer_size - pos, "%s", get_level_color(level));
	}

	pos +=
		snprintf(buffer + pos, buffer_size - pos, "[%s] [%s] ", timestamp, get_level_string(level));

	pos += snprintf(buffer + pos, buffer_size - pos, "[%s:%d] %s(): ", filename, line, func);

	vsnprintf(buffer + pos, buffer_size - pos, format, args);

	if (g_log_state.use_colors) {
		size_t len = strlen(buffer);
		snprintf(buffer + len, buffer_size - len, "\033[0m");
	}
}

void bapi_log_message(bapi_log_level_t level, const char *file, int line, const char *func,
					  const char *format, ...)
{
	if (!g_log_state.initialized || level < g_log_state.min_level) {
		return;
	}

	const plat_interface_t *plat = plat_get();

	char	buffer[1024];
	va_list args;

	va_start(args, format);
	format_message(buffer, sizeof(buffer), level, file, line, func, format, args);
	va_end(args);

	if (plat && g_log_state.mutex != NULL) {
		plat->sync.lock_mutex(g_log_state.mutex);
	}

	switch (level) {
	case BAPI_LOG_LEVEL_DEBUG:
		if (plat) plat->core.log_debug("%s", buffer);
		break;
	case BAPI_LOG_LEVEL_INFO:
		if (plat) plat->core.log_info("%s", buffer);
		break;
	case BAPI_LOG_LEVEL_WARN:
		if (plat) plat->core.log_warn("%s", buffer);
		break;
	case BAPI_LOG_LEVEL_ERROR:
		if (plat) plat->core.log_error("%s", buffer);
		break;
	case BAPI_LOG_LEVEL_CRITICAL:
		if (plat) plat->core.log_critical("%s", buffer);
		break;
	default:
		if (plat) plat->core.log_info("%s", buffer);
		break;
	}

	if (g_log_state.use_file) {
		log_write_to_file(buffer);
	}

	if (plat && g_log_state.mutex != NULL) {
		plat->sync.unlock_mutex(g_log_state.mutex);
	}
}
