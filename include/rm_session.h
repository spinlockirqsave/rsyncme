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
	pthread_t               ch_ch_tid;
	pthread_t               delta_tid;

    /* receiver */
    struct twlist_head      rx_ch_ch_list;
    pthread_mutex_t         rx_ch_ch_list_mutex;

	struct rsyncme          *rm;    /* pointer to global
                                       rsyncme object */

    enum rm_session_type    type;
    void                    *prvt;
};

/* Receiver of file (delta vector) */
struct rm_session_push_rx
{
    int     fd; /* socket handle */
};

/* Receiver of file (delta vector) */
struct rm_session_pull_rx
{
    int     fd; /* socket handle */
};

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

struct rm_session_delta_tx_f_arg
{
    struct twhlist_head     *h;             /* nonoverlapping checkums */
    FILE                    *f_x;           /* file on which rolling is performed */              
    rm_delta_f              *delta_f;       /* tx/reconstruct callback */
};
/* @brief       Process delta reconstruction data (tx|reconstruct|etc...).
 * @details     Runs rolling checksums procedure and compares checksums
 *              to those calculated on nonoverlapping blocks of @y in (B)
 *              producing delta elements. Calls callback on each element
 *              to transmit those to (B) or reconstruct file locally, etc.
 */
void *
rm_session_delta_tx_f(void *arg);

void *
rm_session_delta_rx_f(void *arg);


#endif  /* RSYNCME_SESSION_H */

