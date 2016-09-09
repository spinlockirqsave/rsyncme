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
 * @return      RM_ERR_OK - success,
 *              RM_ERR_FAIL - failure */
int
rm_util_dt(char *buf);

/* @brief       Get detailed date timestamp.
 * @details     @buf must be at least 27 chars.
 * @return      RM_ERR_OK - success,
 *              RM_ERR_FAIL - failure */
int
rm_util_dt_detail(char *buf);

/* @details     @dir does include terminating '/',
 *              may be "./" for current dir
 * @return      RM_ERR_OK - success,
 *              RM_ERR_FAIL - failure,
 *              RM_ERR_BAD_CALL - @dir is NULL
 *              RM_ERR_MEM - malloc failed,
 *              RM_ERR_GENERAL_ERROR - rm_util_dt_detail failed,
 *              RM_ERR_FOPEN_STDOUT - fopen failed for stdout log,
 *              RM_ERR_FREOPEN_STDOUT - stream redirection failed,
 *              RM_ERR_FOPEN_STDERR - fopen failed for stderr log,
 *              RM_ERR_FREOPEN_STDERR - stream redirection failed */
int
rm_util_openlogs(const char *dir, const char *name) __attribute__((nonnull(1)));

/* @return      RM_ERR_OK - success,
 *              RM_ERR_FAIL - failure */
int
rm_util_log(FILE *stream, const char *fmt, ...);

/* @return      RM_ERR_OK - success,
 *              RM_ERR_FAIL - failure */
int
rm_util_log_perr(FILE *stream, const char *fmt, ...);

/* @return      RM_ERR_OK - success,
 *              RM_ERR_BAD_CALL - logname is NULL,
 *              RM_ERR_FAIL - failure (first fork failed),
 *              RM_ERR_SETSID - 1st child failed to become session leader,
 *              RM_ERR_FORK -  second fork failed,
 *              RM_ERR_IO_ERROR - can't open logs,
 *              RM_ERR_CHDIR - can't chdir */
int
rm_util_daemonize(const char *dir, int noclose, const char *logname) __attribute__((nonnull(3)));

/* @return      RM_ERR_OK - success,
 *              RM_ERR_CHDIR - can't chdir,
 *              RM_ERR_IO_ERROR - can't open logs */
int
rm_util_chdir_umask_openlog(const char *dir, int noclose, const char *logname, uint8_t ignore_signals);

#ifdef DDEBUG
#define RM_D_ERR(fmt, args...) fprintf(stderr, "DEBUG ERR: %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##args)
#else
#define RM_D_ERR(fmt, ...)    do { } while (0)
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
#define RM_LOG_ERR(fmt, ...) rm_util_log(stderr, "%s\t%s:%d:%s():\t" fmt, "ERR ", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define RM_LOG_PERR(fmt, ...) rm_util_log_perr(stderr, "%s\t%s:%d:%s():\t" fmt, "ERR ", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define RM_LOG_INFO(fmt, ...) rm_util_log(stderr, "%s\t%s:%d:%s():\t" fmt, "INFO", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define RM_LOG_WARN(fmt, ...) rm_util_log(stderr, "%s\t%s:%d:%s():\t" fmt, "WARN", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define RM_LOG_CRIT(fmt, ...) rm_util_log(stderr, "%s\t%s:%d:%s():\t" fmt, "CRIT", __FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef DDEBUG
    #define RM_DEBUG_LOG_ERR(fmt, ...) RM_LOG_ERR(fmt, ...)
    #define RM_DEBUG_LOG_PERR(fmt, ...) RM_LOG_PERR(fmt, ...)
    #define RM_DEBUG_LOG_INFO(fmt, ...) RM_LOG_INFO(fmt, ...)
    #define RM_DEBUG_LOG_WARN(fmt, ...) RM_LOG_WARN(fmt, ...)
    #define RM_DEBUG_LOG_CRIT(fmt, ...) RM_LOG_CRIT(fmt, ...)
#else
    #define RM_DEBUG_LOG_ERR(fmt, ...) do {} while (0)
    #define RM_DEBUG_LOG_PERR(fmt, ...) do {} while (0)
    #define RM_DEBUG_LOG_INFO(fmt, ...) do {} while (0)
    #define RM_DEBUG_LOG_WARN(fmt, ...) do {} while (0)
    #define RM_DEBUG_LOG_CRIT(fmt, ...) do {} while (0)
#endif


#endif	/* RSYNCME_UTIL_H */
