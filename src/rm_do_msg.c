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
	int		err;
	uint32_t	session_id;

        assert(rm != NULL && buf != NULL);

	// generate GUID (session id)
	session_id = 0;
        err = rm_core_session_start(rm, session_id,
                        rm_session_push_in_f, buf);
        if (err == -1)
                return err;
        return 0;
}

int
rm_do_msg_push_out(struct rsyncme *rm,
			unsigned char *buf)
{
	int		err;
	uint32_t	session_id;

        assert(rm != NULL && buf != NULL);

	// generate GUID (session id)
	session_id = 0;

        err = rm_core_session_start(rm, session_id,
                        rm_session_push_out_f, buf);
        if (err == -1)
                return err;
        return 0;
}

int
rm_do_msg_pull_in(struct rsyncme *rm,
			unsigned char *buf)
{
	int		err;
	uint32_t	session_id;

        assert(rm != NULL && buf != NULL);

	// generate GUID (session id)
	session_id = 0;

        err = rm_core_session_start(rm, session_id,
                        rm_session_pull_in_f, buf);
        if (err == -1)
                return err;
        return 0;
}

int
rm_do_msg_pull_out(struct rsyncme *rm,
			unsigned char *buf)
{
	int		err;
	uint32_t	session_id;

        assert(rm != NULL && buf != NULL);

	// generate GUID (session id)
	session_id = 0;

        err = rm_core_session_start(rm, session_id,
                        rm_session_pull_out_f, buf);
        if (err == -1)
                return err;
        return 0;
}
