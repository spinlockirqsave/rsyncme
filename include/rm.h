/* @file        rm.h
 * @brief       Simple rsync algorithm implementation
 *              as described in Tridgell A., "Efficient Algorithms
 *              for Sorting and Synchronization", 1999.
 * @details     Common definitions used by sender and receiver.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        1 Jan 2016 07:50 PM
 * @copyright   LGPLv2.1 */


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
    uint32_t        f_ch;   /*Fast and very cheap
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

enum RM_DELTA_ELEMENT_TYPE
{
    RM_DELTA_ELEMENT_REFERENCE, /* reference to block */
    RM_DELTA_ELEMENT_RAW_BYTES, /* data bytes. Bytes contained in delta raw element are always contiguous bytes from @x */
    RM_DELTA_ELEMENT_ZERO_DIFF, /* sent always as single element in delta vector, bytes matched(raw_bytes_n set) == file_sz <= L
                                   when L => f_x.sz and checksums computed on the whole file match,
                                   means files are the same. raw_bytes_n set to file size */
    RM_DELTA_ELEMENT_TAIL       /* match is found on the tail, bytes matched(raw_bytes_n set) < L < file_sz,
                                 * raw_bytes_n set to number of bytes that matched */
};

/* HIGH LEVEL API
 * Delta vector element. As created from bytes on wire. */
struct rm_delta_e
{
    enum RM_DELTA_ELEMENT_TYPE  type;
    size_t                      ref;
    unsigned char               *raw_bytes;
    size_t                      raw_bytes_n;
    struct twlist_head          link;           /* to link me in list/stack/queue */
};
enum rm_delta_tx_status
{
    RM_DELTA_TX_STATUS_OK               = 0,    /* most wanted */
    RM_DELTA_TX_STATUS_ROLLING_PROC_FAIL  = 1   /* error in rolling checkum procedure */
};
enum rm_delta_rx_status
{
    RM_DELTA_RX_STATUS_OK               = 0,    /* most wanted */
    RM_DELTA_RX_STATUS_INTERNAL_ERR     = 1,    /* bad call, NULL session, prvt session or file pointers */
    RM_DELTA_RX_STATUS_DELTA_PROC_FAIL  = 2     /* error processing delta element */
};
enum rm_reconstruct_method
{
    RM_RECONSTRUCT_METHOD_DELTA_RECONSTRUCTION  = 0,    /* usual */
    RM_RECONSTRUCT_METHOD_COPY_BUFFERED         = 1     /* @y doesn't exist and --forced flag is specified, rolling proc is not used, file is simply copied,
                                                           rec_ctx->delta_raw_n == 1, rec_ctx->rec_by_raw == file size */
};
struct rm_delta_reconstruct_ctx
{
    enum rm_reconstruct_method  method;
    size_t                      rec_by_ref, rec_by_raw,
                                delta_ref_n, delta_raw_n,
                                rec_by_tail, rec_by_zero_diff,
                                delta_tail_n, delta_zero_diff_n;
    size_t                      L;
    size_t                      copy_all_threshold, copy_tail_threshold, send_threshold;
    struct timespec             time_real;
    double                      time_cpu;
    size_t                      collisions_1st_level, collisions_2nd_level, collisions_3rd_level;
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
rm_fast_check_block(const unsigned char *data, size_t len);

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
rm_adler32_1(const unsigned char *data, size_t len);

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
rm_adler32_2(uint32_t adler, const unsigned char *data, size_t len);

/* @brief   Rolls fast checksum @adler computed on block [k,k+L-1]
 *          to [k+1,k+L].
 * @details	Updates @adler by removal of byte k and addition
 *          of byte k+L using RM_ADLER32_MODULUS. */
uint32_t
rm_adler32_roll(uint32_t adler, unsigned char a_k,
        unsigned char a_kL, size_t L);
/* @brief   Rolls fast checksum @adler computed on bytes [k,k+L-1]
 *          to [k+1,k+L].
 * @details	Updates @adler by removal of byte k and addition
 *          of byte k+L using RM_FASTCHECK_MODULUS. */
uint32_t
rm_fast_check_roll(uint32_t adler, unsigned char a_k,
        unsigned char a_kL, size_t L);

uint32_t
rm_fast_check_roll_tail(uint32_t adler, unsigned char a_k, size_t L);

/* @brief   Calculate rolling checkum on a given file block
 *          of size @len starting from @data, modulo @M.
 * @details @M MUST be less than 2^16, 0x10000 */
uint32_t
rm_rolling_ch(const unsigned char *data, size_t len, uint32_t M); 

/* @brief   Calculate strong checkum on a given file block
 *          of size @len starting from @data.
 * @details Implemented reusing Brad Conte's MD5 code from his
 *          crypto-algorithms repo (see include/md5.h). */
void
rm_md5(const unsigned char *data, size_t len, unsigned char res[16]);

/* @brief   Copy @bytes_n bytes from @x into @y.
 * @details Calls fread/fwrite buffered API functions.
 *          Files must be already opened.
 * @return  RM_ERR_OK: success,
 *          RM_ERR_FEOF: eof set on @x,
 *          RM_ERR_FERROR: ferror set on either @x or @y,
 *          RM_ERR_TOO_MUCH_REQUESTED: not enough data */
int
rm_copy_buffered(FILE *x, FILE *y, size_t bytes_n);

/* @brief   Copy @bytes_n bytes from @x starting at @offset
 *          into @dst buffer.
 * @details Calls fread buffered API functions writing directly to @dst.
 *          File @x must be already opened.
 * @return  RM_ERR_OK: success,
 *          RM_ERR_FSEEK: fseek failed,
 *          RM_ERR_FERROR: ferror set on @x,
 *          RM_ERR_TOO_MUCH_REQUESTED: not enough data */
int
rm_copy_buffered_2(FILE *x, size_t offset, void *dst, size_t bytes_n);

/* @brief   Read @items_n blocks of @size bytes each from stream @f
 *          at offset @offset.
 * @return  As fread, on success the number of blocks (each of @size size)
 *          read by fread. This number equals the number of bytes only when @size
 *          is sizeof(char).*/
size_t
rm_fpread(void *buf, size_t size, size_t items_n, size_t offset, FILE *f);

size_t
rm_fpwrite(const void *buf, size_t size, size_t items_n, size_t offset, FILE *f);

/* @brief   Copy @bytes_n bytes from @x at offset @x_offset into @y at @y_offset.
 * @details Calls rm_fpread/rm_fpwrite buffered API functions.
 *          Files must be already opened.
 * @return  RM_ERR_OK: success,
 *          RM_ERR_WRITE: rm_fpwrite failed,
 *          RM_ERR_FEOF: eof set on @x,
 *          RM_ERR_FERROR: error set on @x or @y,
 *          RM_ERR_TOO_MUCH_REQUESTED: not enough data */
int
rm_copy_buffered_offset(FILE *x, FILE *y, size_t bytes_n, size_t x_offset, size_t y_offset);

typedef int (rm_delta_f)(void*);

struct rm_session;

/* @brief   Rolling checksum procedure.
 * @details Runs rolling checksum procedure using @rm_fast_check_roll
 *          to move the checksum, starting from byte @from.
 * @param   h - hashtable of nonoverlapping checkums,
 * @param   f_x - file on which rolling is performed, must be already opened,
 * @param   delta_f - tx/reconstruct callback,
 * @param   L - block size,
 * @param   from - starting point, 0 to start from beginning
 * PARAMETERS TAKEN FROM session's RECONSTRUCTION CONTEXT
 * @param   copy_all_threshold - single RM_DELTA_ELEMENT_RAW_BYTES element
 *          will be passed to delta_f callback if file f_x size is below this threshold,
 * @param   copy_tail_threshold - tail will be sent as single RM_DELTA_ELEMENT_RAW_BYTES
 *          element if less than this bytes to roll has left
 * @param   send_threshold - raw bytes will not be sent if there is less than this number of them
 *          in the buffer unless delta reference elements is being produced, that means
 *          raw bytes will be sent if delta element comes or @send_threshold has been reached
 * @return  RM_ERR_OK - success,
 *          RM_ERR_BAD_CALL - NULL session has been passed,
 *          RM_ERR_FSTAT_X - fstat failed on @x,
 *          RM_ERR_TOO_MUCH_REQUESTED - not enough data in file (from >= file size),
 *          RM_ERR_MEM - malloc failed,
 *          RM_ERR_READ - fpread failed
 *          RM_ERR_TX_RAW - tx failed on raw delta element,
 *          RM_ERR_TX_REF - tx ref failed,
 *          RM_ERR_TX_TAIL - tx on tail failed,
 *          RM_ERR_TX_ZERO_DIFF - zero difference tx failed */
enum rm_error
rm_rolling_ch_proc(struct rm_session *s, const struct twhlist_head *h,
        FILE *f_x, rm_delta_f *delta_f, size_t from) __attribute__ ((nonnull(1)));

/* @brief   Start execution of @f function in new thread.
 * @details Thread is started in @detachstate with @arg argument passed to @f.
 * return   RM_ERR_OK - sccess,
 *          RM_ERR_FAIL - error initializing thread's environment */
int
rm_launch_thread(pthread_t *t, void*(*f)(void*), void *arg, int detachstate); 


struct rm_roll_proc_cb_arg
{
    struct rm_delta_e       *delta_e;
    const struct rm_session *s;
};
/* @brief   Tx delta element locally (RM_PUSH_LOCAL).
 * @details Rolling proc callback. Called synchronously.
 *          This is being called from rolling checksum proc
 *          rm_rolling_ch_proc. Enqueues delta elements to queue
 *          and signals this to delta_rx_tid in local push session.
 * @return  RM_ERR_OK - success,
 *          RM_ERR_BAD_CALL - callback argument and/or session and/or delta
 *          and/or private session object is NULL */
rm_delta_f
rm_roll_proc_cb_1 __attribute__((nonnull(1)));

/* @brief   Tx delta element from (A) to (B) (RM_PUSH_TX).
 * @details Rolling proc callback. Called synchronously.
 *          This is being called from rolling checksum proc
 *          rm_rolling_ch_proc. Transmits delta elements to remote (B)
 *          once called each time rolling procedure produces new delta
 *          element (may want to transmit already buffered bytes first). */
rm_delta_f
rm_roll_proc_cb_2;

/* @brief   Compare @bytes_n bytes of @x with @y starting from @x_offset
 *          in @x and @y_offset in @y.
 * @return  RM_ERR_OK - success, files content is the same,
 *          RM_ERR_FAIL - fail, files content differs,
 *          RM_ERR_FSEEK - fseek failed on @x or @y,
 *          RM_ERR_READ - fread failed,
 *          RM_ERR_IO_ERROR - I/O operation failed */
int
rm_file_cmp(FILE *x, FILE *y, size_t x_offset, size_t y_offset, size_t bytes_n);

/* @brief   Generate unique string.
 * @details Uses uuid generation support, the character array must be at least 37 bytes. */
void
rm_get_unique_string(char name[RM_UNIQUE_STRING_LEN]);

#endif	/* RSYNCME_H */
