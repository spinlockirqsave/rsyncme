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
    struct rm_session       *s;
    enum rm_session_type    t;
    struct rm_session_push_local    *prvt_local;
    struct rm_session_push_tx       *prvt_tx;
    int err;

	s = (struct rm_session*) arg;
	assert(s != NULL);

    t = s->type;

    switch (t)
    {
        case RM_PUSH_LOCAL:

            prvt_local = s->prvt;
            if (prvt_local == NULL)
            {
                goto exit;
            }
            h       = prvt_local->h;
            f_x     = prvt_local->f_x;
            delta_f = prvt_local->delta_f;
            break;

        case RM_PUSH_TX:
            break;

        default:
            goto exit;
    }

    /* 1. run rolling checksum procedure */
    err = rm_rolling_ch_proc(s, h, f_x, delta_f, s->L, 0);

    pthread_mutex_lock(&s->session_mutex);
    prvt_local->delta_tx_status = err;
    pthread_mutex_unlock(&s->session_mutex);

    /* normal exit */
	pthread_exit(&prvt_local->delta_tx_status);

exit:
    /* abnormal exit */
    return NULL;
}


void *
rm_session_delta_rx_f(void *arg)
{
	struct rm_session *s =
			(struct rm_session *)arg;
	assert(s != NULL);
    /* TODO complete, call reconstruct proc */
    /* call this in there */
    /* check delta type 
    switch (delta_e->type)
    {
        case RM_DELTA_ELEMENT_REFERENCE:
            break;
        case RM_DELTA_ELEMENT_RAW_BYTES:
            break;
        default:
            RM_LOG_CRIT("WTF WTF WTF! Unknown delta type?! Have you added"
                " some neat code recently?");
        assert(1 == 0);;
        return -5;
    }
	return 0;
    */
    return NULL;
}
