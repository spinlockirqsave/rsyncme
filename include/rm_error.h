/*
 * @file        rm_error.h
 * @brief       Errors.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        02 Jan 2016 02:23 PM
 * @copyright   LGPLv2.1
 */


#ifndef RSYNCME_ERROR_H
#define RSYNCME_ERROR_H


#include "rm_defs.h"
#include "rm_util.h"


/* @brief       Prints error to standard error stream. */
void rm_err(const char *s);

/* @brief       Prints error to standard error stream
 *              explains the errno and returns. */
void rm_perr(const char *s);

/* @brief       Prints error to standard error stream
 *              explains the errno and aborts the execution. */
void rm_perr_abort(const char *s)
__attribute__((noreturn));


#endif  /* RSYNCME_ERROR_H */

