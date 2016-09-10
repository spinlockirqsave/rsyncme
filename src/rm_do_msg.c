/*
 * @file        rm_do_msg.c
 * @brief       Execute TCP messages.
 * @author	    Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        02 Nov 2016 04:29 PM
 * @copyright   LGPLv2.1
 */

#include "rm_defs.h"
#include "rm_core.h"
#include "rm_serialize.h"
#include "rm_session.h"


int
rm_do_msg_push_rx(struct rsyncme *rm, unsigned char *buf) {
	int                         err;
	struct rm_session	        *s;
    struct rm_session_push_rx   *prvt;

    (void) buf;
    assert(rm != NULL && buf != NULL);

    /* L = 0;   TODO get L from message */

	/* create session, assign SID, insert session into table */
    s = rm_core_session_add(rm, RM_PUSH_RX);
    if (s == NULL) {
        return -1;
    }
    prvt = (struct rm_session_push_rx*) s->prvt;
	
	/* start tx_ch_ch and rx delta threads,
     * save pids in session object */
	err = rm_launch_thread(&prvt->ch_ch_tx_tid, rm_session_ch_ch_tx_f, s, PTHREAD_CREATE_JOINABLE);
	if (err != RM_ERR_OK)
        goto fail;

	/* start rx delta thread */
	err = rm_launch_thread(&prvt->delta_rx_tid, rm_session_delta_rx_f_remote, s, PTHREAD_CREATE_JOINABLE);
	if (err != RM_ERR_OK)
        goto fail;
    return 0;

fail:
	if (s != NULL)
		rm_session_free(s);
	return -1;
}

int
rm_do_msg_pull_tx(struct rsyncme *rm, unsigned char *buf) {
	struct rm_session           *s;
    struct rm_session_pull_tx   *prvt;

    (void) buf;
    assert(rm != NULL && buf != NULL);
    /* L = 0;   TODO get L from message */
    s = rm_core_session_add(rm, RM_PULL_TX); /* create session, assign SID, insert session into table */
    if (s == NULL) {
        return -1;
    }
    prvt = (struct rm_session_pull_tx*) s->prvt;
    (void)prvt;
    return 0;
}

int
rm_do_msg_pull_rx(struct rsyncme *rm, unsigned char *buf) {
	struct rm_session           *s;
    struct rm_session_pull_rx   *prvt;

    (void) buf;
    assert(rm != NULL && buf != NULL);
    /* L = 0;   TODO get L from message */
    s = rm_core_session_add(rm, RM_PULL_RX);
    if (s == NULL) {
        return -1;
    }
    prvt = (struct rm_session_pull_rx*) s->prvt;
    (void)prvt;
    return 0;
}
