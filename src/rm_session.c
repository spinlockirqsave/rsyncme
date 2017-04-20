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
	memset(prvt, 0, sizeof(struct rm_session_push_rx));
	TWINIT_LIST_HEAD(&prvt->rx_delta_e_queue);
	pthread_mutex_init(&prvt->rx_delta_e_queue_mutex, NULL);
	pthread_cond_init(&prvt->rx_delta_e_queue_signal, NULL);
	prvt->msg_push = NULL;
	prvt->fd = -1;
	prvt->ch_ch_fd = -1;
	prvt->delta_fd = -1;
	return;
}

void rm_session_push_rx_free(struct rm_session_push_rx *prvt)
{
	pthread_mutex_destroy(&prvt->rx_delta_e_queue_mutex);
	pthread_cond_destroy(&prvt->rx_delta_e_queue_signal);
	if (prvt->msg_push != NULL) {
		rm_msg_push_free(prvt->msg_push);
	}
	if (prvt->delta_fd >= 0) {
		close(prvt->delta_fd);
		prvt->delta_fd = -1;
	}
	if (prvt->fd >= 0) {
		close(prvt->fd);
		prvt->fd = -1;
	}
	free(prvt);
	return;
}

void rm_session_push_tx_init(struct rm_session_push_tx *prvt)
{
	memset(prvt, 0, sizeof(struct rm_session_push_tx));
	rm_session_push_local_init(&prvt->session_local);
	prvt->session_local.delta_rx_f = rm_rx_tx_delta_element;
	//pthread_mutex_init(&prvt->ch_ch_hash_mutex, NULL);	/* moved to it's session_local object */
	return;
}

/* frees private session, DON'T TOUCH private session after this returns */ 
void rm_session_push_tx_free(struct rm_session_push_tx *prvt)
{
	rm_session_push_local_deinit(&prvt->session_local);
	//pthread_mutex_destroy(&prvt->ch_ch_hash_mutex); /* moved to it's sesion_local object */
	free(prvt);
	return;
}

void rm_session_push_local_init(struct rm_session_push_local *prvt)
{
	memset(prvt, 0, sizeof(struct rm_session_push_local));
	pthread_mutex_init(&prvt->h_mutex, NULL);
	TWINIT_LIST_HEAD(&prvt->tx_delta_e_queue);
	pthread_mutex_init(&prvt->tx_delta_e_queue_mutex, NULL);
	pthread_cond_init(&prvt->tx_delta_e_queue_signal, NULL);
	prvt->delta_rx_f = rm_rx_process_delta_element;
	return;
}

static void rm_session_push_local_deinit(struct rm_session_push_local *prvt)
{
	/* queue of delta elements MUST be empty now */
	if (twlist_empty(&prvt->tx_delta_e_queue) != 0)
		RM_LOG_ERR("%s", "Delta elements queue NOT EMPTY!\n");
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

enum rm_error rm_session_assign_validate_from_msg_push(struct rm_session *s, struct rm_msg_push *m, int fd)
{
	int fd_y = -1;
	struct stat fs;
	size_t y_sz = 0;
	struct rm_session_push_rx   *push_rx = NULL;

	if (m->L == 0) {                                                                    /* L can't be 0 */
		return RM_ERR_BLOCK_SIZE;
	}
	s->rec_ctx.L = m->L;

	if (m->x_sz == 0) {                                                                 /* @x not set */
		return RM_ERR_X_ZERO_SIZE;
	}

	switch (s->type) {

		case RM_PUSH_RX:                                                                /* validate remote PUSH RX */
			push_rx = s->prvt;
			push_rx->msg_push = m;
			push_rx->fd = fd;
			s->f_x = NULL;
			s->f_x_sz = push_rx->msg_push->bytes;										/* bytes to RX, size of file to receive */
			if (m->y_sz > 0) {
				strncpy(s->f_y_basename, m->y, PATH_MAX);								/* copy strings for use with basename/dirname which may return pointer to statically alloced memory */
				strncpy(s->f_y_dirname, m->y, PATH_MAX);
				s->f_y_bname = basename(s->f_y_basename);
				s->f_y_dname = dirname(s->f_y_dirname);
			}
			if (m->z_sz > 0) {
				strncpy(s->f_z_basename, m->z, PATH_MAX);								/* copy strings for use with basename/dirname which may return pointer to statically alloced memory */
				strncpy(s->f_z_dirname, m->z, PATH_MAX);
				s->f_z_bname = basename(s->f_z_basename);
				s->f_z_dname = dirname(s->f_z_dirname);
			}
			if ((m->hdr->flags & RM_BIT_6) && (m->z_sz == 0 || (strcmp(m->y, m->z) == 0))) { /* if do not delete @y after @z has been synced, but @z name is not given or is same as @y - error */
				return RM_ERR_Y_Z_SYNC;
			}
			if (m->y_sz == 0) {
				return RM_ERR_Y_NULL;                                                   /* error: what is the name of the result file? OR error: what is the name of the file you want to sync with? */
			}
			s->f_y = fopen(m->y, "rb");                                                 /* try to open */
			if (s->f_y != NULL) {
				fd_y = fileno(s->f_y);
				memset(&fs, 0, sizeof(fs));
				if (fstat(fd_y, &fs) != 0)
					return RM_ERR_FSTAT_Y;
				y_sz = fs.st_size;
				s->f_y_sz = y_sz;														/* use in DELTA_ZERO_DIFF */
				push_rx->ch_ch_n = y_sz / m->L + (y_sz % m->L ? 1 : 0);                 /* # of nonoverlapping checksums to be sent to remote transmitter */
			} else {																	/* s->f_y is NULL */
				push_rx->ch_ch_n = 0;													/* no checksums to send... */
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
			if (getcwd(s->pwd_init, PATH_MAX) == NULL)									/* change working directory */
				return RM_ERR_GETCWD;
			if (s->f_z != NULL) {
				if (chdir(s->f_z_dname) == -1)
					return RM_ERR_CHDIR_Z;
			} else {
				if (chdir(s->f_y_dname) == -1)
					return RM_ERR_CHDIR_Y;
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
	struct rm_session	*s = NULL;
	uuid_t				uuid = {0};

	s = malloc(sizeof *s);
	if (s == NULL)
		return NULL;
	memset(s, 0, sizeof(struct rm_session));
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

/* in PUSH RX: TX checksums to transmitter of file */
void *rm_session_ch_ch_tx_f(void *arg)
{
	enum rm_error                   err = RM_ERR_OK;
	struct rm_session               *s = (struct rm_session *) arg;
	enum rm_session_type            s_type = 0;
	struct rm_session_push_rx       *rm_push_rx = NULL;
	struct rm_session_pull_rx       *rm_pull_rx = NULL;
	struct rm_msg_push              *msg_push = NULL;
	size_t                          L = 0;
	FILE                            *f_y = NULL;   /* file for taking non-overlapping blocks */
	FILE                            *f_z = NULL;   /* result */
	int                             fd_y = -1;
	uint8_t                         reference_file_exist = 0;
	struct stat                     fs = {0};
	size_t                          y_sz = 0, blocks_n_exp = 0, blocks_n = 0;
	int                             fd = -1;

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
			fd = rm_push_rx->fd;																				/* TODO use separate socket for checksums TX? (ch_ch_port in receiver?) */
			L = rm_push_rx->msg_push->L;
			f_y = s->f_y;
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
				if (RM_LOGLEVEL > RM_LOGLEVEL_NORMAL)
					RM_LOG_INFO("[%s] -> [%s], [%u]: TX-ed [%zu] nonoverlapping checksum elements", s->ssid1, s->ssid2, s->hashed_hash, blocks_n_exp);
			} else {
				if (msg_push->hdr->flags & RM_BIT_4) {                                                          /* force creation if @y doesn't exist? */
					if (msg_push->z != NULL) {                                                                  /* use different name? */
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
					/* expected rec_rec_ctx.->delta_raw_n = 1; */
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

/* in PUSH TX: RX nonoverlapping checksums and insert into hashtable */
void *rm_session_ch_ch_rx_f(void *arg)
{
	int fd = -1;
	size_t ch_ch_n = 0;
	struct rm_session_push_tx   *prvt = NULL;
	struct rm_msg_push_ack		*ack = NULL;
	enum rm_error				err = RM_ERR_OK;
	struct rm_ch_ch_ref_hlink	*e = NULL;
	size_t						entries_n = 0;
	struct twhlist_head			*h = NULL;
	pthread_mutex_t				*h_mutex = NULL;
	enum rm_rx_status			status = RM_RX_STATUS_OK;

	struct rm_session *s = (struct rm_session *) arg;
	prvt = s->prvt;
	ack = prvt->msg_push_ack;
	h = prvt->session_local.h;
	h_mutex = &prvt->session_local.h_mutex;

	fd = prvt->fd;																		/* TODO Do we need to use separate ch_ch channel (ch_ch_port) instead of main socket? */
	ch_ch_n = ack->ch_ch_n;
	if (ch_ch_n == 0)
		goto done;

	while (ch_ch_n > 0) {
		e = malloc(sizeof (*e));
		if (e == NULL)	 {
			status = RM_RX_STATUS_CH_CH_RX_MEM;
			goto err_exit;
		}

		uint32_t f_ch = 0;
		err = rm_tcp_rx(fd, &f_ch, sizeof(f_ch));
		if (err != RM_ERR_OK) {
			status = RM_RX_STATUS_CH_CH_RX_TCP_FAIL;
			goto err_exit;
		}
		rm_deserialize_u32((unsigned char *) &f_ch, &e->data.ch_ch.f_ch);

		err = rm_tcp_rx(fd, &e->data.ch_ch.s_ch, RM_STRONG_CHECK_BYTES);
		if (err != RM_ERR_OK) {
			status = RM_RX_STATUS_CH_CH_RX_TCP_FAIL;
			goto err_exit;
		}
		RM_LOG_INFO("[RX]: checksum [%u]", e->data.ch_ch.f_ch);

		e->data.ref = entries_n;														/* assign offset */
		TWINIT_HLIST_NODE(&e->hlink);

		pthread_mutex_lock(h_mutex); /* TODO Verify hashtable locking needs for rm_rolling_ch_proc <-> rm_session_ch_ch_rx_f */
		twhash_add_bits(h, &e->hlink, e->data.ch_ch.f_ch, RM_NONOVERLAPPING_HASH_BITS);	/* insert into hashtable, hashing fast checksum */
		pthread_mutex_unlock(h_mutex);

		entries_n++;
		ch_ch_n--;
	}

done:
	pthread_mutex_lock(&s->mutex);
	prvt->ch_ch_rx_status = RM_RX_STATUS_OK;
	pthread_mutex_unlock(&s->mutex);
	return NULL; /* this thread must be created in joinable state */

err_exit:
	pthread_mutex_lock(&s->mutex);
	prvt->ch_ch_rx_status = status;
	pthread_mutex_unlock(&s->mutex);
	return NULL; /* this thread must be created in joinable state */
}

/* enqueue delta into queue (both in RM_PUSH_LOCAL & in RM_PUSH_TX) */
void *rm_session_delta_tx_f(void *arg)
{
	struct twhlist_head     *h;             /* nonoverlapping checkums */
	FILE                    *f_x;           /* file on which rolling is performed */
	rm_delta_f              *delta_tx_f;    /* tx/reconstruct callback */
	struct rm_session       *s;
	enum rm_session_type    t;
	struct rm_session_push_local    *prvt_local;
	struct rm_session_push_tx       *prvt_tx;
	int                     err;
	enum rm_tx_status       status = RM_TX_STATUS_OK;
	pthread_mutex_t			*h_mutex = NULL;

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
			delta_tx_f = prvt_local->delta_tx_f;
			break;

		case RM_PUSH_TX:
			prvt_tx = s->prvt;
			if (prvt_tx == NULL)
				goto exit;
			h       = prvt_tx->session_local.h;
			h_mutex = &prvt_tx->session_local.h_mutex;
			delta_tx_f = prvt_tx->session_local.delta_tx_f;
			break;

		default:
			goto exit;
	}
	pthread_mutex_unlock(&s->mutex);


	err = rm_rolling_ch_proc(s, h, h_mutex, f_x, delta_tx_f, 0); /* 1. run rolling checksum procedure */
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

/* in PUSH TX: dequeue delta elements and TX them to receiver of file */
void *rm_session_delta_rx_f_local(void *arg)
{
	FILE                            *f_y = NULL;		/* reference file, on which reconstruction is performed */
	FILE                            *f_z = NULL;		/* result file */
	struct rm_session_push_local    *prvt_local = NULL;
	twfifo_queue                    *q = NULL;
	pthread_mutex_t                 *q_mutex = NULL;
	pthread_cond_t					*q_signal = NULL;
	const struct rm_delta_e         *delta_e = NULL;	/* iterator over delta elements */
	struct twlist_head              *lh = NULL;
	size_t                          bytes_to_rx;
	struct rm_session               *s = NULL;
	struct rm_rx_delta_element_arg delta_pack = {0};
	struct rm_delta_reconstruct_ctx rec_ctx = {0};		/* describes result of reconstruction, we will copy this to session reconstruct context after all is done to avoid locking on each delta element */
	int err;
	enum rm_rx_status               status = RM_RX_STATUS_OK;
	struct rm_session_push_tx		*prvt_tx = NULL;
	struct rm_msg_push_ack			*ack = NULL;
	enum rm_error					res = RM_ERR_OK;
	const char						*err_str = NULL;

	uint16_t	timeout_s = 10;							/* TODO get timeouts from the user */
	uint16_t	timeout_us = 0;

	s = (struct rm_session*) arg;
	if (s == NULL) {
		status = RM_RX_STATUS_INTERNAL_ERR;
		goto err_exit;
	}
	assert(s != NULL);

	pthread_mutex_lock(&s->mutex);
	memcpy(&rec_ctx, &s->rec_ctx, sizeof(struct rm_delta_reconstruct_ctx));

	if (s->type != RM_PUSH_TX && s->type != RM_PUSH_LOCAL) {
		pthread_mutex_unlock(&s->mutex);
		status = RM_RX_STATUS_INTERNAL_ERR;
		goto err_exit;
	}

	rec_ctx.L = s->rec_ctx.L;								/* init reconstruction context */
	bytes_to_rx = s->f_x_sz;								/* bytes to be xferred (by delta and/or by raw) */

	if (s->type == RM_PUSH_LOCAL) {							/* RM_PUSH_LOCAL */
		prvt_local = s->prvt;
		if (prvt_local == NULL) {
			pthread_mutex_unlock(&s->mutex);
			status = RM_RX_STATUS_INTERNAL_ERR;
			goto err_exit;
		}
		f_y         = s->f_y;
		f_z         = s->f_z;
		q = &prvt_local->tx_delta_e_queue;
		q_mutex = &prvt_local->tx_delta_e_queue_mutex;
		q_signal = &prvt_local->tx_delta_e_queue_signal;
	} else {												/* RM_PUSH_TX */
		prvt_tx = s->prvt;
		if (prvt_tx == NULL) {
			pthread_mutex_unlock(&s->mutex);
			status = RM_RX_STATUS_INTERNAL_ERR;
			goto err_exit;
		}
		prvt_local = &prvt_tx->session_local;
		ack = prvt_tx->msg_push_ack;
		q = &prvt_tx->session_local.tx_delta_e_queue;
		q_mutex = &prvt_tx->session_local.tx_delta_e_queue_mutex;
		q_signal = &prvt_tx->session_local.tx_delta_e_queue_signal;

		struct sockaddr peer_addr;
		socklen_t addrlen = sizeof(peer_addr);
		if (getpeername(prvt_tx->fd, &peer_addr, &addrlen) == -1) {
			pthread_mutex_unlock(&s->mutex);
			status = RM_ERR_GETPEERNAME;
			goto err_exit;
		}
		((struct sockaddr_in*)&peer_addr)->sin_port = htons(ack->delta_port);		/* use receiver's delta port from ACK */
		res = rm_tcp_connect_nonblock_timeout_sockaddr(&prvt_tx->fd_delta_tx, &peer_addr, timeout_s, timeout_us, &err_str);
		if (res != RM_ERR_OK) {
			pthread_mutex_unlock(&s->mutex);
			if (res == RM_ERR_CONNECT_TIMEOUT)
				status = RM_RX_STATUS_CONNECT_TIMEOUT;
			else if (res == RM_ERR_CONNECT_REFUSED)
				status = RM_RX_STATUS_CONNECT_REFUSED;
			else if (res == RM_ERR_CONNECT_HOSTUNREACH)
				status = RM_RX_STATUS_CONNECT_HOSTUNREACH;
			else
				status = RM_RX_STATUS_CONNECT_GEN_ERR;
			goto err_exit;
		}
		delta_pack.fd = prvt_tx->fd_delta_tx;										/* tell delta_rx_f callback about new delta channel */ 											
	}
	assert(((prvt_local != NULL) && (prvt_tx != NULL)) ^ ((prvt_local != NULL) && (prvt_tx == NULL)));
	pthread_mutex_unlock(&s->mutex);

	assert((s->type == RM_PUSH_LOCAL && f_y != NULL) || s->type == RM_PUSH_TX);
	assert((s->type == RM_PUSH_LOCAL && f_z != NULL) || s->type == RM_PUSH_TX);
	if (s->type == RM_PUSH_LOCAL && (f_y == NULL || f_z == NULL)) {
		status = RM_RX_STATUS_INTERNAL_ERR;
		goto err_exit;
	}

	if (bytes_to_rx == 0)
		goto done;

	delta_pack.f_y = f_y;
	delta_pack.f_z = f_z;
	delta_pack.rec_ctx = &rec_ctx;

	pthread_mutex_lock(q_mutex); /* sleep on delta queue and reconstruct element once awoken */

	while (bytes_to_rx > 0) {
		if (bytes_to_rx == 0) { /* checking for missing signal is not really needed here, as bytes_to_rx is local variable, nevertheless... */
			pthread_mutex_unlock(q_mutex);
			goto done;
		}
		/* process delta element */
		for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh)) { /* in case of local sync there can be only single element enqueued each time conditional variable is signaled, but in other cases it will be possible to be different most likely */
			delta_e = tw_container_of(lh, struct rm_delta_e, link);
			//err = rm_rx_process_delta_element(delta_e, f_y, f_z, &rec_ctx);
			delta_pack.delta_e = delta_e;
			err = prvt_local->delta_rx_f(&delta_pack);
			if (err != 0) {
				pthread_mutex_unlock(q_mutex);
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
			pthread_cond_wait(q_signal, q_mutex);
		}
	}
	pthread_mutex_unlock(&prvt_local->tx_delta_e_queue_mutex);

done:
	pthread_mutex_lock(&s->mutex);
	if (s->type == RM_PUSH_LOCAL) {
		assert(rec_ctx.rec_by_ref + rec_ctx.rec_by_raw == s->f_x_sz);
		assert(rec_ctx.delta_tail_n == 0 || rec_ctx.delta_tail_n == 1);
		rec_ctx.collisions_1st_level = s->rec_ctx.collisions_1st_level; /* tx thread might have assigned to collisions variables already and memcpy would overwrite them */
		rec_ctx.collisions_2nd_level = s->rec_ctx.collisions_2nd_level;
		rec_ctx.copy_all_threshold_fired = s->rec_ctx.copy_all_threshold_fired; /* tx thread might have assigned to threshold_fired variables already and memcpy would overwrite them */
		rec_ctx.copy_tail_threshold_fired = s->rec_ctx.copy_tail_threshold_fired;
		memcpy(&s->rec_ctx, &rec_ctx, sizeof(struct rm_delta_reconstruct_ctx));
		prvt_local->delta_rx_status = RM_RX_STATUS_OK;
	} else {															/* RM_PUSH_TX */
		prvt_tx->session_local.delta_rx_status = RM_RX_STATUS_OK;
		if (prvt_tx->fd_delta_tx != -1) {
			close(prvt_tx->fd_delta_tx);
			prvt_tx->fd_delta_tx = -1;
		}
	}
	pthread_mutex_unlock(&s->mutex);
	return NULL; /* this thread must be created in joinable state */

err_exit:
	pthread_mutex_lock(&s->mutex);
	if (s->type == RM_PUSH_LOCAL)
		prvt_local->delta_rx_status = status;
	else {
		prvt_tx->session_local.delta_rx_status = status;
		if (prvt_tx->fd_delta_tx != -1) {
			close(prvt_tx->fd_delta_tx);
			prvt_tx->fd_delta_tx = -1;
		}
	}
	pthread_mutex_unlock(&s->mutex);
	return NULL; /* this thread must be created in joinable state */
}

void* rm_session_delta_rx_f_remote(void *arg)
{
	FILE                            *f_y = NULL;		/* file on which reconstruction is performed */
	FILE                            *f_z = NULL;		/* result file */
	/*twfifo_queue                    *q; */
	/*const struct rm_delta_e         *delta_e;        iterator over delta elements */
	/*struct twlist_head              *lh;*/
	struct rm_session_push_rx       *prvt_rx = NULL;
	rm_push_flags					push_flags = 0;
	char                            f_z_name[RM_UNIQUE_STRING_LEN];
	int								listen_fd = -1, fd = -1;
	size_t                          bytes_to_rx = 0, y_sz = 0;
	struct rm_session               *s = NULL;
	struct rm_rx_delta_element_arg delta_pack = {0};
	struct rm_delta_reconstruct_ctx rec_ctx = {0};		/* describes result of reconstruction, we will copy this to session reconstruct context after all is done to avoid locking on each delta element */
	enum rm_error					err = RM_ERR_OK;
	enum rm_rx_status				status = RM_RX_STATUS_OK;

	struct sockaddr_storage cli_addr = {0};;
	socklen_t               cli_len = 0;

	s = (struct rm_session*) arg;
	if (s == NULL) {
		goto err_exit;
	}
	assert(s != NULL);

	pthread_mutex_lock(&s->mutex);

	if (s->type != RM_PUSH_RX) {
		pthread_mutex_unlock(&s->mutex);
		goto err_exit;
	}
	prvt_rx = s->prvt;
	if (prvt_rx == NULL) {
		pthread_mutex_unlock(&s->mutex);
		goto err_exit;
	}
	assert(prvt_rx != NULL);
	push_flags	= (rm_push_flags) prvt_rx->msg_push->hdr->flags;
	bytes_to_rx = prvt_rx->msg_push->bytes;
	f_y         = s->f_y;
	y_sz		= s->f_y_sz;
	listen_fd	= prvt_rx->delta_fd;
	memcpy(&rec_ctx, &s->rec_ctx, sizeof(struct rm_delta_reconstruct_ctx));													/* init reconstruction context (L set in assign_validate() */

	pthread_mutex_unlock(&s->mutex);

	if (f_y == NULL) {
		assert(push_flags & RM_BIT_4);																						/* assert force creation if @y doesn't exist? */
		if (!(push_flags & RM_BIT_4)) {
			err = RM_ERR_BAD_CALL;
			goto err_exit;
		}
	} else {
		rm_get_unique_string(f_z_name);
		f_z = fopen(f_z_name, "wb+");																						/* create and open @f_z for reading and writing in @z path */
		if (f_z == NULL) {
			err = RM_ERR_OPEN_TMP;
			goto err_exit;
		}
		strncpy(s->f_z_name, f_z_name, RM_UNIQUE_STRING_LEN);
		s->f_z = f_z;
	}

	cli_len = sizeof(cli_addr);
	if ((fd = accept(listen_fd, (struct sockaddr *) &cli_addr, &cli_len)) < 0) {											/* accept transmitter's incoming connection */
		status = RM_RX_STATUS_DELTA_RX_ACCEPT_FAIL;
		goto err_exit;
	}

	if (bytes_to_rx == 0) {
		goto done;
	}

	delta_pack.f_y = f_y;
	delta_pack.f_z = f_z;
	delta_pack.rec_ctx = &rec_ctx;

	/* RX delta over TCP using delta protocol */
	struct rm_delta_e delta_e;
	while (bytes_to_rx > 0) {
		memset(&delta_e, 0, sizeof(struct rm_delta_e));

		/* RX delta over TCP using delta protocol */
		if (rm_tcp_rx(fd, (void*) &delta_e.type, RM_DELTA_ELEMENT_TYPE_FIELD_SIZE) != RM_ERR_OK) {							/* rx delta type over TCP connection */
			status = RM_RX_STATUS_DELTA_RX_TCP_FAIL;
			goto err_exit;
		}
		RM_LOG_INFO("[RX]: delta type[%u]", delta_e.type);

		switch (delta_e.type) {

			case RM_DELTA_ELEMENT_REFERENCE:																				/* copy referenced bytes from @f_y to @f_z */
				if (rm_tcp_rx(fd, (void*) &delta_e.ref, RM_DELTA_ELEMENT_REF_FIELD_SIZE) != RM_ERR_OK)						/* rx ref over TCP connection */
					goto err_exit;
				delta_e.raw_bytes_n = rec_ctx.L;																			/* by definition */
				break;

			case RM_DELTA_ELEMENT_TAIL:																						/* copy referenced bytes from @f_y to @f_z */
				if (rm_tcp_rx(fd, (void*) &delta_e.ref, RM_DELTA_ELEMENT_REF_FIELD_SIZE) != RM_ERR_OK)						/* rx ref over TCP connection */
					goto err_exit;
				delta_e.raw_bytes_n = bytes_to_rx;																			/* by definition */
				break;

			case RM_DELTA_ELEMENT_RAW_BYTES:																				/* copy raw bytes to @f_z directly */
				if (rm_tcp_rx(fd, (void*) &delta_e.raw_bytes_n, RM_DELTA_ELEMENT_BYTES_FIELD_SIZE) != RM_ERR_OK)			/* rx bytes size over TCP connection */
					goto err_exit;
				delta_e.raw_bytes = malloc(delta_e.raw_bytes_n);															/* malloc buffer */
				if (delta_e.raw_bytes == NULL)
					goto err_exit;
				if (rm_tcp_rx(fd, (void*) delta_e.raw_bytes, delta_e.raw_bytes_n) != RM_ERR_OK)								/* rx bytes over TCP connection */
					goto err_exit;
				break;

			case RM_DELTA_ELEMENT_ZERO_DIFF:
				delta_e.raw_bytes_n = y_sz;																					/* by definition */
				break;

			default:
				assert(1 == 0 && "Unknown delta element type!");
				goto err_exit;
		}

		delta_pack.delta_e = &delta_e;
		err = rm_rx_process_delta_element(&delta_pack);																		/* do reconstruction */
		if (err != 0) {
			status = RM_RX_STATUS_DELTA_PROC_FAIL;
			goto err_exit;
		}

		switch (delta_e.type) {

			case RM_DELTA_ELEMENT_REFERENCE:
			case RM_DELTA_ELEMENT_RAW_BYTES:
				bytes_to_rx -= delta_e.raw_bytes_n;
				break;

			case RM_DELTA_ELEMENT_TAIL:
			case RM_DELTA_ELEMENT_ZERO_DIFF:
				bytes_to_rx = 0;
				break;
		}
	}

done:

	pthread_mutex_lock(&s->mutex);

	if (fd != -1) {																											/* close accepted socket connection */
		close(fd);
		fd = -1;
	}
	if (prvt_rx->delta_fd != -1) {
		close(prvt_rx->delta_fd);																							/* close listening socket */
		prvt_rx->delta_fd = -1;
	}

	assert(rec_ctx.rec_by_ref + rec_ctx.rec_by_raw == s->f_x_sz);															/* f_x_sz assigned from msg_push in assign_validate method */
	assert(rec_ctx.delta_tail_n == 0 || rec_ctx.delta_tail_n == 1);
	memcpy(&s->rec_ctx, &rec_ctx, sizeof(struct rm_delta_reconstruct_ctx));
	prvt_rx->delta_rx_status = RM_RX_STATUS_OK;

	pthread_mutex_unlock(&s->mutex);

	return NULL; /* this thread must be created in joinable state */

err_exit:

	pthread_mutex_lock(&s->mutex);

	if (fd != -1) {																											/* close accepted socket connection */
		close(fd);
		fd = -1;
	}
	if (prvt_rx->delta_fd != -1) {
		close(prvt_rx->delta_fd);																							/* close listening socket */
		prvt_rx->delta_fd = -1;
	}
	prvt_rx->delta_rx_status = status;

	pthread_mutex_unlock(&s->mutex);

	return NULL; /* this thread must be created in joinable state */
}
