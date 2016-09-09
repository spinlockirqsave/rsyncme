/* @file        rm_tx.h
 * @brief       Definitions used by rsync transmitter (A).
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        2 Jan 2016 11:18 AM
 * @copyright   LGPLv2.1 */


#ifndef RSYNCME_TX_H
#define RSYNCME_TX_H


#include "rm.h"
#include "rm_rx.h"


/* @brief   Locally sync files @x and @y such that
 *          @y becomes same as @x.
 * @details 1. Calc nonoverlapping checksums (insert into hashtable)
 *          2. Run rolling checksums procedure (calc delta vector)
 *          3. Reconstruct the file
 *          If flags 0 bit is set @y will be created if it doesn't exist and sync will be simple
 *          copying of @x into new file with name @y.
 * @param   @z - optional name of result file, used if not NULL
 * @param   flags: bits
 *          0: if set, create file @y if it doesn't exist
 *          1:
 *          2:
 *          3:
 *          4:
 *          5:
 *          6:
 *          7:
 * @return  0 on success, negative value otherwise:
 *          -1 couldn't open @x
 *          -2 @y doesn't exist and --force set but couldn't create @y
 *          -3 couldn't open @y and --force not set, what should I use?
 *          -4 couldn't stat @x
 *          -5 couldn't stat @y
 *          -6 internal error: in rm_rx_insert_nonoverlapping_ch_ch_local
 *          -7 buffered copy failed */
enum rm_error
rm_tx_local_push(const char *x, const char *y, const char *z, size_t L, size_t copy_all_threshold,
        size_t copy_tail_threshold, size_t send_threshold, rm_push_flags flags, struct rm_delta_reconstruct_ctx *rec_ctx) __attribute__ ((nonnull(1,2,9)));

/* Initialize PUSH, ask for nonoverlapping checksums, send delta vector. */
int
rm_tx_remote_push(const char *x, const char *y, const char *z, size_t L, size_t copy_all_threshold,
        size_t copy_tail_threshold, size_t send_threshold, rm_push_flags flags,
        struct rm_delta_reconstruct_ctx *rec_ctx, const char *addr, struct sockaddr_in *remote_addr, const char **err_str) __attribute__ ((nonnull(1,2,9,10,11,12)));


#endif	/* RSYNCME_TX_H */
