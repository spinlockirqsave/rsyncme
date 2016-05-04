/*
 * @file        rm.c
 * @brief       Simple rsync algorithm implementation
 *              as described in Tridgell A., "Efficient Algorithms
 *              for Sorting and Synchronization", 1999.
 * @details     Common definitions used by sender and receiver.
 * @author      Piotr Gregor piotrek.gregor at gmail.com
 * @version     0.1.2
 * @date        1 Jan 2016 07:50 PM
 * @copyright   LGPLv2.1v2.1
 */


#include "rm.h"
#include "rm_util.h"
#include "rm_session.h"


uint32_t
rm_fast_check_block(const unsigned char *data, uint32_t len)
{
#ifdef DEBUG
	uint32_t res;
#endif
	uint32_t	r1, r2, i;

	assert(data != NULL);
	r1 = 0;
	r2 = 0;
	i = 0;
	for (; i < len; ++i)
	{
		r1 = (r1 + data[i]) % RM_FASTCHECK_MODULUS;
		r2 = (r2 + r1) % RM_FASTCHECK_MODULUS;
	}
#ifdef DEBUG
	res = (r2 << 16) | r1;
	return res;
#endif
	return (r2 << 16) | r1;
}

uint32_t
rm_adler32_1(const unsigned char *data, uint32_t len)
{
#ifdef DEBUG
	uint32_t res;
#endif
	uint32_t	r1, r2, i;

	assert(data != NULL);
	r1 = 1;
	r2 = 0;
	i = 0;
	for (; i < len; ++i)
	{
		r1 = (r1 + data[i]) % RM_ADLER32_MODULUS;
		r2 = (r2 + r1) % RM_ADLER32_MODULUS;
	}
#ifdef DEBUG
	res = (r2 << 16) | r1;
	return res;
#endif
	return (r2 << 16) | r1;
}

#define RM_DO1(buf, i)	{ r1 += buf[i]; r2 += r1; }
#define RM_DO2(buf, i)	RM_DO1(buf, i); RM_DO1(buf, i + 1)
#define RM_DO4(buf, i)	RM_DO2(buf, i); RM_DO2(buf, i + 2)
#define RM_DO8(buf, i)	RM_DO4(buf, i); RM_DO4(buf, i + 4)
#define RM_DO16(buf)	RM_DO8(buf,0); RM_DO8(buf,8)

uint32_t
rm_adler32_2(uint32_t adler, const unsigned char *data, uint32_t L)
{
#ifdef DEBUG
	uint32_t res;
#endif
	uint32_t	k;
	uint32_t	r1 = adler & 0xffff;
	uint32_t	r2 = (adler >> 16) & 0xffff;

	assert(data != NULL);
	while(L)
	{
		k = L < RM_ADLER32_NMAX ? L : RM_ADLER32_NMAX;
		L -= k;
		// process 16bits blocks
		while (k >= 16)
		{
			RM_DO16(data);
			data += 16;
			k-=16;
		}
		// remainder
		if (k > 0)
		{
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
#endif
	return (r2 << 16) | r1;
}

// rolling adler with prime modulus won't work
uint32_t
rm_adler32_roll(uint32_t adler, unsigned char a_k,
		unsigned char a_kL, uint32_t L)
{
#ifdef DEBUG
	uint32_t res;
#endif
	uint32_t	r1, r2;
	// r1 and r2 from adler on block [k,k+L]]
	r1 = adler & 0xFFFF;
	r2 = (adler >> 16) & 0xFFFF;
	// update
	r1 = (r1 - a_k + a_kL) % RM_ADLER32_MODULUS;
	r2 = (r2 + r1 - L*a_k - 1) % RM_ADLER32_MODULUS;
#ifdef DEBUG
	res = (r2 << 16) | r1;
	return res;
#endif
	return (r2 << 16) | r1;
}

// modulus MUST be even
uint32_t
rm_fast_check_roll(uint32_t adler, unsigned char a_k,
		unsigned char a_kL, uint32_t L)
{
#ifdef DEBUG
	uint32_t res;
#endif
	uint32_t	r1, r2;
	// r1 and r2 from adler on block [k,k+L]]
	r1 = adler & 0xFFFF;
	r2 = (adler >> 16) & 0xFFFF;
	// update
	r1 = (r1 - a_k + a_kL) % RM_FASTCHECK_MODULUS;
	r2 = (r2 + r1 - L*a_k) % RM_FASTCHECK_MODULUS;
#ifdef DEBUG
	res = (r2 << 16) | r1;
	return res;
#endif
	return (r2 << 16) | r1;
}

uint32_t
rm_rolling_ch(const unsigned char *data, uint32_t len,
				uint32_t M)
{
	uint32_t	r1, r2, i;

#ifdef DEBUG
	uint32_t res;
#endif
	assert(data != NULL);
	r1 = r2 = 0;
	i = 0;
	for (; i < len; ++i)
	{
		r1 = (r1 + data[i]) % M;
		r2 = (r2 + r1) % M;
	}
#ifdef DEBUG
	res = (r2 << 16) | r1;
	return res;
#endif
	return (r2 << 16) | r1;
}
 

void
rm_md5(const unsigned char *data, uint32_t len,
		unsigned char res[16])
{
	MD5_CTX ctx;
	md5_init(&ctx);
	md5_update(&ctx, data, len);
	md5_final(&ctx, res);
}

int
rm_copy_buffered(FILE *x, FILE *y, size_t bytes_n)
{
    size_t read, read_exp;
    char buf[RM_L1_CACHE_RECOMMENDED];

    read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
                        RM_L1_CACHE_RECOMMENDED : bytes_n;
    while ((read = fread(buf, 1, read_exp, x)) == read_exp)
    {
        if (fwrite(buf, 1, read_exp, y) != read_exp)
            return -1;
        bytes_n -= read;
        read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
                        RM_L1_CACHE_RECOMMENDED : bytes_n;
    }

    if (read == 0)
    {
        /* EOF reached */
        return 0;
    }

    if (ferror(x) != 0)
        return -2;
    if (ferror(y) != 0)
        return -3;

    return 0;
}

size_t
rm_fpread(void *buf, size_t size, size_t items_n, size_t offset, FILE *f)
{
    if (fseek(f, offset, SEEK_SET) != 0)
        return 0;
    return fread(buf, size, items_n, f);
}

int
rm_rolling_ch_proc(const struct rm_session *s, const struct twhlist_head *h,
        FILE *f_x, rm_delta_f *delta_f, uint32_t L, size_t from)
{
    uint32_t        hash;
    unsigned char   buf[L];
    int             fd;
	struct stat     fs;
    size_t          file_sz, read_left, read_now, read;
    uint8_t         match;
    const struct rm_ch_ch_ref_hlink   *e;
    struct rm_ch_ch ch;
    struct rm_roll_proc_cb_arg  cb_arg;         /* callback argument */
    struct rm_delta_e           *delta_e;
    size_t                      raw_bytes_n;
    unsigned char               *raw_bytes;     /* buffer */
    size_t                      a_k_pos, a_kL_pos;
    unsigned char               a_k, a_kL;      /* bytes to remove/add from rolling checksum */
    size_t          collisions_1st_level = 0;
    size_t          collisions_2nd_level = 0;

    if (L == 0 || s == NULL)
        return -1;
    /* setup callback argument */
    cb_arg.s = s;

    /* get file size */
    fd = fileno(f_x);
    if (fstat(fd, &fs) != 0)
    {
        /* Can't fstat file */
        return -2;
    }
    file_sz = fs.st_size;
    read_left = file_sz - from;
    if (read_left == 0)
    {
        goto end;
    }
    read_now = rm_min(L, read_left);

    read = fread(buf, 1, read_now, f_x);
    if (read != read_now)
    {
        return -3;
    }

    if (read_left < L)
    {
        /* TODO copy all raw bytes in single delta */
        goto copy_tail;
    }

    /* 1. initial checksum */
    ch.f_ch = rm_fast_check_block(buf, L);

    a_k_pos = 0;
    a_kL_pos = L;

    /* roll hash, lookup in table, produce delta elemenets */
    do
    {
        match = 0;

        /* buffer for a raw bytes, callback will take ownership of this! */
        raw_bytes = malloc(L * sizeof(*raw_bytes));
        if (raw_bytes == NULL)
        {
            return -4;
        }
        memset(raw_bytes, 0, L * sizeof(*raw_bytes));
        raw_bytes_n = 0;

        /* hash lookup */
        hash = twhash_min(ch.f_ch, RM_NONOVERLAPPING_HASH_BITS);
        twhlist_for_each_entry(e, &h[hash], hlink)
        {
            /* hit 1, 1st Level match, hashtable hash match */
            if (e->data.ch_ch.f_ch == ch.f_ch)
            {
                /* hit 2, 2nd Level match, fast rolling checksum match */
                /* compute strong checksum */
                rm_md5(buf, read, ch.s_ch.data);
                if (0 == memcmp(&e->data.ch_ch.s_ch.data, &ch.s_ch.data,
                                                        RM_STRONG_CHECK_BYTES))
                {
                    /* hit 3, 3rd Level match, strong checksum match */
                    /* tx e->data.ref */
                    /* OK, FOUND */
                    match = 1;
                    break;
                } else {
                    /* 2nd Level collision, fast checksum match but strong checksum doesn't */
                    ++collisions_2nd_level;
                }
            } else {
                /* 1st Level collision, fast checksums are different but hashed
                * to the same bucket */
                ++collisions_1st_level;
            }
        }
        /* build delta, TODO free in callback! */
        /* found a match? */
        if (match == 1)
        {
            /* tx RM_DELTA_ELEMENT_REFERENCE */
            /* any raw bytes buffered? */
            if (raw_bytes_n > 0)
            {
                /* send them first */
                delta_e = malloc(sizeof *delta_e);
                if (delta_e == NULL)
                {
                    return -5;
                }
                delta_e->type = RM_DELTA_ELEMENT_RAW_BYTES;
                delta_e->ref = 0;
                delta_e->raw_bytes = raw_bytes;
                delta_e->raw_bytes_n = raw_bytes_n;         /* move ownership, TODO cleanup in callback! */
                TWINIT_LIST_HEAD(&delta_e->link);
                /* tx, signal delta_rx_tid, etc */
                cb_arg.delta_e = delta_e;
                delta_f(&cb_arg);                           /* TX, enqueue delta */
                /* cleanup */
                raw_bytes = NULL;
                raw_bytes_n = 0;
            }
            /* tx delta ref */
            delta_e = malloc(sizeof *delta_e);
            if (delta_e == NULL)
            {
                return -7;
            }
            delta_e->type = RM_DELTA_ELEMENT_REFERENCE;
            delta_e->ref = e->data.ref;
            delta_e->raw_bytes = NULL;
            delta_e->raw_bytes_n = 0;
            TWINIT_LIST_HEAD(&delta_e->link);
            cb_arg.delta_e = delta_e;
            delta_f(&cb_arg);                               /* TX, enqueue delta */
            read_left -= L;
        } else {
            /* tx raw byte */
            if (raw_bytes == NULL)
            {
                /* alloc new buffer */
                raw_bytes = malloc(L * sizeof(*raw_bytes));
                if (raw_bytes == NULL)
                {
                    return -8;
                }
                memset(raw_bytes, 0, L * sizeof(*raw_bytes));
                raw_bytes_n = 0;
            }
            /* copy byte into the buffer*/
            raw_bytes[raw_bytes_n] = a_k;
            read_left -= 1;
            ++raw_bytes_n;
            /* tx? */
            if ((raw_bytes_n == L) || (read_left == 0))     /* TODO there will be more conditions on final transmit here! */
            {
                /* tx */
                delta_e = malloc(sizeof *delta_e);
                if (delta_e == NULL)
                {
                    return -9;
                }
                delta_e->type = RM_DELTA_ELEMENT_RAW_BYTES;
                delta_e->ref = 0;
                delta_e->raw_bytes = raw_bytes;
                delta_e->raw_bytes_n = raw_bytes_n;         /* move ownership, TODO cleanup in callback! */
                TWINIT_LIST_HEAD(&delta_e->link);
                /* tx, signal delta_rx_tid, etc */
                cb_arg.delta_e = delta_e;
                delta_f(&cb_arg);                           /* TX, enqueue delta */
                /* cleanup */
                raw_bytes = NULL;
                raw_bytes_n = 0;
            }
        } /* match */

        /* roll */
        if (match == 1)
        {
            read_now = rm_min(L, read_left);
            if (read_now < L)
            {
                goto copy_tail;
            }
            read = rm_fpread(buf, 1, L, a_kL_pos - 1, f_x);
            if (read != read_now)
            {
                return -10;
            }
            /* fast checksum */
            ch.f_ch = rm_fast_check_block(buf, L);

            /* move */
            a_k_pos = a_kL_pos;
            a_kL_pos = a_k + L;
        } else {
            /* read a_k, a_kL bytes */
            if (a_k_pos < L)
            {
                /* we have read L bytes at the beginning */
                a_k = buf[a_k_pos];
            } else {
                if (rm_fpread(&a_k, sizeof(unsigned char), 1, a_k_pos, f_x) != 1)
                {
                    return -11;
                }
            }
            if (a_kL_pos < file_sz - 1)
            {
                /* usual case */
                if (rm_fpread(&a_kL, sizeof(unsigned char), 1, a_kL_pos, f_x) != 1)
                {
                    return -12;
                }
                ch.f_ch = rm_fast_check_roll(ch.f_ch, a_k, a_kL, L);

                /* move */
                ++a_k_pos;
                ++a_kL_pos;
            } else {
                /* TODO */
                /* we are on the tail, this must be handled in a different way */
            }
        } /* roll */
    } while (read_left > 0);

end:
    return 0;
copy_tail:
    return 0;
}

int
rm_launch_thread(pthread_t *t, void*(*f)(void*), void *arg, int detachstate)
{
	int                 err;
	pthread_attr_t      attr;

	err = pthread_attr_init(&attr);
	if (err != 0)
    {
		return -1;
    }
	err = pthread_attr_setdetachstate(&attr, detachstate);
	if (err != 0)
    {
		goto fail;
    }
	err = pthread_create(t, &attr, f, arg);
	if (err != 0)
    {
        goto fail;
    }
    return 0;
fail:
	pthread_attr_destroy(&attr);
	return -1;
}

int
rm_roll_proc_cb_1(void *arg)
{
    struct rm_roll_proc_cb_arg      *cb_arg;         /* callback argument */
    const struct rm_session         *s;
    struct rm_session_push_local    *prvt;
    struct rm_delta_e               *delta_e;

    cb_arg = (struct rm_roll_proc_cb_arg*) arg;
    if (cb_arg == NULL)
    {
        RM_LOG_CRIT("WTF! NULL callback argument?! Have you added"
                " some neat code recently?");
        assert(cb_arg != NULL);
        return -1;
    }

    s = cb_arg->s;
    delta_e = cb_arg->delta_e;
    if (s == NULL)
    {
        RM_LOG_CRIT("WTF! NULL session?! Have you added"
                " some neat code recently?");
        assert(s != NULL);
        return -2;
    }
    if (delta_e == NULL)
    {
        RM_LOG_CRIT("WTF! NULL delta element?! Have you added"
                " some neat code recently?");
        assert(delta_e != NULL);
        return -3;
    }
    prvt = (struct rm_session_push_local*) s->prvt;
    if (prvt == NULL)
    {
        RM_LOG_CRIT("WTF! NULL private session?! Have you added"
                " some neat code recently?");
        assert(prvt != NULL);
        return -4;
    }

    /* enqueue delta (and move ownership to delta_rx_tid!) */
    pthread_mutex_lock(&prvt->tx_delta_e_queue_mutex);
    twfifo_enqueue(&delta_e->link, &prvt->tx_delta_e_queue);
    pthread_cond_signal(&prvt->tx_delta_e_queue_signal);
    pthread_mutex_unlock(&prvt->tx_delta_e_queue_mutex);

    return 0;
}
