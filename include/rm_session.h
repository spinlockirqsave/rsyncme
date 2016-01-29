/// @file	rm_session.h
/// @brief	Rsync session.
/// @author	Piotr Gregor piotrek.gregor at gmail.com
/// @version	0.1.2
/// @date	02 Nov 2016 04:08 PM
/// @copyright	LGPLv2.1


#ifndef RSYNCME_SESSION_H
#define RSYNCME_SESSION_H

#include "rm_defs.h"
#include "twlist.h"


struct rm_session
{
	struct twhlist_node     hlink;		// hashtable handle
	struct twlist_head      link;		// list handle

	uint32_t                session_id;
	pthread_mutex_t         session_mutex;
	pthread_t               ch_ch_tid;
	pthread_t               delta_tid;

	struct rsyncme		*rm;		// pointer to global
						// rsyncme object
};


/// @brief	Creates new session.
struct rm_session *
rm_session_create(struct rsyncme *engine);

void
rm_session_free(struct rm_session *s);

/// @brief      Rx checksums calculated by receiver (B) on nonoverlapping
///		blocks.
void *
rm_session_ch_ch_rx_f(void *arg);

/// @brief      Tx delta reconstruction data.
void *
rm_session_delta_tx_f(void *arg);

/// @brief      Tx checksums calculated by receiver (B) on nonoverlapping
///		blocks to sender (A).
void *
rm_session_ch_ch_tx_f(void *arg);

/// @brief      Rx delta reconstruction data and do reconstruction.
void *
rm_session_delta_rx_f(void *arg);


#endif  // RSYNCME_SESSION_H

