/// @file       rm_tx.h
/// @brief      Definitions used by rsync transmitter (A).
/// @author     Piotr Gregor piotrek.gregor at gmail.com
/// @version    0.1.2
/// @date       2 Jan 2016 11:18 AM
/// @copyright  LGPLv2.1


#ifndef RSYNCME_TX_H
#define RSYNCME_TX_H


#include "rm.h"


int
rm_tx_local_push(const char *x, const char *y);

int
rm_tx_remote_push(const char *x, const char *y,
		struct sockaddr_in *remote_addr);

#endif	// RSYNCME_TX_H
