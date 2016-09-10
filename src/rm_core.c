/* @file	    rm_core.c
 * @brief	    Daemon's start up.
 * @author	    Piotr Gregor <piotrgregor@rsyncme.org>
 * @date	    02 Jan 2016 02:50 PM
 * @copyright	LGPLv2.1 */


#include "rm_core.h"
#include "rm_do_msg.h"
#include "twlist.h"


int
rm_core_init(struct rsyncme *rm) {
    assert(rm != NULL);
    memset(rm, 0, sizeof *rm);
    twhash_init(rm->sessions);
    TWINIT_LIST_HEAD(&rm->sessions_list);
    rm->state = RM_CORE_ST_INITIALIZED;
    return 0;
}

struct rm_session *
rm_core_session_find(struct rsyncme *rm,
        uint32_t session_id) {
    struct rm_session	*s;
    uint32_t                h;

    assert(rm != NULL);
    pthread_mutex_lock(&rm->mutex);
    h = twhash_min(session_id, RM_SESSION_HASH_BITS);
    twhlist_for_each_entry(s, &rm->sessions[h], hlink) {
        if (s->session_id == session_id) {
            pthread_mutex_lock(&s->session_mutex);
            return s;
        }
    }
    pthread_mutex_unlock(&rm->mutex);
    return NULL;
}

struct rm_session *
rm_core_session_add(struct rsyncme *rm, enum rm_session_type type) {
    struct rm_session	*s = NULL;
    assert(rm != NULL);

    s = rm_session_create(type);
    if (s == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&rm->mutex);
    s->session_id = rm->sessions_n + 1; /* create SID */
    twlist_add(&rm->sessions_list, &s->link);
    twhash_add(rm->sessions, &s->hlink, s->session_id);
    rm->sessions_n++;
    pthread_mutex_unlock(&rm->mutex);
    return s;
}

int
rm_core_session_stop(struct rm_session *s) {
    assert(s != NULL);
    twlist_del(&s->link);
    twhash_del(&s->hlink);
    return 0;
}

int
rm_core_authenticate(struct sockaddr_in *cli_addr) {
    char a[INET_ADDRSTRLEN];
    /* IPv4
     * cli_ip = cli_addr.sin_addr.s_addr;
     * inet_ntop(AF_INET, &cli_ip, cli_ip_str, INET_ADDRSTRLEN);
     * cli_port = ntohs(cli_addr.sin_port); */
    inet_ntop(AF_INET, &cli_addr->sin_addr, a, INET_ADDRSTRLEN);
    if (strcmp(a, RM_IP_AUTH) != 0) {
        return -1;
    }
    return 0;
}

int
rm_core_tcp_msg_valid_pt(uint8_t pt) {
    return (pt == RM_MSG_PUSH || pt == RM_MSG_PULL ||
            pt == RM_MSG_BYE);
}

int
rm_core_tcp_msg_validate(unsigned char *buf, int read_n) {
    uint32_t	hash;
    uint8_t		pt;

    assert(buf != NULL && read_n >= 0);
    if (read_n == 0) {
        rm_perr_abort("TCP message size is 0");
    }
    hash = RM_MSG_HDR_HASH(buf); /* validate hash */
    if (hash != RM_CORE_HASH_OK) {
        RM_LOG_ERR("%s", "incorrect hash");
        return -1;
    }
    pt = rm_msg_hdr_pt(buf);
    if (rm_core_tcp_msg_valid_pt(pt) == 0) {
        return -1;
    }
    return pt;
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
