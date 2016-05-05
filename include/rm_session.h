/*
 * @file        rm_session.h
 * @brief       Rsync session.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        02 Nov 2016 04:08 PM
 * @copyright   LGPLv2.1
 */


#ifndef RSYNCME_SESSION_H
#define RSYNCME_SESSION_H

#include "rm.h"
#include "twlist.h"


struct rm_session
{
	struct twhlist_node     hlink;  /* hashtable handle */
	struct twlist_head      link;   /* list handle */

	uint32_t                session_id;
	pthread_mutex_t         session_mutex;

	struct rsyncme          *rm;    /* pointer to global
                                       rsyncme object */

    enum rm_session_type    type;
    size_t                  L;
    void                    *prvt;
};

/* Receiver of file (delta vector) */
struct rm_session_push_rx
{
    int fd;                                     /* socket handle */
	pthread_t               ch_ch_tx_tid;       /* transmitter of nonoverlapping checksums */
	pthread_t               delta_rx_tid;       /* receiver of delta elements */
    twfifo_queue    rx_delta_e_queue;           /* rx queue of delta elements */
    pthread_mutex_t rx_delta_e_queue_mutex;
    pthread_cond_t  rx_delta_e_queue_signal;    /* signalled by receiving proc when
                                                   delta elements are received on the socket */
};

/* Transmitter of file (delta vector) */
struct rm_session_push_tx
{
    int fd;                                     /* socket handle */
	pthread_t               ch_ch_rx_tid;       /* receiver of nonoverlapping checksums */
	pthread_t               delta_tx_tid;       /* producer (of delta elements, rolling checksum proc) */
    struct twhlist_head     *h;                 /* nonoverlapping checkums */
    FILE                    *f_x;               /* file on which rolling is performed */              
    twfifo_queue    tx_delta_e_queue;           /* queue of delta elements */
    pthread_mutex_t tx_delta_e_queue_mutex;
    pthread_cond_t  tx_delta_e_queue_signal;    /* signalled by rolling proc when
                                                   new delta element has been produced */
};

/* Receiver of file (delta vector) */
struct rm_session_pull_rx
{
    int     fd; /* socket handle */
	pthread_t               ch_ch_tid;
	pthread_t               delta_tid;
};

/* Transmitter/receiver, local. */
struct rm_session_push_local
{
    int fd;                                     /* socket handle */
	pthread_t               delta_tx_tid;       /* producer (of delta elements, rolling checksum proc) */
    int                     delta_tx_status;
    struct twhlist_head     *h;                 /* nonoverlapping checkums */
    FILE                    *f_x;               /* file on which rolling is performed */              
    twfifo_queue    tx_delta_e_queue;           /* queue of delta elements */
    pthread_mutex_t tx_delta_e_queue_mutex;
    pthread_cond_t  tx_delta_e_queue_signal;    /* signalled by rolling proc when
                                                   new delta element has been produced */
	pthread_t               delta_rx_tid;       /* consumer (of delta elements, reconstruction function) */
    int                     delta_rx_status;
    rm_delta_f              *delta_f;           /* reconstruct callback */
};

void
rm_session_push_rx_init(struct rm_session_push_rx *prvt);
void
rm_session_push_tx_init(struct rm_session_push_tx *prvt);
void
rm_session_push_local_init(struct rm_session_push_local *prvt);

/* @brief   Creates new session. */
struct rm_session *
rm_session_create(struct rsyncme *engine, enum rm_session_type t);

void
rm_session_free(struct rm_session *s);


/* HIGH LEVEL API */

/* @brief   Tx nonoverlapping checksums (B calculates
 *          them and B calls this method)*/
void *
rm_session_ch_ch_tx_f(void *arg);

/* @brief   Rx checksums calculated by receiver (B)
 *          on nonoverlapping blocks (B calculates
 *          them and A calls this method). */
void *
rm_session_ch_ch_rx_f(void *arg);

/* @brief       Process delta reconstruction data (tx|reconstruct|etc...).
 * @details     Runs rolling checksums procedure and compares checksums
 *              to those calculated on nonoverlapping blocks of @y in (B)
 *              producing delta elements. Calls callback on each element
 *              to transmit those to (B) or reconstruct file locally, etc.
 *              This function must check what kind of session it has been
 *              called on (RM_PUSH_LOCAL/RM_PUSH_TX) and act accordingly.
 */
void *
rm_session_delta_tx_f(void *arg);

void *
rm_session_delta_rx_f(void *arg);


#endif  /* RSYNCME_SESSION_H */

