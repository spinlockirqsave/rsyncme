/// @file	rm_util.c
/// @brief	Utilities.
/// @author	peterg
/// @version	0.1.2
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

		// append microseconds, buf must be at least
		// 28 chars
		sprintf(buf, "%s:%06ld", tmp, tv.tv_usec);
		return 0;
	}
	return -1;
}

// rsyncme_2016-01-05_16-52-45:081207.log
int
rm_util_openlogs(const char *dir, const char *name)
{
	FILE	*stream;
	int	err;
	char	*full_path;

	if (dir == NULL) return -1;
	full_path = malloc(strlen(dir) + strlen(name)
			+ 1 + 27 + 1 + 3 + 1);
	if (full_path == NULL)
		return -1;

	sprintf(full_path, "%s%s_", dir, name);
	if (rm_util_dt_detail(full_path + strlen(dir)
			+ strlen(name) + 1) == -1)
	{
		err = -2;
		goto fail;
	}
	// stdout log
	strcpy(full_path + strlen(dir) + strlen(name)
		+ 27, ".log");
	if ((stream = fopen(full_path, "w+")) == NULL)
	{
		err = -3;
		goto fail;
	}
	fclose(stream);
	// redirect stdout
	if (freopen(full_path, "w", stdout) == NULL)
	{
		perror("rsyncme");
		err = -4;
		goto fail;
	}
	// stderr log
	strcpy(full_path + strlen(dir) + strlen(name)
		+ 27, ".err");
	if ((stream = fopen(full_path, "w+")) == NULL)
	{
		err = -5;
		goto fail;
	}
	fclose(stream);
	// redirect stderr
	if (freopen(full_path, "w", stderr) == NULL)
	{
		err = -6;
		goto fail;
	}
	return 0;
fail:
	if (full_path != NULL)
		free(full_path);
	return err;
}

int
rm_util_log(FILE *stream, const char *fmt, ...)
{
	int	ret;
	va_list	args;
	char 	buf[4096];
	char	dt[28];

	ret = rm_util_dt_detail(dt);
	if (ret == -1) return -1;
	
	va_start(args, fmt);
	snprintf(buf, sizeof(buf), "%s\t%s\n", dt, fmt);
	ret = vfprintf(stream, buf, args);
	va_end(args);
	fflush(stream);

	return ret; 
}

int
rm_util_log_perr(FILE *stream, const char *fmt, ...)
{
	int	ret;
	va_list	args;
	char 	buf[4096];
	char	dt[28];

	ret = rm_util_dt_detail(dt);
	if (ret == -1) return -1;
	
	va_start(args, fmt);
	snprintf(buf, sizeof(buf), "%s\t%s, %s\n",
			dt, fmt, strerror(errno));
	ret = vfprintf(stream, buf, args);
	va_end(args);
	fflush(stream);

	return ret; 
}

int
rm_util_daemonize(const char *dir, int noclose,
				char *logname)
{
	pid_t	pid, sid;
	int 	fd;

	assert(logname != NULL);

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

	// set file mode to 0x622 (rw-r--r--)
	// umask syscall always succeedes
	umask(S_IWGRP | S_IWOTH);

	// TODO: open log here
	//openlog("rsyncme", LOG_CONS | LOG_PID, LOG_DAEMON);
	fd = rm_util_openlogs("./log/", logname);
	if (fd < 0)
		return -4;

	// change dir
	if (dir != NULL)
		if (chdir(dir) == -1)
			return -5;

	if (noclose == 0)
	{
		// close open descriptors
		fd = sysconf(_SC_OPEN_MAX);
		for (; fd > 2; fd--)
		{
			close(fd);
		}
	}
	
	return 0;
}

int
rm_util_chdir_umask_openlog(const char *dir,
		int noclose, char *logname)
{
	int 	fd;

	assert(logname != NULL);

	// TODO: handle signals
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	// set file mode to 0x622 (rw-r--r--)
	// umask syscall always succeedes
	umask(S_IWGRP | S_IWOTH);

	// change dir
	if (dir != NULL)
		if (chdir(dir) == -1)
			return -1;

	// TODO: open log here
	//openlog("rsyncme", LOG_CONS | LOG_PID, LOG_DAEMON);
	fd = rm_util_openlogs("./log/", logname);
	if (fd < 0)
		return -2;

	if (noclose == 0)
	{
		// close open descriptors
		fd = sysconf(_SC_OPEN_MAX);
		for (; fd > 2; fd--)
		{
			close(fd);
		}
	}
	return 0;
}
