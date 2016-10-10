/* @file        rm_daemon.c
 * @brief       Daemeon startup.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        8 Oct 2016 08:05 PM
 * @copyright   LGPLv2.1 */


#include "rm_defs.h"
#include "rm_error.h"
#include "rm_core.h"
#include "rm_config.h"
#include "rm_signal.h"
#include "rm_util.h"
#include "rm_wq.h"
#include "rm_daemon.h"


struct rsyncme  rm;


static void rm_daemon_sigint_handler(int signo) {
    if (signo != SIGINT) {
        return;
    }
    fprintf(stderr, "\n\n==Received SIGINT==\n\nState:\n");
    fprintf(stderr, "workers_n                             \t[%u]\n", rm.wq.workers_n);
    fprintf(stderr, "workers_active_n                      \t[%u]\n", rm.wq.workers_active_n);
    fprintf(stderr, "\n\n");
    return;
}

static void rm_daemon_sigtstp_handler(int signo) {
    if (signo != SIGTSTP) {
        return;
    }
    fprintf(stderr, "\n\n==Received SIGTSTP==\n\nQuiting...\n");
    rm.state = RM_CORE_ST_SHUT_DOWN;
    fprintf(stderr, "\n\n");
    return;
}

static void
do_it_all(int fd, struct rsyncme* rm) {
    int                     to_read;
    unsigned char           *buf = NULL, *body_raw = NULL;
    struct rm_msg_hdr       *hdr = NULL;
    struct rm_msg           *msg = NULL;
    int                     err;
    char                    peer_addr_buf[INET6_ADDRSTRLEN];
    const char              *peer_addr_str = NULL;
    struct sockaddr_storage peer_addr;
    struct sockaddr_in      *peer_addr_in = NULL;
    struct sockaddr_in6     *peer_addr_in6 = NULL;
    socklen_t               peer_len;
    uint16_t                peer_port;

    char                    cli_addr_buf[INET6_ADDRSTRLEN];
    const char              *cli_addr_str = NULL;
    struct sockaddr_storage cli_addr;
    struct sockaddr_in      *cli_addr_in = NULL;
    struct sockaddr_in6     *cli_addr_in6 = NULL;
    socklen_t               cli_len;
    uint16_t                cli_port;
    enum rm_pt_type         pt;
    void                    *work = NULL;

    cli_len = sizeof(cli_addr);
    getsockname(fd, (struct sockaddr*)&cli_addr, &cli_len); /* get our side of TCP connection */
    if (cli_addr.ss_family == AF_INET) {
        cli_addr_in = (struct sockaddr_in*)&cli_addr;
        cli_addr_str = inet_ntop(AF_INET, &cli_addr_in->sin_addr, cli_addr_buf, sizeof cli_addr_buf);
        cli_port = ntohs(cli_addr_in->sin_port);
    } else { /* AF_INET6 */
        cli_addr_in6 = (struct sockaddr_in6*)&cli_addr;
        cli_addr_str = inet_ntop(AF_INET6, &cli_addr_in6->sin6_addr, cli_addr_buf, sizeof cli_addr_buf);
        cli_port = ntohs(cli_addr_in6->sin6_port);
    }
    if (cli_addr_str == NULL) {
        RM_LOG_ERR("Can't convert binary host address to presentation format, [%s]", strerror(errno));
    }
    peer_len = sizeof peer_addr;
    getpeername(fd, (struct sockaddr*)&peer_addr, &peer_len);   /* get their's side of TCP connection */
    if (peer_addr.ss_family == AF_INET) {
        peer_addr_in = (struct sockaddr_in*)&peer_addr;
        peer_addr_str = inet_ntop(AF_INET, &peer_addr_in->sin_addr, peer_addr_buf, peer_len);
        peer_port = ntohs(peer_addr_in->sin_port);
    } else { /* AF_INET6 */
        peer_addr_in6 = (struct sockaddr_in6*)&cli_addr;
        peer_addr_str = inet_ntop(AF_INET6, &peer_addr_in6->sin6_addr, peer_addr_buf, sizeof peer_addr_buf);
        peer_port = ntohs(peer_addr_in6->sin6_port);
    }
    if (peer_addr_str == NULL) {
        RM_LOG_ERR("Can't convert binary peer's address to presentation format, [%s]", strerror(errno));
    }
    if (peer_addr_str == NULL) {
        if (cli_addr_str == NULL) {
            RM_LOG_INFO("Incoming connection, port [%u], handled on local port [%u]", peer_port, cli_port);
        } else {
            RM_LOG_INFO("Incoming connection, port [%u], handled on local interface [%s] port [%u]", peer_port, cli_addr_str, cli_port);
        }
    } else {
        if (cli_addr_str == NULL) {
            RM_LOG_INFO("Incoming connection, peer [%s] port [%u], handled on local port [%u]", peer_addr_str, peer_port, cli_port);
        } else {
            RM_LOG_INFO("Incoming connection, peer [%s] port [%u], handled on local interface [%s] port [%u]", peer_addr_str, peer_port, cli_addr_str, cli_port);
        }
    }

    if (rm_core_authenticate(peer_addr_in) != RM_ERR_OK) {
        RM_LOG_ALERT("%s", "Authentication failed.\n");
        goto err_exit;
    }

    hdr = malloc(sizeof(*hdr));
    if (hdr == NULL) {
        RM_LOG_CRIT("%s", "Couldn't allocate message header. Not enough memory");
        goto err_exit;
    }
    to_read = rm_calc_msg_hdr_len(hdr);
    buf = malloc(to_read); /* buffer for incoming message header */
    if (buf == NULL) {
        RM_LOG_CRIT("%s", "Couldn't allocate buffer for the message header. Not enough memory");
        goto err_exit;
    }

    err = rm_tcp_read(fd, buf, to_read);    /* wait for incoming header */
    if (err == RM_ERR_EOF) {
        if (peer_addr_str != NULL) {
            RM_LOG_INFO("Closing conenction in passive mode, peer [%s] port [%u]", peer_addr_str, peer_port);
        } else {
            RM_LOG_INFO("Closing connection in passive mode, peer port [%u]", peer_port);
        }
        goto err_exit;
    }
    if (err == RM_ERR_READ) {
        RM_LOG_PERR("%s", "Read failed on TCP control socket");
        goto err_exit;
    }
    err = rm_core_tcp_msg_hdr_validate(buf, to_read);   /* validate the potential header of the message: check hash and pt, read_n can't be < 0 in call to validate */
    if (err != RM_ERR_OK) {
        RM_LOG_ERR("%s", "TCP control socket: bad message");
        switch (err) {

            case RM_ERR_FAIL:
                RM_LOG_ERR("%s", "Invalid hash");
                break;

            case RM_ERR_MSG_PT_UNKNOWN:
                RM_LOG_ERR("%s", "Unknown message type");
                break;

            default:
                RM_LOG_ERR("%s", "Unknown error");
                break;
        }
        goto err_exit;
    }

    rm_deserialize_msg_hdr(buf, hdr);   /* buffer is indeed the header of our message */
    free(buf);
    buf = NULL;

    pt = hdr->pt;
    to_read = hdr->len - rm_calc_msg_hdr_len(hdr);

    err = rm_core_tcp_msg_assemble(fd, pt, (void*)&body_raw, to_read);
    if (err != RM_ERR_OK) { /* TODO handle properly */
        goto err_exit;
    }
    msg = rm_deserialize_msg(pt, hdr, body_raw);
    if (msg == NULL) {
        RM_LOG_CRIT("%s", "Error deserializing message. Not enough memory");
        goto err_exit;
    }

    switch (pt) { /* message OK, process it */
        case RM_PT_MSG_PUSH:
            work = rm_work_create(RM_WORK_PROCESS_MSG_PUSH, rm, msg, rm_do_msg_push_rx); /* worker takes ownerhip of memory allocated for msg (including hdr) */
            if (work == NULL) {
                RM_LOG_CRIT("%s", "Couldn't allocate work. Not enough memory");
                goto err_exit;
            }
            rm_wq_queue_work(&rm->wq, work); 
            break;

        case RM_PT_MSG_PULL:
            break;

        case RM_PT_MSG_BYE:
            break;

        default:
            RM_LOG_ERR("%s", "Unknown TCP message type, this can't happen");
            goto err_exit;
    }

    if (body_raw != NULL) {
        free(body_raw);
        body_raw = NULL;
    }
    close(fd);
    return;

err_exit:
    if (hdr != NULL) {
        free(hdr);
        hdr = NULL;
    }
    if (body_raw != NULL) {
        free(body_raw);
        body_raw = NULL;
    }
    if (msg != NULL) {
        free(msg);
        msg = NULL;
    }
    close(fd);
}

int
main(void) {
    int             listenfd, connfd;
    int             err, errsv;
    struct sockaddr_in      srv_addr_in;
    struct sockaddr_storage cli_addr;
    socklen_t               cli_len;
    struct sigaction        sa;

    if (RM_CORE_DAEMONIZE == 1) {
        err = rm_util_daemonize("/usr/local/rsyncme", 0, "rsyncme");
        if (err != RM_ERR_OK) {
            exit(EXIT_FAILURE); /* TODO handle other errors */
        }
    } else {
        err = rm_util_chdir_umask_openlog("/usr/local/rsyncme/", 1, "rsyncme", 1);
        if (err != RM_ERR_OK) {
            exit(EXIT_FAILURE);
        }
    }
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    memset(&srv_addr_in, 0, sizeof(srv_addr_in));
    srv_addr_in.sin_family      = AF_INET;
    srv_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr_in.sin_port        = htons(RM_DEFAULT_PORT);

    int reuseaddr_on = 1;
    err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, sizeof(reuseaddr_on));
    if (err < 0) {
        RM_LOG_ERR("%s", "Setting of SO_REUSEADDR on server's managing socket failed");
    }

    err = bind(listenfd, (struct sockaddr*)&srv_addr_in, sizeof(srv_addr_in));
    if (err < 0) {
        RM_LOG_PERR("%s", "Bind of server's port to managing socket failed");
        exit(EXIT_FAILURE);
    }

    err = rm_core_init(&rm);
    if (err != RM_ERR_OK) {
        RM_LOG_CRIT("%s", "Can't initialize the engine");
        switch (err) {

            case RM_ERR_WORKQUEUE_CREATE:
                RM_LOG_ERR("%s", "Couldn't start main work queue");
                break;
            default:
                break;
        }
        exit(EXIT_FAILURE);
    }
    if (rm.wq.workers_active_n != RM_WORKERS_N) {   /* TODO CPU checking, choose optimal number of threads */
        RM_LOG_WARN("Couldn't start all workers for main work queue, [%u] requested but only [%u] started", RM_WORKERS_N, rm.wq.workers_n);
    } else {
        RM_LOG_INFO("Main work queue started with [%u] worker threads", rm.wq.workers_n);
    }

    err = listen(listenfd, RM_SERVER_LISTENQ);
    if (err < 0) {
        errsv = errno;
        RM_LOG_PERR("%s", "TCP listen on server's port failed");
        switch(errsv) {
            case EADDRINUSE:
                RM_LOG_ERR("%s", "Another socket is already listening on the same port");
            case EBADF:
                RM_LOG_ERR("%s", "Not a valid descriptor");
            case ENOTSOCK:
                RM_LOG_ERR("%s", "Not a socket");
            case EOPNOTSUPP:
                RM_LOG_ERR("%s", "The socket is not of a type that supports the listen() call");
            default:
                RM_LOG_ERR("%s", "Unknown error");
        }
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = rm_daemon_sigint_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) != 0) {
        RM_LOG_PERR("%s", "Couldn't set signal handler for SIGINT");
    }
    sa.sa_handler = rm_daemon_sigtstp_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTSTP, &sa, NULL) != 0) {
        RM_LOG_PERR("%s", "Couldn't set signal handler for SIGTSTP");
    }
    while(rm.state != RM_CORE_ST_SHUT_DOWN) {
        cli_len = sizeof(cli_addr);
        if ((connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &cli_len)) < 0) {
            errsv = errno;
            if (errsv == EINTR) {
                RM_LOG_PERR("%s", "Accept interrupted");
                continue;
            } else {
                RM_LOG_PERR("%s", "Accept error");
                continue;
            }
        }
        do_it_all(connfd, &rm);
        /* continue to listen for the next message */
    }

    RM_LOG_INFO("%s", "Shutting down");
    return 0;
}
