/* @file        rm_defs.h
 * @brief       Basic includes and definitions.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        2 Jan 2016 11:29 AM
 * @copyright	LGPLv2.1 */

#ifndef RSYNCME_DEFS_H
#define RSYNCME_DEFS_H


#define RM_VERSION "0.1.2"


#define _POSIX_C_SOURCE 200112L
#define _LARGEFILE64_SOURCE 1
#ifndef _REENTRANT
#define _REENTRANT 1
#endif
#ifndef _THREAD_SAFE
#define _THREAD_SAFE 1
#endif

/* To make off_t (used in calls to fseeko) 64 bits type
 * on 32-bit architectures.
 * This will make the names of the (otherwise 32 bit) functions
 * and data types refer to their 64 bit counterparts.
 * off_t will be off64_t, lseek() will use lseek64(), etc.
 * The 32 bit interface will forward calls to 64 bit versions. */
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>             /* everything */
#include <stdio.h>              /* most I/O */
#include <sys/types.h>          /* syscalls */
#include <sys/stat.h>           /* umask, fstat */
#include <sys/socket.h>         /* socket.h etc. */
#include <netinet/in.h>         /* networking */
#include <linux/netdevice.h>
#include <arpa/inet.h>
#include <string.h>             /* memset, strdup, etc. */
#include <fcntl.h>              /* open, R_ONLY */
#include <unistd.h>             /* close */
#include <errno.h>
#include <assert.h>
#include <pthread.h>            /* POSIX threads */
#include <stddef.h>             /* offsetof */
#include <signal.h>
#include <syslog.h>
#include <stdint.h>
#include <ctype.h>              /* isprint */
#include <libgen.h>             /* dirname */
#include <uuid/uuid.h>
#include <sys/time.h>


#include "twlist.h"
#include "twhash.h"


#define RM_BIT_0	(1u << 0)  
#define RM_BIT_1	(1u << 1)
#define RM_BIT_2	(1u << 2)
#define RM_BIT_3	(1u << 3)
#define RM_BIT_4	(1u << 4)
#define RM_BIT_5	(1u << 5)
#define RM_BIT_6	(1u << 6)
#define RM_BIT_7	(1u << 7)

/* TCP messages */
#define RM_MSG_PUSH                 2           /* rsync push */
#define RM_MSG_PULL                 3           /* rsync pull */
#define RM_MSG_BYE                  255         /* close the controlling connection */

#define RM_SESSION_HASH_BITS        10          /* 10 bits hash, array size == 1024 */
#define RM_NONOVERLAPPING_HASH_BITS 16          /* 16 bits hash, array size == 65536 */
#define RM_FILE_LEN_MAX             150         /* max len of names of @x, @y files */

#define RM_ADLER32_MODULUS          65521L      /* biggest prime int less than 2^16 */
#define RM_FASTCHECK_MODULUS        65536L      /* 2^16, this makes rolling calculation possible */
#define RM_ADLER32_NMAX             5552L       /* biggest integer that allows for deferring
                                                 * of modulo reduction so that s2 will still fit in
                                                 * 32 bits when modulo is being done with 65521 value */

/* CORE */
#define RM_CORE_ST_INITIALIZED      0           /* init function returned with no errors */
#define RM_CORE_ST_SHUT_DOWN        255         /* shutting down, no more requests */
#define RM_CORE_CONNECTIONS_MAX     1           /* max number of simultaneous connections */
#define RM_CORE_HASH_OK             84
#define RM_CORE_DAEMONIZE           0           /* become daemon or not, turn it to off
                                                   while debugging for convenience */
#define RM_STRONG_CHECK_BYTES       16
#define RM_CH_CH_SIZE (sizeof((((struct rm_ch_ch_ref*)0)->ch_ch)))
#define RM_CH_CH_REF_SIZE (RM_CH_CH_SIZE + \
        (sizeof(((struct rm_ch_ch_ref*)0)->ref)))
#define RM_NANOSEC_PER_SEC          1000000000U

/* defaults */
#define RM_DEFAULT_L                512         /* default block size in bytes */
#define RM_L1_CACHE_RECOMMENDED     8192        /* buffer size, so that it should fit into
                                                 * L1 cache on most architectures */

#define rm_container_of(ptr, type, member) __extension__({  \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define rm_max(a,b) __extension__	({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define rm_min(a,b)	__extension__ ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _b : _a; })

typedef uint8_t rm_push_flags;  /* Bit  meaning
                                 * 0    push/pull request,
                                 * 1    @x name given,
                                 * 2    @y name given,
                                 * 3    @z name given,
                                 * 4    (--force) force creation of @y if it doesn't exist
                                 * 5    IPv4 given,
                                 * 6    (--leave) do not delete @y after @z has been reconstructed,
                                 * 7 */

enum rm_session_type
{
    RM_PUSH_LOCAL,
    RM_PUSH_RX,     /* receiver of file (delta vector) in PUSH request, and transmitter of nonoverlapping checksums */
    RM_PUSH_TX,     /* transmitter of delta vector in PUSH request, and receiver of nonoverlapping checksums, initiates the request */
    RM_PULL_RX,     /* receiver of file (delta vector) in PULL request, and transmitter of nonoverlapping checksums, initiates the request */
    RM_PULL_TX     /* transmitter of delta vector in PULL request, and receiver of nonoverlapping checksums */
};

enum rm_error {
    RM_ERR_OK = 0,
    RM_ERR_FAIL = 1,
    RM_ERR_IO_ERROR = 2,
    RM_ERR_GENERAL_ERROR = 3,
    RM_ERR_BAD_CALL = 4,
    RM_ERR_OPEN_X = 5,
    RM_ERR_OPEN_Y = 6,
    RM_ERR_OPEN_Z = 7,
    RM_ERR_OPEN_TMP = 8,
    RM_ERR_READ = 9,
    RM_ERR_WRITE = 10,
    RM_ERR_FSTAT_X = 11,
    RM_ERR_FSTAT_Y = 12,
    RM_ERR_FSTAT_Z = 13,
    RM_ERR_NONOVERLAPPING_INSERT = 14,
    RM_ERR_COPY_BUFFERED = 15,
    RM_ERR_COPY_BUFFERED_2 = 16,
    RM_ERR_COPY_OFFSET = 17,
    RM_ERR_ROLLING_CHECKSUM = 18,
    RM_ERR_RECONSTRUCTION = 19,
    RM_ERR_CREATE_SESSION = 20,
    RM_ERR_DELTA_TX_THREAD_LAUNCH = 21,
    RM_ERR_DELTA_RX_THREAD_LAUNCH = 22,
    RM_ERR_DELTA_TX_THREAD = 23,
    RM_ERR_DELTA_RX_THREAD = 24,
    RM_ERR_FILE_SIZE = 25,
    RM_ERR_FILE_SIZE_REC_MISMATCH = 26,
    RM_ERR_UNLINK_Y = 27,
    RM_ERR_RENAME_TMP_Y = 28,
    RM_ERR_RENAME_TMP_Z = 29,
    RM_ERR_MEM = 30,
    RM_ERR_CHDIR = 31,
    RM_ERR_GETCWD = 32,
    RM_ERR_FEOF = 33,
    RM_ERR_FERROR = 34,
    RM_ERR_TOO_MUCH_REQUESTED = 35,
    RM_ERR_FSEEK = 36,
    RM_ERR_RX = 37,
    RM_ERR_TX = 38,
    RM_ERR_TX_RAW = 39,
    RM_ERR_TX_ZERO_DIFF = 40,
    RM_ERR_TX_TAIL = 41,
    RM_ERR_TX_REF = 42
};

/* prototypes */
char *strdup(const char *s);


#endif  /* RSYNCME_DEFS_H */
