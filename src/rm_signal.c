///  @file      rm_signal.c
///  @brief     Signal handlers.
///  @author    peterg
///  @version   0.1.1
///  @date      02 Nov 2016 02:42 PM
///  @copyright LGPLv2.1


#include "rm_signal.h"


void
rm_sigint_h(int signum)
{
        psignal(signum, "[rm] ");
        fprintf(stderr, "[rm] Thread [%lu] "
                "received SIGINT",
                (long unsigned int)getpid());
        exit(0);
}

