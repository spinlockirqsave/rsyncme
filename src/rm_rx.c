/* @file        rm_rx.c
 * @brief       Definitions used by rsync receiver (B).
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        2 Jan 2016 11:18 AM
 * @copyright   LGPLv2.1 */


#include "rm_rx.h"


int rm_rx_f_tx_ch_ch(const struct f_tx_ch_ch_ref_arg_1 arg)
{
	const struct rm_ch_ch_ref       *e;
	const struct rm_session         *s;
	enum rm_session_type            s_type;
	struct rm_session_push_rx       *rm_push_rx;
	struct rm_session_pull_rx       *rm_pull_rx;
	int                             fd;

	e = arg.e;
	s = arg.s;
	if (e == NULL || s == NULL) {
		return RM_ERR_BAD_CALL;
	}
	s_type = s->type;

	switch (s_type) {
		case RM_PUSH_RX:
			rm_push_rx = (struct rm_session_push_rx *) s->prvt;
			if (rm_push_rx == NULL)
				return RM_ERR_RX;
			fd = rm_push_rx->ch_ch_fd;
			break;
		case RM_PULL_RX:
			rm_pull_rx = (struct rm_session_pull_rx *) s->prvt;
			if (rm_pull_rx == NULL)
				return RM_ERR_RX;
			fd = rm_pull_rx->fd;
			break;
		default:
			return RM_ERR_ARG;
	}
	if (rm_tcp_tx_ch_ch(fd, e) < 0)
		return RM_ERR_TX;
	return RM_ERR_OK;
}

int rm_rx_f_tx_ch_ch_ref_1(const struct f_tx_ch_ch_ref_arg_1 arg)
{
	const struct rm_ch_ch_ref       *e;
	const struct rm_session         *s;
	enum rm_session_type            s_type;
	struct rm_session_push_rx       *rm_push_rx;
	struct rm_session_pull_rx       *rm_pull_rx;

	e = arg.e;
	s = arg.s;
	if (e == NULL || s == NULL)
		return RM_ERR_BAD_CALL;
	s_type = s->type;

	switch (s_type) {
		case RM_PUSH_RX:
			rm_push_rx = (struct rm_session_push_rx*) s->prvt;
			if (rm_push_rx == NULL)
				return RM_ERR_RX;
			if (rm_tcp_tx_ch_ch_ref(rm_push_rx->ch_ch_fd, e) < 0)
				return RM_ERR_TX;
			break;
		case RM_PULL_RX:
			rm_pull_rx = (struct rm_session_pull_rx*) s->prvt;
			if (rm_pull_rx == NULL)
				return RM_ERR_RX;
			if (rm_tcp_tx_ch_ch_ref(rm_pull_rx->fd, e) < 0)
				return RM_ERR_TX;
			break;
		default:
			return RM_ERR_ARG;
	}

	return RM_ERR_OK;
}

int rm_rx_insert_nonoverlapping_ch_ch_ref(int fd, FILE *f, const char *fname, struct twhlist_head *h, size_t L,
		int (*f_tx_ch_ch_ref)(int fd, const struct rm_ch_ch_ref *e), size_t limit, size_t *blocks_n, pthread_mutex_t *file_mutex)
{
	int                 ffd = -1, res = -1;
	enum rm_error       err = 0;
	struct stat         fs = {0};
	size_t				file_sz = 0, read_left = 0, read_now = 0, read = 0;
	size_t              entries_n = 0;
	struct rm_ch_ch_ref_hlink	*e = NULL;
	unsigned char	    *buf = NULL;

	if (f == NULL || fname == NULL || L == 0 || fd < 0) {
		err = RM_ERR_BAD_CALL;
		goto done;
	}

	entries_n = 0;

	if (file_mutex != NULL)
		pthread_mutex_lock(file_mutex);

	ffd = fileno(f); /* get file size */

	if (file_mutex != NULL)
		pthread_mutex_unlock(file_mutex);

	res = fstat(ffd, &fs);
	if (res != 0) {
		RM_LOG_PERR("Can't fstat file [%s]", fname);
		err = RM_ERR_FSTAT;
		goto done;
	}
	file_sz = fs.st_size;

	if (file_sz == 0) {
		err = RM_ERR_OK;
		goto done;
	}

	read_left = file_sz;																	/* read L bytes chunks */
	read_now = rm_min(L, read_left);
	buf = malloc(read_now);
	if (buf == NULL) {
		RM_LOG_ERR("Malloc failed, L [%u], read_now [%u]", L, read_now);
		err = RM_ERR_MEM;
		goto done;
	}

	do {
		read = rm_fpread(buf, 1, read_now, L * entries_n, f, file_mutex);
		if (read != read_now) {
			RM_LOG_PERR("Error reading file [%s]", fname);
			free(buf);
			err = RM_ERR_READ;
			goto done;
		}
		e = malloc(sizeof (*e));															/* alloc new table entry */
		if (e == NULL)	 {
			RM_LOG_PERR("%s", "Can't allocate table entry, malloc failed");
			free(buf);
			err = RM_ERR_MEM;
			goto done;
		}

		e->data.ch_ch.f_ch = rm_fast_check_block(buf, read);								/* compute checksums */
		rm_md5(buf, read, e->data.ch_ch.s_ch.data);

		e->data.ref = entries_n;															/* assign offset */

		if (h != NULL) {																	/* insert into hashtable, hashing fast checksum */
			TWINIT_HLIST_NODE(&e->hlink);
			twhash_add_bits(h, &e->hlink, e->data.ch_ch.f_ch, RM_NONOVERLAPPING_HASH_BITS);
		}
		entries_n++;

		if (f_tx_ch_ch_ref != NULL) {														/* tx checksums to remote A ? */
			if (f_tx_ch_ch_ref(fd, &e->data) != RM_ERR_OK) {
				free(buf);
				err = RM_ERR_TX;
				goto done;
			}
		}
		read_left -= read;
		read_now = rm_min(L, read_left);
	} while (read_now > 0 && entries_n < limit);

	free(buf);
	err = RM_ERR_OK;

done:
	if (blocks_n != NULL)
		*blocks_n = entries_n;

	return err;
}

int rm_rx_insert_nonoverlapping_ch_ch_array(FILE *f, const char *fname, struct rm_ch_ch *checksums, size_t L,
		int (*f_tx_ch_ch)(const struct rm_ch_ch *), size_t limit, size_t *blocks_n)
{
	int         ffd, res;
	struct stat fs;
	uint32_t	file_sz, read_left, read_now, read;
	size_t      entries_n;
	struct rm_ch_ch	*e;
	unsigned char	*buf;

	assert(f != NULL);
	assert(fname != NULL);
	assert(L > 0);
	assert(checksums != NULL);
	if (f == NULL || fname == NULL || L == 0 || checksums == NULL)
		return RM_ERR_BAD_CALL;

	ffd = fileno(f);
	res = fstat(ffd, &fs);
	if (res != 0) {
		RM_LOG_PERR("Can't fstat file [%s]", fname);
		return RM_ERR_FSTAT;
	}
	file_sz = fs.st_size;

	read_left = file_sz; /* read L bytes chunks */
	read_now = rm_min(L, read_left);
	buf = malloc(read_now);
	if (buf == NULL) {
		RM_LOG_ERR("Malloc failed, L [%u], read_now [%u]", L, read_now);
		return RM_ERR_MEM;
	}

	entries_n = 0;
	e = &checksums[0];
	do {
		read = fread(buf, 1, read_now, f);
		if (read != read_now) {
			RM_LOG_PERR("Error reading file [%s]", fname);
			free(buf);
			return RM_ERR_READ;
		}

		e->f_ch = rm_fast_check_block(buf, read); /* compute checksums */
		rm_md5(buf, read, e->s_ch.data);

		if (f_tx_ch_ch != NULL) { /* tx checksums to remote A ? */
			if (f_tx_ch_ch(e) != RM_ERR_OK) {
				free(buf);
				return RM_ERR_TX;
			}
		}
		++entries_n;
		++e;

		read_left -= read;
		read_now = rm_min(L, read_left);
	} while (read_now > 0 && entries_n < limit);

	if (blocks_n != NULL)
		*blocks_n = entries_n;
	free(buf);
	return RM_ERR_OK;
}

int rm_rx_insert_nonoverlapping_ch_ch_ref_link(FILE *f, const char *fname, struct twlist_head *l,
		size_t L, size_t limit, size_t *blocks_n)
{
	int                     ffd, res;
	struct stat             fs;
	uint32_t	            file_sz, read_left, read_now, read;
	size_t                  entries_n;
	struct rm_ch_ch_ref_link *e;
	unsigned char           *buf;

	assert(f != NULL);
	assert(fname != NULL);
	assert(l != NULL);
	assert(L > 0);
	if (f == NULL || fname == NULL || L == 0 || l == NULL)
		return RM_ERR_BAD_CALL;

	ffd = fileno(f); /* get file size */
	res = fstat(ffd, &fs);
	if (res != 0) {
		RM_LOG_PERR("Can't fstat file [%s]", fname);
		return RM_ERR_FSTAT;
	}
	file_sz = fs.st_size;

	read_left = file_sz; /* read L bytes chunks */
	read_now = rm_min(L, read_left);
	buf = malloc(read_now);
	if (buf == NULL) {
		RM_LOG_ERR("Malloc failed, L [%u], read_now [%u]", L, read_now);
		return RM_ERR_MEM;
	}

	entries_n = 0;
	do {
		read = fread(buf, 1, read_now, f);
		if (read != read_now) {
			RM_LOG_PERR("Error reading file [%s]", fname);
			free(buf);
			return RM_ERR_READ;
		}
		e = malloc(sizeof (*e)); /* alloc new table entry */
		if (e == NULL) {
			free(buf);
			RM_LOG_PERR("%s", "Can't allocate list entry, malloc failed");
			return RM_ERR_MEM;
		}

		e->data.ch_ch.f_ch = rm_fast_check_block(buf, read); /* compute checksums */
		rm_md5(buf, read, e->data.ch_ch.s_ch.data);

		TWINIT_LIST_HEAD(&e->link); /* insert into list */
		e->data.ref = entries_n;
		twlist_add_tail(&e->link, l);

		entries_n++;

		read_left -= read;
		read_now = rm_min(L, read_left);
	} while (read_now > 0 && entries_n < limit);

	*blocks_n = entries_n;
	free(buf);

	return	RM_ERR_OK;
}

/* Callback of rm_session_delta_rx_f_local in local push. */
enum rm_error rm_rx_process_delta_element(void *arg)
{
	size_t  z_offset;   /* current offset in @f_z */

	struct rm_rx_delta_element_arg	*delta_pack = arg;
	const struct rm_delta_e			*delta_e = delta_pack->delta_e;
	FILE							*f_y = delta_pack->f_y;
	FILE							*f_z = delta_pack->f_z;
	struct rm_delta_reconstruct_ctx	*ctx = delta_pack->rec_ctx;
	pthread_mutex_t					*m = delta_pack->file_mutex;

	assert(delta_e != NULL && f_z != NULL && ctx != NULL);
	if (delta_e == NULL || f_z == NULL || ctx == NULL)
		return RM_ERR_BAD_CALL;
	z_offset = ctx->rec_by_ref + ctx->rec_by_raw;

	switch (delta_e->type) {

		case RM_DELTA_ELEMENT_REFERENCE:
			if (rm_copy_buffered_offset(f_y, f_z, delta_e->raw_bytes_n, delta_e->ref * ctx->L, z_offset, m) != RM_ERR_OK)  /* copy referenced bytes from @f_y to @f_z */
				return RM_ERR_COPY_OFFSET;
			ctx->rec_by_ref += ctx->L;																					/* L bytes copied from @y */
			++ctx->delta_ref_n;
			break;

		case RM_DELTA_ELEMENT_RAW_BYTES:
			if (rm_fpwrite(delta_e->raw_bytes, delta_e->raw_bytes_n * sizeof(unsigned char), 1, z_offset, f_z, m) != 1)    /* copy raw bytes to @f_z directly */
				return RM_ERR_WRITE;
			ctx->rec_by_raw += delta_e->raw_bytes_n;
			++ctx->delta_raw_n;
			break;

		case RM_DELTA_ELEMENT_ZERO_DIFF:
			if (rm_copy_buffered(f_y, f_z, delta_e->raw_bytes_n, m) != RM_ERR_OK)                                          /* copy all bytes from @f_y to @f_z */
				return RM_ERR_COPY_BUFFERED;
			ctx->rec_by_ref += delta_e->raw_bytes_n; /* delta ZERO_DIFF has raw_bytes_n set to indicate bytes that matched (whole file) so we can nevertheless check here at receiver that is correct */
			++ctx->delta_ref_n;
			ctx->rec_by_zero_diff += delta_e->raw_bytes_n;
			++ctx->delta_zero_diff_n;
			break;

		case RM_DELTA_ELEMENT_TAIL:

			if (rm_copy_buffered_offset(f_y, f_z, delta_e->raw_bytes_n, delta_e->ref * ctx->L, z_offset, m) != RM_ERR_OK)  /* copy referenced bytes from @f_y to @f_z */
				return RM_ERR_COPY_OFFSET;
			ctx->rec_by_ref += delta_e->raw_bytes_n; /* delta TAIL has raw_bytes_n set to indicate bytes that matched (that tail) so we can nevertheless check here at receiver there is no error */
			++ctx->delta_ref_n;
			ctx->rec_by_tail += delta_e->raw_bytes_n;
			++ctx->delta_tail_n;
			ctx->delta_tail_n = 1;
			break;

		default:
			assert(1 == 0 && "Unknown delta element type!");
			return RM_ERR_ARG;
	}

	return RM_ERR_OK;
}

/* In PUSH TX: Callback of rm_session_delta_rx_f_local for TX of delta over TCP using DELTA protocol
 * PROTOCOL
 * Currently protocol is simple:
 * 1.	TX delta type
 * 2.	If it is DELTA_REFERENCE || DELTA_TAIL
 *			then TX ref
 *		else if it is DELTA_RAW_BYTES
 *			then	TX bytes size,
 *					TX bytes
 *		else
 *			it is DELTA_ZERO_DIFF, do not TX anything, we are done*/
enum rm_error rm_rx_tx_delta_element(void *arg)
{
	// size_t  z_offset = 0;																							/* current offset in @f_z */
	int fd = -1;
	struct rm_rx_delta_element_arg	*delta_pack = arg;
	const struct rm_delta_e			*delta_e = delta_pack->delta_e;
	struct rm_delta_reconstruct_ctx	*ctx = delta_pack->rec_ctx;

	fd = delta_pack->fd;

	if (delta_e == NULL || ctx == NULL)
		return RM_ERR_BAD_CALL;
	// z_offset = ctx->rec_by_ref + ctx->rec_by_raw;
	RM_LOG_INFO("[TX]: delta type[%u]", delta_e->type);

	/* TX delta over TCP using delta protocol */
	if (rm_tcp_tx(fd, (void*) &delta_e->type, RM_DELTA_ELEMENT_TYPE_FIELD_SIZE) != RM_ERR_OK)							/* tx delta type over TCP connection */
		return RM_ERR_WRITE;

	switch (delta_e->type) {

		case RM_DELTA_ELEMENT_REFERENCE:																				/* receiver will copy referenced bytes from @f_y to @f_z */
			if (rm_tcp_tx(fd, (void*) &delta_e->ref, RM_DELTA_ELEMENT_REF_FIELD_SIZE) != RM_ERR_OK)						/* tx ref over TCP connection */
				return RM_ERR_WRITE;
			ctx->rec_by_ref += delta_e->raw_bytes_n;                                                                    /* L == delta_e->raw_bytes_n for REFERNECE delta elements*/
			++ctx->delta_ref_n;
			break;

		case RM_DELTA_ELEMENT_TAIL:																						/* receiver will copy referenced bytes from @f_y to @f_z */
			if (rm_tcp_tx(fd, (void*) &delta_e->ref, RM_DELTA_ELEMENT_REF_FIELD_SIZE) != RM_ERR_OK)						/* tx ref over TCP connection */
				return RM_ERR_WRITE;
			ctx->rec_by_ref += delta_e->raw_bytes_n; /* delta TAIL has raw_bytes_n set to indicate bytes that matched (that tail) so we can nevertheless check here at receiver there is no error */
			++ctx->delta_ref_n;
			ctx->rec_by_tail += delta_e->raw_bytes_n;
			++ctx->delta_tail_n;
			break;

		case RM_DELTA_ELEMENT_RAW_BYTES:																				/* receiver will copy raw bytes to @f_z directly */
			if (rm_tcp_tx(fd, (void*) &delta_e->raw_bytes_n, RM_DELTA_ELEMENT_BYTES_FIELD_SIZE) != RM_ERR_OK)			/* tx bytes size over TCP connection */
				return RM_ERR_WRITE;
			if (rm_tcp_tx(fd, (void*) delta_e->raw_bytes, delta_e->raw_bytes_n) != RM_ERR_OK)							/* tx bytes over TCP connection */
				return RM_ERR_WRITE;
			ctx->rec_by_raw += delta_e->raw_bytes_n;
			++ctx->delta_raw_n;
			break;

		case RM_DELTA_ELEMENT_ZERO_DIFF:
			ctx->rec_by_ref += delta_e->raw_bytes_n; /* delta ZERO_DIFF has raw_bytes_n set to indicate bytes that matched (whole file) so we can nevertheless check here at receiver that is correct */
			++ctx->delta_ref_n;
			ctx->rec_by_zero_diff += delta_e->raw_bytes_n;
			++ctx->delta_zero_diff_n;
			break;

		default:
			assert(1 == 0 && "Unknown delta element type!");
			return RM_ERR_ARG;
	}

	return RM_ERR_OK;
}
