/// @file       rm_tx.c
/// @brief      Definitions used by rsync transmitter (A).
/// @author     Piotr Gregor piotrek.gregor at gmail.com
/// @version    0.1.2
/// @date       2 Jan 2016 11:19 AM
/// @copyright  LGPLv2.1


#include "rm_tx.h"


int
rm_tx_local_push(const char *x, const char *y)
{
	return 0;
}

int
rm_tx_remote_push(const char *x, const char *y,
		struct sockaddr_in *remote_addr)
{
	return 0;
}
