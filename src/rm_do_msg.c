/// @file      rm_do_msg.c
/// @brief     Execute TCP messages.
/// @author    peterg
/// @version   0.1.1
/// @date      02 Nov 2016 04:29 PM
/// @copyright LGPLv2.1

#include "rm_defs.h"
#include "rm_core.h"
#include "rm_serialize.h"
#include "rm_session.h"

int
rm_do_msg_push_in(struct rsyncme *rm,
			unsigned char *buf)
{
	uint32_t	session_id;
	struct rm_session	*s;

        assert(rm != NULL && buf != NULL);

	// generate GUID (session id)
	session_id = 0;
        s = rm_core_session_start(rm, session_id);
        if (s == NULL)
                return -1;
        return 0;
}

int
rm_do_msg_push_out(struct rsyncme *rm,
			unsigned char *buf)
{
	int		err;
	uint32_t	session_id;
	pthread_attr_t          attr;
	struct rm_session	*s;

        assert(rm != NULL && buf != NULL);

	// generate GUID (session id)
	session_id = 0;
	// insert session into table
        s = rm_core_session_start(rm, session_id);
        if (s == NULL)
                return -1;

	// start ch_ch thread and delta thread
	// save pids in session object
	err = pthread_attr_init(&attr);
	if (err != 0)
		return -1;
	err = pthread_attr_setdetachstate(&attr,
			PTHREAD_CREATE_JOINABLE);
	if (err != 0)
		goto fail;

	err = pthread_create(&s->ch_ch_tid, &attr,
			rm_session_push_out_ch_ch_f, s);
	if (err != 0)
                goto fail;

	// delta thread
	pthread_attr_destroy(&attr);

	err = pthread_attr_init(&attr);
	if (err != 0)
		return -1;
	err = pthread_attr_setdetachstate(&attr,
			PTHREAD_CREATE_JOINABLE);
	if (err != 0)
		goto fail;

	err = pthread_create(&s->delta_tid, &attr,
			rm_session_push_out_delta_f, s);
	if (err != 0)
                goto fail;

        return 0;
fail:
	pthread_attr_destroy(&attr);
	if (s != NULL)
		rm_session_free(s);
	return -1;
}

int
rm_do_msg_pull_in(struct rsyncme *rm,
			unsigned char *buf)
{
	uint32_t	session_id;
	struct rm_session	*s;

        assert(rm != NULL && buf != NULL);

	// generate GUID (session id)
	session_id = 0;

        s = rm_core_session_start(rm, session_id);
        if (s == NULL)
                return -1;
        return 0;
}

int
rm_do_msg_pull_out(struct rsyncme *rm,
			unsigned char *buf)
{
	uint32_t	session_id;
	struct rm_session	*s;

        assert(rm != NULL && buf != NULL);

	// generate GUID (session id)
	session_id = 0;

        s = rm_core_session_start(rm, session_id);
        if (s == NULL)
                return -1;
        return 0;
}
