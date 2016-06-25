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
rm_session_push_rx_init(struct rm_session_push_rx *prvt) {
    if (prvt == NULL) {
        return;
    }
    TWINIT_LIST_HEAD(&prvt->rx_delta_e_queue);
    pthread_mutex_init(&prvt->rx_delta_e_queue_mutex, NULL);
    pthread_cond_init(&prvt->rx_delta_e_queue_signal, NULL);
    return;
}

void
rm_session_push_tx_init(struct rm_session_push_tx *prvt) {
    if (prvt == NULL) {
        return;
    }
    memset(prvt, 0, sizeof(*prvt));
    TWINIT_LIST_HEAD(&prvt->tx_delta_e_queue);
    pthread_mutex_init(&prvt->tx_delta_e_queue_mutex, NULL);
    pthread_cond_init(&prvt->tx_delta_e_queue_signal, NULL);
    return;
}
/* frees private session, DON'T TOUCH private session after this returns */ 
void
rm_session_push_tx_free(struct rm_session_push_tx *prvt) {
    if (prvt == NULL) {
        return;
    }
    /* queue of delta elements MUST be empty now */
    assert(twlist_empty(&prvt->tx_delta_e_queue) != 0 
            && "Delta elements queue NOT EMPTY!\n");
    pthread_mutex_destroy(&prvt->tx_delta_e_queue_mutex);
    pthread_cond_destroy(&prvt->tx_delta_e_queue_signal);
    free(prvt);
    return;
}

void
rm_session_push_local_init(struct rm_session_push_local *prvt) {
    if (prvt == NULL) {
        return;
    }
    memset(prvt, 0, sizeof(*prvt));
    TWINIT_LIST_HEAD(&prvt->tx_delta_e_queue);
    pthread_mutex_init(&prvt->tx_delta_e_queue_mutex, NULL);
    pthread_cond_init(&prvt->tx_delta_e_queue_signal, NULL);
    return;
}
/* frees private session, DON'T TOUCH private session after this returns */ 
void
rm_session_push_local_free(struct rm_session_push_local *prvt) {
    if (prvt == NULL) {
        return;
    }
    /* queue of delta elements MUST be empty now */
    assert(twlist_empty(&prvt->tx_delta_e_queue) != 0 
            && "Delta elements queue NOT EMPTY!\n");
    pthread_mutex_destroy(&prvt->tx_delta_e_queue_mutex);
    pthread_cond_destroy(&prvt->tx_delta_e_queue_signal);
    free(prvt);
    return;
}

/* TODO: generate GUID here */
struct rm_session *
rm_session_create(enum rm_session_type t, size_t L) {
    struct rm_session   *s;

	s = malloc(sizeof *s);
	if (s == NULL) {
		return NULL;
    }
	memset(s, 0, sizeof(*s));
    s->type = t;
    s->rec_ctx.L = L;
	pthread_mutex_init(&s->session_mutex, NULL);

    switch (t) {
        case RM_PUSH_RX:
            s->prvt = malloc(sizeof(struct rm_session_push_rx));
            if (s->prvt == NULL) {
                goto fail;
            }
            break;
        case RM_PULL_RX:
            s->prvt = malloc(sizeof(struct rm_session_pull_rx));
            if (s->prvt == NULL) {
                goto fail;
            }
            break;
        case RM_PUSH_TX:
            s->prvt = malloc(sizeof(struct rm_session_push_tx));
            if (s->prvt == NULL) {
                goto fail;
            }
            rm_session_push_tx_init(s->prvt);
            break;
        case RM_PUSH_LOCAL:
            s->prvt = malloc(sizeof(struct rm_session_push_local));
            if (s->prvt == NULL) {
                goto fail;
            }
            rm_session_push_local_init(s->prvt);
            break;
        default:
            goto fail;
    }
    return s;

fail:
    if (s->prvt) {
        free(s->prvt);
    }
    pthread_mutex_destroy(&s->session_mutex);
    free(s);
    return NULL;
}

void
rm_session_free(struct rm_session *s) {
    enum rm_session_type    t;

	assert(s != NULL);
	pthread_mutex_destroy(&s->session_mutex);
    t = s->type;
    if (s->prvt == NULL) {
        goto end;
    }

    switch (t) {
        case RM_PUSH_RX:
            break;
        case RM_PULL_RX:
            break;
        case RM_PUSH_TX:
            rm_session_push_tx_free(s->prvt);
            break;
        case RM_PUSH_LOCAL:
            rm_session_push_local_free(s->prvt);
            break;
        default:
            assert(0 == 1 && "WTF Unknown prvt type!\n");
    }

end:
    pthread_mutex_destroy(&s->session_mutex);
    free(s);
    return;
}

void *
rm_session_ch_ch_tx_f(void *arg) {
	struct rm_session *s =
			(struct rm_session *)arg;
	assert(s != NULL);
    if (s == NULL) {
        goto exit;
    }
exit:
	return NULL;
}

void *
rm_session_ch_ch_rx_f(void *arg) {
	struct rm_session *s =
			(struct rm_session *)arg;
	assert(s != NULL);
    if (s == NULL) {
        goto exit;
    }
exit:
	return NULL;
}

void *
rm_session_delta_tx_f(void *arg) {
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

    switch (t) {
        case RM_PUSH_LOCAL:

            prvt_local = s->prvt;
            if (prvt_local == NULL) {
                goto exit;
            }
            h       = prvt_local->h;
            f_x     = prvt_local->f_x;
            delta_f = prvt_local->delta_f;
            break;

        case RM_PUSH_TX:
            prvt_tx = s->prvt;
            if (prvt_tx == NULL) {
                goto exit;
            }
            h       = prvt_tx->h;
            f_x     = prvt_tx->f_x;
            delta_f = prvt_tx->delta_f;
            break;

        default:
            goto exit;
    }

    /* 1. run rolling checksum procedure */
    err = rm_rolling_ch_proc(s, h, f_x, delta_f, 0);

    pthread_mutex_lock(&s->session_mutex);
    prvt_local->delta_tx_status = err;
    pthread_mutex_unlock(&s->session_mutex);

exit:
	pthread_exit(NULL);
}


void *
rm_session_delta_rx_f_local(void *arg) {
    FILE                            *f_y;           /* reference file, on which reconstruction is performed */
    FILE                            *f_z;           /* result file */
    struct rm_session_push_local    *prvt_local;
    twfifo_queue                    *q;
    const struct rm_delta_e         *delta_e;       /* iterator over delta elements */
    struct twlist_head              *lh;
    size_t                          bytes_to_rx;
	struct rm_session               *s;
    struct rm_delta_reconstruct_ctx rec_ctx = {0};  /* describes result of reconstruction,
                                                       we will copy this to session reconstruct context
                                                       after all is done to avoid locking on each delta element */
    int err;
    enum rm_delta_rx_status         status = RM_DELTA_RX_STATUS_OK;

    s = (struct rm_session*) arg;
    if (s == NULL) {
        status = RM_DELTA_RX_STATUS_INTERNAL_ERR;
        goto err_exit;
    }
    assert(s != NULL);

    pthread_mutex_lock(&s->session_mutex);
    if (s->type != RM_PUSH_LOCAL) {
        pthread_mutex_unlock(&s->session_mutex);
        status = RM_DELTA_RX_STATUS_INTERNAL_ERR;
        goto err_exit;
    }
    prvt_local = s->prvt;
    if (prvt_local == NULL) {
        pthread_mutex_unlock(&s->session_mutex);
        status = RM_DELTA_RX_STATUS_INTERNAL_ERR;
        goto err_exit;
    }
    assert(prvt_local != NULL);

    bytes_to_rx = prvt_local->f_x_sz;
    f_y         = prvt_local->f_y;
    f_z         = prvt_local->f_z;
    /* init reconstruction context */
    rec_ctx.L = s->rec_ctx.L;
    pthread_mutex_unlock(&s->session_mutex);

    assert(f_y != NULL);
    assert(f_z != NULL);
    if (f_y == NULL || f_z == NULL) {
        status = RM_DELTA_RX_STATUS_INTERNAL_ERR;
        goto err_exit;
    }

    if (bytes_to_rx == 0) {
        goto done;
    }

    /* TODO sleep on delta queue and reconstruct element once awoken */
    pthread_mutex_lock(&prvt_local->tx_delta_e_queue_mutex);
    q = &prvt_local->tx_delta_e_queue;

    while (bytes_to_rx > 0) {
        if (bytes_to_rx == 0) { /* checking for missing signal is not needed here as bytes_to_rx is local variable */
            pthread_mutex_unlock(&prvt_local->tx_delta_e_queue_mutex);
            goto done;
        }
        /* TODO process delta element */
        for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh)) {
            delta_e = tw_container_of(lh, struct rm_delta_e, link);
            err = rm_rx_process_delta_element(delta_e, f_y, f_z, &rec_ctx);
            if (err != 0) {
                pthread_mutex_unlock(&prvt_local->tx_delta_e_queue_mutex);
                status = RM_DELTA_RX_STATUS_DELTA_PROC_FAIL;
                goto err_exit;
            }
            bytes_to_rx -= delta_e->raw_bytes_n;
            if (delta_e->type == RM_DELTA_ELEMENT_RAW_BYTES) {
                free(delta_e->raw_bytes);
            }
            free((void*)delta_e);
        }
        if (bytes_to_rx > 0) {
            pthread_cond_wait(&prvt_local->tx_delta_e_queue_signal, &prvt_local->tx_delta_e_queue_mutex);
        }
    }
    pthread_mutex_unlock(&prvt_local->tx_delta_e_queue_mutex);

done:
    pthread_mutex_lock(&s->session_mutex);
    assert(rec_ctx.rec_by_ref + rec_ctx.rec_by_raw == prvt_local->f_x_sz);
    assert(rec_ctx.delta_tail_n == 0 || rec_ctx.delta_tail_n == 1);
    memcpy(&s->rec_ctx, &rec_ctx, sizeof(struct rm_delta_reconstruct_ctx));
    prvt_local->delta_rx_status = RM_DELTA_RX_STATUS_OK;
    pthread_mutex_unlock(&s->session_mutex);
	pthread_exit(NULL);

err_exit:

    pthread_mutex_lock(&s->session_mutex);
    prvt_local->delta_rx_status = status;
    pthread_mutex_unlock(&s->session_mutex);
	pthread_exit(NULL);
}

void *
rm_session_delta_rx_f_remote(void *arg) {
    FILE                            *f_y;           /* file on which reconstruction is performed */
    //twfifo_queue                    *q;
    //const struct rm_delta_e         *delta_e;       /* iterator over delta elements */
    //struct twlist_head              *lh;
    struct rm_session_push_rx       *prvt_rx;
    size_t                          bytes_to_rx;
	struct rm_session               *s;

    (void) f_y;
    s = (struct rm_session*) arg;
    if (s == NULL) {
        goto exit;
    }
    assert(s != NULL);

    if (s->type != RM_PUSH_RX) {
        goto exit;
    }
    prvt_rx = s->prvt;
    if (prvt_rx == NULL) {
        goto exit;
    }
    assert(prvt_rx != NULL);

    pthread_mutex_lock(&s->session_mutex);
    bytes_to_rx = prvt_rx->f_x_sz;
    /* f_y         = prvt_rx->f_y; */
    pthread_mutex_unlock(&s->session_mutex);

    if (bytes_to_rx == 0) {
        goto done;
    }

    /* TODO read delta elements from rx socket and do reconstruction */

exit:
    return NULL;
done:
    return NULL;
}
