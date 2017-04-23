/* @file        rm_defs.h
 * @brief       Basic includes and definitions.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        2 Jan 2016 11:29 AM
 * @copyright	LGPLv2.1 */

#ifndef RSYNCME_DEFS_H
#define RSYNCME_DEFS_H


#define RM_VERSION "0.1.2"


#define _XOPEN_SOURCE 500
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
#include <linux/limits.h>		/* PATH_MAX */
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
#include <stdarg.h>
#include <stddef.h>


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

#define RM_UNIQUE_STRING_LEN        37u         /* including '\0' at the end, MUST be longer than sizeof(uuid_t)! */
#define RM_SESSION_HASH_BITS        10          /* 10 bits hash, array size == 1024 */
#define RM_NONOVERLAPPING_HASH_BITS 17          /* 17 bits hash, array size == 131 072 */
#define RM_FILE_LEN_MAX             250         /* max len of names of @x, @y files, MUST be > RM_UNIQUE_STRING_LEN */
#define RM_UUID_LEN                 16u			/* as uuid_t on Debian */

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
#define RM_CH_CH_SIZE				(sizeof(struct rm_ch_ch))
#define RM_CH_CH_REF_SIZE			(RM_CH_CH_SIZE + (sizeof(((struct rm_ch_ch_ref*)0)->ref)))
#define RM_NANOSEC_PER_SEC          1000000000U
#define RM_CORE_HASH_CHALLENGE_BITS 32u

#define RM_DELTA_ELEMENT_TYPE_FIELD_SIZE	1	/* we need 2 bits to be honest */
#define RM_DELTA_ELEMENT_BYTES_FIELD_SIZE	8	/* it is size_t now, but we will change it to be uint64_t explicitly */
#define RM_DELTA_ELEMENT_REF_FIELD_SIZE		8	/* it is size_t now, but we will change it to be uint64_t explicitly */

/* defaults */
#define RM_DEFAULT_L                512         /* default block size in bytes */
#define RM_L1_CACHE_RECOMMENDED     8192        /* buffer size, so that it should fit into
												 * L1 cache on most architectures */
#define RM_WORKERS_N                8u          /* default number of workers for main work queue */

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

enum rm_session_type {
	RM_PUSH_LOCAL,
	RM_PUSH_RX,     /* receiver of file (delta vector) in PUSH request, and transmitter of nonoverlapping checksums */
	RM_PUSH_TX,     /* transmitter of delta vector in PUSH request, and receiver of nonoverlapping checksums, initiates the request */
	RM_PULL_RX,     /* receiver of file (delta vector) in PULL request, and transmitter of nonoverlapping checksums, initiates the request */
	RM_PULL_TX      /* transmitter of delta vector in PULL request, and receiver of nonoverlapping checksums */
};

enum rm_loglevel {
	RM_LOGLEVEL_NOTHING, /* well, log nothing */                                 /* 0 */
	RM_LOGLEVEL_NORMAL,  /* log just the most important information */           /* 1 */
	RM_LOGLEVEL_THREADS, /* log also information coming from worker threads */   /* 2 */
	RM_LOGLEVEL_VERBOSE  /* log all information that can be logged */            /* 3 */
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
	RM_ERR_FSTAT = 11,
	RM_ERR_FSTAT_X = 12,
	RM_ERR_FSTAT_Y = 13,
	RM_ERR_FSTAT_Z = 14,
	RM_ERR_FOPEN = 15,
	RM_ERR_FOPEN_STDOUT = 16,
	RM_ERR_FOPEN_STDERR = 17,
	RM_ERR_FREOPEN = 18,
	RM_ERR_FREOPEN_STDOUT = 19,
	RM_ERR_FREOPEN_STDERR = 20,
	RM_ERR_NONOVERLAPPING_INSERT = 21,
	RM_ERR_COPY_BUFFERED = 22,
	RM_ERR_COPY_BUFFERED_2 = 23,
	RM_ERR_X_ZERO_SIZE = 24,
	RM_ERR_COPY_OFFSET = 25,
	RM_ERR_ROLLING_CHECKSUM = 26,
	RM_ERR_RECONSTRUCTION = 27,
	RM_ERR_CREATE_SESSION = 28,
	RM_ERR_DELTA_TX_THREAD_LAUNCH = 29,
	RM_ERR_DELTA_RX_THREAD_LAUNCH = 30,
	RM_ERR_DELTA_TX_THREAD = 31,
	RM_ERR_DELTA_RX_THREAD = 32,
	RM_ERR_CH_CH_RX_THREAD_LAUNCH = 33,
	RM_ERR_FILE_SIZE = 34,
	RM_ERR_FILE_SIZE_REC_MISMATCH = 35,
	RM_ERR_UNLINK_Y = 36,
	RM_ERR_RENAME_TMP_Y = 37,
	RM_ERR_RENAME_TMP_Z = 38,
	RM_ERR_MEM = 39,
	RM_ERR_CHDIR = 40,
	RM_ERR_CHDIR_Y = 41,
	RM_ERR_CHDIR_Z = 42,
	RM_ERR_GETCWD = 43,
	RM_ERR_TOO_MUCH_REQUESTED = 44,
	RM_ERR_FERROR = 45,
	RM_ERR_FEOF = 46,
	RM_ERR_FSEEK = 47,
	RM_ERR_RX = 48,
	RM_ERR_TX = 49,
	RM_ERR_TX_RAW = 50,
	RM_ERR_TX_ZERO_DIFF = 51,
	RM_ERR_TX_TAIL = 52,
	RM_ERR_TX_REF = 53,
	RM_ERR_FILE = 54,
	RM_ERR_DIR = 55,
	RM_ERR_SETSID = 56,
	RM_ERR_FORK = 57,
	RM_ERR_ARG = 58,
	RM_ERR_QUEUE_NOT_EMPTY = 59,
	RM_ERR_LAUNCH_WORKER = 60,
	RM_ERR_WORKQUEUE_CREATE = 61,
	RM_ERR_WORKQUEUE_STOP = 62,
	RM_ERR_GETADDRINFO = 63,
	RM_ERR_GETPEERNAME = 64,
	RM_ERR_CONNECT_GEN_ERR = 65,
	RM_ERR_CONNECT_TIMEOUT = 66,
	RM_ERR_CONNECT_REFUSED = 67,
	RM_ERR_CONNECT_HOSTUNREACH = 68,
	RM_ERR_MSG_PT_UNKNOWN = 69,
	RM_ERR_EOF = 70,
	RM_ERR_CH_CH_TX_THREAD = 71,
	RM_ERR_CH_CH_RX_THREAD = 72,
	RM_ERR_Y_NULL = 73,
	RM_ERR_Y_Z_SYNC = 74,
	RM_ERR_BLOCK_SIZE = 75,
	RM_ERR_RESULT_F_NAME = 76,
	RM_ERR_BUSY = 77,
	RM_ERR_AUTH = 78,
	RM_ERR_Y_ZERO_SIZE = 79,
	RM_ERR_Z_ZERO_SIZE = 80,
	RM_ERR_FSTAT_TMP = 81,
	RM_ERR_UNKNOWN_ERROR = 82
		/* max error code limited by size of flags in rm_msg_push_ack (8 bits, 255) */ 
};

enum rm_io_direction {
	RM_READ,
	RM_WRITE
};

/* change rm_core_tcp_msg_hdr_validate and rm_core_tcp_msg_valid_pt if payload types are changed */
enum rm_pt_type {
	RM_PT_MSG_PUSH,
	RM_PT_MSG_PUSH_ACK,
	RM_PT_MSG_PULL,
	RM_PT_MSG_PULL_ACK,
	RM_PT_MSG_ACK,
	RM_PT_MSG_BYE
};

/* prototypes */

struct rsyncme;
struct rm_msg_hdr;
struct rm_msg;
struct rm_msg_push;
struct rm_msg_pull;

char *strdup(const char *s);


#endif  /* RSYNCME_DEFS_H */
