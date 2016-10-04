/* @file        rm_tx.c
 * @brief       Definitions used by rsync transmitter (A).
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        2 Jan 2016 11:19 AM
 * @copyright   LGPLv2.1 */


#include "rm_tx.h"
#include "rm_rx.h"


enum rm_error
rm_tx_local_push(const char *x, const char *y, const char *z, size_t L, size_t copy_all_threshold,
        size_t copy_tail_threshold, size_t send_threshold, rm_push_flags flags, struct rm_delta_reconstruct_ctx *rec_ctx) {
    enum rm_error  err = RM_ERR_OK;
    FILE        *f_x = NULL;   /* original file, to be synced into @y */
    FILE        *f_y = NULL;   /* file for taking non-overlapping blocks */
    FILE        *f_z = NULL;   /* result (with same name as @y) */
    int         fd_x, fd_y, fd_z;
    uint8_t     reference_file_exist = 0;
    struct stat fs;
    size_t      x_sz, y_sz, z_sz, blocks_n_exp, blocks_n;
    size_t                          bkt;    /* hashtable deletion */
    const struct rm_ch_ch_ref_hlink *e;
    struct twhlist_node             *tmp;
    struct rm_session               *s = NULL;
    struct rm_session_push_local    *prvt;
    /*char                            *y_copy = NULL; *cwd = NULL;*/
    char                            f_z_name[38];
    struct timespec         clk_realtime_start, clk_realtime_stop;
    double                  clk_cputime_start, clk_cputime_stop;
    struct timespec         real_time;
    double                  cpu_time;
    twfifo_queue            *q;
    const struct rm_delta_e *delta_e;
    struct twlist_head      *lh;

    if ((x == NULL) || (y == NULL) || (L == 0) || (rec_ctx == NULL) || (send_threshold == 0)) {
        return RM_ERR_BAD_CALL;
    }

    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);

    /*cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        return RM_ERR_GETCWD;
    }*/
    /*y_copy = strdup(y);
    if (y_copy == NULL) {
        free(cwd);
        return RM_ERR_MEM;
    }*/
    /*if (chdir(dirname(y_copy)) != 0) {
        free(y_copy);
        y_copy = NULL;
        free(cwd);
        cwd = NULL;
        return RM_ERR_CHDIR;
    }*/
    f_x = fopen(x, "rb");
    if (f_x == NULL) {
        /*free(y_copy);
        y_copy = NULL;
        chdir(cwd);
        free(cwd);
        cwd = NULL;*/
        return RM_ERR_OPEN_X;
    }
    fd_x = fileno(f_x);     /* get input file size */
    memset(&fs, 0, sizeof(fs));
    if (fstat(fd_x, &fs) != 0) {
        err = RM_ERR_FSTAT_X;
        goto err_exit;
    }
    x_sz = fs.st_size;

    f_y = fopen(y, "rb");
    if (f_y != NULL) { /* if reference file exists, split it and calc checksums */
        reference_file_exist = 1;
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {
            err = RM_ERR_FSTAT_Y;
            goto err_exit;
        }
        y_sz = fs.st_size;

        blocks_n_exp = y_sz / L + (y_sz % L ? 1 : 0);   /* split @y file into non-overlapping blocks and calculate checksums on these blocks, expected number of blocks is */
        if (rm_rx_insert_nonoverlapping_ch_ch_ref(f_y, y, h, L, NULL, blocks_n_exp, &blocks_n) != RM_ERR_OK) {
            err = RM_ERR_NONOVERLAPPING_INSERT;
            goto  err_exit;
        }
        assert (blocks_n == blocks_n_exp && "rm_tx_local_push ASSERTION failed  indicating ERROR in blocks count either here or in rm_rx_insert_nonoverlapping_ch_ch_ref");
    } else {
        if (flags & RM_BIT_4) { /* force creation if @y doesn't exist? */
            if (z != NULL) { /* use different name? */
                f_z = fopen(z, "w+b");
            } else {
                f_z = fopen(y, "w+b");
            }
            if (f_z == NULL) {
                err = RM_ERR_OPEN_Z;
                goto err_exit;
            }
            clock_gettime(CLOCK_REALTIME, &clk_realtime_start);
            clk_cputime_start = clock() / CLOCKS_PER_SEC;
            if (rm_copy_buffered(f_x, f_z, x_sz) != RM_ERR_OK) { /* @y doesn't exist and --forced flag is specified */
                err = RM_ERR_COPY_BUFFERED;
                goto err_exit;
            }
            if (rec_ctx != NULL) {  /* fill in reconstruction context if given */
                rec_ctx->method = RM_RECONSTRUCT_METHOD_COPY_BUFFERED;
                rec_ctx->delta_raw_n = 1;
                rec_ctx->rec_by_raw = x_sz;
            }
            goto done;
        } else {
            err = RM_ERR_OPEN_Y;
            goto err_exit;
        }
    }
    /* @y exists and is opened for reading  (f_y != NULL), reference_file_exist == 1 */
    /* Do NOT fclose(f_y); as it must be opened in session rx for reading */

    rm_get_unique_string(f_z_name);
    f_z = fopen(f_z_name, "wb+");  /* and open @f_z for reading and writing in @z path */
    if (f_z == NULL) {
        err = RM_ERR_OPEN_TMP;
        goto err_exit;
    }

    s = rm_session_create(RM_PUSH_LOCAL);    /* calc rolling checksums, produce delta vector and do file reconstruction in local session */
    if (s == NULL) {
        err = RM_ERR_CREATE_SESSION;
        goto err_exit;
    }
    s->rec_ctx.method = RM_RECONSTRUCT_METHOD_DELTA_RECONSTRUCTION;
    s->rec_ctx.L = L;
    s->rec_ctx.copy_all_threshold = copy_all_threshold;
    s->rec_ctx.copy_tail_threshold = copy_tail_threshold;
    s->rec_ctx.send_threshold = send_threshold;
    prvt = s->prvt; /* setup private session's arguments */
    prvt->h = h;
    s->f_x = f_x;
    s->f_y = f_y;
    s->f_z = f_z;
    prvt->delta_f = rm_roll_proc_cb_1;
    s->f_x_sz = x_sz;
 
    err = rm_launch_thread(&prvt->delta_tx_tid, rm_session_delta_tx_f, s, PTHREAD_CREATE_JOINABLE); /* start tx delta vec thread (enqueue delta elements and signal to delta_rx_tid thread */
    if (err != RM_ERR_OK) {
        err = RM_ERR_DELTA_TX_THREAD_LAUNCH;
        goto err_exit;
    }
    err = rm_launch_thread(&prvt->delta_rx_tid, rm_session_delta_rx_f_local, s, PTHREAD_CREATE_JOINABLE);   /* reconstruct */
    if (err != RM_ERR_OK) {
        err = RM_ERR_DELTA_RX_THREAD_LAUNCH;
        goto err_exit;
    }
    pthread_join(prvt->delta_tx_tid, NULL);
    pthread_join(prvt->delta_rx_tid, NULL);
    if (prvt->delta_tx_status != RM_DELTA_TX_STATUS_OK) {
        err = RM_ERR_DELTA_TX_THREAD;
    }
    if (prvt->delta_rx_status != RM_DELTA_RX_STATUS_OK) {
        err = RM_ERR_DELTA_RX_THREAD;
    }

done:

    if (f_x != NULL) {
        fclose(f_x);
        f_x = NULL;
    }
    fflush(f_z);
    fd_z = fileno(f_z);
    memset(&fs, 0, sizeof(fs));
    if (fstat(fd_z, &fs) != 0) {
        err = RM_ERR_FSTAT_Z;
        goto err_exit;
    }
    fclose(f_z);
    f_z = NULL;
    z_sz = fs.st_size;
    if (z_sz != x_sz) {
        err =  RM_ERR_FILE_SIZE;
        goto err_exit;
    }
    if (reference_file_exist == 1) {            /* delta reconstruction has happened */
        if (f_y != NULL) {
            fclose(f_y);
            f_y = NULL;
        }
        bkt = 0;
        twhash_for_each_safe(h, bkt, tmp, e, hlink) {
            twhash_del((struct twhlist_node*)&e->hlink);
            free((struct rm_ch_ch_ref_hlink*)e);
        }
        if (z_sz != s->rec_ctx.rec_by_ref + s->rec_ctx.rec_by_raw) {
            err = RM_ERR_FILE_SIZE_REC_MISMATCH;
            goto err_exit;
        }
        s->clk_cputime_stop = (double) clock() / CLOCKS_PER_SEC; 
        cpu_time = s->clk_cputime_stop - s->clk_cputime_start;
        clock_gettime(CLOCK_REALTIME, &s->clk_realtime_stop);    
        real_time.tv_sec = s->clk_realtime_stop.tv_sec - s->clk_realtime_start.tv_sec; 
        real_time.tv_nsec = s->clk_realtime_stop.tv_nsec - s->clk_realtime_start.tv_nsec; 
        memcpy(rec_ctx, &s->rec_ctx, sizeof (struct rm_delta_reconstruct_ctx));

        /* queue of delta elements MUST be empty now */
        assert(twlist_empty(&prvt->tx_delta_e_queue) != 0 && "Delta elements queue NOT EMPTY!\n");
        q = &prvt->tx_delta_e_queue;
        if (!twlist_empty(q)) {
            err = RM_ERR_QUEUE_NOT_EMPTY;
            goto err_exit;
        }
        rm_session_free(s);
        s = NULL;
        if (((flags & RM_BIT_6) == 0u) && (unlink(y) != 0)) { /* if --leave not set and unlink failed */
            err = RM_ERR_UNLINK_Y;
            goto err_exit;
        }
        if (z != NULL) { /* use different name? */
            if (rename(f_z_name, z) == -1) {
                err = RM_ERR_RENAME_TMP_Z;
                goto err_exit;
            }
        } else {
            if (rename(f_z_name, y) == -1) {
                err = RM_ERR_RENAME_TMP_Y;
                goto err_exit;
            }
        }
    } else {                                    /* copy buffered has happened */
        if (z_sz != rec_ctx->rec_by_raw) {
            err = RM_ERR_FILE_SIZE_REC_MISMATCH;
            goto err_exit;
        }
        clk_cputime_stop = (double) clock() / CLOCKS_PER_SEC; 
        cpu_time = clk_cputime_stop - clk_cputime_start;
        clock_gettime(CLOCK_REALTIME, &clk_realtime_stop);    
        real_time.tv_sec = clk_realtime_stop.tv_sec - clk_realtime_start.tv_sec; 
        real_time.tv_nsec = clk_realtime_stop.tv_nsec - clk_realtime_start.tv_nsec; 
    }
    rec_ctx->time_cpu = cpu_time;
    rec_ctx->time_real = real_time;
    /*if (y_copy != NULL) {
        free(y_copy);
        y_copy = NULL;
    }*/
    if (f_z != NULL) {
        fclose(f_z);
        f_z = NULL;
    }
    /*chdir(cwd);
    if (cwd != NULL) {
        free(cwd);
        cwd = NULL;
    }*/
    return RM_ERR_OK;

err_exit:
    /*if (y_copy != NULL) {
        free(y_copy);
        y_copy = NULL;
    }*/
    if (f_x != NULL) {
        fclose(f_x);
        f_x = NULL;
    }
    if (f_y != NULL) {
        fclose(f_y);
        f_y = NULL;
    }
    if (f_z != NULL) {
        fclose(f_z);
        f_z = NULL;
    }
    if (reference_file_exist == 1) {
        bkt = 0;
        twhash_for_each_safe(h, bkt, tmp, e, hlink) {
            twhash_del((struct twhlist_node*)&e->hlink);
            free((struct rm_ch_ch_ref_hlink*)e);
        }
    }
    if (s != NULL) {
        memcpy(rec_ctx, &s->rec_ctx, sizeof (struct rm_delta_reconstruct_ctx));
        if (prvt != NULL) {
            q = &prvt->tx_delta_e_queue;
            if (!twlist_empty(q)) {
                for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh)) {    /* dequeue, so can free */
                    delta_e = tw_container_of(lh, struct rm_delta_e, link);
                    free((void*)delta_e);
                }
            }
        }
        rm_session_free(s);
        s = NULL;
    }
    /*if (cwd != NULL) {
        chdir(cwd);
        free(cwd);
        cwd = NULL;
    }*/
    return err;
}

int
rm_tx_remote_push(const char *x, const char *y, const char *z, size_t L, size_t copy_all_threshold,
        size_t copy_tail_threshold, size_t send_threshold, rm_push_flags flags,
        struct rm_delta_reconstruct_ctx *rec_ctx, const char *addr, uint16_t port, uint16_t timeout_s, uint16_t timeout_us, const char **err_str) {
    enum rm_error  err = RM_ERR_OK;
    FILE        *f_x = NULL;   /* original file, to be synced into @y */
    int         fd_x;
    struct stat fs;
    size_t      x_sz;
    struct rm_session               *s = NULL;
    struct rm_session_push_tx       *prvt = NULL;

    struct rm_msg_push  msg;
    unsigned char       *msg_raw = NULL;

    (void) y;
    (void) z;
    (void) copy_all_threshold;
    (void) copy_tail_threshold;
    (void) flags;

    if ((x == NULL) || (L == 0) || (rec_ctx == NULL) || (send_threshold == 0)) {
        return RM_ERR_BAD_CALL;
    }

    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);

    f_x = fopen(x, "rb");
    if (f_x == NULL) {
        return RM_ERR_OPEN_X;
    }
    fd_x = fileno(f_x); /* get input file size */
    memset(&fs, 0, sizeof(fs));
    if (fstat(fd_x, &fs) != 0) {
        err = RM_ERR_FSTAT_X;
        goto err_exit;
    }
    x_sz = fs.st_size;
    if (x_sz == 0) {
        return RM_ERR_X_ZERO_SIZE;
    }

    s = rm_session_create(RM_PUSH_TX);  /* calc rolling checksums, produce delta vector and do file reconstruction in local session */
    if (s == NULL) {
        err = RM_ERR_CREATE_SESSION;
        goto err_exit;
    }

    prvt = s->prvt;
    err = rm_tcp_connect_nonblock_timeout(&prvt->fd, addr, port, AF_INET, timeout_s, timeout_us, err_str);
    if (err != RM_ERR_OK) {
        goto err_exit;
    }

    memset(&msg, 0, sizeof msg);
    if (rm_msg_push_init(&msg) != RM_ERR_OK) {
        return RM_ERR_MEM;
    }
    msg.hdr->pt = RM_PT_MSG_PUSH;
    msg.hdr->flags = flags;
    memcpy(msg.ssid, s->id, RM_UUID_LEN);
    msg.L = L;

    msg.x_sz = strlen(x) + 1;
    strcpy(msg.x, x); /* commandline tool will not pass here string longer than RM_FILE_LEN_MAX which is also the size of file name buffers in msg push */
    if (y != NULL) {
        msg.y_sz = strlen(y) + 1;
        strcpy(msg.y, y); /* copies the string including the NULL terminator */
    } else {
        msg.y_sz = 0;
    }
    if (z != NULL) {
        msg.z_sz = strlen(z) + 1;
        strcpy(msg.z, z);
    } else {
        msg.z_sz = 0;
    }
    msg.hdr->len = rm_calc_msg_len(&msg);
    msg.hdr->hash = rm_core_hdr_hash(msg.hdr);

    /* TODO malloc raw buffer for push message */
    msg_raw = malloc(msg.hdr->len);
    if (msg_raw == NULL) {
        return RM_ERR_TOO_MUCH_REQUESTED;
    }
    /* TODO add serialization of all fields of push message */
    rm_serialize_msg_push(msg_raw, &msg);
    err = rm_tcp_write(prvt->fd, msg_raw, msg.hdr->len);
    if (err != RM_ERR_OK) {
        return RM_ERR_WRITE;
    }


    rm_session_free(s);
    s = NULL;
    free(msg_raw);
    msg_raw = NULL;
    free(msg.hdr);
    msg.hdr = NULL;

    return RM_ERR_OK;

err_exit:
    if (f_x != NULL) {
        fclose(f_x);
        f_x = NULL;
    }
    if (s != NULL) {
        rm_session_free(s);
        s = NULL;
    }
    if (msg_raw != NULL) {
        free(msg_raw);
        msg_raw = NULL;
    }
    if (msg.hdr != NULL) {
        free(msg.hdr);
        msg.hdr = NULL;
    }
    return err;
}
