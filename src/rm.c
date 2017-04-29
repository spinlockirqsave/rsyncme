/* @file        rm.c
 * @brief       Simple rsync algorithm implementation
 *              as described in Tridgell A., "Efficient Algorithms
 *              for Sorting and Synchronization", 1999.
 * @details     Common definitions used by sender and receiver.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        1 Jan 2016 07:50 PM
 * @copyright   LGPLv2.1v2.1 */


#include "rm.h"
#include "rm_util.h"
#include "rm_session.h"


uint32_t
rm_fast_check_block(const unsigned char *data, size_t len) {
#ifdef DEBUG
	uint32_t res;
#endif
	uint32_t	r1, r2;
	size_t      i;

	assert(data != NULL);
	r1 = 0;
	r2 = 0;
	i = 0;
	for (; i < len; ++i) {
		r1 = (r1 + data[i]) % RM_FASTCHECK_MODULUS;
		r2 = (r2 + r1) % RM_FASTCHECK_MODULUS;
	}
#ifdef DEBUG
	res = (r2 << 16) | r1;
	return res;
#else
	return (r2 << 16) | r1;
#endif
}

uint32_t
rm_adler32_1(const unsigned char *data, size_t len) {
#ifdef DEBUG
	uint32_t res;
#endif
	uint32_t	r1, r2;
	size_t      i;

	assert(data != NULL);
	r1 = 1;
	r2 = 0;
	i = 0;
	for (; i < len; ++i) {
		r1 = (r1 + data[i]) % RM_ADLER32_MODULUS;
		r2 = (r2 + r1) % RM_ADLER32_MODULUS;
	}
#ifdef DEBUG
	res = (r2 << 16) | r1;
	return res;
#else
	return (r2 << 16) | r1;
#endif
}

#define RM_DO1(buf, i)	{ r1 += buf[i]; r2 += r1; }
#define RM_DO2(buf, i)	RM_DO1(buf, i); RM_DO1(buf, i + 1)
#define RM_DO4(buf, i)	RM_DO2(buf, i); RM_DO2(buf, i + 2)
#define RM_DO8(buf, i)	RM_DO4(buf, i); RM_DO4(buf, i + 4)
#define RM_DO16(buf)	RM_DO8(buf,0); RM_DO8(buf,8)

uint32_t
rm_adler32_2(uint32_t adler, const unsigned char *data, size_t L) {
#ifdef DEBUG
	uint32_t res;
#endif
	uint32_t	k;
	uint32_t	r1 = adler & 0xffff;
	uint32_t	r2 = (adler >> 16) & 0xffff;

	assert(data != NULL);
	while(L) {
		k = L < RM_ADLER32_NMAX ? L : RM_ADLER32_NMAX;
		L -= k;
		while (k >= 16) { /* process 16bits blocks */
			RM_DO16(data);
			data += 16;
			k-=16;
		}
		if (k > 0) { /* remainder */
			do
			{
				r1 += *data++;
				r2 += r1;
			} while (--k);
		}

		r1 = r1  % RM_ADLER32_MODULUS;
		r2 = r2  % RM_ADLER32_MODULUS;
	}
#ifdef DEBUG
	res = (r2 << 16) | r1;
	return res;
#else
	return (r2 << 16) | r1;
#endif
}

/* rolling adler with prime modulus won't work */
uint32_t
rm_adler32_roll(uint32_t adler, unsigned char a_k,
		unsigned char a_kL, size_t L) {
#ifdef DEBUG
	uint32_t res;
#endif
	uint32_t	r1, r2;
	r1 = adler & 0xFFFF; /* r1 and r2 from adler on block [k,k+L]] */
	r2 = (adler >> 16) & 0xFFFF;
	r1 = (r1 - a_k + a_kL) % RM_ADLER32_MODULUS; /* update */
	r2 = (r2 + r1 - L*a_k - 1) % RM_ADLER32_MODULUS;
#ifdef DEBUG
	res = (r2 << 16) | r1;
	return res;
#else
	return (r2 << 16) | r1;
#endif
}

/* modulus MUST be even */
uint32_t
rm_fast_check_roll(uint32_t adler, unsigned char a_k,
		unsigned char a_kL, size_t L) {
#ifdef DEBUG
	uint32_t res;
#endif
	uint32_t	r1, r2;
	r1 = adler & 0xFFFF; /* r1 and r2 from adler on block [k,k+L]] */
	r2 = (adler >> 16) & 0xFFFF;
	r1 = (r1 - a_k + a_kL) % RM_FASTCHECK_MODULUS; /* update */
	r2 = (r2 + r1 - L*a_k) % RM_FASTCHECK_MODULUS;
#ifdef DEBUG
	res = (r2 << 16) | r1;
	return res;
#else
	return (r2 << 16) | r1;
#endif
}


uint32_t
rm_fast_check_roll_tail(uint32_t adler, unsigned char a_k, size_t L) {
	uint32_t r1, r2;

	r1 = adler & 0xFFFF;
	r2 = (adler >> 16) & 0xFFFF;

	r1 = (r1 - a_k) % RM_FASTCHECK_MODULUS;
	r2 = (r2 - L * a_k) % RM_FASTCHECK_MODULUS;
	return (r2 << 16) | r1;
}

uint32_t
rm_rolling_ch(const unsigned char *data, size_t len,
		uint32_t M) {
	uint32_t	r1, r2;
	size_t      i;

#ifdef DEBUG
	uint32_t res;
#endif
	assert(data != NULL);
	r1 = r2 = 0;
	i = 0;
	for (; i < len; ++i) {
		r1 = (r1 + data[i]) % M;
		r2 = (r2 + r1) % M;
	}
#ifdef DEBUG
	res = (r2 << 16) | r1;
	return res;
#else
	return (r2 << 16) | r1;
#endif
}


void
rm_md5(const unsigned char *data, size_t len,
		unsigned char res[16]) {
	MD5_CTX ctx;
	md5_init(&ctx);
	md5_update(&ctx, data, len);
	md5_final(&ctx, res);
}

enum rm_error
rm_copy_buffered(FILE *x, FILE *y, size_t bytes_n, pthread_mutex_t *file_mutex)
{
	size_t read, read_exp;
	char buf[RM_L1_CACHE_RECOMMENDED];
	enum rm_error err = RM_ERR_OK;

	if (file_mutex != NULL)
		pthread_mutex_lock(file_mutex);

	rewind(x);
	rewind(y);
	read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
		RM_L1_CACHE_RECOMMENDED : bytes_n;
	while (bytes_n > 0 && ((read = fread(buf, 1, read_exp, x)) == read_exp)) {
		if (fwrite(buf, 1, read_exp, y) != read_exp) {
			err = RM_ERR_WRITE;
			goto exit;
		}
		bytes_n -= read;
		read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
			RM_L1_CACHE_RECOMMENDED : bytes_n;
	}

	if (bytes_n == 0) { /* read all bytes_n or EOF reached */
		if (feof(x)) {
			err = RM_ERR_FEOF;
			goto exit;
		}
		err = RM_ERR_OK;
		goto exit;
	}
	if (ferror(x) != 0 || ferror(y) != 0) {
		err = RM_ERR_FERROR;
		goto exit;
	}
	err = RM_ERR_TOO_MUCH_REQUESTED;

exit:
	if (file_mutex != NULL)
		pthread_mutex_unlock(file_mutex);
	return err;
}

enum rm_error
rm_copy_buffered_2(FILE *x, size_t offset, void *dst, size_t bytes_n, pthread_mutex_t *file_mutex)
{
	enum rm_error err = RM_ERR_OK;
	size_t read = 0, read_exp;

	if (file_mutex != NULL)
		pthread_mutex_lock(file_mutex);

	if (fseek(x, offset, SEEK_SET) != 0) {
		err = RM_ERR_FSEEK;
		goto exit;
	}
	read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
		RM_L1_CACHE_RECOMMENDED : bytes_n;
	while (bytes_n > 0 && ((read = fread(dst, 1, read_exp, x)) == read_exp)) {
		bytes_n -= read;
		dst = (unsigned char*)dst + read;
		read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
			RM_L1_CACHE_RECOMMENDED : bytes_n;
	}

	if (bytes_n == 0) { /* read all bytes_n or EOF reached */
		err = RM_ERR_OK;
		goto exit;
	}
	if (ferror(x) != 0) {
		err = RM_ERR_FERROR;
		goto exit;
	}
	err = RM_ERR_TOO_MUCH_REQUESTED;

exit:
	if (file_mutex != NULL)
		pthread_mutex_lock(file_mutex);
	return err;
}

/* @details This method can't be used with file locking because of internal buffering
 *          made by buffered I/O library, read() system call should be used instead. */
size_t
rm_fpread(void *buf, size_t size, size_t items_n, size_t offset, FILE *f, pthread_mutex_t *file_mutex)
{
	size_t res = 0;

	if (file_mutex != NULL)
		pthread_mutex_lock(file_mutex);

	if (fseek(f, offset, SEEK_SET) != 0) {
		return 0;
	}
	res = fread(buf, size, items_n, f);

	if (file_mutex != NULL)
		pthread_mutex_unlock(file_mutex);

	return res;
}

/* @details This method can't be used with file locking because of internal buffering
 *          made by buffered I/O library, write() system call should be used instead. */
size_t
rm_fpwrite(const void *buf, size_t size, size_t items_n, size_t offset, FILE *f, pthread_mutex_t *file_mutex)
{
	size_t res = 0;

	if (file_mutex != NULL)
		pthread_mutex_lock(file_mutex);

	if (fseek(f, offset, SEEK_SET) != 0) {
		return 0;
	}
	res = fwrite(buf, size, items_n, f);

	if (file_mutex != NULL)
		pthread_mutex_unlock(file_mutex);

	return res;
}

enum rm_error
rm_copy_buffered_offset(FILE *x, FILE *y, size_t bytes_n, size_t x_offset, size_t y_offset, pthread_mutex_t *file_mutex)
{
	enum rm_error err = RM_ERR_OK;
	size_t read, read_exp;
	size_t offset;
	char buf[RM_L1_CACHE_RECOMMENDED];

	if (file_mutex != NULL)
		pthread_mutex_lock(file_mutex);

	read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
		RM_L1_CACHE_RECOMMENDED : bytes_n;
	offset = 0;
	while (bytes_n > 0 && ((read = rm_fpread(buf, 1, read_exp, x_offset + offset, x, NULL)) == read_exp)) {
		if (rm_fpwrite(buf, 1, read_exp, y_offset + offset, y, NULL) != read_exp) {
			err = RM_ERR_WRITE;
			goto exit;
		}
		bytes_n -= read;
		offset += read;
		read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
			RM_L1_CACHE_RECOMMENDED : bytes_n;
	}

	if (bytes_n == 0) { /* read all bytes_n or EOF reached */
		if (feof(x)) {
			err = RM_ERR_FEOF;
			goto exit;
		}
		err =  RM_ERR_OK;
		goto exit;
	}
	if (ferror(x) != 0 || ferror(y) != 0) {
		err = RM_ERR_FERROR;
		goto exit;
	}
	err =  RM_ERR_TOO_MUCH_REQUESTED;

exit:
	if (file_mutex != NULL)
		pthread_mutex_unlock(file_mutex);
	return err;
}

/* If there are raw bytes to tx copy them here! */
/* NOTE: this is static function, it's body must be copied to test suite 5 for testing */
static enum rm_error
rm_rolling_ch_proc_tx(struct rm_roll_proc_cb_arg  *cb_arg, rm_delta_f *delta_f, enum RM_DELTA_ELEMENT_TYPE type,
		size_t ref, unsigned char *raw_bytes, size_t raw_bytes_n) {
	struct rm_delta_e           *delta_e;

	if ((cb_arg == NULL) || (delta_f == NULL)) {
		return RM_ERR_BAD_CALL;
	}
	delta_e = malloc(sizeof(*delta_e));
	if (delta_e == NULL) {
		return RM_ERR_MEM;
	}
	delta_e->type = type;
	delta_e->ref = ref;
	if (type == RM_DELTA_ELEMENT_RAW_BYTES) {
		if (raw_bytes == NULL) {   /* TODO Add tests for this execution path! */
			free(delta_e);
			return RM_ERR_IO_ERROR;
		}
		delta_e->raw_bytes = raw_bytes;   /* take ownership and cleanup in callback! */
	} else {
		delta_e->raw_bytes = NULL;
	}
	delta_e->raw_bytes_n = raw_bytes_n;
	TWINIT_LIST_HEAD(&delta_e->link);
	cb_arg->delta_e = delta_e;                  /* tx, signal delta_rx_tid, etc */
	delta_f(cb_arg);                            /* TX, enqueue delta */

	return RM_ERR_OK;
}

/* NOTE: @f_x MUST be already opened
 * @param   delta_f - tx/reconstruct callback, NOTE: this callback takes ownership
 *          of the delta elements allocated by rolling proc - this function MUST
 *          assert memory is freed */
/* TODO Verify hashtable locking needs for rm_rolling_ch_proc <-> rm_session_ch_ch_rx_f */
enum rm_error
rm_rolling_ch_proc(struct rm_session *s, const struct twhlist_head *h, pthread_mutex_t *h_mutex,
		FILE *f_x, rm_delta_f *delta_f, size_t from) {
	size_t          L = 0;
	size_t          copy_all_threshold = 0, copy_tail_threshold = 0, send_threshold = 0;
	uint32_t        hash = 0;
	unsigned char   *buf = NULL;
	int             fd = -1;
	struct stat     fs = { 0 };
	size_t          file_sz = 0, send_left = 0, read_now = 0, read = 0, read_begin = 0;
	uint8_t         match, beginning_bytes_in_buf;
	const struct rm_ch_ch_ref_hlink   *e = NULL;
	size_t			ref = 0;
	struct rm_ch_ch ch;
	struct rm_roll_proc_cb_arg  cb_arg = { 0 };																/* callback argument */
	size_t                      raw_bytes_n = 0, raw_bytes_max = 0;
	unsigned char               *raw_bytes = NULL;															/* buffer for literal bytes */
	size_t                      a_k_pos = 0, a_kL_pos = 0;
	unsigned char               a_k = 0, a_kL = 0;															/* bytes to remove/add from rolling checksum */
	size_t          collisions_1st_level = 0;
	size_t          collisions_2nd_level = 0;
	uint8_t         copy_all = 0, copy_all_threshold_fired = 0, copy_tail_threshold_fired = 0;

	/*(void) h_mutex;  Verify hashtable locking needs for rm_rolling_ch_proc <-> rm_session_ch_ch_rx_f */

	if ((s == NULL) || (f_x == NULL) || (delta_f == NULL)) {
		return RM_ERR_BAD_CALL;
	}
	cb_arg.s = s;                           /* setup callback argument */
	L = s->rec_ctx.L;
	if (L == 0) {
		return RM_ERR_BAD_CALL;
	}
	copy_all_threshold  = s->rec_ctx.copy_all_threshold;
	copy_tail_threshold = s->rec_ctx.copy_tail_threshold;
	send_threshold      = s->rec_ctx.send_threshold;
	if (send_threshold == 0) {
		return RM_ERR_BAD_CALL;
	}

	raw_bytes_max = rm_max(L, send_threshold);

	fd = fileno(f_x);
	if (fstat(fd, &fs) != 0) {
		return RM_ERR_FSTAT_X;
	}
	file_sz = fs.st_size;
	if (from >= file_sz) {
		return RM_ERR_TOO_MUCH_REQUESTED;   /* Nothing to do */
	}
	send_left = file_sz - from;             /* positive value */

	if ((send_left < copy_all_threshold) || (h == NULL)) {   /* copy all bytes */
		copy_all_threshold_fired = 1;
		copy_all = 1;
		goto copy_tail;
	}
	if (send_left <= copy_tail_threshold) {   /* copy all bytes */
		copy_tail_threshold_fired = 1;
		copy_all = 1;
		goto copy_tail;
	}

	buf = malloc(L * sizeof(unsigned char));
	if (buf == NULL) {
		return RM_ERR_MEM;
	}
	a_k_pos = a_kL_pos = 0;
	match = 1;
	beginning_bytes_in_buf = 0;
	do {
		if (send_left <= copy_tail_threshold) { /* send last bytes instead of doing normal lookup? */
			copy_tail_threshold_fired = 1;
			goto copy_tail;
		}
		if (match == 1) {
			read_now = rm_min(L, send_left);
			read = rm_fpread(buf, 1, read_now, a_kL_pos, f_x, NULL);
			if (read != read_now) {
				return RM_ERR_READ;
			}
			if (read_begin == 0) {
				read_begin = read;
				beginning_bytes_in_buf = 1;
			} else {
				beginning_bytes_in_buf = 0;
			}
			ch.f_ch = rm_fast_check_block(buf, read);
			a_k_pos = a_kL_pos;                             /* move a_k for next fast checksum calculation */
			a_kL_pos = rm_min(file_sz - 1, a_k_pos + L);    /* a_kL for next fast checksum calculation */
		} else {
			if (read == L && (a_kL_pos - a_k_pos == L)) {
				if (rm_fpread(&a_kL, sizeof(unsigned char), 1, a_kL_pos, f_x, NULL) != 1) {
					return RM_ERR_READ;
				}
				ch.f_ch = rm_fast_check_roll(ch.f_ch, a_k, a_kL, L);
				read = read_now = rm_max(1u, a_kL_pos - a_k_pos);
				++a_k_pos;
				a_kL_pos = rm_min(a_kL_pos + 1, file_sz - 1);
			} else {
				read = read_now = rm_max(1u, a_kL_pos - a_k_pos);
				ch.f_ch = rm_fast_check_roll_tail(ch.f_ch, a_k, a_kL_pos - a_k_pos + 1); /* previous ch was calculated on a_kL_pos - a_k_pos + 1 bytes */
				++a_k_pos;
			}
		} /* roll */
		match = 0;
		hash = twhash_min(ch.f_ch, RM_NONOVERLAPPING_HASH_BITS);
		if (h_mutex != NULL)
			pthread_mutex_lock(h_mutex);
		twhlist_for_each_entry(e, &h[hash], hlink) {        /* hit 1, 1st Level match? (hashtable hash match) */
			if (e->data.ch_ch.f_ch == ch.f_ch) {            /* hit 2, 2nd Level match?, (fast rolling checksum match) */
				if (rm_copy_buffered_2(f_x, a_k_pos, buf, read, NULL) != RM_ERR_OK) {
					return RM_ERR_COPY_BUFFERED;
				}
				beginning_bytes_in_buf = 0;
				rm_md5(buf, read, ch.s_ch.data);            /* compute strong checksum. TODO something other than MD5? */
				if (0 == memcmp(&e->data.ch_ch.s_ch.data, &ch.s_ch.data, RM_STRONG_CHECK_BYTES)) {  /* hit 3, 3rd Level match? (strong checksum match) */
					match = 1;								/* OK, FOUND */
					ref = e->data.ref;						/* reference */
					break;
				} else {
					++collisions_2nd_level;                 /* 2nd Level collision, fast checksum match but strong checksum doesn't */
				}
			} else {
				++collisions_1st_level;                     /* 1st Level collision, fast checksums are different but hashed to the same bucket */
			}
		}

		if (h_mutex != NULL)
			pthread_mutex_unlock(h_mutex);

		if (match == 1) { /* tx RM_DELTA_ELEMENT_REFERENCE, TODO free delta object in callback!*/
			if (raw_bytes_n > 0) {    /* but first: any raw bytes buffered? */
				if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_RAW_BYTES, ref - raw_bytes_n, raw_bytes, raw_bytes_n) != RM_ERR_OK) { /* send them first, move ownership of raw bytes, reference is not used for RM_DELTA_ELEMENT_RAW_BYTES*/
					return RM_ERR_TX_RAW;
				}
				raw_bytes_n = 0;
				raw_bytes = NULL;
			}
			if (read == file_sz) {
				if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_ZERO_DIFF, ref, NULL, file_sz) != RM_ERR_OK) {
					return RM_ERR_TX_ZERO_DIFF;
				}
			} else if (read < L) {
				if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_TAIL, ref, NULL, read) != RM_ERR_OK) {
					return RM_ERR_TX_TAIL;
				}
			} else {
				if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_REFERENCE, ref,  NULL, L) != RM_ERR_OK) {
					return RM_ERR_TX_REF;
				}
			}
			send_left -= read;
		} else { /* tx raw bytes */
			if (raw_bytes_n == 0) {
				raw_bytes = malloc(raw_bytes_max * sizeof(*raw_bytes));
				if (raw_bytes == NULL) {
					return RM_ERR_MEM;
				}
				memset(raw_bytes, 0, raw_bytes_max * sizeof(*raw_bytes));
			}
			if (beginning_bytes_in_buf == 1 && a_k_pos < read_begin) {  /* if we have still L bytes read at the beginning in the buffer */
				a_k = buf[a_k_pos];                                     /* read a_k byte */
			} else {
				if (rm_fpread(&a_k, sizeof(unsigned char), 1, a_k_pos, f_x, NULL) != 1) {
					return RM_ERR_READ;
				}
			}
			raw_bytes[raw_bytes_n] = a_k;                               /* enqueue raw byte */
			send_left -= 1;
			++raw_bytes_n;
			if ((raw_bytes_n == send_threshold) || (send_left == 0)) {               /* tx? TODO there will be more conditions on final transmit here! */
				if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_RAW_BYTES, a_k_pos, raw_bytes, raw_bytes_n) != RM_ERR_OK) {   /* tx, move ownership of raw bytes, reference is not used for RM_DELTA_ELEMENT_RAW_BYTES */
					return RM_ERR_TX_RAW;
				}
				raw_bytes_n = 0;
				raw_bytes = NULL;
			}
		} /* match */
	} while (send_left > 0);

	pthread_mutex_lock(&s->mutex);
	s->rec_ctx.collisions_1st_level = collisions_1st_level;
	s->rec_ctx.collisions_2nd_level = collisions_2nd_level;
	s->rec_ctx.copy_all_threshold_fired = copy_all_threshold_fired;
	s->rec_ctx.copy_tail_threshold_fired = copy_tail_threshold_fired;
	pthread_mutex_unlock(&s->mutex);

	if (raw_bytes != NULL) {
		free(raw_bytes);
	}
	if (buf != NULL) {
		free(buf);
	}
	return RM_ERR_OK;

copy_tail:
	pthread_mutex_lock(&s->mutex);
	s->rec_ctx.collisions_1st_level = collisions_1st_level;
	s->rec_ctx.collisions_2nd_level = collisions_2nd_level;
	s->rec_ctx.copy_all_threshold_fired = copy_all_threshold_fired;
	s->rec_ctx.copy_tail_threshold_fired = copy_tail_threshold_fired;
	pthread_mutex_unlock(&s->mutex);

	if ((copy_all == 0) && (copy_tail_threshold_fired == 1)) { /* if copy tail but not all */
		if (match == 0) {
			a_k_pos++;
		} else {
			a_k_pos = a_kL_pos;
		}
	}
	if (raw_bytes_n > 0) {    /* but first: any raw bytes buffered? */
		if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_RAW_BYTES, a_k_pos - raw_bytes_n, raw_bytes, raw_bytes_n) != RM_ERR_OK) { /* send them first, move ownership of raw bytes */
			return RM_ERR_TX_RAW;
		}
		raw_bytes_n = 0;
		raw_bytes = NULL;
	}
	raw_bytes = malloc(send_left * sizeof(*raw_bytes));
	if (raw_bytes == NULL) {
		if (buf != NULL) free(buf);
		return RM_ERR_MEM;
	}
	if (rm_copy_buffered_2(f_x, a_k_pos, raw_bytes, send_left, NULL) != RM_ERR_OK) {
		if (buf != NULL) free(buf);
		free(raw_bytes);
		return RM_ERR_COPY_BUFFERED_2;
	}
	if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_RAW_BYTES, a_k_pos, raw_bytes, send_left) != RM_ERR_OK) {   /* tx, move ownership of raw bytes */
		if (buf != NULL) free(buf);
		free(raw_bytes);
		return RM_ERR_TX_RAW;
	}
	if (buf != NULL) {
		free(buf);
	}
	return RM_ERR_OK;
}

enum rm_error
rm_launch_thread(pthread_t *t, void*(*f)(void*), void *arg, int detachstate) {
	int                 err;
	pthread_attr_t      attr;

	err = pthread_attr_init(&attr);
	if (err != 0) {
		return RM_ERR_FAIL;
	}
	err = pthread_attr_setdetachstate(&attr, detachstate);
	if (err != 0) {
		goto fail;
	}
	err = pthread_create(t, &attr, f, arg);
	if (err != 0) {
		goto fail;
	}
	pthread_attr_destroy(&attr);
	return 0;
fail:
	pthread_attr_destroy(&attr);
	return RM_ERR_FAIL;
}

enum rm_error
rm_roll_proc_cb_1(void *arg) {
	struct rm_roll_proc_cb_arg		*cb_arg = NULL;         /* callback argument */
	struct rm_session				*s = NULL;
	struct rm_session_push_local	*prvt_local = NULL;
	struct rm_session_push_tx		*prvt_tx = NULL;
	struct rm_delta_e				*delta_e = NULL;
	enum rm_session_type			t = 0;
	struct rm_delta_reconstruct_ctx	*ctx = NULL;
	uint8_t							update_ctx = 0;

	cb_arg = (struct rm_roll_proc_cb_arg*) arg;
	if (cb_arg == NULL) {
		RM_DEBUG_LOG_CRIT("%s", "WTF! NULL callback argument?! Have you added some neat code recently?");
		assert(cb_arg != NULL);
		return RM_ERR_BAD_CALL;
	}

	s = cb_arg->s;
	delta_e = cb_arg->delta_e;
	if (s == NULL) {
		RM_DEBUG_LOG_CRIT("%s", "WTF! NULL session?! Have you added  some neat code recently?");
		assert(s != NULL);
		return RM_ERR_BAD_CALL;
	}
	if (delta_e == NULL) {
		RM_DEBUG_LOG_CRIT("%s", "WTF! NULL delta element?! Have you added some neat code recently?");
		assert(delta_e != NULL);
		return RM_ERR_BAD_CALL;
	}

	ctx = &s->rec_ctx;
	t = s->type;
	switch (t) {
		case RM_PUSH_LOCAL:
			prvt_local = s->prvt;
			update_ctx = 0;																									/* ctx is updated in rm_rx_process_delta_element */
			break;

		case RM_PUSH_TX:
			prvt_tx = s->prvt;
			prvt_local = (struct rm_session_push_local*) &prvt_tx->session_local;
			update_ctx = 1;																									/* in remote TX this is **really** transmitter, so we do update reconstruction ctx */ 
			break;

		default:
			return RM_ERR_BAD_CALL;
	}
	if (prvt_local == NULL) {
		RM_DEBUG_LOG_CRIT("%s", "WTF! NULL private session?! Have you added  some neat code recently?");
		assert(prvt_local != NULL);
		return RM_ERR_BAD_CALL;
	}

	if (update_ctx) {																										/* remote TX ? */
		pthread_mutex_lock(&s->mutex);
		switch (delta_e->type) {

			case RM_DELTA_ELEMENT_REFERENCE:
				ctx->rec_by_ref += ctx->L;																					/* L bytes copied from @y */
				++ctx->delta_ref_n;
				break;

			case RM_DELTA_ELEMENT_RAW_BYTES:
				ctx->rec_by_raw += delta_e->raw_bytes_n;
				++ctx->delta_raw_n;
				break;

			case RM_DELTA_ELEMENT_ZERO_DIFF:
				ctx->rec_by_ref += delta_e->raw_bytes_n; /* delta ZERO_DIFF has raw_bytes_n set to indicate bytes that matched (whole file) so we can nevertheless check here at receiver that is correct */
				++ctx->delta_ref_n;
				ctx->rec_by_zero_diff += delta_e->raw_bytes_n;
				++ctx->delta_zero_diff_n;
				break;

			case RM_DELTA_ELEMENT_TAIL:
				ctx->rec_by_ref += delta_e->raw_bytes_n; /* delta TAIL has raw_bytes_n set to indicate bytes that matched (that tail) so we can nevertheless check here at receiver there is no error */
				++ctx->delta_ref_n;
				ctx->rec_by_tail += delta_e->raw_bytes_n;
				++ctx->delta_tail_n;
				ctx->delta_tail_n = 1;
				break;

			default:
				pthread_mutex_unlock(&s->mutex);
				assert(1 == 0 && "Unknown delta element type!");
				return RM_ERR_ARG;
		}
		pthread_mutex_unlock(&s->mutex);
	}

	pthread_mutex_lock(&prvt_local->tx_delta_e_queue_mutex);    /* enqueue delta (and move ownership to delta_rx_tid!) */
	twfifo_enqueue(&delta_e->link, &prvt_local->tx_delta_e_queue);
	pthread_cond_signal(&prvt_local->tx_delta_e_queue_signal);
	pthread_mutex_unlock(&prvt_local->tx_delta_e_queue_mutex);

	return RM_ERR_OK;
}

int
rm_file_cmp(FILE *x, FILE *y, size_t x_offset, size_t y_offset, size_t bytes_n) {
	size_t read = 0, read_exp;
	unsigned char buf1[RM_L1_CACHE_RECOMMENDED], buf2[RM_L1_CACHE_RECOMMENDED];

	if (fseek(x, x_offset, SEEK_SET) != 0 || fseek(y, y_offset, SEEK_SET) != 0) {
		return RM_ERR_FSEEK;
	}
	read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
		RM_L1_CACHE_RECOMMENDED : bytes_n;
	while ((bytes_n > 0) && ((read = fread(buf1, 1, read_exp, x)) == read_exp)) {
		if (fread(buf2, 1, read_exp, y) != read_exp) {
			return RM_ERR_READ;
		}
		if (memcmp(buf1, buf2, read) != 0) {
			return RM_ERR_FAIL;
		}
		bytes_n -= read;
		read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ? RM_L1_CACHE_RECOMMENDED : bytes_n;
	}

	if (bytes_n == 0) { /* read all bytes_n or EOF reached */
		if (feof(x) || feof(y)) {
			return RM_ERR_FEOF;
		}
		if (ferror(x) != 0 || ferror(y) != 0) {
			return RM_ERR_FERROR;
		}
		return RM_ERR_OK;
	}
	return RM_ERR_IO_ERROR;   /* I/O operation failed */
}

void
rm_get_unique_string(char name[RM_UNIQUE_STRING_LEN]) {
	uuid_t out_and_then_in;
	uuid_generate(out_and_then_in);
	uuid_unparse(out_and_then_in, (char *) name);
	name[RM_UNIQUE_STRING_LEN - 1] = '\0';                        /* assert string is null terminated */
}

uint64_t rm_gettid(void) {
	pthread_t ptid = pthread_self();
	uint64_t tid = 0;
	memcpy(&tid, &ptid, rm_min(sizeof(tid), sizeof(ptid)));
	return tid;
}
