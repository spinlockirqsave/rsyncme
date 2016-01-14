/// @file	test_rm.c
/// @author	peterg
/// @version	0.1.1
/// @date	10 Jan 2016 04:13 PM
/// @copyright	LGPLv2.1


#include "test_rm.h"


char*		rm_test_fnames[RM_TEST_FNAMES_N] = { "rm_f_0.dat", "rm_f_1.dat",
"rm_f_100.dat", "rm_f_511.dat", "rm_f_512.dat", "rm_f_513.dat", "rm_f_1023.dat",
"rm_f_1024.dat", "rm_f_1025.dat" };

uint32_t	rm_test_fsizes[RM_TEST_FNAMES_N] = { 0, 1, 100, 511, 512, 513,
							1023, 1024, 1025 };

uint32_t
rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE] = { 1, 13, 50, 64, 100, 127, 128, 129,
					200, 400, 499, 500, 501, 511, 512, 513,
					600, 800, 1000, 1100, 1123, 1124, 1125,
					1200 };

int
test_rm_setup(void **state)
{
	int 		err;
	uint32_t	i,j;
	FILE 		*f;

	err = rm_util_chdir_umask_openlog(
		"../build", 1, "rsyncme_test");
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
				fputc(rand() % 0x100, f);
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
			RM_LOG_ERR("File [%s] size [%u] is bigger ",
				"than testing buffer's size of [%u],",
				" reading only first [%u] bytes", fname,
				file_sz, RM_TEST_L_MAX, RM_TEST_L_MAX);
			file_sz = RM_TEST_L_MAX;
		}
		// read bytes
		read = fread(buf, 1, file_sz, f);
		if (read != file_sz)
		{
			RM_LOG_PERR("Error reading file [%s], ",
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
			RM_LOG_ERR("File [%s] size [%u] is bigger ",
				"than testing buffer's size of [%u],",
				" reading only first [%u] bytes", fname,
				file_sz, RM_TEST_L_MAX, RM_TEST_L_MAX);
			file_sz = RM_TEST_L_MAX;
		}
		// read bytes
		read = fread(buf, 1, file_sz, f);
		if (read != file_sz)
		{
			RM_LOG_PERR("Error reading file [%s], ",
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
test_rm_adler32_roll(void **state)
{
	FILE		*f;
	int		fd;
	unsigned char	buf[RM_TEST_L_MAX];
	uint32_t	i, j, L, adler, k, k_max,
			file_sz, read, read_left, read_now;
	long		idx_min, idx_max, idx;
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
			if (file_sz < L)
			{
				RM_LOG_INFO("File [%s] size [%u] is smaller "
				"than block size L [%u], skipping", fname,
				file_sz, L);
				continue;
			}
			if (RM_TEST_L_MAX < L)
			{
				RM_LOG_INFO("Block size L [%u] is bigger than "
				" testing buffer [%u], skipping file [%s]",
				L, RM_TEST_L_MAX, fname);
				continue;
			}
			
			RM_LOG_INFO("Tesing Adler32 rolling checksum, "
				"file [%s], size [%u], block size L [%u]",
				fname, file_sz, L);
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
			adler = rm_adler32_2(1, buf, L);
			// move file pointer back
			fseek(f, 0, SEEK_SET);	// equivalent to rewind(f)

			// number of times rolling will be called
			k_max = file_sz - L;

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
					// checksum for offset [idx]
					adler = rm_adler32_roll(adler, buf[idx-1],
								buf[idx+L-1], L);
					++idx;
				}
				read_left -= read;
				if (read_left > 0)
				{
					// there are bytes remaining to read
					// we must start L bytes to the left from idx_max
					// to include a_k
					read_left += L;
					fseek(f, -L, SEEK_CUR);
				}
			} while (read_left > 0);
			
			RM_LOG_INFO("PASS Adler32 rolling checksum [%u] OK, "
				"file [%s], size [%u], L [%u]", adler, fname,
				file_sz, L);
			// move file pointer back to the beginning
			rewind(f);
		}
		fclose(f);
	}
}
