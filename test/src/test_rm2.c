/// @file	test_rm.c
/// @author	peterg
/// @version	0.1.1
/// @date	10 Jan 2016 04:13 PM
/// @copyright	LGPLv2.1


#include "test_rm.h"


char*		rm_test_fnames[RM_TEST_FNAMES_N] = { "rm_f_0.dat", "rm_f_1.dat",
"rm_f_2.dat","rm_f_65.dat", "rm_f_100.dat", "rm_f_511.dat", "rm_f_512.dat",
"rm_f_513.dat", "rm_f_1023.dat", "rm_f_1024.dat", "rm_f_1025.dat",
"rm_f_4096.dat", "rm_f_20100.dat"};

uint32_t	rm_test_fsizes[RM_TEST_FNAMES_N] = { 0, 1, 2, 65, 100, 511, 512, 513,
						1023, 1024, 1025, 4096, 20100 };

uint32_t
rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE] = { 0, 1, 13, 50, 64, 100, 127, 128, 129,
					200, 400, 499, 500, 501, 511, 512, 513,
					600, 800, 1000, 1100, 1123, 1124, 1125,
					1200, 100000 };

int
test_rm_setup(void **state)
{
	int 		err;
	uint32_t	i,j;
	FILE 		*f;

#ifdef DEBUG
	err = rm_util_chdir_umask_openlog(
		"../build/debug", 1, "rsyncme_test");
#else
	err = rm_util_chdir_umask_openlog(
		"../build/release", 1, "rsyncme_test");
#endif
	if (err != 0)
		exit(EXIT_FAILURE);
	rm_state.l = rm_test_L_blocks;
	*state = &rm_state;

	i = 0;
	for (; i < RM_TEST_FNAMES_N; ++i)
	{
		f = fopen(rm_test_fnames[i], "rb+");
		if (f == NULL)
		{
			// file doesn't exist, create
			RM_LOG_INFO("Creating file [%s]",
					rm_test_fnames[i]);
			f = fopen(rm_test_fnames[i], "wb");
			if (f == NULL)
				exit(EXIT_FAILURE);
			j = rm_test_fsizes[i];
			RM_LOG_INFO("Writing [%u] random bytes"
			" to file [%s]", j, rm_test_fnames[i]);
			srand(time(NULL));
			while (j--)
			{
				fputc(rand(), f);
			}		
		} else {
			RM_LOG_INFO("Using previously created "
				"file [%s]", rm_test_fnames[i]);
		}
		fclose(f);
	}
	return 0;
}

int
test_rm_teardown(void **state)
{
	int	i;
	FILE	*f;
	struct test_rm_state *rm_state;

	rm_state = *state;
	assert_true(rm_state != NULL);
	if (RM_TEST_DELETE_FILES == 1)
	{
		// delete all test files
		i = 0;
		for (; i < RM_TEST_FNAMES_N; ++i)
		{
			f = fopen(rm_test_fnames[i], "wb+");
			if (f == NULL)
			{
				RM_LOG_ERR("Can't open file [%s]",
					rm_test_fnames[i]);	
			} else {
				RM_LOG_INFO("Removing file [%s]",
					rm_test_fnames[i]);
				remove(rm_test_fnames[i]);
			}
		}
	}
	return 0;
}

int
__wrap_fread(void)
{
	return 0;
}

void
test_rm_rx_insert_nonoverlapping_ch_ch_2(void **state)
{
	FILE		*f;
	int		fd;
	uint32_t	i, j, L, file_sz;
	struct test_rm_state	*rm_state;
	struct stat	fs;
	char		*fname;
	long long int	res, res_expected;

        TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
	rm_state = *state;
	assert_true(rm_state != NULL);

	// test failed call to fstat
	res_expected = -1;
	// test on all files
	i = 0;
	for (; i < RM_TEST_FNAMES_N; ++i)
	{
		fname = rm_test_fnames[i];
		f = fopen(fname, "rb");
		if (f == NULL)
		{
			RM_LOG_PERR("Can't open file [%s]", fname);
		}
		assert_true(f != NULL);
		// get file size
		fd = fileno(f);
		if (fstat(fd, &fs) != 0)
		{
			RM_LOG_PERR("Can't fstat file [%s]", fname);
			fclose(f);
			assert_true(1 == 0);
		}
		file_sz = fs.st_size; 
		j = 0;
		for (; j < RM_TEST_L_BLOCKS_SIZE; ++j)
		{
			L = rm_test_L_blocks[j];
			RM_LOG_INFO("Validating testing of hashing of non-"
				"overlapping blocks: file [%s], size [%u],"
				" block size L [%u]", fname, file_sz, L);
			if (0 == L)
			{
				RM_LOG_INFO("Block size [%u] is too "
				"small for this test (should be > [%u]), "
				" skipping file [%s]", L, 0, fname);
				continue;
			}
			if (file_sz < 2)
			{
				RM_LOG_INFO("File [%s] size [%u] is to small "
				"for this test, skipping", fname, file_sz);
				continue;
			}
	
			RM_LOG_INFO("Tesing fast rolling checksum: file "
				"[%s], size [%u], block size L [%u], buffer"
				" [%u]", fname, file_sz, L, RM_TEST_L_MAX);
			res = rm_rx_insert_nonoverlapping_ch_ch(
					f, fname, h, L, NULL);
			assert_int_equal(res, res_expected);
			
			RM_LOG_INFO("PASSED test of error reporting correctness"
				" of hashing of non-overlapping blocks");
			// move file pointer back to the beginning
			rewind(f);
		}
		fclose(f);
	}
}
