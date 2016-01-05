///  @file      main.c
///  @brief     Server start up.
///  @author    peterg
///  @version   0.1.1
///  @date      02 Jan 2015 02:35 PM
///  @copyright LGPLv2.1


#include "rm_defs.h"
#include "rm_error.h"
#include "rm_core.h"
#include "rm_config.h"
#include "rm_signal.h"


int
main()
{
        struct rsyncme		rm;	// global rsyncme object
	int			read_n;
	unsigned char		buf[RM_TCP_MSG_MAX_LEN];

	// sockets
	int			listenfd, connfd;
	socklen_t		cli_len;
	struct sockaddr_in	cli_addr, server_addr;
	int			err, errsv;

	fprintf(stderr, "[%s] Starting", __func__);
	listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port        = htons(RM_SERVER_PORT);

	// allow to rebind
	int reuseaddr_on = 1;
	err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
		&reuseaddr_on, sizeof(reuseaddr_on));
	if (err < 0)
	{
		rm_err("Setting of SO_REUSEADDR on server's"
			"managing socket failed");
	}

	err = bind(listenfd, (struct sockaddr_in *)
			&server_addr, sizeof(server_addr));
	if (err < 0)
	{
		rm_perr_abort("Bind of server's port "
			"to managing socket failed");
	}

	err = rm_core_init(&rm);
	if (err < 0)
	{
		rm_perr_abort("Can't initialize the engine");
	}

	err = listen(listenfd, RM_SERVER_LISTENQ);
	if (err < 0)
	{
		errsv = errno;
		rm_err("TCP listen on server's port failed");
		switch(errsv)
		{
		case EADDRINUSE:
			rm_perr_abort("Another socket is already "
				"listening on the same port");
		case EBADF:
			rm_perr_abort("Not a valid descriptor");
		case ENOTSOCK:
			rm_perr_abort("Not a socket");
		case EOPNOTSUPP:
			rm_perr_abort("The socket is not of a type that"
				"supports the listen() call");
		default:
			return -2;
		}
	}

	signal(SIGINT, rm_sigint_h);
	while(rm.state != RM_CORE_ST_SHUT_DOWN)
	{
		cli_len = sizeof(cli_addr);
		if ((connfd = accept(listenfd, &cli_addr, &cli_len)) < 0)
		{
			errsv = errno;
			if (errsv == EINTR)
				continue;
			else
				rm_perr_abort("Accept error.");
		}

		// authenticate
		if (rm_core_authenticate(&cli_addr) == -1)
		{
			// print message and errno
			rm_perr_abort("Authentication failed.\n");
			exit(1);
		}
		
		memset(&buf, 0, sizeof buf);
		read_n = read(connfd, &buf, RM_TCP_MSG_MAX_LEN);
		if (read_n == -1)
		{
			switch (read_n)
			{
				case EAGAIN:
					rm_perr_abort("Nonblocking I/O requested"
						" on TCP control socket");
				case EINTR:
					rm_err("TCP control socket: interrupted");
					continue;
				case EBADF:
					rm_perr_abort("TCP control socket passed "
							"is not a valid descriptor "
							"or nor open for reading");
				case EFAULT:
					rm_perr_abort("TCP control socket: buffer"
							" is outside accessible "
							"address space");
				case EINVAL:
					rm_perr_abort("TCP control socket: unsuitable "
							"for reading or wrong buffer len");
				case EIO:
					rm_perr_abort("TCP control socket: I/O error");
				case EISDIR:
					rm_perr_abort("TCP control socket: socket "
							"descriptor refers to directory");
				default:
					rm_perr_abort("TCP control socket: "
							"unknown error");
			}
		}

		// validate the message: check hash token
		err = rm_core_tcp_msg_validate(buf, read_n);
		if (err < 0)
		{
			rm_err("TCP control socket: not a valid rsyncme message");
			continue;
		}

		// process the message
		switch (err)
		{
			case RM_MSG_PUSH_IN:
				pthread_mutex_lock(&rm.mutex);
				rm_do_msg_push_in(&rm, buf);
				pthread_mutex_unlock(&rm.mutex);
				break;

			case RM_MSG_PUSH_OUT:
				pthread_mutex_lock(&rm.mutex);
				rm_do_msg_push_out(&rm, buf);
				pthread_mutex_unlock(&rm.mutex);
				break;

			case RM_MSG_PULL_IN:
				pthread_mutex_lock(&rm.mutex);
				rm_do_msg_pull_in(&rm, buf);
				pthread_mutex_unlock(&rm.mutex);
				break;

			case RM_MSG_PULL_OUT:
				pthread_mutex_lock(&rm.mutex);
				rm_do_msg_pull_out(&rm, buf);
				pthread_mutex_unlock(&rm.mutex);
				break;

			case RM_MSG_BYE:
				break;

			default:
				rm_perr_abort("Unknown TCP message type");
		}
		// continue to listen for the next message
	}

	fprintf(stderr, "[%s] Shutting down", __func__);
	return 0;
}

