/// @file       rm.c
/// @brief      Simple rsync algorithm implementation
///		as described in Tridgell A., "Efficient Algorithms
///		for Sorting and Synchronization", 1999.
/// @details	Common definitions used by sender and receiver.
/// @author     Piotr Gregor piotrek.gregor at gmail.com
/// @version    0.1.2
/// @date       1 Jan 2016 07:50 PM
/// @copyright  LGPLv2.1v2.1


#include "rm.h"


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
rm_rolling_ch_proc(const struct twhlist_head *h, FILE *f_x, rm_delta_f *delta_f,
        uint32_t L, size_t from)
{
    uint32_t        hash;
    unsigned char   buf[L];
    int             fd;
	struct stat     fs;
    size_t          file_sz, read_left, read_now, read;
    uint8_t         match;
    const struct rm_ch_ch_ref_hlink   *e;
    struct rm_ch_ch ch;
    size_t          collisions_1st_level = 0;
    size_t          collisions_2nd_level = 0;

    if (L == 0)
        return -1;
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

    /* 1. initial checksum and it's hash */
    ch.f_ch = rm_fast_check_block(buf, L);
    hash = twhash_min(ch.f_ch, RM_NONOVERLAPPING_HASH_BITS);

    /* roll hash, lookup in table, produce delta elemenets */
    do
    {
        match = 0;
        /* hash lookup */
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
        /* build delta */
        /* found a match? */
        if (match == 1)
        {
            // TODO tx RM_DELTA_ELEMENT_REFERENCE, move file pointer accordingly
        } else {
            // TODO tx bytes, consider some buffering strategy
        }
        read_left = 0;// temporary
    } while (read_left > 0);

end:
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
