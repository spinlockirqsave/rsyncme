/* @file        rm_tx.c
 * @brief       Definitions used by rsync transmitter (A).
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        2 Jan 2016 11:19 AM
 * @copyright   LGPLv2.1 */


#include "rm_tx.h"
#include "rm_rx.h"


enum rm_error
rm_tx_local_push(const char *x, const char *y, const char *z, size_t L, size_t copy_all_threshold,
		size_t copy_tail_threshold, size_t send_threshold, rm_push_flags flags, struct rm_delta_reconstruct_ctx *rec_ctx, struct rm_tx_options *opt) {
	enum rm_error  err = RM_ERR_OK;
	FILE        *f_x = NULL;   /* original file, to be synced into @y */
	FILE        *f_y = NULL;   /* file for taking non-overlapping blocks */
	FILE        *f_z = NULL;   /* result (with same name as @y) */
	int         fd_x = -1, fd_y = -1, fd_z = -1;
	uint8_t     reference_file_exist = 0;
	struct stat fs;
	size_t      x_sz = 0, y_sz = 0, z_sz = 0, blocks_n_exp = 0, blocks_n = 0;
	size_t                          bkt = 0;    /* hashtable deletion */
	const struct rm_ch_ch_ref_hlink *e = NULL;
	struct twhlist_node             *tmp = NULL;
	struct rm_session               *s = NULL;
	struct rm_session_push_local    *prvt = NULL;
	/*char                            *y_copy = NULL; *cwd = NULL;*/
	char                            f_z_name[RM_UNIQUE_STRING_LEN];
	struct timespec         clk_realtime_start = {0}, clk_realtime_stop = {0};
	double                  clk_cputime_start = 0.0, clk_cputime_stop = 0.0;
	struct timespec         real_time = {0};
	double                  cpu_time = 0.0;
	twfifo_queue            *q = NULL;
	const struct rm_delta_e *delta_e = NULL;
	struct twlist_head      *lh = NULL;
	struct rm_core_options	core_opt = {0};

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
		if (rm_rx_insert_nonoverlapping_ch_ch_ref(0, f_y, y, h, L, NULL, blocks_n_exp, &blocks_n, NULL) != RM_ERR_OK) {
			err = RM_ERR_NONOVERLAPPING_INSERT;
			goto  err_exit;
		}
		assert(blocks_n == blocks_n_exp && "rm_tx_local_push ASSERTION failed  indicating ERROR in blocks count either here or in rm_rx_insert_nonoverlapping_ch_ch_ref");
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
			if (rm_copy_buffered(f_x, f_z, x_sz, NULL) != RM_ERR_OK) {								/* @y doesn't exist and --forced flag is specified */
				err = RM_ERR_COPY_BUFFERED;
				goto err_exit;
			}
			if (rec_ctx != NULL) {																	/* fill in reconstruction context if given */
				rec_ctx->method = RM_RECONSTRUCT_METHOD_COPY_BUFFERED;
				rec_ctx->delta_raw_n = 1;
				rec_ctx->rec_by_raw = x_sz;
				rec_ctx->msg_push_len = 0;
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

	core_opt.loglevel = opt->loglevel;
	s = rm_session_create(RM_PUSH_LOCAL, &core_opt);    /* calc rolling checksums, produce delta vector and do file reconstruction in local session */
	if (s == NULL) {
		err = RM_ERR_CREATE_SESSION;
		goto err_exit;
	}
	s->rec_ctx.method = RM_RECONSTRUCT_METHOD_DELTA_RECONSTRUCTION;
	s->rec_ctx.L = L;
	s->rec_ctx.copy_all_threshold = copy_all_threshold;
	s->rec_ctx.copy_tail_threshold = copy_tail_threshold;
	s->rec_ctx.send_threshold = send_threshold;
	s->rec_ctx.msg_push_len = 0;
	prvt = s->prvt; /* setup private session's arguments */
	prvt->h = h;
	s->f_x = f_x;
	s->f_y = f_y;
	s->f_z = f_z;
	prvt->delta_tx_f = rm_roll_proc_cb_1;
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
	if (prvt->delta_tx_status != RM_TX_STATUS_OK) {
		err = RM_ERR_DELTA_TX_THREAD;
	}
	if (prvt->delta_rx_status != RM_RX_STATUS_OK) {
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
		rm_util_calc_timespec_diff(&clk_realtime_start, &clk_realtime_stop, &real_time);
	}
	rec_ctx->time_cpu = cpu_time;
	rec_ctx->time_real = real_time;

	if (f_z != NULL) {
		fclose(f_z);
		f_z = NULL;
	}

	return RM_ERR_OK;

err_exit:

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

	return err;
}

int rm_tx_remote_push(const char *x, const char *y, const char *z, size_t L, size_t copy_all_threshold, size_t copy_tail_threshold, size_t send_threshold, rm_push_flags flags, struct rm_delta_reconstruct_ctx *rec_ctx, const char *addr, uint16_t port, uint16_t timeout_s, uint16_t timeout_us, const char **err_str, struct rm_tx_options *opt) {
	enum rm_error       err = RM_ERR_OK;
	FILE                *f_x = NULL;	/* original file, to be synced with @y */
	int					fd_x = -1;
	struct stat         fs = {0};
	size_t              x_sz = 0;
	struct rm_session           *s = NULL;
	struct rm_session_push_tx   *prvt = NULL;

	struct timespec         real_time = {0};
	double                  cpu_time = 0.0;

	struct rm_msg_push  msg = {0};
	unsigned char       *msg_raw = NULL;
	unsigned char       *buf = NULL;
	struct rm_msg_push_ack   ack = {0};
	memset(&ack, 0, sizeof(ack));

	struct rm_core_options	core_opt = {0};

	size_t                          bkt = 0;    /* hashtable deletion */
	const struct rm_ch_ch_ref_hlink *e = NULL;
	struct twhlist_node             *tmp = NULL;

	(void) y;
	(void) z;
	(void) flags;

	if ((x == NULL) || (L == 0) || (rec_ctx == NULL) || (send_threshold == 0)) {
		return RM_ERR_BAD_CALL;
	}

	TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);											/* synchronised between threads with sesion_local's hashtable mutex (h_mutex) */
	twhash_init(h);

	f_x = fopen(x, "rb");
	if (f_x == NULL) 
		return RM_ERR_OPEN_X;
	
	fd_x = fileno(f_x);                                                                         /* get input file size */
	memset(&fs, 0, sizeof(fs));
	if (fstat(fd_x, &fs) != 0) {
		err = RM_ERR_FSTAT_X;
		goto err_exit;
	}
	x_sz = fs.st_size;
	if (x_sz == 0) 
		return RM_ERR_X_ZERO_SIZE;

	core_opt.loglevel = opt->loglevel;
	core_opt.delta_conn_timeout_s = timeout_s;
	core_opt.delta_conn_timeout_us = timeout_us;
	s = rm_session_create(RM_PUSH_TX, &core_opt);                                               /* rx nonoverlapping checksums, calc rolling checksums, produce delta vector and tx to receiver */
	if (s == NULL) {
		err = RM_ERR_CREATE_SESSION;
		goto err_exit;
	}

	prvt = s->prvt;
	err = rm_tcp_connect_nonblock_timeout(&prvt->fd, addr, port, AF_INET, timeout_s, timeout_us, err_str);
	if (err != RM_ERR_OK) {
		goto err_exit;
	}

	memset(&msg, 0, sizeof(msg));
	if (rm_msg_push_alloc(&msg) != RM_ERR_OK)
		goto err_exit;                                                                          /* RM_ERR_MEM */

	msg.hdr->pt = RM_PT_MSG_PUSH;
	msg.hdr->flags = flags;
	memcpy(msg.ssid, s->id, RM_UUID_LEN);
	msg.L = L;
	msg.bytes = x_sz;																			/* bytes to be xferred by transmitter (by delta and/or by raw) */

	msg.x_sz = strlen(x) + 1;
	strcpy(msg.x, x);                                                                           /* commandline tool will not pass here string longer than RM_FILE_LEN_MAX which is also the size of file name buffers in msg push */
	if (y != NULL) {
		msg.y_sz = strlen(y) + 1;
		strcpy(msg.y, y);                                                                       /* copies the string including the NULL terminator */
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

	rec_ctx->msg_push_len = msg.hdr->len;

	msg_raw = malloc(msg.hdr->len);
	if (msg_raw == NULL)
		return RM_ERR_TOO_MUCH_REQUESTED;

	rm_serialize_msg_push(msg_raw, &msg);
	err = rm_tcp_write(prvt->fd, msg_raw, msg.hdr->len);											/* tx msg PUSH */
	if (err != RM_ERR_OK)
		goto err_exit;																				/* RM_ERR_WRITE */

	if (rm_msg_push_ack_alloc(&ack) != RM_ERR_OK)													/* prepare for incoming ACK, allocate space for header == ACK */
		goto err_exit;

	buf = malloc(RM_MSG_PUSH_ACK_LEN);																/* buffer for incoming raw message header */
	if (buf == NULL)
		goto err_exit;

	err = rm_tcp_rx(prvt->fd, buf, RM_MSG_ACK_LEN);													/* wait for incoming ACK, generic part */
	if (err != RM_ERR_OK) {																			/* RM_ERR_READ || RM_ERR_EOF */
		if (err == RM_ERR_EOF)
			err = RM_ERR_TCP_DISCONNECT;
		else
			err = RM_ERR_TCP;
		goto err_exit;
	}

	rm_deserialize_msg_ack(buf, &ack.ack);
	if (ack.ack.hdr->pt != RM_PT_MSG_PUSH_ACK) {                                                    /* if not MSG_PUSH_ACK, i.e. generic ACK describibg some error occurred on the receiver side */
		if (ack.ack.hdr->flags != RM_ERR_OK) {                                                      /* if request cannot be handled, maybe AUTH failure or RM_ERR_CHDIR_Z */
			err = ack.ack.hdr->flags;
			goto err_exit;
		} else {
			RM_LOG_CRIT("ACK of type [%u] with status [%u] not expected here", ack.ack.hdr->pt, ack.ack.hdr->flags);
		}
	}
	err = rm_tcp_rx(prvt->fd, buf + RM_MSG_ACK_LEN, RM_MSG_PUSH_ACK_LEN - RM_MSG_ACK_LEN);          /* wait for incoming MSG PUSH part of the ACK */
	if (err != RM_ERR_OK) {																			/* RM_ERR_READ || RM_ERR_EOF */
		if (err == RM_ERR_EOF)
			err = RM_ERR_TCP_DISCONNECT;
		else
			err = RM_ERR_TCP;
		goto err_exit;
	}

	err = rm_core_tcp_msg_ack_validate(buf, RM_MSG_PUSH_ACK_LEN);									/* validate potential ACK message: check header: hash, size and pt */
	if (err != RM_ERR_OK) { /* bad message */
		RM_LOG_ERR("Bad MSG_PUSH_ACK, error [%u]", err);
		switch (err) {
			case RM_ERR_FAIL: /* Invalid hash */
				RM_LOG_ERR("Invalid hash [%u]", ack.ack.hdr->hash);
				break;
			case RM_ERR_MSG_PT_UNKNOWN: /* Unknown message type */
				RM_LOG_ERR("Unknown payload type [%u]", ack.ack.hdr->pt);
				break;
			default: /* Unknown error */
				RM_LOG_CRIT("Unknown error [%u]", err);
				break;
		}
		goto err_exit;
	}

	rm_deserialize_msg_push_ack(buf, &ack);
	free(buf);
	buf = NULL;

	if (ack.ack.hdr->flags != RM_ERR_OK) {                                                          /* if request cannot be handled */
		err = ack.ack.hdr->flags;
		goto err_exit;
	}
	prvt->msg_push_ack = &ack;

	prvt->session_local.h = h;																		/* shared hashtable, assign pointer before launching checksums receiver thread */
	err = rm_launch_thread(&prvt->ch_ch_rx_tid, rm_session_ch_ch_rx_f, s, PTHREAD_CREATE_JOINABLE);	/* start RX nonoverlapping checksums thread (insert checksums into hashtable) */
	if (err != RM_ERR_OK) {
		err = RM_ERR_CH_CH_RX_THREAD_LAUNCH;
		goto err_exit;
	}

	s->rec_ctx.method = RM_RECONSTRUCT_METHOD_DELTA_RECONSTRUCTION;
	s->rec_ctx.L = L;
	s->rec_ctx.copy_all_threshold = copy_all_threshold;
	s->rec_ctx.copy_tail_threshold = copy_tail_threshold;
	s->rec_ctx.send_threshold = send_threshold;
	prvt->session_local.h = h;																		/* shared hashtable */
	s->f_x = f_x;
	s->f_y = NULL;
	s->f_z = NULL;
	prvt->session_local.delta_tx_f = rm_roll_proc_cb_1;												/* enqueue into local session's tx_delta_e_queue for delta_rx_tid thread consumption */
	s->f_x_sz = x_sz;

	err = rm_launch_thread(&prvt->session_local.delta_tx_tid, rm_session_delta_tx_f, s, PTHREAD_CREATE_JOINABLE); /* start tx delta vec thread (enqueue delta elements and signal to delta_rx_tid thread */
	if (err != RM_ERR_OK) {
		err = RM_ERR_DELTA_TX_THREAD_LAUNCH;
		goto err_exit;
	}
	err = rm_launch_thread(&prvt->session_local.delta_rx_tid, rm_session_delta_rx_f_local, s, PTHREAD_CREATE_JOINABLE);   /* TX enqueued deltas*/
	if (err != RM_ERR_OK) {
		err = RM_ERR_DELTA_RX_THREAD_LAUNCH;
		goto err_exit;
	}
	pthread_join(prvt->ch_ch_rx_tid, NULL);
	pthread_join(prvt->session_local.delta_tx_tid, NULL);
	pthread_join(prvt->session_local.delta_rx_tid, NULL);
	if (prvt->ch_ch_rx_status != RM_TX_STATUS_OK) {
		err = RM_ERR_CH_CH_RX_THREAD;
		goto err_exit;
	}
	if (prvt->session_local.delta_tx_status != RM_TX_STATUS_OK) {
		err = RM_ERR_DELTA_TX_THREAD;
		goto err_exit;
	}
	if (prvt->session_local.delta_rx_status != RM_RX_STATUS_OK) {
		err = RM_ERR_DELTA_RX_THREAD;
		goto err_exit;
	}

	pthread_mutex_lock(&s->mutex);
	if (s->f_x != NULL) {
		fclose(s->f_x);
		s->f_x = NULL;
	}
	if (s->f_y != NULL) {
		fclose(s->f_y);
		s->f_y = NULL;
	}

	s->clk_cputime_stop = (double) clock() / CLOCKS_PER_SEC; 
	cpu_time = s->clk_cputime_stop - s->clk_cputime_start;
	clock_gettime(CLOCK_REALTIME, &s->clk_realtime_stop);
	rm_util_calc_timespec_diff(&s->clk_realtime_start, &s->clk_realtime_stop, &real_time);
	s->rec_ctx.time_cpu = cpu_time;
	s->rec_ctx.time_real = real_time;

	memcpy(rec_ctx, &s->rec_ctx, sizeof (struct rm_delta_reconstruct_ctx));

	pthread_mutex_unlock(&s->mutex);
	rm_session_free(s);
	s = NULL;

	free(msg_raw);
	msg_raw = NULL;
	free(msg.hdr);
	msg.hdr = NULL;
	free(ack.ack.hdr);
	ack.ack.hdr = NULL;

	bkt = 0;
	twhash_for_each_safe(h, bkt, tmp, e, hlink) {
		twhash_del((struct twhlist_node*)&e->hlink);
		free((struct rm_ch_ch_ref_hlink*)e);
	}

	return RM_ERR_OK;

err_exit:

	if (ack.ack.hdr != NULL) {
		free(ack.ack.hdr);
		ack.ack.hdr = NULL;
	}
	if (buf != NULL) {
		free(buf);
		buf = NULL;
	}

	switch (err) {

		case RM_ERR_DELTA_RX_THREAD:

			if (prvt->session_local.delta_rx_status == RM_RX_STATUS_CONNECT_TIMEOUT)
				RM_LOG_ERR("%s", "Connection timeout\n");
			else if (prvt->session_local.delta_rx_status == RM_RX_STATUS_CONNECT_REFUSED)
				RM_LOG_ERR("%s", "Connection refused\n");
			else if (prvt->session_local.delta_rx_status == RM_RX_STATUS_CONNECT_HOSTUNREACH)
				RM_LOG_ERR("%s", "Host unreachable\n");
			else if (prvt->session_local.delta_rx_status == RM_RX_STATUS_CONNECT_GEN_ERR)
				RM_LOG_ERR("%s", "Connection error\n");
			else
				RM_LOG_ERR("%s", "General connection error\n");
			break;

		case RM_ERR_CH_CH_RX_THREAD:

			if (prvt->ch_ch_rx_status == RM_RX_STATUS_CH_CH_RX_TCP_DISCONNECT)
				RM_LOG_ERR("%s", "Receiver closed checksums channel prematurely\n");
			else
				RM_LOG_ERR("%s", "Error on checksums channel\n");
			break;

		case RM_ERR_TCP:
			RM_LOG_ERR("%s", "TCP error on connection with receiver\n");
			break;

		case RM_ERR_TCP_DISCONNECT:
			RM_LOG_ERR("%s", "Receiver closed connection prematurely\n");
			break;

		case RM_ERR_MEM:
			RM_LOG_ERR("%s", "Not enough memory\n");
			break;

		case RM_ERR_WRITE:
			/* TODO bad... */
			RM_LOG_ERR("%s", "Can't write\n");
			break;

		default:
			break;
	}

	if (s->f_x != NULL) {
		fclose(s->f_x);
		s->f_x = NULL;
	}
	if (s->f_y != NULL) {
		fclose(s->f_y);
		s->f_y = NULL;
	}
	if (s->f_z != NULL) {
		fflush(s->f_z);
		fclose(s->f_z);
		s->f_z = NULL;
	}
	if (s != NULL) {
		rm_session_free(s);
		s = NULL;
	}

	bkt = 0;
	twhash_for_each_safe(h, bkt, tmp, e, hlink) {
		twhash_del((struct twhlist_node*)&e->hlink);
		free((struct rm_ch_ch_ref_hlink*)e);
	}

	return err;
}
