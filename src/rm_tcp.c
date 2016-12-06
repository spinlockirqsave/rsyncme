/* @file        rm_tcp.c
 * @brief       TCP handling.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        18 Apr 2016 00:45 AM
 * @copyright   LGPLv2.1 */


#include "rm_tcp.h"
#include "rm.h"


ssize_t
rm_tcp_rx(int fd, void *buf, size_t bytes_n) {
    size_t      bytes_read_total = 0;
    ssize_t     bytes_read;

    if (bytes_n == 0) {
        return 0;
    }
    while (bytes_read_total != bytes_n) {
        do {
            bytes_read = read(fd, (unsigned char*) buf + bytes_read_total, (bytes_n - bytes_read_total));
        } while ((bytes_read == -1) && (errno == EINTR));
        if (bytes_read == -1) { /* Bad. */
            return -1;
        }
        bytes_read_total += bytes_read;
    }
    return bytes_read_total;
}

ssize_t
rm_tcp_tx(int fd, void *buf, size_t bytes_n) {
    size_t      bytes_written_total = 0;
    ssize_t     bytes_written;

    while (bytes_written_total != bytes_n) {
        do {
            bytes_written = write(fd, (unsigned char*) buf + bytes_written_total, (bytes_n - bytes_written_total));
        } while ((bytes_written == -1) && (errno == EINTR));
        if (bytes_written == -1) { /* Bad. */
            return -1;
        }
        bytes_written_total += bytes_written;
    }
    return bytes_written_total;
}

int
rm_tcp_tx_ch_ch_ref(int fd, const struct rm_ch_ch_ref *e) {
    ssize_t bytes_written;
    unsigned char buf[RM_CH_CH_REF_SIZE], *pbuf;

    /* serialize data */
    pbuf = rm_serialize_u32(buf, e->ch_ch.f_ch);
    memcpy(pbuf, &e->ch_ch.s_ch, RM_STRONG_CHECK_BYTES);
    pbuf += RM_STRONG_CHECK_BYTES;
    pbuf = rm_serialize_size_t(pbuf, e->ref);

    /* tx over TCP connection */
    bytes_written = rm_tcp_tx(fd, buf, RM_CH_CH_REF_SIZE);
    if (bytes_written == -1 || (size_t)bytes_written != RM_CH_CH_REF_SIZE) {
        return -1;
    }
    return 0;
}

enum rm_error
rm_tcp_tx_msg_ack(int fd, enum rm_pt_type pt, enum rm_error status) {
    struct rm_msg_hdr   hdr = {0};
    struct rm_msg_ack   ack = {0};
    struct rm_msg_ack   raw_msg_ack = {0};

    hdr.pt = pt;
    hdr.flags = status;
    hdr.len = rm_calc_msg_len(&hdr);
    hdr.hash = rm_core_hdr_hash(&hdr);
    ack.hdr = &hdr;
    rm_serialize_msg_ack((unsigned char*)&raw_msg_ack, &ack);
    return rm_tcp_write(fd, &raw_msg_ack, hdr.len);
}

int
rm_tcp_set_socket_blocking_mode(int fd, uint8_t on) {
    if (fd < 0 || on > 1) {
        return -1;
    }

#ifdef WIN32
    unsigned long mode = 1 - on;
    return ioctlsocket(fd, FIONBIO, &mode);
#else
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return -2;
    }
    flags = on ? (flags &~ O_NONBLOCK) : (flags | O_NONBLOCK);
    return fcntl(fd, F_SETFL, flags);
#endif
}

/* @brief   Resolve host names.
 * @details Return 0 on success or EAI_* errcode to be used with gai_strerror(). */
static int
rm_core_resolve_host(const char *host, unsigned int port, struct addrinfo *hints, struct addrinfo **res) {
    char strport[32];
    snprintf(strport, sizeof strport, "%u", port);
    return getaddrinfo(host, strport, hints, res);
}

enum rm_error
rm_core_connect(int *fd, const char *host, uint16_t port, int domain, int type, const char **err_str) {
    int             err;
    struct addrinfo hints, *res, *ressave;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = domain;
    hints.ai_socktype = type;
    hints.ai_flags = 0;
#if HAVE_DECL_AI_ADDRCONFIG
    hints.ai_flags |= AI_ADDRCONFIG;
#endif
#if HAVE_DECL_AI_V4MAPPED
    hints.ai_flags |= AI_V4MAPPED;
#endif
    hints.ai_protocol = 0; 

    err = rm_core_resolve_host(host, port, &hints, &res);
    if (err != 0) {
        *err_str = gai_strerror(err);
        return RM_ERR_GETADDRINFO;  /* use gai_strerror(n) to get error string */
    }

    ressave = res;
    do {
        *fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (*fd < 0) {
            continue;
        }
        if (connect(*fd, res->ai_addr, res->ai_addrlen) == 0) {
            break;      /* success */
        }
        close(*fd);
    } while ((res = res->ai_next) != NULL);

    if (res == NULL) { /* errno set from final connect() */
        *err_str = strerror(errno);
        freeaddrinfo(ressave);
        return RM_ERR_FAIL;
    }

    freeaddrinfo(ressave);
    return RM_ERR_OK;
}

enum rm_error
rm_tcp_connect(int *fd, const char *host, uint16_t port, int domain, const char **err_str) {
    int             err;
    struct addrinfo hints, *res, *ressave;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = domain;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
#if HAVE_DECL_AI_ADDRCONFIG
    hints.ai_flags |= AI_ADDRCONFIG;
#endif
#if HAVE_DECL_AI_V4MAPPED
    hints.ai_flags |= AI_V4MAPPED;
#endif
    hints.ai_protocol = 0; 

    err = rm_core_resolve_host(host, port, &hints, &res);
    if (err != 0) {
        *err_str = gai_strerror(err);
        return RM_ERR_GETADDRINFO;  /* use gai_strerror(n) to get error string */
    }

    ressave = res;
    do {
        *fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (*fd < 0) {
            continue;
        }
        if (connect(*fd, res->ai_addr, res->ai_addrlen) == 0) {
            break;      /* success */
        }
        close(*fd);
    } while ((res = res->ai_next) != NULL);

    if (res == NULL) { /* errno set from final connect() */
        *err_str = strerror(errno);
        freeaddrinfo(ressave);
        return RM_ERR_FAIL;
    }

    freeaddrinfo(ressave);
    return RM_ERR_OK;
}

enum rm_error
rm_tcp_connect_nonblock_timeout_once(int fd, struct addrinfo *res, uint16_t timeout_s, uint16_t timeout_us) {
    int             err, fd_err;
    socklen_t       len;

    rm_tcp_set_socket_blocking_mode(fd, 0);

    err = connect(fd, res->ai_addr, res->ai_addrlen);
    if (err == 0) {
        return RM_ERR_OK;
    } else if ((errno != EINPROGRESS) && (errno != EINTR)) {
        return RM_ERR_FAIL;
    }

    err = rm_core_select(fd, RM_WRITE, timeout_s, timeout_us);
    if (err == -1) {
        err = RM_ERR_FAIL;
        goto exit;
    } else if (err > 0) {
        len = sizeof fd_err;
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &fd_err, &len);
        if (fd_err == 0) {
            err = RM_ERR_OK;
        } else {
            err = RM_ERR_FAIL;
        }
    } else {
        err = RM_ERR_CONNECT_TIMEOUT;
    }

exit:
    rm_tcp_set_socket_blocking_mode(fd, 1);
    return err;
}

enum rm_error
rm_tcp_connect_nonblock_timeout(int *fd, const char *host, uint16_t port, int domain, uint16_t timeout_s, uint16_t timeout_us, const char **err_str) {
    int             err, errsave = RM_ERR_FAIL;
    struct addrinfo hints, *res, *ressave;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = domain;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
#if HAVE_DECL_AI_ADDRCONFIG
    hints.ai_flags |= AI_ADDRCONFIG;
#endif
#if HAVE_DECL_AI_V4MAPPED
    hints.ai_flags |= AI_V4MAPPED;
#endif
    hints.ai_protocol = 0; 

    err = rm_core_resolve_host(host, port, &hints, &res);
    if (err != 0) {
        *err_str = gai_strerror(err);
        return RM_ERR_GETADDRINFO;  /* use gai_strerror(n) to get error string */
    }

    ressave = res;
    do {
        *fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (*fd < 0) {
            continue;
        }
        err = rm_tcp_connect_nonblock_timeout_once(*fd, res, timeout_s, timeout_us);
        if (err == RM_ERR_OK) {
            errsave = RM_ERR_OK;
            break;
        } else if (err == RM_ERR_CONNECT_TIMEOUT) {
            errsave = RM_ERR_CONNECT_TIMEOUT;
        } else {
            errsave = RM_ERR_FAIL;
        }
        close(*fd);
    } while ((res = res->ai_next) != NULL);

    if ((res == NULL) && (errno != EINPROGRESS)) {
        *err_str = strerror(errno);
    }

    freeaddrinfo(ressave);
    return errsave;
}

enum rm_error
rm_tcp_read(int fd, void *dst, size_t bytes_n) {
    ssize_t         read_n = 0;
    unsigned char   *buf = dst;

    while (bytes_n > 0) {
        read_n = read(fd, buf, bytes_n);
        if (read_n == 0) {
            return RM_ERR_EOF;
        }
        if (read_n < 0) {
            return RM_ERR_READ;
        }
        buf += read_n;
        bytes_n -= read_n;
    }
    return RM_ERR_OK;
}

enum rm_error
rm_tcp_write(int fd, const void *src, size_t bytes_n) {
    ssize_t         written = 0;
    const unsigned char *buf = src;

    while (bytes_n > 0) {
        written = write(fd, buf, bytes_n);
        if (written < 0) {
            return RM_ERR_WRITE;
        }
        buf += written;
        bytes_n -= written;
    }
    return RM_ERR_OK;
}
