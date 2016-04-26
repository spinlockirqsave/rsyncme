/*
 * @file        rm_util.h
 * @brief       Utilities, log.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        04 Jan 2016 08:08 PM
 * @copyright   LGPLv2.1
 */


#ifndef RSYNCME_UTIL_H
#define RSYNCME_UTIL_H


#include "rm_defs.h"
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>


/* @brief       Get date timestamp.
 * @details     @buf must be at least 20 chars.
 * @return      0 success, -1 failure */
int
rm_util_dt(char *buf);

/* @brief       Get detailed date timestamp.
 * @details     @buf must be at least 27 chars.
 * @return      0 success, -1 failure */
int
rm_util_dt_detail(char *buf);

/* @details     @dir does include terminating '/',
 *              may be "./" for current dir */
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

#ifdef DDEBUG
#define RM_D_ERR(fmt, args...) fprintf(stderr,        \
                        "DEBUG: %s:%d:%s(): " fmt,      \
                __FILE__, __LINE__, __func__, ##args)
#else
#define RM_D_ERR(fmt, args...)    do { } while (0)
#endif

/*
// logging using syslog API
// #define RM_EMERG(fmt, args...) syslog(LOG_EMERG, \
//                        "%s:%d:%s(): " fmt,       \
//                __FILE__, __LINE__, __func__, ##args)
// #define RM_ALERT(fmt, args...) syslog(LOG_ALERT, \
//                        "%s:%d:%s(): " fmt,       \
//                __FILE__, __LINE__, __func__, ##args)
// #define RM_CRIT(fmt, args...) syslog(LOG_CRIT, \
//                       "%s:%d:%s(): " fmt,       \
//                __FILE__, __LINE__, __func__, ##args)
// #define RM_ERR(fmt, args...) syslog(LOG_ERR, \
//                        "%s:%d:%s(): " fmt,       \
//               __FILE__, __LINE__, __func__, ##args)
// #define RM_WARNING(fmt, args...) syslog(LOG_WARNING, \
//                        "%s:%d:%s(): " fmt,       \
//                __FILE__, __LINE__, __func__, ##args)
// #define RM_NOTICE(fmt, args...) syslog(LOG_NOTICE, \
//                        "%s:%d:%s(): " fmt,       \
//               __FILE__, __LINE__, __func__, ##args)
// #define RM_INFO(fmt, args...) syslog(LOG_INFO, \
//                        "%s:%d:%s(): " fmt,       \
//               __FILE__, __LINE__, __func__, ##args)
// #define RM_DEBUG(fmt, args...) syslog(LOG_DEBUG, \
//                        "%s:%d:%s(): " fmt,       \
//                __FILE__, __LINE__, __func__, ##args)
*/

/* log */
#define RM_LOG_ERR(fmt, args...) rm_util_log(stderr, \
			"%s\t%s:%d:%s(): " fmt,       \
                "ERR", __FILE__, __LINE__, __func__, ##args)
#define RM_LOG_PERR(fmt, args...) rm_util_log_perr(stderr, \
			"%s\t%s:%d:%s(): " fmt,       \
                "ERR", __FILE__, __LINE__, __func__, ##args)
#define RM_LOG_INFO(fmt, args...) rm_util_log(stderr, \
			"%s\t%s:%d:%s(): " fmt,       \
                "INFO", __FILE__, __LINE__, __func__, ##args)
#define RM_LOG_CRIT(fmt, args...) rm_util_log(stderr, \
			"%s\t%s:%d:%s(): " fmt,       \
                "CRIT", __FILE__, __LINE__, __func__, ##args)


#endif	/* RSYNCME_UTIL_H */
