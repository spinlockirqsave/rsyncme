/// @file      rm_do_msg.c
/// @brief     Execute TCP messages.
/// @author    peterg
/// @version   0.1.1
/// @date      02 Nov 2016 04:29 PM
/// @copyright LGPL

#include "rm_defs.h"
#include "rm_core.h"
#include "rm_do_msg.h"
#include "rm_session.h"

int
rm_do_msg_session_add(struct rsyncme *rm,
                        struct rm_msg_session_add *m,
                                        int read_n)
{
	int		err;
	uint32_t	session_id;

        assert(rm != NULL && m != NULL);
        (void)read_n;

	// session id from message
	session_id = 0;

        err = rm_core_session_start(rm, session_id,
                        rm_session_core_f);
        if (err == -1)
                return err;
        return 0;
}

