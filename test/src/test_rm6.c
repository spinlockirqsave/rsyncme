/*
 * @file        test_rm6.c
 * @brief       Test suite #6.
 * @details     Test of checksums on tail calculation correctness.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        13 May 2016 01:52 PM
 * @copyright   LGPLv2.1
 */


#include "test_rm6.h"


const char* rm_test_fnames[RM_TEST_FNAMES_N] = { "rm_f_0.dat", "rm_f_1.dat",
"rm_f_2.dat","rm_f_65.dat", "rm_f_100.dat", "rm_f_511.dat", "rm_f_512.dat",
"rm_f_513.dat", "rm_f_1023.dat", "rm_f_1024.dat", "rm_f_1025.dat",
"rm_f_4096.dat", "rm_f_20100.dat"};

uint32_t	rm_test_fsizes[RM_TEST_FNAMES_N] = { 0, 1, 2, 65, 100, 511, 512, 513,
						1023, 1024, 1025, 4096, 20100 };

uint32_t
rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE] = { 0, 1, 2, 10, 13, 50, 64, 100, 127, 128, 129,
					200, 400, 499, 500, 501, 511, 512, 513,
					600, 800, 1000, 1100, 1123, 1124, 1125,
					1200, 100000 };

int
test_rm_setup(void **state)
{
	int 		err;
	uint32_t	i, j;
	FILE 		*f;

#ifdef DEBUG
	err = rm_util_chdir_umask_openlog(
		"../build/debug", 1, "rsyncme_test_6");
#else
	err = rm_util_chdir_umask_openlog(
		"../build/release", 1, "rsyncme_test_6");
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
			/* file doesn't exist, create */
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

	/* find biggest L */
	i = 0;
	j = 0;
	for (; i < RM_TEST_L_BLOCKS_SIZE; ++i)
		if (rm_test_L_blocks[i] > j) j = rm_test_L_blocks[i];

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
		/* delete all test files */
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

void
test_rm_fast_check_roll_tail(void **state)
{
	FILE                    *f;
	int                     fd;
	unsigned char	        buf[RM_TEST_L_MAX], a_k;
    uint32_t                i, j, L, adler1, adler2, tests_n, tests_max;
    size_t                  file_sz, read, read_left, read_now, a_k_pos;
    struct test_rm_state    *rm_state;
	struct stat             fs;
	const char              *fname;

	rm_state = *state;
	assert_true(rm_state != NULL);

	/* test on all files */
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
		/* get file size */
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
			RM_LOG_INFO("Validating testing of fast rolling "
				"checksum: file [%s], size [%u], block "
				"size L [%u], buffer [%u]", fname, file_sz,
				L, RM_TEST_L_MAX);
			if (file_sz < 2)
			{
				RM_LOG_WARN("File [%s] size [%u] is to small "
				"for this test (shoud be > 1), skipping", fname, file_sz);
				continue;
			}
/*			if (file_sz < L)
			{
				RM_LOG_WARN("File [%s] size [%u] is smaller "
				"than block size L [%u], skipping", fname,
				file_sz, L);
				continue;
			}*/
			if (RM_TEST_L_MAX < L + 1)
			{
				RM_LOG_WARN("Testing buffer [%u] is too "
				"small for this test (should be > [%u]), "
				" skipping file [%s]", RM_TEST_L_MAX, L,
				fname);
				continue;
			}
			if (L < 2)
			{
				RM_LOG_WARN("Block size L [%u] is too "
				"small for this test (should be > 1), "
				" skipping file [%s]", L, fname);
				continue;
			}

            RM_LOG_INFO("Testing fast rolling checksum on tail: file "
                    "[%s], size [%u], block size L [%u]", fname, file_sz, L);

            /* read last bytes from file, up to L or file_sz */
            read_now = rm_min(L, file_sz);
            a_k_pos = file_sz - read_now;
            read = rm_fpread(buf, sizeof(unsigned char), read_now, file_sz - read_now, f);
            if (read != read_now)
            {
                if (feof(f))
                {
                    RM_LOG_PERR("Error reading file [%s], EOF", fname);
                    assert_true(1 == 0);
                }
                if (ferror(f))
                {
                    RM_LOG_PERR("Error reading file [%s], "
                            "error other than EOF", fname);
                    assert_true(1 == 0);
                }
                RM_LOG_PERR("Error reading file [%s], skipping", fname);
                continue;
            }
            assert_true(read == read_now);

            /* initial checksum */
            adler1 = rm_fast_check_block(buf, read);
            adler2 = adler1;    /* tail checksum */

            /* number of times rolling checksum on tail will be calculated */
            tests_max = read - 1;
            tests_n = 0;

            read_left = read - 1;
            while (read_left > 0)
            {
                /* count tests */
                ++tests_n;

                /* read last bytes from file, up to L or file_sz */
                read_now = read_left;
                read = rm_fpread(buf, sizeof(unsigned char), read_now, file_sz - read_now, f);
                if (read != read_now)
                {
                    RM_LOG_PERR("Error reading file [%s], skipping", fname);
                    continue;
                }
                assert_true(read == read_now);
                
                /* calc checksums */
                adler1 = rm_fast_check_block(buf, read);
                /* tail checksum */
                if (rm_fpread(&a_k, sizeof(unsigned char), 1, a_k_pos, f) != 1) { /* rm_fpread returns the number of successfully read blocks, in this case it doesn't really matter */
                    RM_LOG_PERR("Error reading file [%s], skipping", fname);
                    continue;
                }
                adler2 = rm_fast_check_roll_tail(adler2, a_k, read + 1); /* current adler2 was calculated on read + 1 bytes, the a_k * (read + 1) will be subtructed from r2 */
                assert_int_equal(adler1, adler2);

                --read_left;
                ++a_k_pos;
            }
            assert_int_equal(tests_n, tests_max);
			
            RM_LOG_INFO("PASSED [%u] fast rolling checksum on tail tests, "
                    "file [%s], size [%u], L [%u]", tests_n, fname, file_sz, L);
        }
        fclose(f);
    }
}

