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
	// (and therefore reentrant)
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
