/// @file       rm.c
/// @brief      Simple rsync algorithm implementation
///		as described in Tridgell A., "Efficient Algorithms
///		for Sorting and Synchronization", 1999.
/// @details	Common definitions used by sender and receiver.
/// @author     Piotr Gregor piotrek.gregor at gmail.com
/// @version    0.1.1
/// @date       1 Jan 2016 07:50 PM
/// @copyright  LGPLv2.1v2.1


#include "rm.h"


uint32_t
rm_adler32(unsigned char *data, uint32_t len)
{
	uint32_t	r1, r2, i;

#ifdef DEBUG
	uint32_t res;
#endif
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

uint32_t
rm_rolling_ch(unsigned char *data, uint32_t len,
				uint32_t M)
{
	uint32_t	r1, r2, i;

#ifdef DEBUG
	uint32_t res;
#endif
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
rm_md5(unsigned char *data, uint32_t len,
		unsigned char res[16])
{
	MD5_CTX ctx;
	md5_init(&ctx);
	md5_update(&ctx, data, len);
	md5_final(&ctx, res);
}
