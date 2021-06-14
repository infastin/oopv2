#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "Messages.h"
#include "Utils.h"

static const char* message_flag_to_color(MessageFlags flag)
{
	switch (flag) {
		case MESSAGE_ERROR:
			return "\033[1;31m";
		case MESSAGE_CRITICAL:
			return "\033[1;35m";
		case MESSAGE_WARNING:
			return "\033[1;33m";
		case MESSAGE_PRINT:
			return "\033[1;32m";
		case MESSAGE_INFO:
			return "\033[1;36m";
		case MESSAGE_DEBUG:
			return "\033[1;34m";
	}

	return "";
}

static const char* message_flag_to_prefix(MessageFlags flag, bool *to_stdout)
{
	*to_stdout = true;

	switch (flag) {
		case MESSAGE_ERROR:
			*to_stdout = false;
			return "ERROR";
		case MESSAGE_CRITICAL:
			*to_stdout = false;
			return "CRITICAL";
		case MESSAGE_WARNING:
			*to_stdout = false;
			return "WARNING";
		case MESSAGE_PRINT:
			return "MESSAGE";
		case MESSAGE_INFO:
			return "INFO";
		case MESSAGE_DEBUG:
			return "DEBUG";
	}

	return "";
}

void message(MessageFlags flag, const char *msg, ...)
{
	const char *color;
	const char *prefix;
	const char *reset;
	bool to_stdout;
	FILE *stream;

	color = message_flag_to_color(flag);
	prefix = message_flag_to_prefix(flag, &to_stdout);
	reset = "\033[0m";

	if (to_stdout)
		stream = stdout;
	else
		stream = stderr;

	va_list ap;
	va_start(ap, msg);
	char *parsed = strdup_vprintf(msg, &ap);
	va_end(ap);

	fprintf(stream, "** %s%s%s: %s\n", color, prefix, reset, parsed);
	free(parsed);
}

void message_func(MessageFlags flag, const char *func, const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	char *parsed = strdup_vprintf(msg, &ap);
	va_end(ap);

	const char *white = "\033[1;37m";
	const char *reset = "\033[0m";

	message(flag, "%s%s%s: %s", white, func, reset, parsed);

	free(parsed);
}

void return_if_fail_warning(const char* func, const char *expr)
{
	const char *white = "\033[1;37m";
	const char *reset = "\033[0m";

	message(MESSAGE_WARNING, "%s%s%s: assertion '%s' is failed!", white, func, reset, expr);
}

void exit_if_fail_critical(const char* func, const char *expr)
{
	const char *white = "\033[1;37m";
	const char *reset = "\033[0m";

	message(MESSAGE_CRITICAL, "%s%s%s: assertion '%s' is failed!", white, func, reset, expr);
}
