#ifndef MESSAGES_H_V1NHZAPH
#define MESSAGES_H_V1NHZAPH

#include <stdlib.h>

/*
 *  ERROR: 		Something "serious" has gone wrong. But the program can recover from it.
 *  CRITICAL: 	Same as "ERROR", but program can't recover from it.
 *  WARN: 		Something has not worked as it should.
 *  INFO: 		The text is of a purely informative nature.
 *  PRINT: 		Print some message.
 *  DEBUG: 		DEBUG.
 */

#define ENABLE_MSG 0

typedef enum
{
	MESSAGE_ERROR,
	MESSAGE_CRITICAL,
	MESSAGE_WARNING,
	MESSAGE_PRINT,
	MESSAGE_INFO,
	MESSAGE_DEBUG
} MessageFlags;

void message(MessageFlags flag, const char *msg, ...);
void message_func(MessageFlags flag, const char *func, const char *msg, ...);
void return_if_fail_warning(const char* func, const char *expr);
void exit_if_fail_critical(const char *func, const char *expr);

#define return_if_fail(expr) 							\
	if (!(expr)) 										\
	{ 													\
		return_if_fail_warning(STRFUNC, #expr); 		\
		return; 										\
	}

#define return_val_if_fail(expr, val) 					\
	if (!(expr))										\
	{													\
		return_if_fail_warning(STRFUNC, #expr); 		\
		return (val); 									\
	} 

#define exit_if_fail(expr) 								\
	if (!(expr))										\
	{													\
		exit_if_fail_critical(STRFUNC, #expr); 			\
		exit(EXIT_FAILURE); 							\
	}

#define exit_with_code_if_fail(expr, code) 				\
	if (!(expr))										\
	{													\
		exit_if_fail_critical(STRFUNC, #expr); 			\
		exit(code); 									\
	}

#define msg_error(...) 		(message_func(MESSAGE_ERROR, 	STRFUNC, __VA_ARGS__))
#define msg_warn(...) 		(message_func(MESSAGE_WARNING,  STRFUNC, __VA_ARGS__))
#define msg_critical(...) 	(message_func(MESSAGE_CRITICAL, STRFUNC, __VA_ARGS__))
#define msg_info(...) 		(message_func(MESSAGE_INFO,  	STRFUNC, __VA_ARGS__))
#define msg_debug(...) 		(message_func(MESSAGE_DEBUG,  	STRFUNC, __VA_ARGS__))
#define msg_print(...) 		(message_func(MESSAGE_PRINT,  	STRFUNC, __VA_ARGS__))

#endif /* end of include guard: MESSAGES_H_V1NHZAPH */
