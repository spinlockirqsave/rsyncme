/// @file      rm_session.h
/// @brief     Rsync session.
/// @author    peterg
/// @version   0.1.1
/// @date      02 Nov 2016 04:08 PM
/// @copyright LGPLv2.1


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



/// @brief      Rsync push session checksum on nonoverlaping
///		blocks producing thread routine (outbound).
void *
rm_session_push_out_ch_ch_f(void *arg);

/// @brief      Rsync push session delta reconstruction
///		thread routine (outbound).
void *
rm_session_push_out_delta_f(void *arg);

struct rm_session *
rm_session_create(uint32_t session_id,
		struct rsyncme *engine);

void
rm_session_free(struct rm_session *s);


#endif  // RSYNCME_SESSION_H

