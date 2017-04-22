/* @file        rm_tcp.c
 * @brief       TCP handling.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        18 Apr 2016 00:45 AM
 * @copyright   LGPLv2.1 */


#include "rm_tcp.h"
#include "rm.h"


enum rm_error rm_tcp_rx(int fd, void *dst, size_t bytes_n)
{
	size_t      bytes_read_total = 0;
	ssize_t     bytes_read;

	if (bytes_n == 0)
		return 0;
	while (bytes_read_total != bytes_n) {
		do {
			bytes_read = read(fd, (unsigned char*) dst + bytes_read_total, (bytes_n - bytes_read_total));
		} while ((bytes_read == -1) && (errno == EINTR));
		if (bytes_read == 0)
			return RM_ERR_EOF;
		if (bytes_read < 0)
			return RM_ERR_READ;
		bytes_read_total += bytes_read;
	}
	return RM_ERR_OK;
}

enum rm_error rm_tcp_tx(int fd, void *src, size_t bytes_n)
{
	size_t      bytes_written_total = 0;
	ssize_t     bytes_written;

	while (bytes_written_total != bytes_n) {
		do {
			bytes_written = write(fd, (unsigned char*) src + bytes_written_total, (bytes_n - bytes_written_total));
		} while ((bytes_written == -1) && (errno == EINTR));
		if (bytes_written < 0)
			return RM_ERR_WRITE;
		bytes_written_total += bytes_written;
	}
	return RM_ERR_OK;
}

int rm_tcp_tx_ch_ch(int fd, const struct rm_ch_ch_ref *e)
{
	unsigned char buf[RM_CH_CH_REF_SIZE], *pbuf;

	pbuf = rm_serialize_u32(buf, e->ch_ch.f_ch);                                    /* serialize data */
	memcpy(pbuf, &e->ch_ch.s_ch, RM_STRONG_CHECK_BYTES);
	pbuf += RM_STRONG_CHECK_BYTES;
	if (rm_tcp_tx(fd, buf, RM_CH_CH_SIZE) != RM_ERR_OK)                             /* tx over TCP connection */
		return -1;

	RM_LOG_INFO("[TX]: checksum [%u]", e->ch_ch.f_ch);
	return 0;
}

int rm_tcp_tx_ch_ch_ref(int fd, const struct rm_ch_ch_ref *e)
{
	unsigned char buf[RM_CH_CH_REF_SIZE], *pbuf;

	pbuf = rm_serialize_u32(buf, e->ch_ch.f_ch);                                    /* serialize data */
	memcpy(pbuf, &e->ch_ch.s_ch, RM_STRONG_CHECK_BYTES);
	pbuf += RM_STRONG_CHECK_BYTES;
	pbuf = rm_serialize_size_t(pbuf, e->ref);
	if (rm_tcp_tx(fd, buf, RM_CH_CH_REF_SIZE) != RM_ERR_OK)                         /* tx over TCP connection */
		return -1;
	return 0;
}

enum rm_error rm_tcp_tx_msg_ack(int fd, enum rm_pt_type pt, enum rm_error status, struct rm_session *s)
{	
	struct rm_msg_hdr   hdr = {0};
	union rm_msg_ack_u	ack;
	union rm_msg_ack_u	raw_msg_ack;

	memset(&ack, 0, sizeof(ack));
	memset(&raw_msg_ack, 0, sizeof(raw_msg_ack));

	hdr.pt = pt;
	hdr.flags = status;
	hdr.len = rm_calc_msg_len(&hdr);
	hdr.hash = rm_core_hdr_hash(&hdr);
	ack.msg_ack.hdr = &hdr;

	switch (pt) {
		case RM_PT_MSG_PUSH_ACK:
			if (s != NULL) {														/* if session is NULL this is ACK with error, delta port and checkums number are not valid numbers (will be TXed as 0), otherwise take values from PUSH RX session */
				struct rm_session_push_rx *prvt = s->prvt;
				ack.msg_push_ack.delta_port = prvt->delta_port;
				ack.msg_push_ack.ch_ch_n = prvt->ch_ch_n;
			}
			rm_serialize_msg_push_ack((unsigned char*)&raw_msg_ack, &ack.msg_push_ack);
			break;
		case RM_PT_MSG_ACK:
			rm_serialize_msg_ack((unsigned char*)&raw_msg_ack, &ack.msg_ack);
			break;
		default:
			return RM_ERR_BAD_CALL;
	}

	return rm_tcp_tx(fd, &raw_msg_ack, hdr.len);
}

int rm_tcp_set_socket_blocking_mode(int fd, uint8_t on)
{
	if (fd < 0 || on > 1)
		return -1;

#ifdef WIN32
	unsigned long mode = 1 - on;
	return ioctlsocket(fd, FIONBIO, &mode);
#else
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		return -2;
	flags = on ? (flags &~ O_NONBLOCK) : (flags | O_NONBLOCK);
	return fcntl(fd, F_SETFL, flags);
#endif
}

/* @brief   Resolve host names.
 * @details Return 0 on success or EAI_* errcode to be used with gai_strerror(). */
static int rm_core_resolve_host(const char *host, unsigned int port, struct addrinfo *hints, struct addrinfo **res)
{
	char strport[32];
	snprintf(strport, sizeof strport, "%u", port);
	return getaddrinfo(host, strport, hints, res);
}

enum rm_error rm_core_connect(int *fd, const char *host, uint16_t port, int domain, int type, const char **err_str)
{
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
		if (*fd < 0)
			continue;
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

enum rm_error rm_tcp_connect(int *fd, const char *host, uint16_t port, int domain, const char **err_str)
{
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
		if (*fd < 0)
			continue;
		if (connect(*fd, res->ai_addr, res->ai_addrlen) == 0)
			break;      /* success */
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

enum rm_error rm_tcp_connect_nonblock_timeout_once(int fd, struct addrinfo *res, uint16_t timeout_s, uint16_t timeout_us)
{
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
	if (err == ECONNREFUSED)
		return RM_ERR_CONNECT_REFUSED;
	if (err == -1) {
		err = RM_ERR_FAIL;
		goto exit;
	} else if (err > 0) {
		len = sizeof fd_err;
		getsockopt(fd, SOL_SOCKET, SO_ERROR, &fd_err, &len);
		if (fd_err == 0) {
			err = RM_ERR_OK;
		} else if (fd_err == ECONNREFUSED) {
			err = RM_ERR_CONNECT_REFUSED;
		} else if (fd_err == EHOSTUNREACH) {
			err = RM_ERR_CONNECT_HOSTUNREACH;
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

enum rm_error rm_tcp_connect_nonblock_timeout(int *fd, const char *host, uint16_t port, int domain, uint16_t timeout_s, uint16_t timeout_us, const char **err_str)
{
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

	err = rm_core_resolve_host(host, port, &hints, &res); /* TODO do not use resolve host (performance) */
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
		} else if (err == RM_ERR_CONNECT_REFUSED) {
			errsave = RM_ERR_CONNECT_REFUSED;
		} else if (err == RM_ERR_CONNECT_HOSTUNREACH) {
			errsave = RM_ERR_CONNECT_HOSTUNREACH;
		} else {
			errsave = RM_ERR_CONNECT_GEN_ERR;
		}
		close(*fd);
	} while ((res = res->ai_next) != NULL);

	if ((res == NULL) && (errno != EINPROGRESS))
		*err_str = strerror(errno);

	freeaddrinfo(ressave);
	return errsave;
}

enum rm_error rm_tcp_connect_nonblock_timeout_once_sockaddr(int fd, struct sockaddr *peer_addr, uint16_t timeout_s, uint16_t timeout_us)
{
	int             err, fd_err;
	socklen_t       len;

	rm_tcp_set_socket_blocking_mode(fd, 0);

	err = connect(fd, peer_addr, sizeof(*peer_addr));
	if (err == 0) {
		return RM_ERR_OK;
	} else if ((errno != EINPROGRESS) && (errno != EINTR)) {
		return RM_ERR_FAIL;
	}

	err = rm_core_select(fd, RM_WRITE, timeout_s, timeout_us);
	if (err == ECONNREFUSED)
		return RM_ERR_CONNECT_REFUSED;
	if (err == -1) {
		err = RM_ERR_FAIL;
		goto exit;
	} else if (err > 0) {
		len = sizeof fd_err;
		getsockopt(fd, SOL_SOCKET, SO_ERROR, &fd_err, &len);
		if (fd_err == 0) {
			err = RM_ERR_OK;
		} else if (fd_err == ECONNREFUSED) {
			err = RM_ERR_CONNECT_REFUSED;
		} else if (fd_err == EHOSTUNREACH) {
			err = RM_ERR_CONNECT_HOSTUNREACH;
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

enum rm_error rm_tcp_connect_nonblock_timeout_sockaddr(int *fd, struct sockaddr *peer_addr, uint16_t timeout_s, uint16_t timeout_us, const char **err_str)
{
	int             err, errsave = RM_ERR_FAIL;

	*fd = socket(peer_addr->sa_family, SOCK_STREAM, 0);
	if (*fd < 0)
		goto err_exit;

	err = rm_tcp_connect_nonblock_timeout_once_sockaddr(*fd, peer_addr, timeout_s, timeout_us);
	if (err != RM_ERR_OK) {
		if (err == RM_ERR_CONNECT_TIMEOUT)
			errsave = RM_ERR_CONNECT_TIMEOUT;
		else if (err == RM_ERR_CONNECT_REFUSED)
			errsave = RM_ERR_CONNECT_REFUSED;
		else if (err == RM_ERR_CONNECT_HOSTUNREACH)
			errsave = RM_ERR_CONNECT_HOSTUNREACH;
		else
			errsave = RM_ERR_CONNECT_GEN_ERR;

		*err_str = strerror(errno);
		close(*fd);
		goto err_exit;
	}
	return RM_ERR_OK;

err_exit:
	*err_str = strerror(errno);
	return errsave;
}

int rm_tcp_listen(int *listen_fd, in_addr_t addr, uint16_t *port, int reuseaddr, int backlog)
{
	int err;
	struct sockaddr_in      srv_addr_in;

	int listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	memset(&srv_addr_in, 0, sizeof(srv_addr_in));
	srv_addr_in.sin_family      = AF_INET;
	srv_addr_in.sin_addr.s_addr = htonl(addr);
	srv_addr_in.sin_port        = htons(*port);

	if (reuseaddr) {
		err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
		if (err < 0)
			return err;
	}

	err = bind(listenfd, (struct sockaddr*) &srv_addr_in, sizeof(srv_addr_in));
	if (err < 0) {
		return err;
	}

	err = listen(listenfd, backlog);
	if (err < 0) {
		return err;
	}
	*listen_fd = listenfd;
	if (*port == 0) {
		socklen_t len = sizeof(srv_addr_in);
		if (getsockname(listenfd, (struct sockaddr*) &srv_addr_in, &len) == -1)
			return -5555;
		*port = ntohs(srv_addr_in.sin_port);
	}
	return 0;
}

enum rm_error rm_tcp_read(int fd, void *dst, size_t bytes_n)
{
	ssize_t         read_n = 0;
	unsigned char   *buf = dst;

	while (bytes_n > 0) {
		read_n = read(fd, buf, bytes_n);
		if (read_n == 0)
			return RM_ERR_EOF;
		if (read_n < 0)
			return RM_ERR_READ;
		buf += read_n;
		bytes_n -= read_n;
	}
	return RM_ERR_OK;
}

enum rm_error rm_tcp_write(int fd, const void *src, size_t bytes_n)
{
	ssize_t         written = 0;
	const unsigned char *buf = src;

	while (bytes_n > 0) {
		written = write(fd, buf, bytes_n);
		if (written < 0)
			return RM_ERR_WRITE;
		buf += written;
		bytes_n -= written;
	}
	return RM_ERR_OK;
}
