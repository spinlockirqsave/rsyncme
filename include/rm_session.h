/* @file        rm_session.h
 * @brief       Rsync session.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        02 Nov 2016 04:08 PM
 * @copyright   LGPLv2.1 */


#ifndef RSYNCME_SESSION_H
#define RSYNCME_SESSION_H


#include "rm.h"
#include "rm_rx.h"
#include "twlist.h"


extern enum rm_loglevel RM_LOGLEVEL;

struct rm_session
{
    struct twhlist_node     hlink;  /* hashtable handle */
    struct twlist_head      link;   /* list handle */

    unsigned char           id[RM_UUID_LEN];
    struct rm_md5           hash;
	uint32_t				hashed_hash;
    pthread_mutex_t         mutex;

    FILE                    *f_x;               /* file on which rolling is performed */              
    FILE                    *f_y;               /* reference file */              
    FILE                    *f_z;               /* result file */              
    size_t                  f_x_sz;             /* size of @x and the number of bytes to be addressed by delta elements */

    enum rm_session_type    type;
    struct rm_delta_reconstruct_ctx rec_ctx;
    void                    *prvt;
    struct timespec         clk_realtime_start, clk_realtime_stop;
    double                  clk_cputime_start, clk_cputime_stop;
    enum rm_loglevel        loglevel;
    char                    ssid1[RM_UNIQUE_STRING_LEN];
    char                    ssid2[RM_UNIQUE_STRING_LEN];
};

/* Transmitter/receiver, local. */
struct rm_session_push_local
{
    pthread_t               delta_tx_tid;       /* producer (of delta elements, rolling checksum proc) */
    enum rm_tx_status       delta_tx_status;

    struct twhlist_head     *h;                 /* nonoverlapping checksums hashtable, points to stack-allocated table */
    pthread_mutex_t			h_mutex;			/* protects hashtable */

    twfifo_queue    tx_delta_e_queue;           /* queue of delta elements */
    pthread_mutex_t tx_delta_e_queue_mutex;
    pthread_cond_t  tx_delta_e_queue_signal;    /* signalled by rolling proc when
                                                   new delta element has been produced */
    rm_delta_f              *delta_tx_f;        /* delta tx callback (in RM_PUSH_LOCAL enqueues delta elements, in RM_PUSH_TX the same) */

    pthread_t               delta_rx_tid;       /* consumer of delta elements (reconstruction function in local push, delta transmitter in remote push) */
    enum rm_rx_status       delta_rx_status;
    rm_delta_f              *delta_rx_f;        /* delta rx callback (in RM_PUSH_LOCAL dequeues delta elements and does data reconstruction, in RM_PUSH_TX dequeues deltas and transmits them over TCP) */
};

/* Receiver of file (delta vector) */
struct rm_session_push_rx
{
    int fd;                                     /* socket handle */
    pthread_t               ch_ch_tx_tid;       /* transmitter of nonoverlapping checksums */
    enum rm_tx_status       ch_ch_tx_status;

    pthread_t               delta_rx_tid;       /* receiver of delta elements */
    enum rm_rx_status       delta_rx_status;
    twfifo_queue    rx_delta_e_queue;           /* rx queue of delta elements */
    pthread_mutex_t rx_delta_e_queue_mutex;
    pthread_cond_t  rx_delta_e_queue_signal;    /* signaled by receiving proc when
                                                   delta elements are received on the socket */
    struct rm_msg_push      *msg_push;          /* keeps pointer to MSG_PUSH message that describes incomig synchronization request */
};

/* Transmitter of file (delta vector) */
struct rm_session_push_tx
{
	int						fd_delta_tx;		/* delta tx channel */
    struct rm_session_push_local session_local; /* delta producer and delta transmitter threads */

    int fd;                                     /* main handle to the TCP connection */

	int						fd_ch_ch_rx;		/* nonoverlapping checksums rx channel */
    pthread_t               ch_ch_rx_tid;       /* receiver of nonoverlapping checksums */
    int                     ch_ch_rx_status;
    pthread_mutex_t         ch_ch_hash_mutex;   /* hashtable mutex for synchronization with session_local's ch_ch_rx thread */
};

/* Receiver of file (delta vector) */
struct rm_session_pull_rx
{
    int     fd; /* socket handle */
    pthread_t               ch_ch_tid;
    pthread_t               delta_tid;
};

void rm_session_push_rx_init(struct rm_session_push_rx *prvt) __attribute__((nonnull(1)));
void rm_session_push_rx_free(struct rm_session_push_rx *prvt) __attribute__((nonnull(1)));

void rm_session_push_tx_init(struct rm_session_push_tx *prvt) __attribute__((nonnull(1)));

/* @brief   Frees private session, DON'T TOUCH private
 *          session after this returns */ 
void rm_session_push_tx_free(struct rm_session_push_tx *prvt) __attribute__((nonnull(1)));

void rm_session_push_local_init(struct rm_session_push_local *prvt) __attribute__((nonnull(1)));

/* @brief   Frees private session, DON'T TOUCH private
 *          session after this returns */ 
void rm_session_push_local_free(struct rm_session_push_local *prvt) __attribute__((nonnull(1)));
static void rm_session_push_local_deinit(struct rm_session_push_local *prvt) __attribute__((nonnull(1)));

enum rm_error rm_session_assign_validate_from_msg_push(struct rm_session *s, struct rm_msg_push *m, int fd) __attribute__((nonnull(1,2)));
enum rm_error rm_session_assign_validate_from_msg_pull(struct rm_session *s, struct rm_msg_pull *m) __attribute__((nonnull(1,2)));

/* @brief   Creates new session. */
struct rm_session* rm_session_create(enum rm_session_type t);

/* @brief   Frees session with it's private object, DON'T TOUCH
 *          session after this returns */ 
void rm_session_free(struct rm_session *s) __attribute__((nonnull(1)));


/* HIGH LEVEL API */

/* @brief   Tx nonoverlapping checksums (not delta yet!) (B calculates
 *          them and B calls this method)*/
void* rm_session_ch_ch_tx_f(void *arg) __attribute__((nonnull(1)));

/* @brief   Rx checksums (not delta yet!) calculated by receiver (B)
 *          on nonoverlapping blocks (B calculates
 *          them and A calls this method). */
void* rm_session_ch_ch_rx_f(void *arg) __attribute__((nonnull(1)));

/* @brief       Process delta reconstruction data (tx|reconstruct|etc...).
 * @details     Runs rolling checksums procedure and compares checksums
 *              to those calculated on nonoverlapping blocks of @y in (B)
 *              producing delta elements. Calls callback on each element
 *              to transmit those to (B) or reconstruct file locally, etc.
 *              This function must check what kind of session it has been
 *              called on (RM_PUSH_LOCAL/RM_PUSH_TX) and act accordingly.
 */
void* rm_session_delta_tx_f(void *arg);

void* rm_session_delta_rx_f_local(void *arg) __attribute__((nonnull(1)));
void* rm_session_delta_rx_f_remote(void *arg) __attribute__((nonnull(1)));


#endif  /* RSYNCME_SESSION_H */

