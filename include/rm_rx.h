/* @file        rm_rx.h
 * @brief       Definitions used by rsync receiver (B).
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        2 Jan 2016 11:19 AM
 * @copyright   LGPLv2.1 */


#ifndef RSYNCME_RX_H
#define RSYNCME_RX_H


#include "rm_defs.h"
#include "rm.h"
#include "rm_error.h"

#include "rm_session.h"
#include "rm_tcp.h"


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
 *          is returned, -1 on error. 
 int
 rm_rx_insert_nonoverlapping_ch_ch(FILE *f, const char *fname,
 struct twhlist_head *h, uint32_t L,
 int (*f_tx_ch_ch)(const struct rm_ch_ch *),
 size_t limit, size_t *blocks_n);*/

struct f_tx_ch_ch_ref_arg_1
{
    const struct rm_ch_ch_ref   *e;
    const struct rm_session     *s;
};

/* return   RM_ERR_OK - success,
 *          RM_ERR_BAD_CALL - bad parameters
 *          RM_ERR_ARG - session type not handled,
 *          RM_ERR_TX - transmission error */
int
rm_rx_f_tx_ch_ch(const struct f_tx_ch_ch_ref_arg_1 arg);

/* return   RM_ERR_OK - success,
 *          RM_ERR_BAD_CALL - null parameters (session or checksum object)
 *          RM_ERR_ARG - session type not handled,
 *          RM_ERR_RX - null private object,
 *          RM_ERR_TX - transmission error */
int
rm_rx_f_tx_ch_ch_ref_1(const struct f_tx_ch_ch_ref_arg_1 arg);

/* return   RM_ERR_OK - success,
 *          RM_ERR_BAD_CALL - null parameters or zero block size,
 *          RM_ERR_FSTAT - can't fstat file,
 *          RM_ERR_MEM - malloc failed,
 *          RM_ERR_READ - read I/O failed,
 *          RM_ERR_TX - transmission error */
int
rm_rx_insert_nonoverlapping_ch_ch_ref(int fd, FILE *f_x, const char *fname, struct twhlist_head *h, size_t L,
        int (*f_tx_ch_ch_ref)(int fd, const struct rm_ch_ch_ref *e), size_t limit, size_t *blocks_n, pthread_mutex_t *file_mutex) __attribute__((nonnull(2,3)));

/* @brief   Calculates ch_ch structs for all non-overlapping @L bytes blocks (last one may be less than @L)
 *          from file @f and inserts them into array @checkums.
 * @param   checksums - pointer to array of structs rm_ch_ch, array size must be sufficient to contain all checksums,
 *          i.e. it must be >= than (blocks_n = file_sz / L + (file_sz % L ? 1 : 0))
 * @blocks_n    on success - set to the number of inserted entries
 * @return  RM_ERR_OK - success,
 *          RM_ERR_BAD_CALL - null parameters or zero block size,
 *          RM_ERR_FSTAT - can't fstat file,
 *          RM_ERR_MEM - malloc failed,
 *          RM_ERR_READ - read I/O failed,
 *          RM_ERR_TX - transmission error */
int
rm_rx_insert_nonoverlapping_ch_ch_array(FILE *f_x, const char *fname, struct rm_ch_ch *checksums, size_t L,
        int (*f_tx_ch_ch)(const struct rm_ch_ch *), size_t limit, size_t *blocks_n) __attribute__((nonnull(1,2,3)));

/* @brief   Creates ch_ch_ref_link structs for all non-overlapping @L bytes blocks (last one may be less than @L)
 *          from file @f and inserts them into list @l.
 * @details File @f MUST be already opened.
 * @param   l - list to store checksums locally (B), can't be NULL
 * @param   fname - file name, used only for logging
 * @blocks_n    on success - set to the number of inserted entries
 * @return  RM_ERR_OK - success,
 *          RM_ERR_BAD_CALL - null parameters or zero block size,
 *          RM_ERR_FSTAT - can't fstat file,
 *          RM_ERR_MEM - malloc failed,
 *          RM_ERR_READ - read I/O failed */
int
rm_rx_insert_nonoverlapping_ch_ch_ref_link(FILE *f_x, const char *fname, struct twlist_head *l, size_t L,
        size_t limit, size_t *blocks_n) __attribute__((nonnull(1,2,3)));

struct rm_rx_delta_element_arg {
	const struct rm_delta_e			*delta_e;
	FILE							*f_y;
	FILE							*f_z;
	struct rm_delta_reconstruct_ctx	*rec_ctx;
	int								fd;
	pthread_mutex_t					*file_mutex;
};
/* @brief   Used in local session in local push.
 * @details	Reconstruction procedure.
 * @return  RM_ERR_OK - success,
 *          RM_ERR_BAD_CALL - null parameters,
 *          RM_ERR_COPY_OFFSET - copy offset failed,
 *          RM_ERR_WRITE - fpwrite failed,
 *          RM_ERR_COPY_BUFFERED - copy buffered failed,
 *          RM_ERR_ARG - unknown delta type */
enum rm_error
rm_rx_process_delta_element(void *arg) __attribute__((nonnull(1)));

/* @brief	Used in local session's rm_session_delta_rx_f_local() thread proc in remote push as delta TCP TX callback.
 * @details	In remote push this function will tx deltas over TCP socket connected to remote receiver's TCP port
 *			(port number is received by transmitter in RM_MSG_PUSH_ACK message sent by remote receiver and it is stored
 *			in session's ack message pointed to by @msg_push_ack pointer).
 */
enum rm_error
rm_rx_tx_delta_element(void *arg) __attribute__((nonnull(1)));


#endif	/* RSYNCME_RX_H */
