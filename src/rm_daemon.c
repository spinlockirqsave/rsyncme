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

#include <getopt.h>


struct rsyncme  rm;

static void rm_daemon_sigint_handler(int signo) {
	if (signo != SIGINT) {
		return;
	}
	rm.signo = SIGINT;
	rm.signal_pending = 1;
	return;
}

static void rm_daemon_sigtstp_handler(int signo) {
	if (signo != SIGTSTP) {
		return;
	}
	rm.signo = SIGTSTP;
	rm.signal_pending = 1;
	return;
}

static void rm_daemon_signal_handler(int signo) {
	switch (signo) {
		case SIGINT:
			fprintf(stderr, "\n\n==Received SIGINT==\n\nState:\n");
			pthread_mutex_lock(&rm.mutex);
			fprintf(stderr, "workers_n                             \t[%u]\n", rm.wq.workers_n);
			fprintf(stderr, "workers_active_n                      \t[%u]\n", rm.wq.workers_active_n);
			pthread_mutex_unlock(&rm.mutex);
			fprintf(stderr, "\n\n");
			break;

		case SIGTSTP:
			fprintf(stderr, "\n\n==Received SIGTSTP==\n\nQuiting...\n");
			pthread_mutex_lock(&rm.mutex);
			rm.state = RM_CORE_ST_SHUT_DOWN;
			pthread_mutex_unlock(&rm.mutex);
			fprintf(stderr, "\n\n");
			break;

		default:
			break;
	}
	rm.signo = 0;
	rm.signal_pending = 0;
	fflush(stderr);
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
		RM_LOG_ERR("core: Can't convert binary host address to presentation format, [%s]", strerror(errno));
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
		RM_LOG_ERR("core: Can't convert binary peer's address to presentation format, [%s]", strerror(errno));
	}
	if (peer_addr_str == NULL) {
		if (cli_addr_str == NULL) {
			RM_LOG_INFO("core: Incoming connection, port [%u], handled on local port [%u]", peer_port, cli_port);
		} else {
			RM_LOG_INFO("Incoming connection, port [%u], handled on local interface [%s] port [%u]", peer_port, cli_addr_str, cli_port);
		}
	} else {
		if (cli_addr_str == NULL) {
			RM_LOG_INFO("core: Incoming connection, peer [%s] port [%u], handled on local port [%u]", peer_addr_str, peer_port, cli_port);
		} else {
			RM_LOG_INFO("core: Incoming connection, peer [%s] port [%u], handled on local interface [%s] port [%u]", peer_addr_str, peer_port, cli_addr_str, cli_port);
		}
	}

	if (rm->opt.authenticate) {
		if (rm_core_authenticate(peer_addr_in) != RM_ERR_OK) {
			RM_LOG_ALERT("core: Authentication failed. Receiver doesn't accept requests from [%s]. Please skip --auth flag when starting the receiver to disable authentication\n", peer_addr_str);
			err = RM_ERR_AUTH;
			goto err_exit;
		}
	}

	hdr = malloc(sizeof(*hdr));
	if (hdr == NULL) {
		RM_LOG_CRIT("%s", "core: Couldn't allocate message header. Not enough memory");
		goto err_exit;
	}
	to_read = rm_calc_msg_hdr_len(hdr);
	buf = malloc(to_read);																						/* buffer for incoming message header */
	if (buf == NULL) {
		RM_LOG_CRIT("%s", "core: Couldn't allocate buffer for the message header. Not enough memory");
		goto err_exit;
	}

	RM_LOG_INFO("core: Waiting for incoming header from peer [%s] port [%u], on local interface [%s] port [%u]", peer_addr_str, peer_port, cli_addr_str, cli_port);
	err = rm_tcp_read(fd, buf, to_read);																		/* wait for incoming header */
	if (err == RM_ERR_EOF) {
		if (peer_addr_str != NULL) {
			RM_LOG_INFO("core: Closing connection in passive mode, peer [%s] port [%u]", peer_addr_str, peer_port);
		} else {
			RM_LOG_INFO("core: Closing connection in passive mode, peer port [%u]", peer_port);
		}
		goto err_exit;
	}
	if (err == RM_ERR_READ) {
		RM_LOG_PERR("%s", "core: Read failed on TCP control socket");
		goto err_exit;
	}

	RM_LOG_INFO("core: Validating header from peer [%s] port [%u]", peer_addr_str, peer_port);
	err = rm_core_tcp_msg_hdr_validate(buf, to_read);															/* validate the potential header of the message: check hash and pt, read_n can't be < 0 in call to validate */
	if (err != RM_ERR_OK) {
		RM_LOG_ERR("%s", "core: TCP control socket: bad message");
		switch (err) {

			case RM_ERR_FAIL:
				RM_LOG_ERR("%s", "core: Message corrupted: invalid hash or message too short");
				break;

			case RM_ERR_MSG_PT_UNKNOWN:
				RM_LOG_ERR("%s", "core: Unknown message type");
				break;

			default:
				RM_LOG_ERR("%s", "core: Unknown error");
				break;
		}
		goto err_exit;
	}

	rm_deserialize_msg_hdr(buf, hdr);																			/* buffer is indeed the header of our message */
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
		RM_LOG_CRIT("%s", "core: Error deserializing message. Not enough memory");
		goto err_exit;
	}

	switch (pt) {																								/* message OK, process it */
		case RM_PT_MSG_PUSH:
			RM_LOG_INFO("core: Enqueuing MSG_PUSH work from peer [%s] port [%u]", peer_addr_str, peer_port);
			work = rm_work_create(RM_WORK_PROCESS_MSG_PUSH, rm, msg, fd, rm_do_msg_push_rx, rm_msg_push_dtor); /* worker takes the ownership of TCP socket and memory allocated for msg (including hdr) */
			if (work == NULL) {
				RM_LOG_CRIT("%s", "core: Couldn't allocate work. Not enough memory");
				goto err_exit;
			}
			rm_wq_queue_work(&rm->wq, work); 
			break;

		case RM_PT_MSG_PULL:
			RM_LOG_INFO("core: Enqueuing MSG_PULL work from peer [%s] port [%u]", peer_addr_str, peer_port);
			break;

		case RM_PT_MSG_BYE:
			RM_LOG_INFO("core: Enqueuing MSG_BYE work from peer [%s] port [%u]", peer_addr_str, peer_port);
			break;

		default:
			RM_LOG_ERR("%s", "core: Unknown TCP message type, this can't happen");
			goto err_exit;
	}

	if (body_raw != NULL) {
		free(body_raw);
		body_raw = NULL;
	}
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

	rm_tcp_tx_msg_ack(fd, RM_PT_MSG_ACK, err, NULL); /* send general ACK with error */
	RM_LOG_INFO("core: TXed ACK with error [%u] to peer [%s] port [%u]", err, peer_addr_str, peer_port);
	close(fd);
	RM_LOG_INFO("core: Closed connection with peer [%s] port [%u]", peer_addr_str, peer_port);
}

static void rsyncme_d_usage(const char *name)
{
	if (name == NULL) {
		return;
	}
	fprintf(stderr, "\nusage:\t %s [-l loglevel]\n\n", name);
	fprintf(stderr, "     \t -l           : logging level [0-3]\n"
			"     \t                0 - no logging, 1 - normal, 2 - +threads, 3 - verbose\n");
	fprintf(stderr, "     \t --auth       : authenticate requests\n");
	fprintf(stderr, "     \t --help       : display this help and exit\n");
	fprintf(stderr, "     \t --version    : output version information and exit\n");
	fprintf(stderr, "     \t --verbose    : max logging\n");
	fprintf(stderr, "\n");

	fprintf(stderr, "\nExamples:\n");
	fprintf(stderr, "	rsyncme_d\n"
			"\t\tThis will start daemon with standard settings (normal logging level)\n");
	fprintf(stderr, "	rsyncme_d -l 2\n"
			"\t\tThis will start daemon with logging including information\n"
			"\t\tfrom worker threads\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "For more information please consult documentation.\n");
	fprintf(stderr, "\n");
}

static void rsyncme_d_range_error(char argument, unsigned long value)
{
	fprintf(stderr, "\nERR, argument [%c] too big [%lu]\n\n", argument, value);
}

static void rsyncme_d_help_hint(const char *name)
{
	if (name == NULL)
		return;
	fprintf(stderr, "\nTry %s --help for more information.\n", name);
	return;
}

int main(int argc, char *argv[]) {
	char                c = 0;
	char                *pCh = NULL;
	unsigned long long  helper = 0;
	int                     listenfd = -1, connfd = -1;
	int                     err = -1, errsv = -1;
	struct sockaddr_in      srv_addr_in = {0};;
	struct sockaddr_storage cli_addr = {0};;
	socklen_t               cli_len = 0;
	struct sigaction        sa;
	enum rm_error           status = RM_ERR_OK;
	char ip[INET_ADDRSTRLEN];
	const char *ipptr = NULL;
	struct rm_core_options	opt = {0};

	memset(&sa, 0, sizeof(struct sigaction));

	int option_index = 0;
	struct option long_options[] = {
		{ "auth", no_argument, 0, 1 },
		{ "help", no_argument, 0, 2 },
		{ "version", no_argument, 0, 3 },
		{ "verbose", no_argument, 0, 4 },
		{ 0 }
	};

	while ((c = getopt_long(argc, argv, "l:", long_options, &option_index)) != -1) {    /* parse optional command line arguments */
		switch (c) {

			case 0:
				if (long_options[option_index].flag != 0)                               /* If this option set a flag, turn flag on in ioctl struct */
					break;
				fprintf(stderr, "Long option [%s]", long_options[option_index].name);
				if (optarg)
					fprintf(stderr, " with arg [%s]", optarg);
				fprintf(stderr, "\n");
				break;

			case 1:
				opt.authenticate = 1;													/* authenticate requests */
				break;

			case 2:
				rsyncme_d_usage(argv[0]);                                               /* --help */
				exit(EXIT_SUCCESS);
				break;

			case 3:
				fprintf(stderr, "\nversion [%s]\n", RM_VERSION);                        /* --version */
				exit(EXIT_SUCCESS);
				break;

			case 4:
				opt.loglevel = RM_LOGLEVEL_VERBOSE;										/* --verbose */
				break;

			case 'l':
				helper = strtoul(optarg, &pCh, 10);
				if (helper > RM_LOGLEVEL_VERBOSE) {
					rsyncme_d_range_error(c, helper);
					exit(EXIT_FAILURE);
				}
				if ((pCh == optarg) || (*pCh != '\0')) {                                /* check */
					fprintf(stderr, "Invalid argument\n");
					fprintf(stderr, "Parameter conversion error, nonconvertible part is: [%s]\n", pCh);
					rsyncme_d_help_hint(argv[0]);
					exit(EXIT_FAILURE);
				}
				opt.loglevel = helper;
				break;

			case '?':
				if (optopt == 'l')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint(optopt))
					fprintf(stderr,"Unknown option '-%c'.\n", optopt);
				else {
					fprintf(stderr, "Are there any long options? "
							"Please check that you have typed them correctly.\n");
				}
				rsyncme_d_usage(argv[0]);
				exit(EXIT_FAILURE);

			default:
				rsyncme_d_help_hint(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

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
		RM_LOG_ERR("%s", "core: Setting of SO_REUSEADDR on server's managing socket failed");
	}

	err = bind(listenfd, (struct sockaddr*)&srv_addr_in, sizeof(srv_addr_in));
	if (err < 0) {
		RM_LOG_PERR("%s", "core: Bind of server's port to managing socket failed");
		exit(EXIT_FAILURE);
	}

	status = rm_core_init(&rm, &opt);
	if (status != RM_ERR_OK) {
		RM_LOG_CRIT("%s", "core: Can't initialize the engine");
		switch (status) {

			case RM_ERR_WORKQUEUE_CREATE:
				RM_LOG_ERR("%s", "Couldn't start main work queue");
				break;
			default:
				break;
		}
		exit(EXIT_FAILURE);
	}
	if (rm.wq.workers_active_n != RM_WORKERS_N) {   /* TODO CPU checking, choose optimal number of threads */
		RM_LOG_WARN("core: Couldn't start all workers for main work queue, [%u] requested but only [%u] started", RM_WORKERS_N, rm.wq.workers_n);
	} else {
		RM_LOG_INFO("core: Main work queue started with [%u] worker threads", rm.wq.workers_n);
	}

	err = listen(listenfd, RM_SERVER_LISTENQ);
	if (err < 0) {
		errsv = errno;
		RM_LOG_PERR("%s", "core: TCP listen on server's port failed");
		switch(errsv) {
			case EADDRINUSE:
				RM_LOG_ERR("%s", "core: Another socket is already listening on the same port");
			case EBADF:
				RM_LOG_ERR("%s", "core: Not a valid descriptor");
			case ENOTSOCK:
				RM_LOG_ERR("%s", "core: Not a socket");
			case EOPNOTSUPP:
				RM_LOG_ERR("%s", "core: The socket is not of a type that supports the listen() call");
			default:
				RM_LOG_ERR("%s", "core: Unknown error");
		}
		exit(EXIT_FAILURE);
	}
	ipptr = inet_ntop(AF_INET, &srv_addr_in.sin_addr, ip, sizeof(ip));
	if (ipptr == NULL) {
		ipptr = "Unknown IP";
	}
	RM_LOG_INFO("core: Listening on address [%s], port [%u]", ipptr, ntohs(srv_addr_in.sin_port));

	sa.sa_handler = rm_daemon_sigint_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) != 0) {
		RM_LOG_PERR("%s", "core: Couldn't set signal handler for SIGINT");
	}
	sa.sa_handler = rm_daemon_sigtstp_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGTSTP, &sa, NULL) != 0) {
		RM_LOG_PERR("%s", "core: Couldn't set signal handler for SIGTSTP");
	}
	while(rm.state != RM_CORE_ST_SHUT_DOWN) {
		cli_len = sizeof(cli_addr);
		if ((connfd = accept(listenfd, (struct sockaddr *) &cli_addr, &cli_len)) < 0) {
			errsv = errno;
			if (errsv == EINTR) {
				RM_LOG_PERR("%s", "core: Accept interrupted");
				if (rm.signal_pending == 1) {
					rm_daemon_signal_handler(rm.signo);
				}
				continue;
			} else {
				RM_LOG_PERR("%s", "core: Accept error");
				continue;
			}
		}
		do_it_all(connfd, &rm);
	}

	RM_LOG_INFO("%s", "core: Shutdown");

	pthread_mutex_lock(&rm.mutex);
	status = rm_core_deinit(&rm);
	pthread_mutex_unlock(&rm.mutex);
	if (status != RM_ERR_OK) {
		switch (status) {
			case RM_ERR_BUSY:
				RM_LOG_ERR("%s", "core: Sessions hashtable not empty");
				break;
			case RM_ERR_FAIL:
				RM_LOG_ERR("%s", "core: Sessions list not empty");
				break;
			case RM_ERR_WORKQUEUE_STOP:
				RM_LOG_ERR("%s", "core: Error stopping main workqueue");
				break;
			case RM_ERR_MEM:
				RM_LOG_ERR("%s", "core: Error deinitializing main workqueue");
				break;
			default:
				break;
		}
	}

	return 0;
}
