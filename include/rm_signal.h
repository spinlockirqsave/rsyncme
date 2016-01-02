///  @file      rm_signal.h
///  @brief     Signal handlers.
///  @author    peterg
///  @version   0.1.1
///  @date      02 Nov 2016 02:42 PM
///  @copyright LGPLv2.1


#ifndef RSYNCME_SIGNAL_H
#define RSYNCME_SIGNAL_H


#include "rm_core.h"


#include <signal.h>


/// @brief      Abnormal termination.
void
rm_sigint_h(int signum);


#endif  // RSYNCME_SIGNAL_H

