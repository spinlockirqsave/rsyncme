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
    size_t read, written, read_total;
    char buf[RM_L1_CACHE_RECOMMENDED];

    while ((read = fread(buf, 1, RM_L1_CACHE_RECOMMENDED, x)) > 0)
    {
        written = fwrite(buf, 1, read, y);
        if (written != read)
            return -1;
    }

    if (ferror(x) != 0)
        return -2;
    if (ferror(y) != 0)
        return -3;

    return 0;
}

