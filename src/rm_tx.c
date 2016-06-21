/*
 * @file       rm_tx.c
 * @brief      Definitions used by rsync transmitter (A).
 * @author     Piotr Gregor <piotrek.gregor at gmail.com>
 * @version    0.1.2
 * @date       2 Jan 2016 11:19 AM
 * @copyright  LGPLv2.1
 */


#include "rm_tx.h"
#include "rm_rx.h"


int
rm_tx_local_push(const char *x, const char *y, size_t L, size_t copy_all_threshold,
        size_t copy_tail_threshold, size_t send_threshold, rm_push_flags flags, struct rm_delta_reconstruct_ctx *rec_ctx) {
    int         err;
    FILE        *f_x;   /* original file, to be synced into @y */
    FILE        *f_y;   /* file for taking non-overlapping blocks */
    FILE        *f_z;   /* result (with same name as @y) */
    int         fd_x, fd_y, fd_z;
    uint8_t     f_y_used;
    struct      stat	fs;
    size_t      x_sz, y_sz, z_sz, blocks_n_exp, blocks_n;
    size_t                          bkt;    /* hashtable deletion */
    const struct rm_ch_ch_ref_hlink *e;
    struct twhlist_node             *tmp;
    struct rm_session               *s;
    struct rm_session_push_local    *prvt;
    const char *f_z_name = "f_z_tmp";

    f_x = f_y = f_z = NULL;
    f_y_used = 0;
    s = NULL;
    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);

    if (x == NULL || y == NULL || rec_ctx == NULL) {
        return -1;
    }
    f_x = fopen(x, "rb");
    if (f_x == NULL) {
        return -2;
    }
    f_y = fopen(y, "rb");
    if (f_y != NULL) {
        f_y_used = 1;
    } else {
        if (flags & RM_BIT_0) { /* force creation if @y doesn't exist? */
            f_z = fopen(y, "wb+");
            if (f_z == NULL) {
                err = -3;
                goto err_exit;
            }
        } else {
            err = -4;
            goto err_exit;
        }
    }
    /* @y doesn't exist but --forced specified (f_y == NULL) or @y exists and is opened for reading  (f_y != NULL) */

	/* get input file size */
    fd_x = fileno(f_x);
    memset(&fs, 0, sizeof(fs));
    if (fstat(fd_x, &fs) != 0) {
        err = -5;
        goto err_exit;
    }
    x_sz = fs.st_size;

    if (f_y != NULL) {  /* if reference file exists, split it and calc checksums */
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {
            err = -6;
            goto err_exit;
        }
        y_sz = fs.st_size;

        blocks_n_exp = y_sz / L + (y_sz % L ? 1 : 0);   /* split @y file into non-overlapping blocks and calculate checksums on these blocks, expected number of blocks is */
        err = rm_rx_insert_nonoverlapping_ch_ch_ref(f_y, y, h, L, NULL, blocks_n_exp, &blocks_n);
        if (err != 0) {
            err = -7;
            goto  err_exit;
        }
        assert (blocks_n == blocks_n_exp && "rm_tx_local_push ASSERTION failed  indicating ERROR in blocks count either here or in rm_rx_insert_nonoverlapping_ch_ch_ref");
    } else { /* @y doesn't exist and --forced flag is specified */
        err = rm_copy_buffered(f_x, f_z, x_sz);
        if (err < 0) {
            err = -8;
            goto err_exit;
        }
        if (rec_ctx != NULL) {  /* fill in reconstruction context if given */
            rec_ctx->method = RM_RECONSTRUCT_METHOD_COPY_BUFFERED;
            rec_ctx->delta_raw_n = 1;
            rec_ctx->rec_by_raw = x_sz;
        }
        goto done;
    }

    /* Do NOT fclose(f_y); as it must be opened in session rx for reading */
    /* TODO generate unique temporary name based on session uuid for result file, after reconstruction is finished remove @f_y and rename @f_z to @y */
    f_z = fopen(f_z_name, "wb+");  /* and open @f_z for reading and writing */
    if (f_z == NULL) {
        err = -9;
        goto err_exit;
    }

    s = rm_session_create(RM_PUSH_LOCAL, L);    /* calc rolling checksums, produce delta vector and do file reconstruction in local session */
    if (s == NULL) {
        err = -10;
        goto err_exit;
    }
    memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx));    /* init reconstruction context */
    s->rec_ctx.method = RM_RECONSTRUCT_METHOD_DELTA_RECONSTRUCTION;
    s->rec_ctx.L = L;
    s->rec_ctx.copy_all_threshold = copy_all_threshold;
    s->rec_ctx.copy_tail_threshold = copy_tail_threshold;
    s->rec_ctx.send_threshold = send_threshold;
    prvt = s->prvt; /* setup private session's arguments */
    prvt->h = h;
    prvt->f_x = f_x;
    prvt->f_y = f_y;
    prvt->f_z = f_z;
    prvt->delta_f = rm_roll_proc_cb_1;
    prvt->f_x_sz = x_sz;
 
    err = rm_launch_thread(&prvt->delta_tx_tid, rm_session_delta_tx_f, s, PTHREAD_CREATE_JOINABLE); /* start tx delta vec thread (enqueue delta elements and signal to delta_rx_tid thread */
    if (err != 0) {
        err = -11;
        goto err_exit;
    }
    err = rm_launch_thread(&prvt->delta_rx_tid, rm_session_delta_rx_f_local, s, PTHREAD_CREATE_JOINABLE);   /* reconstruct */
    if (err != 0) {
        err = -12;
        goto err_exit;
    }
    pthread_join(prvt->delta_tx_tid, NULL);
    pthread_join(prvt->delta_rx_tid, NULL);

done:

    if (f_y_used == 1) {
        if (f_y != NULL) {
            fclose(f_y);
            f_y = NULL;
        }
        bkt = 0;
        twhash_for_each_safe(h, bkt, tmp, e, hlink) {
            twhash_del((struct twhlist_node*)&e->hlink);
            free((struct rm_ch_ch_ref_hlink*)e);
        }
    }
    if (f_x != NULL) {
        fclose(f_x);
        f_x = NULL;
    }
    fflush(f_z);
    fd_z = fileno(f_z);
    memset(&fs, 0, sizeof(fs));
    if (fstat(fd_z, &fs) != 0) {
        err = -13;
        goto err_exit;
    }
    fclose(f_z);
    f_z = NULL;
    z_sz = fs.st_size;
    if (z_sz != x_sz) {
        err =  -14;
        goto err_exit;
    }
    if (s != NULL) {
        if (z_sz != s->rec_ctx.rec_by_ref + s->rec_ctx.rec_by_raw) {
            err = -15;
            goto err_exit;
        }
        memcpy(rec_ctx, &s->rec_ctx, sizeof (struct rm_delta_reconstruct_ctx));
        rm_session_free(s);
        s = NULL;
    } else {
        if (z_sz != rec_ctx->rec_by_raw) {
            err = -16;
            goto err_exit;
        }
    }
    if (f_y_used == 1) {
        if (unlink(y) != 0) {
            err = -17;
            goto err_exit;
        }
        if (rename(f_z_name, y) == -1) {
            err = -18;
            goto err_exit;
        }
    }
    return 0;

err_exit:
    if (f_x != NULL) {
        fclose(f_x);
    }
    if (f_y != NULL) {
        fclose(f_y);
    }
    if (f_z != NULL) {
        fclose(f_z);
    }
    if (f_y_used == 1) {
        bkt = 0;
        twhash_for_each_safe(h, bkt, tmp, e, hlink) {
            twhash_del((struct twhlist_node*)&e->hlink);
            free((struct rm_ch_ch_ref_hlink*)e);
        }
    }
    if (s != NULL) {
        memcpy(rec_ctx, &s->rec_ctx, sizeof (struct rm_delta_reconstruct_ctx));
        rm_session_free(s);
        s = NULL;
    }
    return err;
}

int
rm_tx_remote_push(const char *x, const char *y,
		struct sockaddr_in *remote_addr,
		uint32_t L) {
    (void) x;
    (void) y;
    (void) remote_addr;
    (void) L;
	return 0;
}
