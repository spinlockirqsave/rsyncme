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
rm_tx_local_push(const char *x, const char *y,
        uint32_t L, rm_push_flags flags)
{
	int         err;
	FILE        *f_x;   /* original file, to be synced into @y */
    FILE        *f_y;   /* file for taking non-overlapping blocks */
    FILE        *f_z;   /* result (with same name as @y) */
	int         fd_x, fd_y, fd_z;
	struct      stat	fs;
	uint32_t    x_sz, y_sz, blocks_n_exp;
    size_t      blocks_n;
    //struct rm_session_delta_tx_f_arg    delta_tx_f_arg;
    struct rm_session               *s;
    struct rm_session_push_local    *prvt;

	(void) x_sz;
    (void) fd_z;
    f_x = f_y = f_z = NULL;
	TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    /*struct twlist_head      l = TWLIST_HEAD_INIT(l);*/

    f_x = fopen(x, "rb");
	if (f_x == NULL)
        return -1;

	f_y = fopen(y, "rb");
	if (f_y == NULL)
	{
        if (flags & RM_BIT_0) /* force creation if @y doesn't exist? */
        {
            f_z = fopen(y, "wb+");
            if (f_z == NULL)
            {
                err = -2;
                goto err_exit;
            }
        } else {
		    err = -3;
            goto err_exit;
	    }
    }
    /* @y doesn't exist but --forced specified (f_y == NULL)
     * or @y exists and is opened for reading  (f_y != NULL) */

	/* get input file size */
	fd_x = fileno(f_x);
	if (fstat(fd_x, &fs) != 0)
	{
		err = -4;
		goto err_exit;
	}
	x_sz = fs.st_size;

    /* get nonoverlapping checksums if we have @y */
    /*TWINIT_LIST_HEAD(&l);*/
	if (f_y != NULL)
    {
        /* reference file exists, split it and calc checksums */
        fd_y = fileno(f_y);
	    if (fstat(fd_y, &fs) != 0)
	    {
		    err = -5;
		    goto err_exit;
	    }
	    y_sz = fs.st_size;

	    /* split @y file into non-overlapping blocks
        * and calculate checksums on these blocks,
        * expected number of blocks is */
	    blocks_n_exp = y_sz / L + (y_sz % L ? 1 : 0);
		err = rm_rx_insert_nonoverlapping_ch_ch_ref(
                f_y, y, h, L, NULL, blocks_n_exp, &blocks_n);
        if (err != 0)
        {
            err = -6;
            goto  err_exit;
        }
        assert (blocks_n == blocks_n_exp && "rm_tx_local_push ASSERTION failed"
                " indicating ERROR in blocks count either here "
                "or in rm_rx_insert_nonoverlapping_ch_ch_ref");
    } else {
        /* @y doesn't exist and --forced flag is specified */
        err = rm_copy_buffered(f_x, f_z, x_sz);
        if (err < 0)
        {
           err = -7;
           goto err_exit;
        }
    }

    /* calc rolling checksums, produce delta vector
     * and do file reconstruction in local session */

    s = rm_session_create(RM_PUSH_LOCAL);
    if (s == NULL)
    {
        err = -8;
        goto err_exit;
    }
    /* setup private session's arguments */
    prvt = s->prvt;
    prvt->h = h;
    prvt->f_x = f_x;
    prvt->delta_f = rm_roll_proc_cb_1;
 
	/* start tx delta vec thread (enqueue delta elements
     * and signal to delta_rx_tid thread */
	err = rm_launch_thread(&prvt->delta_tx_tid, rm_session_delta_tx_f,
                                    s, PTHREAD_CREATE_JOINABLE);
	if (err != 0)
    {
        err = -9;
        goto err_exit;
    }
    /* reconstruct */
	err = rm_launch_thread(&prvt->delta_rx_tid, rm_session_delta_rx_f,
                                    s, PTHREAD_CREATE_JOINABLE);
	if (err != 0)
    {
        err = -10;
        goto err_exit;
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
	if (s != NULL) {
		rm_session_free(s);
    }
	return err;
}

int
rm_tx_remote_push(const char *x, const char *y,
		struct sockaddr_in *remote_addr,
		uint32_t L)
{
    (void) x;
    (void) y;
    (void) remote_addr;
    (void) L;
	return 0;
}
