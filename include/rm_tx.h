/// @file       rm_tx.h
/// @brief      Definitions used by rsync transmitter (A).
/// @author     Piotr Gregor <piotrek.gregor at gmail.com>
/// @version    0.1.2
/// @date       2 Jan 2016 11:18 AM
/// @copyright  LGPLv2.1


#ifndef RSYNCME_TX_H
#define RSYNCME_TX_H


#include "rm.h"


/// @brief	    Locally sync files @x and @y such that
///	            @y becomes same as @x.
/// @return	    0 on success, negative value otherwise:
///		        -1 couldn't open @x
///		        -2 couldn't open @y
int
rm_tx_local_push(const char *x, const char *y,
			uint32_t L);

int
rm_tx_remote_push(const char *x, const char *y,
		struct sockaddr_in *remote_addr,
		uint32_t L);

#endif	// RSYNCME_TX_H
