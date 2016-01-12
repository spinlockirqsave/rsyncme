/// @file	rm_defs.h
/// @brief	Shared includes, definitions.
/// @author	Piotr Gregor piotrek.gregor at gmail.com
/// @version	0.1.1
/// @date	2 Jan 2016 11:29 AM
/// @copyright	LGPLv2.1

#ifndef RSYNCME_DEFS_H
#define RSYNCME_DEFS_H


#include <stdlib.h>             // everything
#include <stdio.h>              // most I/O
#include <sys/types.h>          // syscalls
#include <sys/stat.h>		// umask, fstat
#include <sys/socket.h>         // socket.h etc.
#include <netinet/in.h>         // networking
#include <linux/netdevice.h>
#include <arpa/inet.h>
#include <string.h>             // memset, etc.
#include <fcntl.h>              // open
#include <unistd.h>             // close
#include <errno.h>
#include <assert.h>
#include <pthread.h>            // POSIX threads
#include <stddef.h>             // offsetof
#include <signal.h>
#include <syslog.h>
#include <stdint.h>
#include <ctype.h>		// isprint


#include "twlist.h"
#include "twhash.h"


#define RM_BIT_0	(1 << 0)  
#define RM_BIT_1	(1 << 1)
#define RM_BIT_2	(1 << 2)
#define RM_BIT_3	(1 << 3)
#define RM_BIT_4	(1 << 4)
#define RM_BIT_5	(1 << 5)
#define RM_BIT_6	(1 << 6)
#define RM_BIT_7	(1 << 7)

// TCP messages
#define RM_MSG_PUSH_IN			1	// rsync push inbound
#define RM_MSG_PUSH_OUT			2	// rsync push outbound
#define RM_MSG_PULL_IN			3	// rsync pull inb
#define RM_MSG_PULL_OUT			4	// rsync pull outb
#define RM_MSG_BYE			255	// close the controlling connection

#define RM_SESSION_HASH_BITS		10	// 10 bits hash, array size == 1024
#define RM_FILE_LEN_MAX			150	// max len of -x -y files

#define RM_ADLER32_MODULUS		65521L	// biggest prime int less than 2^16
#define RM_ADLER32_NMAX			5552	// biggest integer that allows for
						// deferring of modulo reduction so that
						// s2 will still fit in 32 bits when modulo
						// is being done with 65521 value

// CORE
#define RM_CORE_ST_INITIALIZED		0	// init function returned with no errors
#define RM_CORE_ST_SHUT_DOWN		255	// shutting down, no more requests
#define RM_CORE_CONNECTIONS_MAX		1	// max number of simultaneous connections
#define RM_CORE_HASH_OK			84
#define RM_CORE_DAEMONIZE		0	// become daemon or not, turn it to off
						// while debugging for convenience

#define rm_container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})


#endif  // RSYNCME_DEFS_H
