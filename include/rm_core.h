/* @file        rm_core.h
 * @brief       Daemon's start up.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        02 Jan 2016 02:50 PM
 * @copyright   LGPLv2.1 */


#ifndef MPEU_CORE_H
#define MPEU_CORE_H


#include "rm_defs.h"
#include "rm_config.h"
#include "rm_error.h"
#include "twlist.h"
#include "twhash.h"
#include "rm_session.h"
#include "rm_serialize.h"

#include <arpa/inet.h>


struct rsyncme
{
    pthread_mutex_t         mutex; 
    int                     state;

    TWDECLARE_HASHTABLE(sessions, RM_SESSION_HASH_BITS);
    struct twlist_head      sessions_list;
    uint32_t                sessions_n;

	uint32_t		L;  /* block size, for a given L the worst case byte overhead of rsyncme is (s_f + s_s) * n/L
                         * where s_f is size of fast signature, s_s is strong signature size, n is the number of bytes in file */
	uint32_t		M;	/* modulus in fast checksum computation, 2^16 is good choice for simplicity and speed */
};

/* @brief  Helper struct to pass connection settings into TCP events thread. */
struct rm_core_con_data
{
        struct rsyncme	*rm;
        int             connfd;
};

/* @brief      TCP connection description stored by rsyncme in the connections list.
 * @details    Might be a member of a single list at a time. */
struct rm_core_con
{
        struct twlist_head      link;
        int                     connfd;
        pthread_t               tid;
        /* probably some additional data, e.g. start time */
};

/* @brief   Initialize daemon. */
int
rm_core_init(struct rsyncme *rm);

/* @return  Pointer to locked session if found (session_mutex locked), NULL otherwise */
struct rm_session* rm_core_session_find(struct rsyncme *rm, uint32_t session_id);

/* @brief	    Creates and adds new sesion into table.
 * @details	Generates SID. */
struct rm_session*
rm_core_session_add(struct rsyncme *rm, enum rm_session_type type, size_t L);

/* @brief      Shut down. */
int
rm_core_shutdown(struct rsyncme *rm);

/* @brief      Authenticate incoming TCP managing connection. */
int
rm_core_authenticate(struct sockaddr_in *cli_addr);

/* @brief      Validate the TCP message. */
int
rm_core_tcp_msg_validate(unsigned char *buf, int read_n);


#endif  /* RSYNCME_CORE_H */

