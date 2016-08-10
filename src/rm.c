/* @file        rm.c
 * @brief       Simple rsync algorithm implementation
 *              as described in Tridgell A., "Efficient Algorithms
 *              for Sorting and Synchronization", 1999.
 * @details     Common definitions used by sender and receiver.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
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

int
rm_copy_buffered(FILE *x, FILE *y, size_t bytes_n) {
    size_t read, read_exp;
    char buf[RM_L1_CACHE_RECOMMENDED];

    rewind(x);
    rewind(y);
    read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
        RM_L1_CACHE_RECOMMENDED : bytes_n;
    while (bytes_n > 0 && ((read = fread(buf, 1, read_exp, x)) == read_exp)) {
        if (fwrite(buf, 1, read_exp, y) != read_exp)
            return -1;
        bytes_n -= read;
        read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
            RM_L1_CACHE_RECOMMENDED : bytes_n;
    }

    if (bytes_n == 0) { /* read all bytes_n or EOF reached */
        if (feof(x)) {
            return RM_ERR_FEOF;
        }
        return RM_ERR_OK;
    }
    if (ferror(x) != 0 || ferror(y) != 0) {
        return RM_ERR_FERROR;
    }
    return RM_ERR_TOO_MUCH_REQUESTED;
}

int
rm_copy_buffered_2(FILE *x, size_t offset, void *dst, size_t bytes_n) {
    size_t read = 0, read_exp;

    if (fseek(x, offset, SEEK_SET) != 0) {
        return RM_ERR_FSEEK;
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
        return RM_ERR_OK;
    }
    if (ferror(x) != 0) {
        return RM_ERR_FERROR;
    }
    return RM_ERR_TOO_MUCH_REQUESTED;
}

size_t
rm_fpread(void *buf, size_t size, size_t items_n, size_t offset, FILE *f) {
    if (fseek(f, offset, SEEK_SET) != 0) {
        return 0;
    }
    return fread(buf, size, items_n, f);
}

size_t
rm_fpwrite(const void *buf, size_t size, size_t items_n, size_t offset, FILE *f) {
    if (fseek(f, offset, SEEK_SET) != 0) {
        return 0;
    }
    return fwrite(buf, size, items_n, f);
}

int
rm_copy_buffered_offset(FILE *x, FILE *y, size_t bytes_n, size_t x_offset, size_t y_offset) {
    size_t read, read_exp;
    size_t offset;
    char buf[RM_L1_CACHE_RECOMMENDED];

    read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
        RM_L1_CACHE_RECOMMENDED : bytes_n;
    offset = 0;
    while (bytes_n > 0 && ((read = rm_fpread(buf, 1, read_exp, x_offset + offset, x)) == read_exp)) {
        if (rm_fpwrite(buf, 1, read_exp, y_offset + offset, y) != read_exp)
            return RM_ERR_WRITE;
        bytes_n -= read;
        offset += read;
        read_exp = RM_L1_CACHE_RECOMMENDED < bytes_n ?
            RM_L1_CACHE_RECOMMENDED : bytes_n;
    }

    if (bytes_n == 0) { /* read all bytes_n or EOF reached */
        if (feof(x)) {
            return RM_ERR_FEOF;
        }
        return RM_ERR_OK;
    }
    if (ferror(x) != 0 || ferror(y) != 0) {
        return RM_ERR_FERROR;
    }
    return RM_ERR_TOO_MUCH_REQUESTED;
}

/* If there are raw bytes to tx copy them here! */
static int
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
        delta_e->raw_bytes = raw_bytes;   /* take ownership and cleanup in callback! */
        if (delta_e->raw_bytes == NULL) {   /* TODO Add tests for this execution path! */
            return RM_ERR_IO_ERROR;
        }
    } else {
        delta_e->raw_bytes = NULL;
    }
    delta_e->raw_bytes_n = raw_bytes_n;
    TWINIT_LIST_HEAD(&delta_e->link);
    cb_arg->delta_e = delta_e;                  /* tx, signal delta_rx_tid, etc */
    delta_f(cb_arg);                            /* TX, enqueue delta */

    return RM_ERR_OK;
}

/* NOTE: @f_x MUST be already opened */
enum rm_error
rm_rolling_ch_proc(struct rm_session *s, const struct twhlist_head *h,
        FILE *f_x, rm_delta_f *delta_f, size_t from) {
    size_t          L;
    size_t          copy_all_threshold, copy_tail_threshold, send_threshold;
    uint32_t        hash;
    unsigned char   *buf;
    int             fd;
    struct stat     fs;
    size_t          file_sz, send_left, read_now, read, read_begin = 0;
    uint8_t         match, beginning_bytes_in_buf;
    const struct rm_ch_ch_ref_hlink   *e;
    struct rm_ch_ch ch;
    struct rm_roll_proc_cb_arg  cb_arg;         /* callback argument */
    size_t                      raw_bytes_n;
    unsigned char               *raw_bytes;     /* buffer for literal bytes */
    size_t                      a_k_pos, a_kL_pos;
    unsigned char               a_k, a_kL;      /* bytes to remove/add from rolling checksum */
    uint8_t         hit;
    size_t          collisions_1st_level = 0;
    size_t          collisions_2nd_level = 0;
    size_t          collisions_3rd_level = 0;

    raw_bytes = NULL;
    raw_bytes_n = 0;
    buf = NULL;
    e = NULL;
    a_k_pos = 0;

    if (s == NULL || f_x == NULL) {
        return RM_ERR_BAD_CALL;
    }
    cb_arg.s = s;                           /* setup callback argument */
    L = s->rec_ctx.L;
    copy_all_threshold  = s->rec_ctx.copy_all_threshold;
    copy_tail_threshold = s->rec_ctx.copy_tail_threshold;
    send_threshold      = s->rec_ctx.send_threshold;

    fd = fileno(f_x);
    if (fstat(fd, &fs) != 0) {
        return RM_ERR_FSTAT_X;
    }
    file_sz = fs.st_size;
    if (from >= file_sz) {
        return RM_ERR_TOO_MUCH_REQUESTED;   /* Nothing to do */
    }
    send_left = file_sz - from;             /* positive value */
    if (send_left < copy_all_threshold || h == NULL) {   /* copy all bytes */
        a_kL_pos = 0;
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
        if (match == 1) {
            if (send_left == 0) { /* done? */
                goto end;
            }
            read_now = rm_min(L, send_left);
            if (send_left < copy_tail_threshold) { /* send last bytes instead of doing normal lookup? */
                goto copy_tail;
            }
            read = rm_fpread(buf, 1, read_now, a_kL_pos, f_x);
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
                if (rm_fpread(&a_kL, sizeof(unsigned char), 1, a_kL_pos, f_x) != 1) {
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
        hit = 0;
        hash = twhash_min(ch.f_ch, RM_NONOVERLAPPING_HASH_BITS);
        twhlist_for_each_entry(e, &h[hash], hlink) {        /* hit 1, 1st Level match? (hashtable hash match) */
            hit = 1;
            if (e->data.ch_ch.f_ch == ch.f_ch) {            /* hit 2, 2nd Level match?, (fast rolling checksum match) */
                if (rm_copy_buffered_2(f_x, a_k_pos, buf, read) != RM_ERR_OK) {
                    return RM_ERR_COPY_BUFFERED;
                }
                beginning_bytes_in_buf = 0;
                rm_md5(buf, read, ch.s_ch.data);            /* compute strong checksum */
                if (0 == memcmp(&e->data.ch_ch.s_ch.data, &ch.s_ch.data, RM_STRONG_CHECK_BYTES)) {  /* hit 3, 3rd Level match? (strong checksum match) */
                    match = 1;  /* OK, FOUND */
                    break;
                } else {
                    ++collisions_2nd_level;                 /* 2nd Level collision, fast checksum match but strong checksum doesn't */
                }
            } else {
                ++collisions_1st_level;                     /* 1st Level collision, fast checksums are different but hashed to the same bucket */
            }
        }

        if (match == 1) { /* tx RM_DELTA_ELEMENT_REFERENCE, TODO free delta object in callback!*/
            if (raw_bytes_n > 0) {    /* but first: any raw bytes buffered? */
                if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_RAW_BYTES, e->data.ref - raw_bytes_n, raw_bytes, raw_bytes_n) != RM_ERR_OK) { /* send them first, move ownership of raw bytes */
                    return RM_ERR_TX_RAW;
                }
                raw_bytes_n = 0;
                raw_bytes = NULL;
            }
            if (read == file_sz) {
                if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_ZERO_DIFF, e->data.ref, NULL, file_sz) != RM_ERR_OK) {
                    return RM_ERR_TX_ZERO_DIFF;
                }
            } else if (read < L) {
                if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_TAIL, e->data.ref, NULL, read) != RM_ERR_OK) {
                    return RM_ERR_TX_TAIL;
                }
            } else {
                if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_REFERENCE, e->data.ref,  NULL, L) != RM_ERR_OK) {
                    return RM_ERR_TX_REF;
                }
            }
            send_left -= read;
        } else { /* tx raw bytes */
            if (hit == 1) {             /* no match found, if hash tag was hit increase 3rd level collisions */
                ++collisions_3rd_level;
            }
            if (raw_bytes_n == 0) {
                raw_bytes = malloc(L * sizeof(*raw_bytes));
                if (raw_bytes == NULL) {
                    return RM_ERR_MEM;
                }
                memset(raw_bytes, 0, L * sizeof(*raw_bytes));
            }
            if (beginning_bytes_in_buf == 1 && a_k_pos < read_begin) {  /* if we have still L bytes read at the beginning in the buffer */
                a_k = buf[a_k_pos];                                     /* read a_k byte */
            } else {
                if (rm_fpread(&a_k, sizeof(unsigned char), 1, a_k_pos, f_x) != 1) {
                    return RM_ERR_READ;
                }
            }
            raw_bytes[raw_bytes_n] = a_k;                               /* enqueue raw byte */
            send_left -= 1;
            ++raw_bytes_n;
            if ((raw_bytes_n == send_threshold) || (send_left == 0)) {               /* tx? TODO there will be more conditions on final transmit here! */
                if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_RAW_BYTES, a_k_pos, raw_bytes, raw_bytes_n) != RM_ERR_OK) {   /* tx, move ownership of raw bytes */
                    return RM_ERR_TX_RAW;
                }
                raw_bytes_n = 0;
                raw_bytes = NULL;
            }
        } /* match */
    } while (send_left > 0);
    
    pthread_mutex_lock(&s->session_mutex);
    s->rec_ctx.collisions_1st_level = collisions_1st_level;
    s->rec_ctx.collisions_2nd_level = collisions_2nd_level;
    s->rec_ctx.collisions_3rd_level = collisions_3rd_level;
    pthread_mutex_unlock(&s->session_mutex);

end:
    if (raw_bytes != NULL) {
        free(raw_bytes);
    }
    if (buf != NULL) {
        free(buf);
    }
    return RM_ERR_OK;

copy_tail:
    raw_bytes = malloc(send_left * sizeof(*raw_bytes));
    if (raw_bytes == NULL) {
        if (buf != NULL) free(buf);
        return RM_ERR_MEM;
    }
    if (rm_copy_buffered_2(f_x, a_kL_pos, raw_bytes, send_left) != RM_ERR_OK) {
        if (buf != NULL) free(buf);
        free(raw_bytes);
        return RM_ERR_COPY_BUFFERED_2;
    }
    if (send_left < file_sz) {
            if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_RAW_BYTES, a_k_pos, raw_bytes, send_left) != RM_ERR_OK) {   /* tx, move ownership of raw bytes */
                if (buf != NULL) free(buf);
                free(raw_bytes);
                return RM_ERR_TX_RAW;
            }
    } else {
            if (rm_rolling_ch_proc_tx(&cb_arg, delta_f, RM_DELTA_ELEMENT_ZERO_DIFF, a_k_pos, raw_bytes, send_left) != RM_ERR_OK) {   /* tx, move ownership of raw bytes */
                if (buf != NULL) free(buf);
                free(raw_bytes);
                return RM_ERR_TX_ZERO_DIFF;
            }
    }
    if (raw_bytes != NULL) {
        free(raw_bytes);
    }
    if (buf != NULL) {
        free(buf);
    }
    return RM_ERR_OK;
}

int
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
    return 0;
fail:
    pthread_attr_destroy(&attr);
    return RM_ERR_FAIL;
}

int
rm_roll_proc_cb_1(void *arg) {
    struct rm_roll_proc_cb_arg      *cb_arg;         /* callback argument */
    const struct rm_session         *s;
    struct rm_session_push_local    *prvt_local;
    struct rm_delta_e               *delta_e;

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
    prvt_local = (struct rm_session_push_local*) s->prvt;
    if (prvt_local == NULL) {
        RM_DEBUG_LOG_CRIT("%s", "WTF! NULL private session?! Have you added  some neat code recently?");
        assert(prvt_local != NULL);
        return RM_ERR_BAD_CALL;
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
    uuid_unparse(out_and_then_in, name);
    name[RM_UNIQUE_STRING_LEN - 1] = '\0';                        /* assert string is null terminated */
}
