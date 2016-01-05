/// @file      rm_error.h
/// @brief     Errors print/log.
/// @author    peterg
/// @version   0.1.1
/// @date      02 Jan 2016 02:23 PM
/// @copyright LGPLv2.1


#ifndef RSYNCME_ERROR_H
#define RSYNCME_ERROR_H


#include "rm_defs.h"


/// @brief      Prints error to standard error stream.
void rm_err(const char *s);

/// @brief      Prints error to standard error stream
///             explains the errno and returns.
void rm_perr(const char *s);

/// @brief      Prints error to standard error stream
///             explains the errno and aborts the execution.
void rm_perr_abort(const char *s)
__attribute__((noreturn));

#ifdef DDEBUG
#define RM_D_ERR(fmt, args...) fprintf(stderr,        \
                        "DEBUG: %s:%d:%s(): " fmt,      \
                __FILE__, __LINE__, __func__, ##args)
#else
#define RM_D_ERR(fmt, args...)    do { } while (0)
#endif

// logging using syslog API
#define RM_EMERG(fmt, args...) syslog(LOG_EMERG, \
                        "%s:%d:%s(): " fmt,       \
                __FILE__, __LINE__, __func__, ##args)
#define RM_ALERT(fmt, args...) syslog(LOG_ALERT, \
                        "%s:%d:%s(): " fmt,       \
                __FILE__, __LINE__, __func__, ##args)
#define RM_CRIT(fmt, args...) syslog(LOG_CRIT, \
                        "%s:%d:%s(): " fmt,       \
                __FILE__, __LINE__, __func__, ##args)
#define RM_ERR(fmt, args...) syslog(LOG_ERR, \
                        "%s:%d:%s(): " fmt,       \
                __FILE__, __LINE__, __func__, ##args)
#define RM_WARNING(fmt, args...) syslog(LOG_WARNING, \
                        "%s:%d:%s(): " fmt,       \
                __FILE__, __LINE__, __func__, ##args)
#define RM_NOTICE(fmt, args...) syslog(LOG_NOTICE, \
                        "%s:%d:%s(): " fmt,       \
                __FILE__, __LINE__, __func__, ##args)
#define RM_INFO(fmt, args...) syslog(LOG_INFO, \
                        "%s:%d:%s(): " fmt,       \
                __FILE__, __LINE__, __func__, ##args)
#define RM_DEBUG(fmt, args...) syslog(LOG_DEBUG, \
                        "%s:%d:%s(): " fmt,       \
                __FILE__, __LINE__, __func__, ##args)

// log file
#define RM_LOG_ERR(fmt, args...) rm_util_log(stderr, \
			"%s\t%s:%d:%s(): " fmt,       \
                "ERR", __FILE__, __LINE__, __func__, ##args)
#define RM_LOG_INFO(fmt, args...) rm_util_log(stderr, \
			"%s\t%s:%d:%s(): " fmt,       \
                "INFO", __FILE__, __LINE__, __func__, ##args)


#endif  // RSYNCME_ERROR_H

