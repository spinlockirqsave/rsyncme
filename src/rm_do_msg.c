/* @file        rm_do_msg.c
 * @brief       Execute TCP messages.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        02 Nov 2016 04:29 PM
 * @copyright   LGPLv2.1 */


#include "rm_defs.h"
#include "rm_core.h"
#include "rm_serialize.h"
#include "rm_session.h"


enum rm_error rm_msg_push_alloc(struct rm_msg_push *msg) {
	msg->hdr = malloc(sizeof(struct rm_msg_hdr));
	if (msg->hdr == NULL) {
		return RM_ERR_FAIL;
	}
	memset(msg->hdr, 0, sizeof(struct rm_msg_hdr));
	return RM_ERR_OK;
}

void rm_msg_push_free(struct rm_msg_push *msg) {
	free(msg->hdr);
	free(msg);
}

enum rm_error rm_msg_ack_alloc(struct rm_msg_ack *ack) {
	ack->hdr = malloc(sizeof(struct rm_msg_hdr));
	if (ack->hdr == NULL) {
		return RM_ERR_FAIL;
	}
	memset(ack->hdr, 0, sizeof(struct rm_msg_hdr));
	return RM_ERR_OK;
}

void rm_msg_ack_free(struct rm_msg_ack *ack) {
	free(ack->hdr);
	ack->hdr = NULL;
	free(ack);
}

enum rm_error rm_msg_push_ack_alloc(struct rm_msg_push_ack *ack) {
	if (rm_msg_ack_alloc(&ack->ack) != RM_ERR_OK)
		return RM_ERR_FAIL;
	ack->delta_port = 0;
	return RM_ERR_OK;
}

void rm_msg_push_ack_free(struct rm_msg_push_ack *ack) {
	free(ack->ack.hdr);
	ack->ack.hdr = NULL;
	free(ack);
}

void* rm_do_msg_push_rx(void* arg) {
	enum rm_error					err = RM_ERR_OK;
	struct rm_session				*s = NULL;
	struct rm_session_push_rx		*prvt = NULL;
	struct rm_msg_push				*msg_push = NULL;
	uint8_t							ack_tx_err = 0;															/* set to 1 if ACK tx failed */
	int								fd_z = -1;
	struct stat                     fs = {0};
	struct rm_core_options			opt = {0};

	struct rm_work* work = (struct rm_work*) arg;
	msg_push = (struct rm_msg_push*) work->msg;

	RM_LOG_INFO("[%s] [0]: work started in worker [%u] thread [%llu]", rm_work_type_str[work->task], work->worker_idx, rm_gettid());

	pthread_mutex_lock(&work->rm->mutex);
	memcpy(&opt, &work->rm->opt, sizeof(struct rm_core_options));
	pthread_mutex_unlock(&work->rm->mutex);

	s = rm_session_create(RM_PUSH_RX, &opt);
	if (s == NULL || s->prvt == NULL) {
		if (rm_tcp_tx_msg_ack(work->fd, RM_PT_MSG_PUSH_ACK, RM_ERR_CREATE_SESSION, NULL) != RM_ERR_OK) {	/* send ACK explaining error */
			ack_tx_err = 1;
		}
		goto fail;
	}

	prvt = (struct rm_session_push_rx*) s->prvt;
	uuid_unparse(msg_push->ssid, s->ssid1);
	uuid_unparse(s->id, s->ssid2);

	RM_LOG_INFO("[%s] [1]: their ssid [%s] -> our ssid [%s]", rm_work_type_str[work->task], s->ssid1, s->ssid2);

	RM_LOG_INFO("[%s] [2]: [%s] -> [%s], x [%s], y [%s], z [%s], L [%zu], flags [%u][0x%02x]. Validating...", rm_work_type_str[work->task], s->ssid1, s->ssid2, msg_push->x, msg_push->y, msg_push->z, msg_push->L, msg_push->hdr->flags, msg_push->hdr->flags);
	if (msg_push->hdr->flags & RM_BIT_4)
		RM_LOG_INFO("[%s] [2]: [%s] -> [%s], --force", rm_work_type_str[work->task], s->ssid1, s->ssid2);
	if (msg_push->hdr->flags & RM_BIT_6)
		RM_LOG_INFO("[%s] [2]: [%s] -> [%s], --leave", rm_work_type_str[work->task], s->ssid1, s->ssid2);

	err = rm_session_assign_validate_from_msg_push(s, msg_push, work->fd);									/* validate, change dir to result's path */
	if (err != RM_ERR_OK) {
		if (rm_tcp_tx_msg_ack(work->fd, RM_PT_MSG_PUSH_ACK, err, s) != RM_ERR_OK) {							/* send ACK with error */
			ack_tx_err = 1;
		}
		goto fail;
	}

	/* open delta port for listening thread (port dynamically assigned as delta_port is initialised to 0) */
	RM_LOG_INFO("[%s] [3]: [%s] -> [%s], Opening ephemeral delta rx port", rm_work_type_str[work->task], s->ssid1, s->ssid2);

	err = rm_tcp_listen(&prvt->delta_fd, INADDR_ANY, &prvt->delta_port, 0, RM_SERVER_LISTENQ); 
	if (err != RM_ERR_OK) {
		if (rm_tcp_tx_msg_ack(work->fd, RM_PT_MSG_PUSH_ACK, err, s) != RM_ERR_OK) {							/* send ACK with error */
			ack_tx_err = 1;
		}
		prvt->delta_fd = -1;
		goto fail;
	}

	RM_LOG_INFO("[%s] [4]: [%s] -> [%s], x [%s], y [%s], z [%s], L [%zu], flags [0x%02x], tmp [%s], listening on delta rx port [%u]", rm_work_type_str[work->task], s->ssid1, s->ssid2, msg_push->x, msg_push->y, msg_push->z, msg_push->L, msg_push->hdr->flags, s->f_z_name, prvt->delta_port);

	if (rm_tcp_tx_msg_ack(work->fd, RM_PT_MSG_PUSH_ACK, RM_ERR_OK, s) != RM_ERR_OK) {						/* send ACK OK */
		ack_tx_err = 1;
		goto fail;
	}

	RM_LOG_INFO("[%s] [5]: [%s] -> [%s], TXed ACK", rm_work_type_str[work->task], s->ssid1, s->ssid2);

	rm_core_session_add(work->rm, s);																		/* insert session into global table and list, hash md5 hash */

	RM_LOG_INFO("[%s] [6]: [%s] -> [%s], Session hashed to [%u][%u]", rm_work_type_str[work->task], s->ssid1, s->ssid2, s->hash, s->hashed_hash);

	err = rm_launch_thread(&prvt->ch_ch_tx_tid, rm_session_ch_ch_tx_f, s, PTHREAD_CREATE_JOINABLE);			/* start tx_ch_ch and rx delta threads, save pids in session object */
	if (err != RM_ERR_OK) {
		goto fail;
	}

	RM_LOG_INFO("[%s] [7]: [%s] -> [%s], Checksums tx thread started", rm_work_type_str[work->task], s->ssid1, s->ssid2);

	err = rm_launch_thread(&prvt->delta_rx_tid, rm_session_delta_rx_f_remote, s, PTHREAD_CREATE_JOINABLE);	/* start rx delta thread */
	if (err != RM_ERR_OK) {
		goto fail;
	}

	RM_LOG_INFO("[%s] [8]: [%s] -> [%s], Delta rx thread started", rm_work_type_str[work->task], s->ssid1, s->ssid2);

	pthread_join(prvt->ch_ch_tx_tid, NULL);

	RM_LOG_INFO("[%s] [9]: [%s] -> [%s], Checksums tx thread joined", rm_work_type_str[work->task], s->ssid1, s->ssid2);

	pthread_join(prvt->delta_rx_tid, NULL);

	RM_LOG_INFO("[%s] [10]: [%s] -> [%s], All threads joined", rm_work_type_str[work->task], s->ssid1, s->ssid2);

	if (prvt->ch_ch_tx_status != RM_TX_STATUS_OK) {
		err = RM_ERR_CH_CH_TX_THREAD;
		goto fail;
	}

	if (prvt->delta_rx_status != RM_RX_STATUS_OK) {
		err = RM_ERR_DELTA_RX_THREAD;
		goto fail;
	}

	if (s->f_y != NULL) {																					/* fflush and close f_y */
		fflush(s->f_y);
		fclose(s->f_y);
	}

	if (s->f_z != NULL) {																					/* fflush and close f_z */
		fflush(s->f_z);
		fd_z = fileno(s->f_z);
		memset(&fs, 0, sizeof(fs));
		if (fstat(fd_z, &fs) != 0) {
			err = RM_ERR_FSTAT_TMP;
			s->f_y = NULL;
			s->f_z = NULL;
			goto fail;
		}
		fclose(s->f_z);
		s->f_z = NULL;
	}

	if ((prvt->msg_push->hdr->flags & RM_BIT_6) == 0u) {													/* if --leave not set */
		if (prvt->msg_push->z_sz > 0) {																		/* if @z is set */
			if (s->f_y && unlink(prvt->msg_push->y) != 0) {													/* remove @y if it exists */
				s->f_y = NULL;
				err = RM_ERR_UNLINK_Y;
				goto fail;
			}
		}
	}

	if (s->f_y != NULL)																						/* it has been flushed & closed already */
		s->f_y = NULL;

	/* sanity check */
	if (s->f_z != NULL)																						/* it has been flushed & closed already */
		s->f_z = NULL;

	if (prvt->msg_push->z_sz > 0) {																			/* use different name? */
		if (rename(s->f_z_name, prvt->msg_push->z) == -1) {
			err = RM_ERR_RENAME_TMP_Z;
			goto fail;
		}
	} else {
		if (rename(s->f_z_name, prvt->msg_push->y) == -1) {
			err = RM_ERR_RENAME_TMP_Y;
			goto fail;
		}
	}

	rm_rx_print_stats(s->rec_ctx, 1, 0);
	RM_LOG_INFO("[%s] [11]: [%s] -> [%s], Session [%u][%u] ended", rm_work_type_str[work->task], s->ssid1, s->ssid2, s->hash, s->hashed_hash);

	if (s != NULL) {
		rm_session_free(s);																					/* frees msg allocated for work as well */
		s = NULL;
		work->msg = NULL;																					/* do not free msg again in work dtor */
	}
	return NULL;

fail:
	if (s == NULL) {																						/* session failed to create */
		RM_LOG_ERR("[%s] [FAIL]: ERR [%u], failed to create session", rm_work_type_str[work->task], err);
	}
	switch (err) {

		case RM_ERR_Y_Z_SYNC:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : request can't be handled, --leave option set (do not delete @y after @z has been synced) but @z name is not given or is same as @y", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_Y_NULL:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : request can't be handled, @y file name must be specified", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_FSTAT_Y:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : request can't be handled, can't fstat @y", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_FSTAT_Z:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : can't fstat @z", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_FSTAT_TMP:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : can't fstat @tmp", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_OPEN_Z:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : request can't be handled, can't open @z", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_OPEN_Y:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : request can't be handled, can't open @y (should transmitter set --force flag?)", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_GETCWD:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : request can't be handled, can't get current working directory", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_CHDIR_Z:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : request can't be handled, can't change current working directory to @z's dir", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_CHDIR_Y:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : request can't be handled, can't change current working directory to @y's dir", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_OPEN_TMP:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : request can't be handled", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_WRITE:																					/* error sending RM_MSG_PUSH_ACK */
			if (s != NULL) {
				RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : error sending PUSH ack", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			} else {
				RM_LOG_ERR("[%s] [FAIL]: ERR [%u] : error sending PUSH ack", rm_work_type_str[work->task], err);
			}
			break;

		case RM_ERR_BAD_CALL:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : request can't be handled, bad arguments", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_UNLINK_Y:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : can't unlink @y", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_RENAME_TMP_Y:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : can't rename @tmp to @y", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_RENAME_TMP_Z:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : can't rename @tmp to @z", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
			break;

		case RM_ERR_CH_CH_TX_THREAD:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : checksums tx thread failed with error [%u]", rm_work_type_str[work->task], s->ssid1, s->ssid2, err, prvt->ch_ch_tx_status);
			break;

		case RM_ERR_DELTA_RX_THREAD:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : delta rx thread failed with error [%u]", rm_work_type_str[work->task], s->ssid1, s->ssid2, err, prvt->delta_rx_status);
			break;

		default:
			RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : default", rm_work_type_str[work->task], s->ssid1, s->ssid2, err);
	}
	if (ack_tx_err == 1) {																					/* failed to send ACK */
		/* TODO reschedule the job? */
	}
	if (s != NULL) {
		rm_session_free(s);
		s = NULL;
		work->msg = NULL;																					/* do not free msg again in work dtor */
	}
	return NULL;
}

int
rm_do_msg_pull_tx(struct rsyncme *rm, unsigned char *buf) {
	struct rm_session           *s = NULL;
	struct rm_session_pull_tx   *prvt = NULL;

	(void) buf;
	assert(rm != NULL && buf != NULL);
	s = rm_session_create(RM_PULL_TX, NULL);
	if (s == NULL) {
		return -1;
	}
	rm_core_session_add(rm, s); /* insert session into global table and list */
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
	s = rm_session_create(RM_PULL_RX, NULL);
	if (s == NULL) {
		return -1;
	}
	rm_core_session_add(rm, s);
	prvt = (struct rm_session_pull_rx*) s->prvt;
	(void)prvt;
	return 0;
}

uint16_t
rm_calc_msg_len(void *arg) {
	struct rm_msg_push  *msg_push;
	struct rm_msg       *msg = (struct rm_msg*) &arg;
	uint16_t            len = 0;

	switch (msg->hdr->pt) {

		case RM_PT_MSG_PUSH:
			msg_push = (struct rm_msg_push*) arg;
			len = RM_MSG_HDR_LEN;
			len += 16;                      /* ssid */
			len += 8;                       /* L    */
			len += (2 + msg_push->x_sz);
			if (msg_push->y_sz > 0) {
				len += (2 + msg_push->y_sz);    /* size fields are the length of the string including NULL terminating byte  */
			} else {
				len += 2;                       /* only file length field */
			}
			if (msg_push->z_sz > 0) {
				len += (2 + msg_push->z_sz);    /* size fields are the length of the string including NULL terminating byte  */
			} else {
				len += 2;                       /* only file length field */
			}
			len += 2;							/* ch_ch_port */
			len += 8;							/* bytes */
			break;

		case RM_PT_MSG_PULL:    /* TODO */
			break;

		case RM_PT_MSG_BYE:     /* TODO */
			break;

		case RM_PT_MSG_PUSH_ACK:
			len = RM_MSG_HDR_LEN;
			len += 2;							/* delta port */
			len += 8;							/* checksums number */
			break;

		case RM_PT_MSG_PULL_ACK:
			len = RM_MSG_HDR_LEN;
			break;

		case RM_PT_MSG_ACK:
			len = RM_MSG_ACK_LEN;
			break;

		default:
			return 0;
	}
	return len;
}

void
rm_msg_push_dtor(void *arg) {
	struct rm_work *work = (struct rm_work*) arg;
	close(work->fd);
	if (work->msg)
		rm_msg_push_free((struct rm_msg_push*) work->msg);
	work->msg = NULL;
	rm_work_free(work);                     /* free the memory allocated for message and work itself */
}
