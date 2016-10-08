/* @file        rm_daemon.c
 * @brief       Daemeon related.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        8 Oct 2016 08:05 PM
 * @copyright   LGPLv2.1 */


#include "rm_daemon.h"


struct rsyncme  rm;


void rm_daemon_sigint_handler(int signo) {
    if (signo != SIGINT) {
        return;
    }
    fprintf(stderr, "\n\n\n\n==Received SIGINT==\n\nState:\n");
    fprintf(stderr, "workers_n                             \t[%u]\n", rm.wq.workers_n);
    fprintf(stderr, "workers_active_n                      \t[%u]\n", rm.wq.workers_active_n);
    fprintf(stderr, "\n\n");
    return;
}
