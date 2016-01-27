/// @file       rm_rx.h
/// @brief      Definitions used by rsync receiver (B).
/// @author     Piotr Gregor piotrek.gregor at gmail.com
/// @version    0.1.2
/// @date       2 Jan 2016 11:19 AM
/// @copyright  LGPLv2.1


#ifndef RSYNCME_RX_H
#define RSYNCME_RX_H


#include "rm_defs.h"
#include "rm.h"
#include "rm_error.h"


/// @brief	Calculates ch_ch structs for all non-overlapping
///		@L bytes blocks (last one may be less than @L)
///		from file @f and inserts them into table @h.
/// @details	File @f MUST be already open.
/// @param	h - hashtable to store checksums locally (B)
/// @param	f_tx_ch_ch - function to transmit checksums
///		to remote end A
/// @return	On success the number of inserted entries
///		is returned, -1 on error.
long long int
rm_rx_insert_nonoverlapping_ch_ch(FILE *f, char *fname,
		struct twhlist_head *h, uint32_t L,
		int (*f_tx_ch_ch)(const struct rm_ch_ch *));


#endif	// RSYNCME_RX_H
