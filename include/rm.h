/*
 * @file        rm.h
 * @brief       Simple rsync algorithm implementation
 *              as described in Tridgell A., "Efficient Algorithms
 *              for Sorting and Synchronization", 1999.
 * @details     Common definitions used by sender and receiver.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        1 Jan 2016 07:50 PM
 * @copyright   LGPLv2.1
 */


#ifndef RSYNCME_H
#define RSYNCME_H

/*
* Implementation is based on Tridgell A.,
* "Efficient Algorithms for Sorting and
* Synchronization", 1999.
* 
* Notation:
*
* sender(A)
* receiver(B)
* F'_A - changed file F in A
* F'_B - changed file F in B (result of sync
*		so that F'_B == F'_A)
* F_A  - previous version of file F in A
* F_B  - previous version of file F in B
*
* When sender (A) made a change to current
* versiion of file F (F_A),
* it has produced new file F'_A and now
* it will synchronize this new file with
* other version F_B (maybe same as F_A,
* maybe not) stored in receiver (B).
*
* ------------------------------------
* A[sender]     ->    B[receiver]
* 1. produces F'_A
*			2. produces ch_ch_h vec
* 3. produces delta_vec
*			4. reconstructs F'_A
*			   with  F'_B 
* F_A, F'_A           F_B,F'_B=f(delta_vec,F_B)
* ------------------------------------
*
* Complete flow:
*
* 1.	A made a change to F_A, produced F'_A
* 	and sent sync request to B.
* ------------------------------------
* A[sender]     ->    B[receiver]
* F_A, F'A    sync_req       F_B
* ------------------------------------
* 2.	B calculates ch_ch vector.
*	and sends ch_ch vector to A.
* ------------------------------------
* A[sender]     <-    B[receiver]
* F_A, F'A    ch_ch        F_B
* ------------------------------------
* 3.	A uses ch_ch vector to produce delta
* 	vector of token matches and/or literal
*	bytes using rolling checksum and sends
*	delta vector to B.
* A[sender]     ->    B[receiver]
* F_A, F'A    delta_vec      F_B
* ------------------------------------
* 4.	B reconstructs F'_A with F'_B from
*	delta_vec and previous version F_B
*	of file F.
* ------------------------------------
* A[sender]     ->    B[receiver]
* F_A, F'_A           F_B,F'_B=f(delta_vec,F_B)
* ------------------------------------
*/


#include "rm_defs.h"
#include "md5.h"

/* @brief   Strong checksum struct. MD5. */
struct rm_md5
{
	unsigned char data[RM_STRONG_CHECK_BYTES];
};

/* @brief   Checksum checksum struct. */
struct rm_ch_ch
{
    uint32_t		f_ch;   /*Fast and very cheap
                            * 32-bit rolling checksum,
                            * MUST be very cheap to compute
                            * at every byte offset */
    struct rm_md5   s_ch;   /* Strong and computationally expensive 128-bit checksum,
                            * MUST have a very low probability of collision.
                            * This is computed only when fast & cheap checksum matches
                            * one of the fast & cheap checksums in ch_ch vector. */
};

/* @brief   Checksum checksum struct used for remote/local syncs.
 * @details Contains offset to keep reference to location
 *          in file these checksums correspond to. */
struct rm_ch_ch_ref
{
    struct rm_ch_ch     ch_ch;
    size_t              ref;    /* The reference to location in B's F_B file
                                 * (taken from ch_ch list), block number */
};

/* @brief   Checksum checksum struct used for local syncs. */
struct rm_ch_ch_ref_link
{
    struct rm_ch_ch_ref data;
	struct twlist_head  link;
};

/* @brief   Checksum checksum struct used for remote/local syncs. */
struct rm_ch_ch_ref_hlink
{
    struct rm_ch_ch_ref data;
	struct twhlist_node hlink;
};

/* Delta vector element. */
struct rm_delta_e
{
};

/* @brief   Calculate similar to adler32 fast checkum on a given
 *          file block of size @len starting from @data.
 * @details Adler checksum uses prime number 65521 as modulus.
 *          This allows for better strength than if 2^16 was used
 *          but requires more computation effort (adler32 is more
 *          reliable than Fletcher16 but less than Fletcher32). But
 *          then rolling is not possible. To enable rolling feature
 *          RM_FASTCHECK_MODULUS is used (2^16). */
uint32_t
rm_fast_check_block(const unsigned char *data, uint32_t len);

/* @brief   Calculate adler32 checkum on a given file block
 *          of size @len starting from @data.
 * @details Adler checksum uses prime number 65521 as modulus.
 *          This allows for better strength than if 2^16 was used
 *          but requires more computation effort. Adler32 is more
 *          reliable than Fletcher16 but less than Fletcher32.
 *          Adler32 detects all burst errors up to 7 bits long.
 *          Excluding burst errors that change data in the data
 *          blocks from all zeros to all ones or vice versa,
 *          all burst errors less than 31 bits are detected.
 *          This is 1-bit less than the Fletcher32 checksum. The
 *          The reason being that Adler32 uses prime modulus. */
uint32_t
rm_adler32_1(const unsigned char *data, uint32_t len);

/* @brief   Efficient version of Adler32. Postpones modulo reduction
 *          up to the point when this is absolutely necessary
 *          to keep second sum s2 in 32 bits by setting block
 *          size to RM_ADLER32_NMAX. Unrolls loop.
 * @details RM_ADLER32_NMAX is found as a bigger integer that satisfy
 *          g(x) = 255*x*(x+1)/2 + (x+1)*(65536-1)-4294967295 <=0.
 *          The rationale for this is: the max of s1 at the beginning
 *          of calculation for any block of size NMAX is s1MAX=BASE-1.
 *          The max value that s2 can get to when calculating on this
 *          block is then
 *          s2MAX = (BASE-1)(n+1) + sum[i in 1:n]255i
 *          s2MAX = (BASE-1)(n+1) + (1+n)n255/2,
 *          and s2MAX MUST fit in 32bits.
 *          @param	adler: initial value, should be 1 if this is beginning */
uint32_t
rm_adler32_2(uint32_t adler, const unsigned char *data, uint32_t len);

/* @brief   Rolling Adler32 from block [k,k+L-1] to [k+1,k+L].
 * @details	Updates @adler by removal of byte k and addition
 *          of byte k+L. */
uint32_t
rm_adler32_roll(uint32_t adler, unsigned char a_k,
		unsigned char a_kL, uint32_t L);
uint32_t
rm_fast_check_roll(uint32_t adler, unsigned char a_k,
		unsigned char a_kL, uint32_t L);

/* @brief   Calculate rolling checkum on a given file block
 *          of size @len starting from @data, modulo @M.
 * @details @M MUST be less than 2^16, 0x10000 */
uint32_t
rm_rolling_ch(const unsigned char *data, uint32_t len, uint32_t M); 

/* @brief   Calculate strong checkum on a given file block
 *          of size @len starting from @data.
 * @details Implemented reusing Brad Conte's MD5 code from his
 *          crypto-algorithms repo (see include/md5.h). */
void
rm_md5(const unsigned char *data, uint32_t len, unsigned char res[16]);

/* @brief   Copy @bytes_n bytes from @x into @y.
 * @details Calls fread/fwrite buffered API functions.
 *          Files must be already opened.
 * @return  0: succes,
 *          -1: fwrite failed,
 *          -2: fread failed,
 *          -3: other error set on @y */
int
rm_copy_buffered(FILE *x, FILE *y, size_t bytes_n);


typedef int (rm_delta_f)(void*);


#endif	/* RSYNCME_H */
