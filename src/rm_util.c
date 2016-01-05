/// @file	rm_util.c
/// @brief	Utilities.
/// @author	peterg
/// @version	0.1.1
/// @date	04 Jan 2016 08:08 PM
/// @copyright	LGPLv2.1


#include "rm_util.h"


int
rm_util_dt(char *buf)
{
	struct tm	t;
	time_t		now = time(0);

	// localtime_r is thread safe
	// (and therefore reentrant)
	if (localtime_r(&now, &t) != 0)
	{
		strftime(buf, 20, "%Y-%m-%d_%H-%M-%S", &t);
		return 0;
	}
	return -1;
}

int
rm_util_dt_detail(char *buf)
{
	struct tm	t;
	struct timeval	tv;

	gettimeofday(&tv, NULL);

	// localtime_r is thread safe
	if (localtime_r(&tv.tv_sec, &t) != 0)
	{
		char tmp[20];
		strftime(tmp, 20, "%Y-%m-%d_%H-%M-%S", &t);
		tmp[19] = '\0';

		// append microseconds, buf must be at least 27 chars
		sprintf(buf, "%s:%06ld", tmp, tv.tv_usec);
		return 0;
	}
	return -1;
}

int
rm_util_daemonize(const char *dir, int noclose)
{
	pid_t	pid, sid;
	int 	fd;

	// background
	if ((pid = fork()) < 0)
	{
		// fork failed
		return -1;
	} else if (pid > 0)
	{
		// parent, terminate
		exit(EXIT_SUCCESS);
	}

	// 1st child

	// become session leader
	if ((sid = setsid()) < 0)
		return -2;

	// fork again, loose controlling terminal forever
	if ((pid = fork()) < 0)
	{
		// fork failed
		return -3;
	} else if (pid > 0)
	{
		// 1st child, terminate
		exit(EXIT_SUCCESS);
	}

	// TODO: handle signals
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	// set file mode to 0x662 (rw-rw-r--)
	// umask syscall always succeedes
	umask(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

	// TODO: open log here
	openlog("rsyncme", LOG_CONS | LOG_PID, LOG_DAEMON);

	// change dir
	if (dir != NULL)
		if (chdir(dir) == -1)
			return -4;

	if (noclose == 0)
	{
		// close open descriptors
		fd = sysconf(_SC_OPEN_MAX);
		for (; fd > 0; fd--)
		{
			close(fd);
		}
	}
	
	return 0;
}
