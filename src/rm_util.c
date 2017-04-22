/* @file        rm_util.c
 * @brief       Utilities.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        04 Jan 2016 08:08 PM
 * @copyright   LGPLv2.1 */


#include "rm_util.h"


int
rm_util_dt(char *buf) {
	struct tm	t;
	time_t		now = time(0);

	if (localtime_r(&now, &t) != 0) { /* localtime_r is thread safe (and therefore reentrant) */
		strftime(buf, 20, "%Y-%m-%d_%H-%M-%S", &t);
		return RM_ERR_OK;
	}
	return RM_ERR_FAIL;
}

int
rm_util_dt_detail(char *buf) {
	struct tm	t;
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	if (localtime_r(&tv.tv_sec, &t) != 0) { /* localtime_r is thread safe */
		char tmp[20];
		strftime(tmp, 20, "%Y-%m-%d_%H-%M-%S", &t);
		tmp[19] = '\0';
		sprintf(buf, "%s:%06ld", tmp, tv.tv_usec); /* append microseconds, buf must be at least 28 chars */
		return RM_ERR_OK;
	}
	return RM_ERR_FAIL;
}

/* rsyncme_2016-01-05_16-52-45:081207.log */
int
rm_util_openlogs(const char *dir, const char *name) {
	FILE	*stream;
	int	err;
	char	*full_path;

	if (dir == NULL) {
		return RM_ERR_BAD_CALL;
	}
	full_path = malloc(strlen(dir) + strlen(name) + 1 + 27 + 1 + 3 + 1);
	if (full_path == NULL) {
		return RM_ERR_MEM;
	}
	sprintf(full_path, "%s%s_", dir, name);
	if (rm_util_dt_detail(full_path + strlen(dir) + strlen(name) + 1) != RM_ERR_OK) {
		err = RM_ERR_GENERAL_ERROR;
		goto fail;
	}
	strcpy(full_path + strlen(dir) + strlen(name) + 27, ".log"); /* stdout log */
	if ((stream = fopen(full_path, "w+")) == NULL) {
		err = RM_ERR_FOPEN_STDOUT;
		goto fail;
	}
	fclose(stream);
	if (freopen(full_path, "w", stdout) == NULL) { /* redirect stdout */
		err = RM_ERR_FREOPEN_STDOUT;
		goto fail;
	}
	strcpy(full_path + strlen(dir) + strlen(name) + 27, ".err"); /* stderr log */
	if ((stream = fopen(full_path, "w+")) == NULL) {
		err = RM_ERR_FOPEN_STDERR;
		goto fail;
	}
	fclose(stream);
	if (freopen(full_path, "w", stderr) == NULL) { /* redirect stderr */
		err = RM_ERR_FREOPEN_STDERR;
		goto fail;
	}
	free(full_path);
	return RM_ERR_OK;

fail:
	if (full_path != NULL) {
		free(full_path);
	}
	return err;
}

int
rm_util_log(FILE *stream, const char *fmt, ...) {
	va_list	args;
	char 	buf[4096];
	char	dt[28];

	if (rm_util_dt_detail(dt) != RM_ERR_OK) {
		return RM_ERR_FAIL;
	}
	va_start(args, fmt);
	snprintf(buf, sizeof(buf), "%s\t%s\n", dt, fmt);
	vfprintf(stream, buf, args);
	va_end(args);
	fflush(stream);

	return RM_ERR_OK; 
}

int
rm_util_log_perr(FILE *stream, const char *fmt, ...) {
	va_list	args;
	char 	buf[4096];
	char	dt[28];

	if (rm_util_dt_detail(dt) != RM_ERR_OK) {
		return RM_ERR_FAIL;
	}
	va_start(args, fmt);
	snprintf(buf, sizeof(buf), "%s\t%s, %s\n", dt, fmt, strerror(errno));
	vfprintf(stream, buf, args);
	va_end(args);
	fflush(stream);

	return RM_ERR_OK; 
}

int
rm_util_daemonize(const char *dir, int noclose, const char *logname) {
	pid_t	pid, sid;
	int 	fd;

	if (logname == NULL) {
		return RM_ERR_BAD_CALL;
	}
	if ((pid = fork()) < 0) { /* background */
		return RM_ERR_FAIL;
	} else if (pid > 0) { /* parent, terminate */
		exit(EXIT_SUCCESS);
	}
	if ((sid = setsid()) < 0) { /* 1st child, become session leader */
		return RM_ERR_SETSID;
	}
	if ((pid = fork()) < 0) { /* fork again, loose controlling terminal forever */
		return RM_ERR_FORK;
	} else if (pid > 0) { /* 1st child, terminate */
		exit(EXIT_SUCCESS);
	}

	signal(SIGINT, SIG_IGN); /* TODO: handle signals */
	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	umask(S_IWGRP | S_IWOTH); /* set file mode to 0x622 (rw-r--r--), umask syscall always succeedes */
	if (rm_util_openlogs("./log/", logname) != RM_ERR_OK) {
		return RM_ERR_IO_ERROR;
	}
	if (dir != NULL) {/* change dir */
		if (chdir(dir) == -1) {
			return RM_ERR_CHDIR;
		}
	}
	if (noclose == 0) { /* close open descriptors */
		fd = sysconf(_SC_OPEN_MAX);
		for (; fd > 2; fd--) {
			close(fd);
		}
	}
	return RM_ERR_OK;
}

int
rm_util_chdir_umask_openlog(const char *dir, int noclose, const char *logname, uint8_t ignore_signals) {
	int 	fd;

	if (ignore_signals != 0) { /* TODO: handle signals */
		signal(SIGINT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
		signal(SIGCHLD, SIG_IGN);
	}
	umask(S_IWGRP | S_IWOTH); /* set file mode to 0x622 (rw-r--r--) umask syscall always succeedes */
	if (dir != NULL) { /* change dir */
		if (chdir(dir) == -1) {
			return RM_ERR_CHDIR;
		}
	}
	if ((logname != NULL) && (rm_util_openlogs("./log/", logname) != RM_ERR_OK)) {
		return RM_ERR_IO_ERROR;
	}
	if (noclose == 0) { /* close open descriptors */
		fd = sysconf(_SC_OPEN_MAX);
		for (; fd > 2; fd--) {
			close(fd);
		}
	}
	return RM_ERR_OK;
}
