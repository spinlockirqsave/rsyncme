/* @file        rm_do_msg.c
 * @brief       Execute TCP messages.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        02 Nov 2016 04:29 PM
 * @copyright   LGPLv2.1 */


#include "rm_defs.h"
#include "rm_core.h"
#include "rm_serialize.h"
#include "rm_session.h"


enum rm_error
rm_msg_push_init(struct rm_msg_push *msg) {
    msg->hdr = malloc(sizeof *(msg->hdr));
    if (msg->hdr == NULL) {
        return RM_ERR_FAIL;
    }
    return RM_ERR_OK;
}

void
rm_msg_push_free(struct rm_msg_push *msg) {
    free(msg->hdr);
    free(msg);
}

void*
rm_do_msg_push_rx(void* arg) {
    enum rm_error               err = RM_ERR_OK;
    struct rm_session	        *s = NULL;
    struct rm_session_push_rx   *prvt = NULL;
    struct rm_msg_push          *msg_push = NULL;
    char                        ssid1[RM_UNIQUE_STRING_LEN];
    char                        ssid2[RM_UNIQUE_STRING_LEN];

    struct rm_work* work = (struct rm_work*) arg;
    msg_push = (struct rm_msg_push*) work->msg;

    s = rm_session_create(RM_PUSH_RX);
    if (s == NULL) {
        /* TODO send ack with error: temporary unavailable, try again ? */
        return NULL;
    }
    uuid_unparse(msg_push->ssid, ssid1);
    uuid_unparse(s->id, ssid2);
    RM_LOG_INFO("[%s] [1]: their ssid [%s] -> our ssid [%s]", rm_work_type_str[work->task], ssid1, ssid2);

    err = rm_session_assign_validate_from_msg_push(s, msg_push);
    if (err != RM_ERR_OK) {
        goto fail;
    }
    RM_LOG_INFO("[%s] [2]: [%s] -> [%s], x [%s], y [%s], z [%s], L [%zu], flags [%u]", rm_work_type_str[work->task], ssid1, ssid2, msg_push->x, msg_push->y, msg_push->z, msg_push->L, msg_push->hdr->flags);
    /* send ACK OK */
    err = rm_tcp_tx_msg_ack(work->fd, RM_PT_MSG_PUSH_ACK, RM_ERR_OK);
    if (err != RM_ERR_OK) {
        goto fail;
    }

    rm_core_session_add(work->rm, s); /* insert session into global table and list */
    RM_LOG_INFO("[%s] [3]: [%s] -> [%s], hashed to [%u]", rm_work_type_str[work->task], ssid1, ssid2, s->hash);

    prvt = (struct rm_session_push_rx*) s->prvt;

    err = rm_launch_thread(&prvt->ch_ch_tx_tid, rm_session_ch_ch_tx_f, s, PTHREAD_CREATE_JOINABLE); /* start tx_ch_ch and rx delta threads, save pids in session object */
    if (err != RM_ERR_OK) {
        goto fail;
    }

    err = rm_launch_thread(&prvt->delta_rx_tid, rm_session_delta_rx_f_remote, s, PTHREAD_CREATE_JOINABLE); /* start rx delta thread */
    if (err != RM_ERR_OK) {
        goto fail;
    }

    pthread_join(prvt->ch_ch_tx_tid, NULL);
    pthread_join(prvt->delta_rx_tid, NULL);
    if (prvt->ch_ch_tx_status != RM_TX_STATUS_OK) {
        err = RM_ERR_CH_CH_TX_THREAD;
    }
    if (prvt->delta_rx_status != RM_RX_STATUS_OK) {
        err = RM_ERR_DELTA_RX_THREAD;
    }

    RM_LOG_INFO("[%s] [4]: [%s] -> [%s], Session [%u] ended", rm_work_type_str[work->task], ssid1, ssid2, s->hash);
    rm_msg_push_free(msg_push);
    if (s != NULL) {
        rm_session_free(s);
    }
    return NULL;

fail:
    switch (err) {

        case RM_ERR_Y_Z_SYNC:
        case RM_ERR_Y_NULL:
        case RM_ERR_OPEN_Z:
        case RM_ERR_OPEN_Y:
        case RM_ERR_OPEN_TMP:
            /* TODO send tcp response with error code */
            RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u]", rm_work_type_str[work->task], ssid1, ssid2, err);
            break;

        case RM_ERR_WRITE:
            /* error sending RM_MSG_PUSH_ACK */
            RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u] : error sending push ack", rm_work_type_str[work->task], ssid1, ssid2, err);
            break;

        default:
            RM_LOG_ERR("[%s] [FAIL]: [%s] -> [%s], ERR [%u]", rm_work_type_str[work->task], ssid1, ssid2, err);
    }
    if (s != NULL) {
        rm_session_free(s);
    }
    return NULL;
}

int
rm_do_msg_pull_tx(struct rsyncme *rm, unsigned char *buf) {
    struct rm_session           *s = NULL;
    struct rm_session_pull_tx   *prvt = NULL;

    (void) buf;
    assert(rm != NULL && buf != NULL);
    s = rm_session_create(RM_PULL_TX);
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
    s = rm_session_create(RM_PULL_RX);
    if (s == NULL) {
        return -1;
    }
    rm_core_session_add(rm, s);
    prvt = (struct rm_session_pull_rx*) s->prvt;
    (void)prvt;
    return 0;
}

uint16_t
rm_calc_msg_hdr_len(struct rm_msg_hdr *hdr) {
    return (sizeof(hdr->hash) + sizeof(hdr->pt) + sizeof(hdr->flags) + sizeof(hdr->len));
}

uint16_t
rm_calc_msg_len(void *arg) {
    struct rm_msg_push  *msg = (struct rm_msg_push*) arg;
    uint16_t            len = 0;

    switch (msg->hdr->pt) {

        case RM_PT_MSG_PUSH:
            len = rm_calc_msg_hdr_len(msg->hdr);
            len += 16;                      /* ssid */
            len += 8;                       /* L    */
            len += (2 + msg->x_sz);
            if (msg->y_sz > 0) {
                len += (2 + msg->y_sz);     /* size fields are the length of the string including NULL terminating byte  */
            } else {
                len += 2;                   /* only file length field */
            }
            if (msg->z_sz > 0) {
                len += (2 + msg->z_sz);     /* size fields are the length of the string including NULL terminating byte  */
            } else {
                len += 2;                   /* only file length field */
            }
            break;

        case RM_PT_MSG_PULL:    /* TODO */
            break;

        case RM_PT_MSG_BYE:     /* TODO */
            break;

        case RM_PT_MSG_PUSH_ACK:
        case RM_PT_MSG_PULL_ACK:
            len = rm_calc_msg_hdr_len(msg->hdr);
            break;

        default:
            return 0;
    }
    return len;
}
