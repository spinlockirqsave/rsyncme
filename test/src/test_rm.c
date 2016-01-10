/// @file	test_rm.c
/// @author	peterg
/// @version	0.1.1
/// @date	10 Jan 2016 04:13 PM
/// @copyright	LGPLv2.1


#include "test_rm.h"


char	rm_f_100_name[RM_FILE_LEN_MAX] = "rm_f_100.dat";

int
test_rm_setup(void **state)
{
	int err, i;
	FILE *f;

	err = rm_util_chdir_umask_openlog(
		"../build", 1, "rsyncme_test");
	if (err != 0)
		exit(EXIT_FAILURE);
	*state = &rm_state;

	f = fopen(rm_f_100_name, "rb+");
	if (f == NULL)
	{
		// file doesn't exist, create
		RM_LOG_INFO("Creating file [%s]",
				rm_f_100_name);
		f = fopen("rm_100.dat", "wb");
		if (f == NULL)
			exit(EXIT_FAILURE);
		RM_LOG_INFO("Writing 100 random bytes"
			" to file [%s]", rm_f_100_name);
		srand(time(NULL));
		i = 100;
		while (i--)
		{
			fputc(rand() % 0x100, f);
		}
		
	} else {
		RM_LOG_INFO("Using previously created "
			"file [%s]", rm_f_100_name);
	}
	fclose(f);
	return 0;
}

int
test_rm_teardown(void **state)
{
	FILE *f;
	struct test_rm_state *rm_state;

	rm_state = *state;
	if (RM_TEST_DELETE_FILES == 1)
	{
		// delete all test files
		f = fopen(rm_f_100_name, "wb+");
		if (f == NULL)
		{
			RM_LOG_ERR("Can't open file [%s]",
				rm_f_100_name);	
		} else {
			RM_LOG_INFO("Removing file [%s]",
				rm_f_100_name);
			remove(rm_f_100_name);
		}
	}
	return 0;
}

void
test_rm_adler32_1(void **state)
{
	(void) state;

	int err;

	assert_true(1 == 1);
}
