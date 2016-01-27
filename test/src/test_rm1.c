/// @file	test_rm.c
/// @author	peterg
/// @version	0.1.2
/// @date	10 Jan 2016 04:13 PM
/// @copyright	LGPLv2.1


#include "test_rm1.h"


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
		"../build/debug", 1, "rsyncme_test_1");
#else
	err = rm_util_chdir_umask_openlog(
		"../build/release", 1, "rsyncme_test_1");
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

void
test_rm_adler32_1(void **state)
{
	FILE		*f;
	int		fd;
	uint32_t	sf;	// signature fast
	unsigned char	buf[RM_TEST_L_MAX];
	uint32_t	r1, r2, i, j, adler, file_sz, read;
	struct test_rm_state	*rm_state;
	struct stat	fs;
	char		*fname;

	rm_state = *state;
	assert_true(rm_state != NULL);

	// test on all files
	i = 0;
	for (; i < RM_TEST_FNAMES_N; ++i)
	{
		fname = rm_test_fnames[i];
		f = fopen(fname, "rb");
		if (f == NULL)
		{
			RM_LOG_PERR("Can't open file [%s]",
					fname);
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
		if (file_sz > RM_TEST_L_MAX)
		{
			RM_LOG_ERR("File [%s] size [%u] is bigger "
				"than testing buffer's size of [%u],"
				" reading only first [%u] bytes", fname,
				file_sz, RM_TEST_L_MAX, RM_TEST_L_MAX);
			file_sz = RM_TEST_L_MAX;
		}
		// read bytes
		read = fread(buf, 1, file_sz, f);
		if (read != file_sz)
		{
			RM_LOG_PERR("Error reading file [%s], "
				"skipping", fname);
			fclose(f);
			continue;
		}
		fclose(f);
		assert_true(read == file_sz);
		// calc checksum
		r1 = 1;
		r2 = 0;
		j = 0;
		for (; j < file_sz; ++j)
		{
			r1 = (r1 + buf[j]) % RM_ADLER32_MODULUS;
			r2 = (r2 + r1) % RM_ADLER32_MODULUS;
		}
		adler = (r2 << 16) | r1;
	
		sf = rm_adler32_1(buf, file_sz);
		assert_true(adler == sf);
		RM_LOG_INFO("PASS Adler32 (1) checksum [%u] OK, "
		"file [%s], size [%u]", adler, fname, file_sz);
	}
}

void
test_rm_adler32_2(void **state)
{
	FILE		*f;
	int		fd;
	unsigned char	buf[RM_TEST_L_MAX];
	uint32_t	i, adler1, adler2, adler2_0,
			file_sz, read, r1_0, r2_0, r1_1, r2_1;
	struct test_rm_state	*rm_state;
	struct stat	fs;
	char		*fname;

	rm_state = *state;
	assert_true(rm_state != NULL);

	// test on all files
	i = 0;
	for (; i < RM_TEST_FNAMES_N; ++i)
	{
		fname = rm_test_fnames[i];
		f = fopen(fname, "rb");
		if (f == NULL)
		{
			RM_LOG_PERR("Can't open file [%s]",
					fname);
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
		if (file_sz > RM_TEST_L_MAX)
		{
			RM_LOG_ERR("File [%s] size [%u] is bigger "
				"than testing buffer's size of [%u],"
				" reading only first [%u] bytes", fname,
				file_sz, RM_TEST_L_MAX, RM_TEST_L_MAX);
			file_sz = RM_TEST_L_MAX;
		}
		// read bytes
		read = fread(buf, 1, file_sz, f);
		if (read != file_sz)
		{
			RM_LOG_PERR("Error reading file [%s], "
				"skipping", fname);
			fclose(f);
			continue;
		}
		fclose(f);
		assert_true(read == file_sz);
		// checksum	
		adler1 = rm_adler32_1(buf, file_sz);
		adler2 = rm_adler32_2(1, buf, file_sz);
		assert_true(adler1 == adler2);
		adler2_0 = rm_adler32_2(0, buf, file_sz);
		// (1 + X) % MOD == (1 % MOD + X % MOD) % MOD
		r1_0 = adler2_0 & 0xffff;
		r1_1 = (r1_0 + (1 % RM_ADLER32_MODULUS)) % RM_ADLER32_MODULUS;
		assert_true((adler2 & 0xffff) == r1_1);
		r2_0 = (adler2_0 >> 16) & 0xffff;
		r2_1 = (r2_0 + (file_sz % RM_ADLER32_MODULUS)) % RM_ADLER32_MODULUS;
		assert_true(((adler2 >> 16) & 0xffff) == r2_1);
		RM_LOG_INFO("PASS Adler32 (2) checksum [%u] OK, i"
		"file [%s], size [%u]", adler2, fname, file_sz);
	}
}

void
test_rm_fast_check_roll(void **state)
{
	FILE		*f;
	int		fd;
	unsigned char	buf[RM_TEST_L_MAX];
	uint32_t	i, j, L, adler1, adler2, tests_n, tests_max,
			file_sz, read, read_left, read_now;
	long		idx_min, idx_max, idx, idx_buf;
	struct test_rm_state	*rm_state;
	struct stat	fs;
	char		*fname;

	rm_state = *state;
	assert_true(rm_state != NULL);

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
			RM_LOG_INFO("Validating testing of fast rolling "
				"checksum: file [%s], size [%u], block "
				"size L [%u], buffer [%u]", fname, file_sz,
				L, RM_TEST_L_MAX);
			if (file_sz < 2)
			{
				RM_LOG_INFO("File [%s] size [%u] is to small "
				"for this test, skipping", fname, file_sz);
				continue;
			}
			if (file_sz < L)
			{
				RM_LOG_INFO("File [%s] size [%u] is smaller "
				"than block size L [%u], skipping", fname,
				file_sz, L);
				continue;
			}
			if (RM_TEST_L_MAX < L + 1)
			{
				RM_LOG_INFO("Testing buffer [%u] is too "
				"small for this test (should be > [%u]), "
				" skipping file [%s]", RM_TEST_L_MAX, L,
				fname);
				continue;
			}
			
			RM_LOG_INFO("Tesing fast rolling checksum: file "
				"[%s], size [%u], block size L [%u], buffer"
				" [%u]", fname, file_sz, L, RM_TEST_L_MAX);
			// read bytes
			read = fread(buf, 1, L, f);
			if (read != L)
			{
				RM_LOG_PERR("Error reading file [%s], "
					"skipping", fname);
				continue;
			}
			assert_true(read == L);

			// initial checksum
			adler1 = rm_fast_check_block(buf, L);
			// move file pointer back
			fseek(f, 0, SEEK_SET);	// equivalent to rewind(f)

			// number of times rolling checksum will be calculated
			tests_max = file_sz - L;
			tests_n = 0;

			read_left = file_sz;
			do
			{
				idx_min = ftell(f);
				// read all up to buffer size
				read_now = rm_min(RM_TEST_L_MAX, read_left);
				read = fread(buf, 1, read_now, f);
				if (read != read_now)
				{
					RM_LOG_PERR("Error reading file [%s]",
						fname);
				}
				assert_true(read == read_now);
				idx_max = idx_min + read_now;	// idx_max is 1 past the end of buf
				// rolling checksum needs previous byte
				idx = idx_min + 1;
				// checksums at offsets [1,idx_max-L]
				while (idx < idx_max - L + 1)
				{
					// count tests
					++tests_n;
					RM_LOG_INFO("Running test [%u]", tests_n);
					idx_buf = idx - idx_min;
					adler2 = rm_fast_check_block(&buf[idx_buf], L);
					// checksum for offset [idx]
					adler1 = rm_fast_check_roll(adler1, buf[idx_buf-1],
								buf[idx_buf+L-1], L);
					assert_int_equal(adler1, adler2);
					++idx;
				}
				read_left -= read;
				if (read_left > 0)
				{
					// there are bytes remaining to read
					// we must start L bytes to the left from idx_max
					// to include a_k
					read_left += L;
					fseeko(f, -(long long)L, SEEK_CUR);
				}
			} while (read_left > 0);
			assert_int_equal(tests_n, tests_max);
			
			RM_LOG_INFO("PASSED [%u] fast rolling checksum tests, "
				"file [%s], size [%u], L [%u]", tests_n, fname,
				file_sz, L);
			// move file pointer back to the beginning
			rewind(f);
		}
		fclose(f);
	}
}

void
test_rm_rx_insert_nonoverlapping_ch_ch_1(void **state)
{
	FILE		*f;
	int		fd;
	uint32_t	i, j, L, file_sz, blocks_n;
	struct test_rm_state	*rm_state;
	struct stat	fs;
	char		*fname;
	long long int	entries_n;

	TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
	rm_state = *state;
	assert_true(rm_state != NULL);

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
			// number of blocks
			blocks_n = file_sz / L + (file_sz % L ? 1 : 0);
			entries_n = rm_rx_insert_nonoverlapping_ch_ch(
					f, fname, h, L, NULL);
			assert_int_equal(entries_n, blocks_n);
			
			RM_LOG_INFO("PASSED test of hashing of non-overlapping"
				" blocks, file [%s], size [%u], L [%u]", fname,
				file_sz, L);
			// move file pointer back to the beginning
			rewind(f);
		}
		fclose(f);
	}
}
