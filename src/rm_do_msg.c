/// @file	rm_do_msg.c
/// @brief	Execute TCP messages.
/// @author	Piotr Gregor piotrek.gregor at gmail.com
/// @version	0.1.2
/// @date	02 Nov 2016 04:29 PM
/// @copyright	LGPLv2.1

#include "rm_defs.h"
#include "rm_core.h"
#include "rm_serialize.h"
#include "rm_session.h"

int
rm_do_msg_push_rx(struct rsyncme *rm,
			unsigned char *buf)
{
	int			err;
	pthread_attr_t          attr;
	struct rm_session	*s;

        assert(rm != NULL && buf != NULL);

	// create session, assign SID, insert
	// session into table
        s = rm_core_session_add(rm);
        if (s == NULL)
                return -1;
	
	// start tx_ch_ch and rx delta threads
	// save pid in session object
	err = pthread_attr_init(&attr);
	if (err != 0)
		return -1;
	err = pthread_attr_setdetachstate(&attr,
			PTHREAD_CREATE_JOINABLE);
	if (err != 0)
		goto fail;

	err = pthread_create(&s->ch_ch_tid, &attr,
			rm_session_ch_ch_tx_f, s);
	if (err != 0)
                goto fail;

	// rx delta thread
	pthread_attr_destroy(&attr);

	err = pthread_attr_init(&attr);
	if (err != 0)
		return -1;
	err = pthread_attr_setdetachstate(&attr,
			PTHREAD_CREATE_JOINABLE);
	if (err != 0)
		goto fail;

	err = pthread_create(&s->delta_tid, &attr,
			rm_session_delta_rx_f, s);
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
rm_do_msg_push_tx(struct rsyncme *rm,
			unsigned char *buf)
{
	int			err;
	pthread_attr_t          attr;
	struct rm_session	*s;

        assert(rm != NULL && buf != NULL);

	// create session, assign SID, insert
	// session into table
        s = rm_core_session_add(rm);
        if (s == NULL)
                return -1;

	// start rx_ch_ch thread and tx delta threads
	// save pids in session object
	err = pthread_attr_init(&attr);
	if (err != 0)
		return -1;
	err = pthread_attr_setdetachstate(&attr,
			PTHREAD_CREATE_JOINABLE);
	if (err != 0)
		goto fail;

	err = pthread_create(&s->ch_ch_tid, &attr,
			rm_session_ch_ch_rx_f, s);
	if (err != 0)
                goto fail;

	// tx delta thread
	pthread_attr_destroy(&attr);

	err = pthread_attr_init(&attr);
	if (err != 0)
		return -1;
	err = pthread_attr_setdetachstate(&attr,
			PTHREAD_CREATE_JOINABLE);
	if (err != 0)
		goto fail;

	err = pthread_create(&s->delta_tid, &attr,
			rm_session_delta_tx_f, s);
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
rm_do_msg_pull_tx(struct rsyncme *rm,
			unsigned char *buf)
{
	struct rm_session	*s;

        assert(rm != NULL && buf != NULL);

	// create session, assign SID, insert
	// session into table
        s = rm_core_session_add(rm);
        if (s == NULL)
                return -1;
        return 0;
}

int
rm_do_msg_pull_rx(struct rsyncme *rm,
			unsigned char *buf)
{
	struct rm_session	*s;

        assert(rm != NULL && buf != NULL);

	// create session, assign SID, insert
	// session into table
        s = rm_core_session_add(rm);
        if (s == NULL)
                return -1;
        return 0;
}
