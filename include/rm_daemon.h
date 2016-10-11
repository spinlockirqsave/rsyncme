/* @file        rm_daemon.h
 * @brief       Daemeon related.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        8 Oct 2016 08:05 PM
 * @copyright   LGPLv2.1 */


#ifndef RSYNCME_DAEMON_H
#define RSYNCME_DAEMON_H


#include "rm.h"
#include "rm_core.h"

#include <signal.h>


struct rsyncme  rm;


static void rm_daemon_sigint_handler(int signo);
static void rm_daemon_sigtstp_handler(int signo);

static void rm_daemon_signal_handler(int signo);


#endif  /* RSYNCME_DAEMON_H */
