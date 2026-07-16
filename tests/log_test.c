#include "BridgeEngine.h"
#include "internal/platform/platform.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

struct plat_mutex {
	int unused;
};

static struct plat_mutex fake_mutex;
static char captured[2048];

static plat_mutex_t create_mutex(void)
{
	return &fake_mutex;
}

static void destroy_mutex(plat_mutex_t mutex)
{
	(void)mutex;
}

static void lock_mutex(plat_mutex_t mutex)
{
	(void)mutex;
}

static void unlock_mutex(plat_mutex_t mutex)
{
	(void)mutex;
}

static void log_info(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vsnprintf(captured, sizeof(captured), format, args);
	va_end(args);
}

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

int main(int argc, char **argv)
{
	if (argc != 2) return 1;
	const plat_interface_t platform = {
		.sync = {.create_mutex = create_mutex, .destroy_mutex = destroy_mutex,
				 .lock_mutex = lock_mutex, .unlock_mutex = unlock_mutex},
		.core = {.log_info = log_info},
	};
	if (plat_init(&platform) != 0) return 1;

	bapi_log_config_t config = {.min_level = BAPI_LOG_LEVEL_INFO, .use_colors = false,
		.use_file = true, .log_file_path = argv[1]};
	if (!bapi_log_init(&config)) return 1;
	bapi_log_message(BAPI_LOG_LEVEL_DEBUG, "debug.c", 1, "debug", "hidden");
	bapi_log_message(BAPI_LOG_LEVEL_INFO, "path/file.c", 42, "work", "hello %d", 7);
	bapi_log_shutdown();

	FILE *file = fopen(argv[1], "r");
	if (!file) return 1;
	char contents[2048] = {0};
	fread(contents, 1, sizeof(contents) - 1, file);
	fclose(file);
	int result = expect(strstr(contents, "[INFO] [file.c:42] work(): hello 7") != NULL,
		"normal message keeps its text format") &&
		expect(strstr(contents, "hidden") == NULL, "level filtering remains active");

	config.use_colors = true;
	config.use_file = false;
	if (!bapi_log_init(&config)) return 1;
	char long_text[2048];
	memset(long_text, 'x', sizeof(long_text) - 1);
	long_text[sizeof(long_text) - 1] = '\0';
	bapi_log_message(BAPI_LOG_LEVEL_INFO, long_text, 99, long_text, "%s", long_text);
	result = result && expect(captured[sizeof(captured) - 1] == '\0', "long logs remain NUL terminated") &&
		expect(strstr(captured, "\033[32m") == captured, "colors keep their existing prefix");
	bapi_log_shutdown();
	plat_shutdown();
	return result ? 0 : 1;
}
