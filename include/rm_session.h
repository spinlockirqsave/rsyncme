/// @file      rm_session.h
/// @brief     Rsync session.
/// @author    peterg
/// @version   0.1.1
/// @date      02 Nov 2016 04:08 PM
/// @copyright LGPL


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
	pthread_t               tid;

	struct rsyncme		*rm;		// pointer to global
						// rsyncme object
};


/// @brief      Main rsync session thread routine.
void *
rm_session_core_f(void *session);

struct rm_session *
rm_session_create(uint32_t session_id,
		struct rsyncme *engine);

void
rm_session_free(struct rm_session *s);


#endif  // RSYNCME_SESSION_H

