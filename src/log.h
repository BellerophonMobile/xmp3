/**
 * xmp3 - XMPP Proxy
 * log.h - Debugging and logging functions/macros.  From: http://c.learncodethehardway.org/book/learn-c-the-hard-waych21.html
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdio.h>
#include <errno.h>
#include <string.h>

/** Prints a debug message if not compiled with NDEBUG. */
#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

/** Returns errno as a string if its set. */
#define clean_errno() (errno == 0 ? "None" : strerror(errno))

/** Writes an error message to stderr. */
#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

/** Writes a warning message to stderr. */
#define log_warn(M, ...) fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

/** Writes an information message to stderr. */
#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

/** Checks the condition A, logs and jumps to error label on error. */
#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

/** Marks code that should not run.  If it does, log and jump to error. */
#define sentinel(M, ...)  { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

/** Check that A is not null, else log and abort. */
#define check_mem(A) if(!(A)) { log_err("Out of memory."); abort(); }
//#define check_mem(A) check((A), "Out of memory.")

/** Same as check, but doesn't print if NDEBUG is set. */
#define check_debug(A, M, ...) if(!(A)) { debug(M, ##__VA_ARGS__); errno=0; goto error; }
