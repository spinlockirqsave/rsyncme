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

	// sockets
	int			listenfd, connfd;
	socklen_t		cli_len;
	struct sockaddr_in	cli_addr, server_addr;
	int			err, errsv;
	char			cli_ip_str[INET_ADDRSTRLEN];
	uint32_t		cli_ip;
	uint16_t		cli_port;

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

		// IPv4
		cli_ip = cli_addr.sin_addr.s_addr;
		inet_ntop(AF_INET, &cli_ip, cli_ip_str, INET_ADDRSTRLEN);
		cli_port = ntohs(cli_addr.sin_port);
		// authenticate
		if (rm_core_authenticate(&cli_addr) == -1)
		{
			// print message and errno
			rm_perr_abort("Authentication failed.\n");
			exit(1);
		}
		// start processing of TCP events sent by control application
		err = rm_core_process_connection(&rm, connfd);
                if (err < 0)
		{
			close(connfd);
			if (err == -1)
			{
				// connections limit reached
				// send EBUSY
				RM_ERR("Connection on socket [%d] from IP [%s],"
					"port [%u] dropped because of max number"
					" of connections", connfd, cli_ip_str,
					cli_port);
			} else if (err == -2)
			{
				// malloc failed
				// send EBUSY
				RM_ERR("Connection on socket [%d] from IP [%s],"
					"port [%u] dropped because of memory "
					"exhaustion", connfd, cli_ip_str,
					cli_port);
			}
		}

		// connection has been added

		// continue to listen for the next connection
	}

	fprintf(stderr, "[%s] Shutting down", __func__);
	return 0;
}

