/// @file	rm_util.h
/// @brief	Utilities.
/// @author	peterg
/// @version	0.1.2
/// @date	04 Jan 2016 08:08 PM
/// @copyright	LGPLv2.1


#ifndef RSYNCME_UTIL_H
#define RSYNCME_UTIL_H


#include "rm_defs.h"
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>


/// @brief	Get date timestamp.
/// @details	@buf must be at least 20 chars.
/// @return	0 success, -1 failure
int
rm_util_dt(char *buf);

/// @brief	Get detailed date timestamp.
/// @details	@buf must be at least 27 chars.
/// @return	0 success, -1 failure
int
rm_util_dt_detail(char *buf);

/// @details	@dir does include terminating '/',
///		may be "./" for current dir
int
rm_util_openlogs(const char *dir, const char *name);

int
rm_util_log(FILE *stream, const char *fmt, ...);

int
rm_util_log_perr(FILE *stream, const char *fmt, ...);

int
rm_util_daemonize(const char *dir,
		int noclose, char *logname);

int
rm_util_chdir_umask_openlog(const char *dir,
		int noclose, char *logname);


#endif	// RSYNCME_UTIL_H
