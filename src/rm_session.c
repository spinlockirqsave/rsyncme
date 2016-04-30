/*
 * @file        rm_session.c
 * @brief       Rsync session.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        02 Nov 2016 04:08 PM
 * @copyright   LGPLv2.1
 */


#include "rm_session.h"


void
rm_session_push_rx_init(struct rm_session_push_rx *prvt)
{
    if (prvt == NULL)
    {
        return;
    }
    TWINIT_LIST_HEAD(&prvt->rx_delta_e_queue);
    pthread_mutex_init(&prvt->rx_delta_e_queue_mutex, NULL);
    pthread_cond_init(&prvt->rx_delta_e_queue_signal, NULL);
    return;
}

void
rm_session_push_tx_init(struct rm_session_push_tx *prvt)
{
    if (prvt == NULL)
    {
        return;
    }
    TWINIT_LIST_HEAD(&prvt->tx_delta_e_queue);
    pthread_mutex_init(&prvt->tx_delta_e_queue_mutex, NULL);
    pthread_cond_init(&prvt->tx_delta_e_queue_signal, NULL);
    return;
}

void
rm_session_push_local_init(struct rm_session_push_local *prvt)
{
    if (prvt == NULL)
    {
        return;
    }
    TWINIT_LIST_HEAD(&prvt->tx_delta_e_queue);
    pthread_mutex_init(&prvt->tx_delta_e_queue_mutex, NULL);
    pthread_cond_init(&prvt->tx_delta_e_queue_signal, NULL);
    return;
}

/* TODO: generate GUID here */
struct rm_session *
rm_session_create(struct rsyncme *rm, enum rm_session_type t)
{
	struct rm_session *s;

	s = malloc(sizeof *s);
	if (s == NULL)
		return NULL;
	memset(s, 0, sizeof(*s));
    s->type = t;
	pthread_mutex_init(&s->session_mutex, NULL);

	s->rm = rm;

    switch (t)
    {
        case RM_PUSH_RX:
            s->prvt = malloc(sizeof(struct rm_session_push_rx));
            if (s->prvt == NULL)
                goto fail;
            break;
        case RM_PULL_RX:
            s->prvt = malloc(sizeof(struct rm_session_pull_rx));
            if (s->prvt == NULL)
                goto fail;
            break;
        case RM_PUSH_TX:
            s->prvt = malloc(sizeof(struct rm_session_push_tx));
            if (s->prvt == NULL)
                goto fail;
            rm_session_push_tx_init(s->prvt);
        case RM_PUSH_LOCAL:
            s->prvt = malloc(sizeof(struct rm_session_push_local));
            if (s->prvt == NULL)
                goto fail;
            rm_session_push_local_init(s->prvt);
        default:
            goto fail;
    }

    return s;

fail:
    if (s->prvt)
    {
        free(s->prvt);
    }
    pthread_mutex_destroy(&s->session_mutex);
    free(s);
    return NULL;
}

void
rm_session_free(struct rm_session *s)
{
	assert(s != NULL);
	pthread_mutex_destroy(&s->session_mutex);
	free(s);
}

void *
rm_session_ch_ch_tx_f(void *arg)
{
	struct rm_session *s =
			(struct rm_session *)arg;
	assert(s != NULL);
	return 0;
}

void *
rm_session_ch_ch_rx_f(void *arg)
{
	struct rm_session *s =
			(struct rm_session *)arg;
	assert(s != NULL);
	return 0;
}

void *
rm_session_delta_tx_f(void *arg)
{
    struct twhlist_head     *h;             /* nonoverlapping checkums */
    FILE                    *f_x;           /* file on which rolling is performed */
    rm_delta_f              *delta_f;       /* tx/reconstruct callback */

	struct rm_session_delta_tx_f_arg *f_arg =
                        (struct rm_session_delta_tx_f_arg*) arg;
	assert(f_arg != NULL);

    h       = f_arg->h;
    f_x     = f_arg->f_x;
    delta_f = f_arg->delta_f;

    /* 1. run rolling checksum procedure */
	return 0;
}


void *
rm_session_delta_rx_f(void *arg)
{
	struct rm_session *s =
			(struct rm_session *)arg;
	assert(s != NULL);
	return 0;
}
