/*
 * @file        rm_session.c
 * @brief       Rsync session.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        02 Nov 2016 04:08 PM
 * @copyright   LGPLv2.1
 */


#include "rm_session.h"


void rm_session_push_rx_init(struct rm_session_push_rx *prvt)
{
    memset(prvt, 0, sizeof(*prvt));
    TWINIT_LIST_HEAD(&prvt->rx_delta_e_queue);
    pthread_mutex_init(&prvt->rx_delta_e_queue_mutex, NULL);
    pthread_cond_init(&prvt->rx_delta_e_queue_signal, NULL);
    prvt->msg_push = NULL;
    return;
}

void rm_session_push_rx_free(struct rm_session_push_rx *prvt)
{
    pthread_mutex_destroy(&prvt->rx_delta_e_queue_mutex);
    pthread_cond_destroy(&prvt->rx_delta_e_queue_signal);
    if (prvt->msg_push != NULL) {
        rm_msg_push_free(prvt->msg_push);
    }
    free(prvt);
    return;
}

void rm_session_push_tx_init(struct rm_session_push_tx *prvt)
{
    memset(prvt, 0, sizeof(*prvt));
    rm_session_push_local_init(&prvt->session_local);
    pthread_mutex_init(&prvt->ch_ch_hash_mutex, NULL);
    return;
}

/* frees private session, DON'T TOUCH private session after this returns */ 
void rm_session_push_tx_free(struct rm_session_push_tx *prvt)
{
    rm_session_push_local_deinit(&prvt->session_local);
    pthread_mutex_destroy(&prvt->ch_ch_hash_mutex);
    free(prvt);
    return;
}

void rm_session_push_local_init(struct rm_session_push_local *prvt)
{
    memset(prvt, 0, sizeof(*prvt));
    TWINIT_LIST_HEAD(&prvt->tx_delta_e_queue);
    pthread_mutex_init(&prvt->tx_delta_e_queue_mutex, NULL);
    pthread_cond_init(&prvt->tx_delta_e_queue_signal, NULL);
    return;
}

static void rm_session_push_local_deinit(struct rm_session_push_local *prvt)
{
    /* queue of delta elements MUST be empty now */
    assert(twlist_empty(&prvt->tx_delta_e_queue) != 0 && "Delta elements queue NOT EMPTY!\n");
    pthread_mutex_destroy(&prvt->tx_delta_e_queue_mutex);
    pthread_cond_destroy(&prvt->tx_delta_e_queue_signal);
}

/* frees private session, DON'T TOUCH private session after this returns */ 
void rm_session_push_local_free(struct rm_session_push_local *prvt)
{
    /* queue of delta elements MUST be empty now */
    rm_session_push_local_deinit(prvt);
    free(prvt);
    return;
}

enum rm_error rm_session_assign_validate_from_msg_push(struct rm_session *s, struct rm_msg_push *m)
{
    struct rm_session_push_rx   *push_rx = NULL;

    if (m->L == 0) {                                                                    /* L can't be 0 */
        return RM_ERR_BLOCK_SIZE;
    }
    if (m->x_sz == 0) {                                                                 /* @x not set */
        return RM_ERR_X_ZERO_SIZE;
    }

    switch (s->type) {

        case RM_PUSH_RX:                                                                /* validate remote PUSH RX */
            push_rx = s->prvt;
            push_rx->msg_push = m;
            s->f_x = NULL;
            if ((m->hdr->flags & RM_BIT_6) && (m->z_sz == 0 || (strcmp(m->y, m->z) == 0))) { /* if do not delete @y after @z has been synced, but @z name is not given or is same as @y - error */
                return RM_ERR_Y_Z_SYNC;
            }
            if (m->y_sz == 0) {
                return RM_ERR_Y_NULL;                                                   /* error: what is the name of the result file? OR error: what is the name of the file you want to sync with? */
            }
            s->f_y = fopen(m->y, "rb");                                                 /* try to open */ 
            if (s->f_y == NULL) {
                if (m->hdr->flags & RM_BIT_4) {                                         /* force creation if @y doesn't exist? */
                    if (m->z_sz != 0) {                                                 /* use different name? */
                        s->f_z = fopen(m->z, "w+b");
                    } else {
                        s->f_z = fopen(m->y, "w+b");
                    }
                    if (s->f_z == NULL) {
                        return RM_ERR_OPEN_Z;
                    }
                    goto maybe_f_z;                                                     /* @y is NULL though */
                } else {
                    return RM_ERR_OPEN_Y;                                               /* couldn't open @y */
                }
            }
            rm_md5((unsigned char*) m->y, m->y_sz, s->hash.data);
            goto exit;

        case RM_PULL_RX:                                                                /* validate remote PULL RX */
            rm_md5((unsigned char*) m->y, m->y_sz, (unsigned char*) &s->hash);
            goto exit;

        default:                                                                        /* TX and everything else */
            return RM_ERR_FAIL;
    }

maybe_f_z:
    if (s->f_z == NULL) {
        if (m->z_sz == 0) {
            rm_get_unique_string(m->z);
        }
        s->f_z = fopen(m->z, "wb+");                                                    /* and open @f_z for reading and writing */
        if (s->f_z == NULL) {
            return RM_ERR_OPEN_TMP;
        }
    }
exit:
    return RM_ERR_OK;
}

enum rm_error rm_session_assign_from_msg_pull(struct rm_session *s, const struct rm_msg_pull *m)
{
    /* TODO assign pointer, validate whether request can be executed */
    /*s->f_x = fopen(m->x;
      s->f_y = m->y;
      s->f_z = m->z; */
    rm_md5((unsigned char*) m->y, m->y_sz, (unsigned char*) &s->hash);
    return RM_ERR_FAIL;
}

/* TODO: generate GUID here */
struct rm_session *rm_session_create(enum rm_session_type t)
{
    struct rm_session   *s;
    uuid_t              uuid;

    s = malloc(sizeof *s);
    if (s == NULL)
        return NULL;
    memset(s, 0, sizeof(*s));
    TWINIT_HLIST_NODE(&s->hlink);
    TWINIT_LIST_HEAD(&s->link);
    s->type = t;
    pthread_mutex_init(&s->mutex, NULL);

    switch (t) {
        case RM_PUSH_RX:
            s->prvt = malloc(sizeof(struct rm_session_push_rx));
            if (s->prvt == NULL)
                goto fail;
            rm_session_push_rx_init(s->prvt);
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
            break;
        case RM_PUSH_LOCAL:
            s->prvt = malloc(sizeof(struct rm_session_push_local));
            if (s->prvt == NULL)
                goto fail;
            rm_session_push_local_init(s->prvt);
            break;
        default:
            goto fail;
    }
    clock_gettime(CLOCK_REALTIME, &s->clk_realtime_start);
    s->clk_cputime_start = clock() / CLOCKS_PER_SEC;
    uuid_generate(uuid);
    memcpy(&s->id, &uuid, rm_min(sizeof(uuid_t), RM_UUID_LEN));
    return s;

fail:
    if (s->prvt) {
        free(s->prvt);
    }
    pthread_mutex_destroy(&s->mutex);
    free(s);
    return NULL;
}

void rm_session_free(struct rm_session *s)
{
    enum rm_session_type    t;

    assert(s != NULL);
    pthread_mutex_destroy(&s->mutex);
    t = s->type;
    if (s->prvt == NULL)
        goto end;
    switch (t) {
        case RM_PUSH_RX:
            rm_session_push_rx_free(s->prvt);
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
    free(s);
    return;
}

void *rm_session_ch_ch_tx_f(void *arg)
{
    enum rm_error                   err = RM_ERR_OK;
    struct rm_session               *s = (struct rm_session *) arg;
    enum rm_session_type            s_type;
    struct rm_session_push_rx       *rm_push_rx;
    struct rm_session_pull_rx       *rm_pull_rx;
    struct rm_msg_push              *msg_push;
    size_t                          L = 0;
    FILE                            *f_y = NULL;   /* file for taking non-overlapping blocks */
    FILE                            *f_z = NULL;   /* result */
    int                             fd_y;
    uint8_t                         reference_file_exist = 0;
    struct stat                     fs;
    size_t                          y_sz, blocks_n_exp, blocks_n;
    int                             fd;

    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);

    s_type = s->type;

    switch (s_type) {
        case RM_PUSH_RX:
            rm_push_rx = (struct rm_session_push_rx*) s->prvt;
            if (rm_push_rx == NULL) {
                err = RM_ERR_RX;
                goto err_exit;
            }
            msg_push = rm_push_rx->msg_push;
            fd = rm_push_rx->fd;
            L = rm_push_rx->msg_push->L;
            f_y = fopen(msg_push->y, "rb");
            if (f_y != NULL) {                                                                                  /* if reference file exists, split it and calc checksums */
                reference_file_exist = 1;
                fd_y = fileno(f_y);
                memset(&fs, 0, sizeof(fs));
                if (fstat(fd_y, &fs) != 0) {
                    err = RM_ERR_FSTAT_Y;
                    goto err_exit;
                }
                y_sz = fs.st_size;

                blocks_n_exp = y_sz / L + (y_sz % L ? 1 : 0);                                                   /* split @y file into non-overlapping blocks and calculate checksums on these blocks, expected number of blocks is */
                if (rm_rx_insert_nonoverlapping_ch_ch_ref(fd, f_y, msg_push->y, h, L, rm_tcp_tx_ch_ch, blocks_n_exp, &blocks_n) != RM_ERR_OK) {  /* tx ch_ch only, no ref */
                    err = RM_ERR_NONOVERLAPPING_INSERT;
                    goto  err_exit;
                }
                assert(blocks_n == blocks_n_exp && "rm_do_msg_push_rx ASSERTION failed  indicating ERROR in blocks count either here or in rm_rx_insert_nonoverlapping_ch_ch_ref");
            } else {
                if (msg_push->hdr->flags & RM_BIT_4) { /* force creation if @y doesn't exist? */
                    if (msg_push->z != NULL) { /* use different name? */
                        f_z = fopen(msg_push->z, "w+b");
                    } else {
                        f_z = fopen(msg_push->y, "w+b");
                    }
                    if (f_z == NULL) {
                        err = RM_ERR_OPEN_Z;
                        goto err_exit;
                    }
                    clock_gettime(CLOCK_REALTIME, &s->clk_realtime_start);
                    s->clk_cputime_start = clock() / CLOCKS_PER_SEC;
                    s->rec_ctx.method = RM_RECONSTRUCT_METHOD_COPY_BUFFERED;
                    /* expected rec_ctx->delta_raw_n = 1; */
                    /* expected rec_ctx->rec_by_raw = x_sz; */
                    goto done;
                } else {
                    err = RM_ERR_OPEN_Y;
                    goto err_exit;
                }
            }
            break;
        case RM_PULL_RX:
            rm_pull_rx = (struct rm_session_pull_rx*) s->prvt;
            if (rm_pull_rx == NULL) {
                err = RM_ERR_RX;
                goto err_exit;
            }
            fd = rm_pull_rx->fd;
            break;
        default:
            err = RM_ERR_ARG;
            goto err_exit;
    }

err_exit:
    /* TODO set session's status error */
    if (reference_file_exist == 1) {    /* TODO */
    }
done:
    if (reference_file_exist == 1) {    /* TODO */
    }
    pthread_mutex_lock(&s->mutex);
    rm_push_rx->ch_ch_tx_status = err;
    pthread_mutex_unlock(&s->mutex);
    return NULL;
}

void *rm_session_ch_ch_rx_f(void *arg)
{
    struct rm_session *s = (struct rm_session *) arg;
    assert(s != NULL);
    if (s == NULL)
        goto exit;
exit:
    return NULL;
}

void *rm_session_delta_tx_f(void *arg)
{
    struct twhlist_head     *h;             /* nonoverlapping checkums */
    FILE                    *f_x;           /* file on which rolling is performed */
    rm_delta_f              *delta_f;       /* tx/reconstruct callback */
    struct rm_session       *s;
    enum rm_session_type    t;
    struct rm_session_push_local    *prvt_local;
    struct rm_session_push_tx       *prvt_tx;
    int                     err;
    enum rm_tx_status       status = RM_TX_STATUS_OK;

    s = (struct rm_session*) arg;
    assert(s != NULL);

    pthread_mutex_lock(&s->mutex);
    f_x     = s->f_x;
    t = s->type;
    switch (t) {
        case RM_PUSH_LOCAL:
            prvt_local = s->prvt;
            if (prvt_local == NULL)
                goto exit;
            h       = prvt_local->h;
            delta_f = prvt_local->delta_f;
            break;

        case RM_PUSH_TX:
            prvt_tx = s->prvt;
            if (prvt_tx == NULL)
                goto exit;
            h       = prvt_tx->session_local.h;
            delta_f = prvt_tx->session_local.delta_f;
            break;

        default:
            goto exit;
    }
    pthread_mutex_unlock(&s->mutex);
    err = rm_rolling_ch_proc(s, h, f_x, delta_f, 0); /* 1. run rolling checksum procedure */
    if (err != RM_ERR_OK)
        status = RM_TX_STATUS_ROLLING_PROC_FAIL; /* TODO switch err to return more descriptive errors from here to delta tx thread's status */
    pthread_mutex_lock(&s->mutex);
    if (t == RM_PUSH_LOCAL) {
        prvt_local->delta_tx_status = status;
    } else {
        prvt_tx->session_local.delta_tx_status = status; /* remote push, local session part */
    }

exit:
    pthread_mutex_unlock(&s->mutex);
    return NULL; /* this thread must be created in joinable state */
}


void *rm_session_delta_rx_f_local(void *arg)
{
    FILE                            *f_y;           /* reference file, on which reconstruction is performed */
    FILE                            *f_z;           /* result file */
    struct rm_session_push_local    *prvt_local;
    twfifo_queue                    *q;
    const struct rm_delta_e         *delta_e;       /* iterator over delta elements */
    struct twlist_head              *lh;
    size_t                          bytes_to_rx;
    struct rm_session               *s;
    struct rm_delta_reconstruct_ctx rec_ctx = {0};  /* describes result of reconstruction, we will copy this to session reconstruct context after all is done to avoid locking on each delta element */
    int err;
    enum rm_rx_status               status = RM_RX_STATUS_OK;

    s = (struct rm_session*) arg;
    if (s == NULL) {
        status = RM_RX_STATUS_INTERNAL_ERR;
        goto err_exit;
    }
    assert(s != NULL);
    memcpy(&rec_ctx, &s->rec_ctx, sizeof(struct rm_delta_reconstruct_ctx));

    pthread_mutex_lock(&s->mutex);
    if (s->type != RM_PUSH_LOCAL) {
        pthread_mutex_unlock(&s->mutex);
        status = RM_RX_STATUS_INTERNAL_ERR;
        goto err_exit;
    }
    prvt_local = s->prvt;
    if (prvt_local == NULL) {
        pthread_mutex_unlock(&s->mutex);
        status = RM_RX_STATUS_INTERNAL_ERR;
        goto err_exit;
    }
    assert(prvt_local != NULL);

    bytes_to_rx = s->f_x_sz;
    f_y         = s->f_y;
    f_z         = s->f_z;
    rec_ctx.L = s->rec_ctx.L; /* init reconstruction context */
    pthread_mutex_unlock(&s->mutex);

    assert(f_y != NULL);
    assert(f_z != NULL);
    if (f_y == NULL || f_z == NULL) {
        status = RM_RX_STATUS_INTERNAL_ERR;
        goto err_exit;
    }
    if (bytes_to_rx == 0)
        goto done;

    pthread_mutex_lock(&prvt_local->tx_delta_e_queue_mutex); /* sleep on delta queue and reconstruct element once awoken */
    q = &prvt_local->tx_delta_e_queue;

    while (bytes_to_rx > 0) {
        if (bytes_to_rx == 0) { /* checking for missing signal is not really needed here, as bytes_to_rx is local variable, nevertheless... */
            pthread_mutex_unlock(&prvt_local->tx_delta_e_queue_mutex);
            goto done;
        }
        /* process delta element */
        for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh)) { /* in case of local sync there can be only single element enqueued each time conditional variable is signaled, but in other cases it will be possible to be different most likely */
            delta_e = tw_container_of(lh, struct rm_delta_e, link);
            err = rm_rx_process_delta_element(delta_e, f_y, f_z, &rec_ctx);
            if (err != 0) {
                pthread_mutex_unlock(&prvt_local->tx_delta_e_queue_mutex);
                status = RM_RX_STATUS_DELTA_PROC_FAIL;
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
    pthread_mutex_lock(&s->mutex);
    assert(rec_ctx.rec_by_ref + rec_ctx.rec_by_raw == s->f_x_sz);
    assert(rec_ctx.delta_tail_n == 0 || rec_ctx.delta_tail_n == 1);
    rec_ctx.collisions_1st_level = s->rec_ctx.collisions_1st_level; /* tx thread might have assigned to collisions variables already and memcpy would overwrite them */
    rec_ctx.collisions_2nd_level = s->rec_ctx.collisions_2nd_level;
    rec_ctx.copy_all_threshold_fired = s->rec_ctx.copy_all_threshold_fired; /* tx thread might have assigned to threshold_fired variables already and memcpy would overwrite them */
    rec_ctx.copy_tail_threshold_fired = s->rec_ctx.copy_tail_threshold_fired;
    memcpy(&s->rec_ctx, &rec_ctx, sizeof(struct rm_delta_reconstruct_ctx));
    prvt_local->delta_rx_status = RM_RX_STATUS_OK;
    pthread_mutex_unlock(&s->mutex);
    return NULL; /* this thread must be created in joinable state */

err_exit:
    pthread_mutex_lock(&s->mutex);
    prvt_local->delta_rx_status = status;
    pthread_mutex_unlock(&s->mutex);
    return NULL; /* this thread must be created in joinable state */
}

void* rm_session_delta_rx_f_remote(void *arg)
{
    FILE                            *f_y;           /* file on which reconstruction is performed */
    /*twfifo_queue                    *q; */
    /*const struct rm_delta_e         *delta_e;        iterator over delta elements */
    /*struct twlist_head              *lh;*/
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

    pthread_mutex_lock(&s->mutex);
    bytes_to_rx = s->f_x_sz;
    /* f_y         = prvt_rx->f_y; */
    pthread_mutex_unlock(&s->mutex);

    if (bytes_to_rx == 0) {
        goto done;
    }

    /* TODO read delta elements from rx socket and do reconstruction */

exit:
    return NULL;
done:
    return NULL;
}
