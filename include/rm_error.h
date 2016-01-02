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

#define RM_ERR(fmt, args...) fprintf(stderr,  \
                        "ERR: %s:%d:%s(): " fmt,        \
                __FILE__, __LINE__, __func__, ##args)

#define RM_INFO(fmt, args...) fprintf(stderr, \
                        "INFO: %s:%d:%s(): " fmt,       \
                __FILE__, __LINE__, __func__, ##args)

#endif  // RSYNCME_ERROR_H

