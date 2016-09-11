/* @file        main.c
 * @brief       Server start up.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        02 Jan 2015 02:35 PM
 * @copyright   LGPLv2.1 */


#include "rm_defs.h"
#include "rm_error.h"
#include "rm_core.h"
#include "rm_config.h"
#include "rm_signal.h"
#include "rm_util.h"
#include "rm_wq.h"


int
main(void) {
    struct rsyncme  rm;
    int             read_n, read_now, to_read;
    unsigned char   *buf = NULL, *hdr = NULL, *body_raw = NULL;
    int             listenfd, connfd;
    int             err, errsv;
    struct sockaddr_in      srv_addr_in;

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
    void*                   work;

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
    RM_LOG_INFO("%s", "Starting main work queue");

    if (rm_wq_workqueue_init(&rm.wq, RM_WORKERS_N, "main_queue") != RM_ERR_OK) {   /* TODO CPU checking, choose optimal number of threads */
        RM_LOG_ERR("%s", "Couldn't start main work queue");
        exit(EXIT_FAILURE);
    }
    if (rm.wq.workers_active_n != RM_WORKERS_N) {   /* TODO CPU checking, choose optimal number of threads */
        RM_LOG_WARN("Couldn't start all workers for main work queue, [%u] requested but only [%u] started", RM_WORKERS_N, rm.wq.workers_n);
    } else {
        RM_LOG_INFO("Main work queue started with [%u] worker threads", rm.wq.workers_n);
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
    if (err < 0) {
        RM_LOG_PERR("%s", "Can't initialize the engine");
        exit(EXIT_FAILURE);
    }

    err = listen(listenfd, RM_SERVER_LISTENQ);
    if (err < 0) {
        errsv = errno;
        RM_LOG_PERR("%s", "TCP listen on server's port failed");
        switch(errsv) {
            case EADDRINUSE:
                RM_LOG_ERR("%s", "Another socket is already "
                        "listening on the same port");
            case EBADF:
                RM_LOG_ERR("%s", "Not a valid descriptor");
            case ENOTSOCK:
                RM_LOG_ERR("%s", "Not a socket");
            case EOPNOTSUPP:
                RM_LOG_ERR("%s", "The socket is not of a type that"
                        "supports the listen() call");
            default:
                RM_LOG_ERR("%s", "Unknown error");
        }
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, rm_sigint_h);
    while(rm.state != RM_CORE_ST_SHUT_DOWN) {
        cli_len = sizeof(cli_addr);
        if ((connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &cli_len)) < 0) {
            errsv = errno;
            if (errsv == EINTR) {
                continue;
            } else {
                RM_LOG_PERR("%s", "Accept error.");
                exit(EXIT_FAILURE);
            }
        }
        cli_len = sizeof(cli_addr);
        getsockname(connfd, (struct sockaddr*)&cli_addr, &cli_len); /* get our side of TCP connection */
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
        getpeername(connfd, (struct sockaddr*)&peer_addr, &peer_len);   /* get their's side of TCP connection */
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

        if (rm_core_authenticate(cli_addr_in) != RM_ERR_OK) {
            RM_LOG_ERR("%s", "Authentication failed.\n");
            close(connfd);
            continue;
        }

        to_read = sizeof(struct rm_msg_hdr);
        read_n = 0;
        read_now = 0;
        buf = malloc(to_read);
        if (buf == NULL) {
            RM_LOG_CRIT("%s", "Couldn't allocate message header. Not enough memory");
            continue;
        }
        while ((read_n < to_read) && ((read_now = read(connfd, &buf[read_n], to_read - read_n)) > 0)) {
            read_n += read_now;
        } 
        if (read_n == 0) {
            if (peer_addr_str != NULL) {
                RM_LOG_INFO("Closing conenction in passive mode, peer [%s] port [%u]", peer_addr_str, peer_port);
            } else {
                RM_LOG_INFO("Closing connection in passive mode, peer port [%u]", peer_port);
            }
            close(connfd);
            continue;
        }
        if (read_n < 0) {
            RM_LOG_PERR("%s", "Read failed on TCP control socket");
            switch (read_n) {
                case EAGAIN:
                    RM_LOG_ERR("%s", "Nonblocking I/O requested on TCP control socket");
                    break;
                case EINTR:
                    RM_LOG_ERR("%s", "TCP control socket: interrupted");
                    continue;
                case EBADF:
                    RM_LOG_ERR("%s", "TCP control socket passed is not a valid descriptor or nor open for reading");
                    break;
                case EFAULT:
                    RM_LOG_ERR("%s", "TCP control socket: buffer is outside accessible address space");
                    break;
                case EINVAL:
                    RM_LOG_ERR("%s", "TCP control socket: unsuitable for reading or wrong buffer len");
                    break;
                case EIO:
                    RM_LOG_ERR("%s", "TCP control socket: I/O error");
                    break;
                case EISDIR:
                    RM_LOG_ERR("%s", "TCP control socket: socket descriptor refers to directory");
                    break;
                default:
                    RM_LOG_ERR("%s", "Unknown error on TCP control socket");
                    break;
            }
            exit(EXIT_FAILURE);
        }
        err = rm_core_tcp_msg_hdr_validate(buf, read_n);    /* validate the potential header of the message: check hash and pt, read_n can't be < 0 in call to validate */
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
            continue;
        }
        hdr = buf;

        pt = rm_get_msg_hdr_pt(buf);

        switch (pt) { /* message OK, process it */
            case RM_PT_MSG_PUSH:
                work = rm_work_create(RM_WORK_PROCESS_MSG_PUSH, &rm, hdr, body_raw, rm_do_msg_push_rx);
                if (work == NULL) {
                    RM_LOG_CRIT("%s", "Couldn't allocate work. Not enough memory");
                    continue;
                }
                rm_wq_queue_work(&rm.wq, work); 
                //pthread_mutex_lock(&rm.mutex);
                //rm_do_msg_push_rx(&);
                //pthread_mutex_unlock(&rm.mutex);
                break;

                /*			case RM_MSG_PUSH_OUT:
                            RM_LOG_ERR("Push outbound message sent to daemon "
                            "process, (please use rsyncme program to "
                            "make outgoing requests");
                            break;
                            */
            case RM_PT_MSG_PULL:
                pthread_mutex_lock(&rm.mutex);
                rm_do_msg_pull_rx(&rm, buf);
                pthread_mutex_unlock(&rm.mutex);
                break;

                /*			case RM_MSG_PULL_OUT:
                            RM_LOG_ERR("Pull outbound message sent to daemon "
                            "process, (please use rsyncme program to "
                            "make outgoing requests");
                            break;
                            */
            case RM_PT_MSG_BYE:
                break;

            default:
                RM_LOG_ERR("%s", "Unknown TCP message type, this can't happen");
                exit(EXIT_FAILURE);
        }
        /* continue to listen for the next message */
    }

    RM_LOG_INFO("%s", "Shutting down");
    return 0;
}

