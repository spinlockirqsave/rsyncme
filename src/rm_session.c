/// @file      rm_session.c
/// @brief     Rsync session.
/// @author    peterg
/// @version   0.1.1
/// @date      02 Nov 2016 04:08 PM
/// @copyright LGPLv2.1


#include "rm_session.h"


void*
rm_session_core_f(void *arg)
{
        struct rm_session *s =
                        (struct rm_session *)arg;
        assert(s != NULL);
        return 0;
}

struct rm_session *
rm_session_create(uint32_t session_id,
		struct rsyncme *rm)
{
	struct rm_session *s;
	s = malloc(sizeof *s);
	if (s == NULL)
		return NULL;
	memset(s, 0, sizeof(*s));
	s->session_id = session_id;
	pthread_mutex_init(&s->session_mutex, NULL);
	s->rm = rm;
        return s;
}

void
rm_session_free(struct rm_session *s)
{
	assert(s != NULL);
	pthread_mutex_destroy(&s->session_mutex);
	free(s);
}

