/*
 * @file        rm_rx.h
 * @brief       Definitions used by rsync receiver (B).
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        2 Jan 2016 11:19 AM
 * @copyright   LGPLv2.1
 */


#ifndef RSYNCME_RX_H
#define RSYNCME_RX_H


#include "rm_defs.h"
#include "rm.h"
#include "rm_error.h"


/* @brief   Calculates ch_ch structs for all non-overlapping
 *          @L bytes blocks (last one may be less than @L)
 *          from file @f and inserts them into hashtable @h.
 * @details	File @f MUST be already opened.
 * @param   h - hashtable to store checksums locally (B),
 *          may be NULL, if NULL then do not insert checksums
 * @param   f_tx_ch_ch - function to transmit checksums
 *          to remote end (A), may be NULL
 * @param   fname - file name, used only for logging
 * @return  On success the number of inserted entries
 *          is returned, -1 on error. */
long long int
rm_rx_insert_nonoverlapping_ch_ch(FILE *f, char *fname,
		struct twhlist_head *h, uint32_t L,
		int (*f_tx_ch_ch)(const struct rm_ch_ch *));

/* @brief       Calculates ch_ch structs for all non-overlapping
 *              @L bytes blocks (last one may be less than @L)
 *              from file @f and inserts them into array @checkums.
 * @param       checksums - array of structs rm_ch_ch with size suitable
 *              to contain all checksums
 * @blocks_n    on success - set to the number of inserted entries
 * @return      0 on succes, negative error code on error */
int
rm_rx_insert_nonoverlapping_ch_ch_array(FILE *f, const char *fname,
		struct rm_ch_ch checksums[], uint32_t L,
		int (*f_tx_ch_ch)(const struct rm_ch_ch *), size_t *blocks_n);

/* @brief       Calculates ch_ch_local structs for all non-overlapping
 *              @L bytes blocks (last one may be less than @L)
 *              from file @f and inserts them into list @l.
 * @details	    File @f MUST be already opened.
 * @param       l - list to store checksums locally (B), can't be NULL
 * @param       fname - file name, used only for logging
 * @blocks_n    on success - set to the number of inserted entries
 * @return      0 on success, negative error code on error. */
int
rm_rx_insert_nonoverlapping_ch_ch_local(FILE *f, const char *fname,
		struct twlist_head *l, uint32_t L, size_t *blocks_n);


#endif	// RSYNCME_RX_H
