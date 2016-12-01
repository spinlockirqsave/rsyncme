/* @file	    rm_core.c
 * @brief	    Daemon's start up.
 * @author	    Piotr Gregor <piotrgregor@rsyncme.org>
 * @date	    02 Jan 2016 02:50 PM
 * @copyright	LGPLv2.1 */


#include "rm_core.h"
#include "rm_do_msg.h"
#include "twlist.h"


enum rm_error rm_core_init(struct rsyncme *rm) {
    assert(rm != NULL);
    memset(rm, 0, sizeof *rm);
    twhash_init(rm->sessions);
    TWINIT_LIST_HEAD(&rm->sessions_list);
    rm->state = RM_CORE_ST_INITIALIZED;

    RM_LOG_INFO("%s", "Starting main work queue");

    if (rm_wq_workqueue_init(&rm->wq, RM_WORKERS_N, "main_queue") != RM_ERR_OK) {   /* TODO CPU checking, choose optimal number of threads */
        return RM_ERR_WORKQUEUE_CREATE;
    }
    return RM_ERR_OK;
}

enum rm_error rm_core_deinit(struct rsyncme *rm) {
    if (twhash_empty(rm->sessions) != 0) {
        return RM_ERR_BUSY;
    }
    if (twlist_empty(&rm->sessions_list) != 1) {
        return RM_ERR_FAIL;
    }

    RM_LOG_INFO("%s", "Stopping main work queue");

    if (rm_wq_workqueue_stop(&rm->wq) != RM_ERR_OK) {
        return RM_ERR_WORKQUEUE_STOP;
    }
    if (rm_wq_workqueue_deinit(&rm->wq) != RM_ERR_OK) {
        return RM_ERR_MEM;
    }
    return RM_ERR_OK;
}

struct rm_session *
rm_core_session_find(struct rsyncme *rm, unsigned char session_id[RM_UUID_LEN]) {
    struct rm_session	*s;
    uint32_t            h;
    uint16_t            key;

    assert(rm != NULL);
    pthread_mutex_lock(&rm->mutex);
    memcpy(&key, session_id, rm_min(RM_UUID_LEN, sizeof(key)));
    h = twhash_min(key, RM_SESSION_HASH_BITS);
    twhlist_for_each_entry(s, &rm->sessions[h], hlink) {
        if (memcmp(&s->id, session_id, RM_UUID_LEN) == 0) {
            pthread_mutex_lock(&s->mutex);
            return s;
        }
    }
    pthread_mutex_unlock(&rm->mutex);
    return NULL;
}

void
rm_core_session_add(struct rsyncme *rm, struct rm_session *s) {
    assert(rm != NULL);
    assert(s != NULL);

    pthread_mutex_lock(&rm->mutex);
    twlist_add(&rm->sessions_list, &s->link);

    twhash_add(rm->sessions, &s->hlink, (uint32_t)s->hash.data);
    rm->sessions_n++;
    pthread_mutex_unlock(&rm->mutex);
    return;
}

int
rm_core_session_stop(struct rm_session *s) {
    assert(s != NULL);
    twlist_del(&s->link);
    twhash_del(&s->hlink);
    return 0;
}

enum rm_error
rm_core_authenticate(struct sockaddr_in *cli_addr) {
    char a[INET_ADDRSTRLEN];
    /* IPv4
     * cli_ip = cli_addr.sin_addr.s_addr;
     * inet_ntop(AF_INET, &cli_ip, cli_ip_str, INET_ADDRSTRLEN);
     * cli_port = ntohs(cli_addr.sin_port); */
    inet_ntop(AF_INET, &cli_addr->sin_addr, a, INET_ADDRSTRLEN);
    if (strcmp(a, RM_IP_AUTH) != 0) {
        return RM_ERR_FAIL;
    }
    return RM_ERR_OK;
}

uint32_t
rm_core_hdr_hash(struct rm_msg_hdr *hdr) {
    return twhash_32(((hdr->pt << 24) | (hdr->flags << 16) | hdr->len), RM_CORE_HASH_CHALLENGE_BITS);
}

enum rm_error
rm_core_validate_hash(unsigned char *buf) {
    uint32_t    hash, challenge;
    uint8_t     pt, flags;
    uint16_t    len;

    hash = rm_get_msg_hdr_hash(buf);
    pt = rm_get_msg_hdr_pt(buf);
    flags = rm_get_msg_hdr_flags(buf);
    len = rm_get_msg_hdr_len(buf);

    challenge = twhash_32(((pt << 24) | (flags << 16) | len), RM_CORE_HASH_CHALLENGE_BITS);
    if (hash != challenge) {
        return RM_ERR_FAIL;
    }
    return RM_ERR_OK;
}

enum rm_error
rm_core_tcp_msg_valid_pt(unsigned char* buf) {
    uint8_t pt = rm_get_msg_hdr_pt(buf);
    if (pt == RM_PT_MSG_PUSH || pt == RM_PT_MSG_PULL || pt == RM_PT_MSG_BYE) {
        return RM_ERR_OK;
    }
    return RM_ERR_FAIL;
}

enum rm_error
rm_core_tcp_msg_hdr_validate(unsigned char *buf, int read_n) {
    if ((size_t)read_n < sizeof(struct rm_msg_hdr)) {
        return RM_ERR_FAIL;
    }
    if (rm_core_validate_hash(buf) != RM_ERR_OK) {
        return RM_ERR_FAIL;
    }
    if (rm_core_tcp_msg_valid_pt(buf) != RM_ERR_OK) {
        return RM_ERR_MSG_PT_UNKNOWN;
    }
    /* TODO validate all fields */
    return RM_ERR_OK;
}

int
rm_core_select(int fd, enum rm_io_direction io_direction, uint16_t timeout_s, uint16_t timeout_us) {
    fd_set fdset;
    struct timeval tv;

    tv.tv_sec = timeout_s;
    tv.tv_usec = timeout_us;

    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);

    switch (io_direction) {

        case RM_READ:
            return select(fd + 1, &fdset, NULL, NULL, &tv);

        case RM_WRITE:
            return select(fd + 1, NULL, &fdset, NULL, &tv);

        default:
            return -1;
    }
}

enum rm_error
rm_core_tcp_msg_assemble(int fd, enum rm_pt_type pt, void **body_raw, size_t bytes_n) {
    enum rm_error err;

    switch (pt) {

        case RM_PT_MSG_PUSH:
        case RM_PT_MSG_PULL:

            *body_raw = malloc(bytes_n);
            if (*body_raw == NULL) {
                return RM_ERR_MEM;
            }
            err = rm_tcp_read(fd, *body_raw, bytes_n);
            if (err != RM_ERR_OK) {
                free(*body_raw);
                *body_raw = NULL;
                return RM_ERR_READ;
            }
            break;

        case RM_PT_MSG_BYE:
            break;

        default:
            break;
    }
    return RM_ERR_OK;
}
