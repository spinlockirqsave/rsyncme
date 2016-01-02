/// @file       rm_rx.h
/// @brief      Definitions used by rsync receiver (B).
/// @author     Piotr Gregor piotrek.gregor at gmail.com
/// @version    0.1.1
/// @date       2 Jan 2016 11:19 AM
/// @copyright  LGPL


#ifndef RSYNCME_RX_H
#define RSYNCME_RX_H


#include "rm.h"


/// @brief	Produce list of ch_ch_h structs
///		from file @f.
/// @return	On success the size of list is returned,
///		-1 on error.
int
rx_ch_ch_h(FILE *f, struct twlist_head *h); 


#endif	// RSYNCME_RX_H
