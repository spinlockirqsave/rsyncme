///  @file      main.c
///  @brief     Server start up.
/// @author     Piotr Gregor piotrek.gregor at gmail.com
///  @version   0.1.2
///  @date      02 Jan 2015 02:35 PM
///  @copyright LGPLv2.1


#include "rm_defs.h"
#include "rm_error.h"
#include "rm_core.h"
#include "rm_config.h"
#include "rm_signal.h"
#include "rm_util.h"


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

	if (RM_CORE_DAEMONIZE == 1)
	{
		err = rm_util_daemonize("/usr/local/rsyncme",
					0, "rsyncme");
		if (err < 0)
			exit(EXIT_FAILURE);
	} else {
		err = rm_util_chdir_umask_openlog(
			"/usr/local/rsyncme/", 1, "rsyncme");
		if (err != 0)
			exit(EXIT_FAILURE);
	}

	RM_LOG_INFO("Starting");
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
		RM_LOG_ERR("Setting of SO_REUSEADDR on server's"
			"managing socket failed");
	}

	err = bind(listenfd, (struct sockaddr_in *)
			&server_addr, sizeof(server_addr));
	if (err < 0)
	{
		RM_LOG_PERR("Bind of server's port "
			"to managing socket failed");
		exit(EXIT_FAILURE);
	}

	err = rm_core_init(&rm);
	if (err < 0)
	{
		RM_LOG_PERR("Can't initialize the engine");
		exit(EXIT_FAILURE);
	}

	err = listen(listenfd, RM_SERVER_LISTENQ);
	if (err < 0)
	{
		errsv = errno;
		RM_LOG_PERR("TCP listen on server's port failed");
		switch(errsv)
		{
		case EADDRINUSE:
			RM_LOG_ERR("Another socket is already "
				"listening on the same port");
		case EBADF:
			RM_LOG_ERR("Not a valid descriptor");
		case ENOTSOCK:
			RM_LOG_ERR("Not a socket");
		case EOPNOTSUPP:
			RM_LOG_ERR("The socket is not of a type that"
				"supports the listen() call");
		default:
			RM_LOG_ERR("Unknown error");
		}
		exit(EXIT_FAILURE);
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
			{
				RM_LOG_PERR("Accept error.");
				exit(EXIT_FAILURE);
			}
		}

		// authenticate
		if (rm_core_authenticate(&cli_addr) == -1)
		{
			// print message and errno
			RM_LOG_ERR("Authentication failed.\n");
			exit(EXIT_FAILURE);
		}
		
		memset(&buf, 0, sizeof buf);
		read_n = read(connfd, &buf, RM_TCP_MSG_MAX_LEN);
		if (read_n == -1)
		{
			RM_LOG_PERR("Read failed on TCP control socket");
			switch (read_n)
			{
				case EAGAIN:
					RM_LOG_ERR("Nonblocking I/O requested"
						" on TCP control socket");
				case EINTR:
					RM_LOG_ERR("TCP control socket: interrupted");
					continue;
				case EBADF:
					RM_LOG_ERR("TCP control socket passed "
							"is not a valid descriptor "
							"or nor open for reading");
				case EFAULT:
					RM_LOG_ERR("TCP control socket: buffer"
							" is outside accessible "
							"address space");
				case EINVAL:
					RM_LOG_ERR("TCP control socket: unsuitable "
							"for reading or wrong buffer len");
				case EIO:
					RM_LOG_ERR("TCP control socket: I/O error");
				case EISDIR:
					RM_LOG_ERR("TCP control socket: socket "
							"descriptor refers to directory");
				default:
					RM_LOG_ERR("Unknown error on TCP control socket");
			}
			exit(EXIT_FAILURE);
		}

		// validate the message: check hash token
		err = rm_core_tcp_msg_validate(buf, read_n);
		if (err < 0)
		{
			RM_LOG_ERR("TCP control socket: not a valid rsyncme message");
			continue;
		}

		// valid message
		// process
		switch (err)
		{
			case RM_MSG_PUSH:
				pthread_mutex_lock(&rm.mutex);
				rm_do_msg_push_rx(&rm, buf);
				pthread_mutex_unlock(&rm.mutex);
				break;

//			case RM_MSG_PUSH_OUT:
//				RM_LOG_ERR("Push outbound message sent to daemon "
//					"process, (please use rsyncme program to "
//					"make outgoing requests");
//				break;

			case RM_MSG_PULL:
				pthread_mutex_lock(&rm.mutex);
				rm_do_msg_pull_rx(&rm, buf);
				pthread_mutex_unlock(&rm.mutex);
				break;

//			case RM_MSG_PULL_OUT:
//				RM_LOG_ERR("Pull outbound message sent to daemon "
//					"process, (please use rsyncme program to "
//					"make outgoing requests");
//				break;

			case RM_MSG_BYE:
				break;

			default:
				RM_LOG_ERR("Unknown TCP message type,"
					" this can't happen");
				exit(EXIT_FAILURE);
		}
		// continue to listen for the next message
	}

	RM_LOG_INFO("Shutting down");
	return 0;
}

