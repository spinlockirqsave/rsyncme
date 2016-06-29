/* @file        test_rm1.c
 * @brief       Test suite #1.
 * @details     Test of basic routines and rolling checksums and nonoverlapping
 *              checksums calculation correctness.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        10 Jan 2016 04:13 PM
 * @copyright   LGPLv2.1 */


#include "test_rm1.h"


const char* rm_test_fnames[RM_TEST_FNAMES_N] = { "rm_f_0_ts1", "rm_f_1_ts1",
"rm_f_2_ts1","rm_f_65_ts1", "rm_f_100_ts1", "rm_f_511_ts1", "rm_f_512_ts1",
"rm_f_513_ts1", "rm_f_1023_ts1", "rm_f_1024_ts1", "rm_f_1025_ts1",
"rm_f_4096_ts1", "rm_f_20100_ts1"};

size_t	rm_test_fsizes[RM_TEST_FNAMES_N] = { 0, 1, 2, 65, 100, 511, 512, 513,
						1023, 1024, 1025, 4096, 20100 };

size_t
rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE] = { 0, 1, 13, 50, 64, 100, 127, 128, 129,
					200, 400, 499, 500, 501, 511, 512, 513,
					600, 800, 1000, 1100, 1123, 1124, 1125,
					1200, 100000 };

int
test_rm_setup(void **state) {
    int             err;
    size_t          i, j;
    FILE            *f;
    struct rm_ch_ch *array;
    unsigned long const seed = time(NULL);

#ifdef DEBUG
    err = rm_util_chdir_umask_openlog("../build/debug", 1, "rsyncme_test_1");
#else
    err = rm_util_chdir_umask_openlog("../build/release", 1, "rsyncme_test_1");
#endif
    if (err != 0) {
        exit(EXIT_FAILURE);
    }
    rm_state.l = rm_test_L_blocks;
    *state = &rm_state;

    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f = fopen(rm_test_fnames[i], "rb+");
        if (f == NULL) { /* file doesn't exist, create */
            RM_LOG_INFO("Creating file [%s]", rm_test_fnames[i]);
            f = fopen(rm_test_fnames[i], "wb");
            if (f == NULL) {
                exit(EXIT_FAILURE);
            }
            j = rm_test_fsizes[i];
            RM_LOG_INFO("Writing [%zu] random bytes to file [%s]", j, rm_test_fnames[i]);
            srand(seed);
            while (j--) {
                fputc(rand(), f);
            }		
        } else {
            RM_LOG_INFO("Using previously created file [%s]", rm_test_fnames[i]);
        }
        fclose(f);
    }

    i = 0; /* find biggest L */
    j = 0;
    for (; i < RM_TEST_L_BLOCKS_SIZE; ++i) {
        if (rm_test_L_blocks[i] > j) {
            j = rm_test_L_blocks[i];
        }
    }

    array = malloc(j * sizeof (*array)); /* allocate array */
    if (array == NULL) {
        RM_LOG_ERR("Can't allocate memory for array buffer of [%zu] bytes, malloc failed", j * sizeof(*array));
    }
    assert_true(array != NULL);
    rm_state.array_entries = j;
    rm_state.array = array;

    memcpy(file_content_payload, "abcdefghij", RM_TEST_1_2_BUF_SZ);
    strcpy(rm_state.f_2.name, "f_2");
    rm_state.f_2.f = fopen(rm_state.f_2.name, "w+b");
    if (rm_state.f_2.f == NULL) {
        RM_LOG_ERR("Can't create test file [%s]", rm_state.f_2.name);
    }
    if (1 != rm_fpwrite(file_content_payload, RM_TEST_1_2_BUF_SZ, 1, 0, rm_state.f_2.f)) {
        RM_LOG_ERR("Error writing to the test file [%s]", rm_state.f_2.name);
        assert_true(1 == 0 && "Error writing to the test file");
    }
    fclose(rm_state.f_2.f);
    rm_state.f_2.f = NULL;

    return 0;
}

int
test_rm_teardown(void **state) {
	size_t  i;
	FILE	*f;
	struct test_rm_state *rm_state;

	rm_state = *state;
	assert_true(rm_state != NULL);
	if (RM_TEST_DELETE_FILES == 1) { /* delete all test files */
		i = 0;
		for (; i < RM_TEST_FNAMES_N; ++i) {
			f = fopen(rm_test_fnames[i], "wb+");
			if (f == NULL) {
				RM_LOG_ERR("Can't open file [%s]",
					rm_test_fnames[i]);	
			} else {
                fclose(f);
				RM_LOG_INFO("Removing file [%s]", rm_test_fnames[i]);
				remove(rm_test_fnames[i]);
			}
		}
	}
    if (rm_state->array != NULL) {
        free(rm_state->array);
    }
	return 0;
}

void
test_rm_copy_buffered(void **state) {
    FILE        *f_x, *f_y;
    int         fd, err;
	struct stat fs;
	const char  *fname, *f_y_name = "rm_test_1_1_tmp";
    size_t      i, file_sz, k;
    unsigned char cx, cy;

    (void) state;

	i = 0; /* test on all files */
	for (; i < RM_TEST_FNAMES_N; ++i) {
		fname = rm_test_fnames[i];
		f_x = fopen(fname, "rb");
		if (f_x == NULL) {
			RM_LOG_PERR("Can't open file [%s]", fname);
		}
		assert_true(f_x != NULL && "Can't open @x file");
		fd = fileno(f_x);
		if (fstat(fd, &fs) != 0) {
			RM_LOG_PERR("Can't fstat file [%s]", fname);
			if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
			assert_true(1 == 0 && "Can't fstat @x");
		}
		file_sz = fs.st_size;
		f_y = fopen(f_y_name, "w+b");
		if (f_y == NULL) {
			RM_LOG_PERR("%s", "Can't open @y file");
            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
		}
		assert_true(f_y != NULL && "Can't open @y file");
        err = rm_copy_buffered(f_x, f_y, file_sz);
        if (err != 0) {
            RM_LOG_ERR("Copy buffered failed with error [%d], file [%s]", err, fname);
            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
            if (f_y != NULL) {
                fclose(f_y);
                f_y = NULL;
            }
        }
        assert(err == 0 && "Copy buffered failed");
        k = 0; /* verify files are the same */
        while (k < file_sz) {
            if (rm_fpread(&cx, sizeof(unsigned char), 1, k, f_x) != 1) {
                RM_LOG_CRIT("Error reading file [%s]!", fname);
                if (f_x != NULL) {
                    fclose(f_x);
                    f_x = NULL;
                }
                if (f_y != NULL) {
                    fclose(f_y);
                    f_y = NULL;
                }
                assert_true(1 == 0 && "ERROR reading byte in file @x!");
            }
            if (rm_fpread(&cy, sizeof(unsigned char), 1, k, f_y) != 1) {
                RM_LOG_CRIT("Error reading file [%s]!", f_y_name);
                if (f_x != NULL) {
                    fclose(f_x);
                    f_x = NULL;
                }
                if (f_y != NULL) {
                    fclose(f_y);
                    f_y = NULL;
                }
                assert_true(1 == 0 && "ERROR reading byte in file @y!");
            }
            if (cx != cy) {
                RM_LOG_CRIT("Bytes [%zu] differ: cx [%zu], cy [%u]\n", k, cx, cy);
            }
            assert_true(cx == cy && "Bytes differ!");
            ++k;
        }
        if (f_x != NULL) {
            fclose(f_x);
            f_x = NULL;
        }
        if (f_y != NULL) {
            fclose(f_y);
            f_y = NULL;
        }
        RM_LOG_INFO("PASSED test #1 (copy buffered), file [%s]", fname);
    }
    RM_LOG_INFO("%s", "PASSED test #1 (copy buffered)");
}

void
test_rm_copy_buffered_2_1(void **state) {
    FILE        *f_x;
    int         fd, err;
	struct stat fs;
	const char  *fname;
    size_t      i, file_sz, bytes_requested, k;
    unsigned char cx, buf[RM_TEST_1_2_BUF_SZ];

    (void) state;

	i = 0; /* test on all files */
	for (; i < RM_TEST_FNAMES_N; ++i) {
		fname = rm_test_fnames[i];
		f_x = fopen(fname, "rb");
		if (f_x == NULL) {
			RM_LOG_PERR("Can't open file [%s]", fname);
		}
		assert_true(f_x != NULL && "Can't open @x file");
		fd = fileno(f_x);
		if (fstat(fd, &fs) != 0) {
			RM_LOG_PERR("Can't fstat file [%s]", fname);
			if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
			assert_true(1 == 0 && "Can't fstat @x");
		}
		file_sz = fs.st_size;
        bytes_requested = rm_min(RM_TEST_1_2_BUF_SZ, file_sz);
        err = rm_copy_buffered_2(f_x, 0, buf, bytes_requested);
        if (err != 0) {
            RM_LOG_ERR("Copy buffered failed with error [%d], file [%s]", err, fname);
            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
        }
        assert(err == 0 && "Copy buffered failed");
        k = 0; /* verify files are the same */
        while (k < bytes_requested) {
            if (rm_fpread(&cx, sizeof(unsigned char), 1, k, f_x) != 1) {
                RM_LOG_CRIT("Error reading file [%s]!", fname);
                if (f_x != NULL) {
                    fclose(f_x);
                    f_x = NULL;
                }
                assert_true(1 == 0 && "ERROR reading byte in file @x!");
            }
            if (cx != buf[k]) {
                RM_LOG_CRIT("Bytes [%zu] differ: cx [%zu], buf [%u]\n", k, cx, buf[k]);
            }
            assert_true(cx == buf[k] && "Bytes differ!");
            ++k;
        }
        if (f_x != NULL) {
            fclose(f_x);
            f_x = NULL;
        }
        RM_LOG_INFO("PASSED test #2 (copy buffered2 test #1), file [%s]", fname);
    }
    RM_LOG_INFO("%s", "PASSED test #2 (copy buffered2 test #1)");
}

static int
test_rm_copy_buffered_bytes_cmp(unsigned char buf[], size_t n, size_t offset) {
    size_t i;

    i = 0; /* verify bytes returned */
    while (i < n) {
        if (buf[i] != file_content_payload[i + offset]) {
            RM_LOG_ERR("Wrong bytes returned at index [%zu] offset [%zu]!", i, offset);
            return -1;
        }
        ++i;
    }
    return 0;
}

void
test_rm_copy_buffered_2_2(void **state) {
    FILE                    *f;
    struct test_rm_state    *rm_state;
    int                     err;
	const char              *fname;
    size_t                  bytes_requested, offset;
    unsigned char           buf[RM_TEST_1_2_BUF_SZ];

    rm_state = *state;
    assert_true(rm_state != NULL && "Omg, cmocka failed?");

    fname = rm_state->f_2.name;
    f = fopen(fname, "rb");
    if (f == NULL) {
        RM_LOG_PERR("Can't open test file [%s]", fname);
    }
    assert_true(f != NULL && "Can't open test file");
    /* 1 request zero bytes */
    bytes_requested = 0;
    offset = 0;
    err = rm_copy_buffered_2(f, offset, buf, bytes_requested);
    if (err != 0) {
        RM_LOG_ERR("Copy buffered failed with error [%d], file [%s]", err, fname);
        if (f != NULL) {
            fclose(f);
            f = NULL;
        }
    }
    assert(err == 0 && "Copy buffered failed");
    /* 2 request 1 byte, first byte */
    bytes_requested = 1;
    offset = 0;
    err = rm_copy_buffered_2(f, offset, buf, bytes_requested);
    if (err != 0) {
        RM_LOG_ERR("Copy buffered failed with error [%d], file [%s]", err, fname);
        if (f != NULL) {
            fclose(f);
            f = NULL;
        }
    }
    assert(err == 0 && "Copy buffered failed");
    if (0 != test_rm_copy_buffered_bytes_cmp(buf, bytes_requested, offset)) { /* verify bytes returned */
            RM_LOG_ERR("%s", "Wrong bytes returned!");
            assert_true(1 == 0 && "Wrong bytes returned!");
    }
    /* 3 request all possible segments */
    offset = 0;
    for (; offset < RM_TEST_1_2_BUF_SZ; ++offset) {
        bytes_requested = 0;
        do {
            err = rm_copy_buffered_2(f, offset, buf, bytes_requested);
            if (err != 0) {
                RM_LOG_ERR("Copy buffered failed with error [%d], file [%s]", err, fname);
                if (f != NULL) {
                    fclose(f);
                    f = NULL;
                }
            }
            assert(err == 0 && "Copy buffered failed");
            if (0 != test_rm_copy_buffered_bytes_cmp(buf, bytes_requested, offset)) { /* verify bytes returned */
                RM_LOG_ERR("%s", "Wrong bytes returned!");
                assert_true(1 == 0 && "Wrong bytes returned!");
            }
            ++bytes_requested;
        } while (bytes_requested + offset < RM_TEST_1_2_BUF_SZ + 1);
    }
    /* 4 request too much */
    bytes_requested = RM_TEST_1_2_BUF_SZ + 1;
    offset = 0;
    err = rm_copy_buffered_2(f, offset, buf, bytes_requested);
    if (err != -2) {
        RM_LOG_ERR("Copy buffered failed with WRONG error [%d], file [%s]", err, fname);
        if (f != NULL) {
            fclose(f);
            f = NULL;
        }
    }
    assert(err == -2 && "Copy buffered failed with WRONG error");
    if (f != NULL) {
        fclose(f);
        f = NULL;
    }

    RM_LOG_INFO("%s", "PASSED test #3 (copy buffered2 test #2)");
}

void
test_rm_copy_buffered_offset(void **state) {
    FILE        *f_x, *f_y;
    int         fd, err;
	struct stat fs;
	const char  *fname, *f_y_name = "rm_test_1_4_tmp";
    size_t      i, file_sz, k, offset_x, offset_y;
    unsigned char cx, cy;

    (void) state;

	i = 0; /* test on all files */
	for (; i < RM_TEST_FNAMES_N; ++i) {
		fname = rm_test_fnames[i];
		f_x = fopen(fname, "rb");
		if (f_x == NULL) {
			RM_LOG_PERR("Can't open file [%s]", fname);
		}
		assert_true(f_x != NULL && "Can't open @x file");
		fd = fileno(f_x);
		if (fstat(fd, &fs) != 0) {
			RM_LOG_PERR("Can't fstat file [%s]", fname);
			if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
			assert_true(1 == 0 && "Can't fstat @x");
		}
		file_sz = fs.st_size;
		f_y = fopen(f_y_name, "w+b");
		if (f_y == NULL) {
			RM_LOG_PERR("%s", "Can't open @y file");
            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
		}
		assert_true(f_y != NULL && "Can't open @y file");
        offset_x = 0;
        offset_y = 0;
        err = rm_copy_buffered_offset(f_x, f_y, file_sz, offset_x, offset_y);
        if (err != 0) {
            RM_LOG_ERR("Copy buffered failed with error [%d], file [%s]", err, fname);
            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
            if (f_y != NULL) {
                fclose(f_y);
                f_y = NULL;
            }
        }
        assert(err == 0 && "Copy buffered failed");
        k = 0; /* verify files are the same */
        while (k < file_sz) {
            if (rm_fpread(&cx, sizeof(unsigned char), 1, k, f_x) != 1) {
                RM_LOG_CRIT("Error reading file [%s]!", fname);
                if (f_x != NULL) {
                    fclose(f_x);
                    f_x = NULL;
                }
                if (f_y != NULL) {
                    fclose(f_y);
                    f_y = NULL;
                }
                assert_true(1 == 0 && "ERROR reading byte in file @x!");
            }
            if (rm_fpread(&cy, sizeof(unsigned char), 1, k, f_y) != 1) {
                RM_LOG_CRIT("Error reading file [%s]!", f_y_name);
                if (f_x != NULL) {
                    fclose(f_x);
                    f_x = NULL;
                }
                if (f_y != NULL) {
                    fclose(f_y);
                    f_y = NULL;
                }
                assert_true(1 == 0 && "ERROR reading byte in file @y!");
            }
            if (cx != cy) {
                RM_LOG_CRIT("Bytes [%zu] differ: cx [%zu], cy [%u]\n", k, cx, cy);
            }
            assert_true(cx == cy && "Bytes differ!");
            ++k;
        }
        if (f_x != NULL) {
            fclose(f_x);
            f_x = NULL;
        }
        if (f_y != NULL) {
            fclose(f_y);
            f_y = NULL;
        }
        RM_LOG_INFO("PASSED test #4 (copy buffered offset), file [%s]", fname);
    }
    RM_LOG_INFO("%s", "PASSED test #4 (copy buffered offset)");
}

void
test_rm_adler32_1(void **state) {
	FILE            *f;
	int             fd;
	uint32_t        sf;	/* signature fast */
	unsigned char	buf[RM_TEST_L_MAX];
	size_t          r1, r2, i, j, adler, file_sz, read;
	struct
    test_rm_state   *rm_state;
	struct stat     fs;
	const char      *fname;

	rm_state = *state;
	assert_true(rm_state != NULL);

	i = 0; /* test on all files */
	for (; i < RM_TEST_FNAMES_N; ++i) {
		fname = rm_test_fnames[i];
		f = fopen(fname, "rb");
		if (f == NULL) {
			RM_LOG_PERR("Can't open file [%s]", fname);
		}
		assert_true(f != NULL);
		fd = fileno(f); /* get file size */
		if (fstat(fd, &fs) != 0) {
			RM_LOG_PERR("Can't fstat file [%s]", fname);
			fclose(f);
			assert_true(1 == 0);
		}
		file_sz = fs.st_size; 
		if (file_sz > RM_TEST_L_MAX) {
			RM_LOG_WARN("File [%s] size [%zu] is bigger than testing buffer's size of [%zu], reading only first [%zu] bytes", fname, file_sz, RM_TEST_L_MAX, RM_TEST_L_MAX);
			file_sz = RM_TEST_L_MAX;
		}
		read = fread(buf, 1, file_sz, f); /* read bytes */
		if (read != file_sz) {
			RM_LOG_WARN("Error reading file [%s], skipping", fname);
			fclose(f);
			continue;
		}
		fclose(f);
		assert_true(read == file_sz);
		r1 = 1; /* calc checksum */
		r2 = 0;
		j = 0;
		for (; j < file_sz; ++j) {
			r1 = (r1 + buf[j]) % RM_ADLER32_MODULUS;
			r2 = (r2 + r1) % RM_ADLER32_MODULUS;
		}
		adler = (r2 << 16) | r1;
	
		sf = rm_adler32_1(buf, file_sz);
		assert_true(adler == sf);
		RM_LOG_INFO("PASS Adler32 (1) checksum [%zu] OK, file [%s], size [%zu]", adler, fname, file_sz);
	}
    RM_LOG_INFO("%s", "PASSED test #5 (Adler32 (1))");
}

void
test_rm_adler32_2(void **state) {
	FILE            *f;
	int             fd;
	unsigned char   buf[RM_TEST_L_MAX];
	size_t          i, adler1, adler2, adler2_0, file_sz, read, r1_0, r2_0, r1_1, r2_1;
	struct test_rm_state   *rm_state;
	struct stat     fs;
	const char      *fname;

	rm_state = *state;
	assert_true(rm_state != NULL);

	i = 0;
	for (; i < RM_TEST_FNAMES_N; ++i) {
		fname = rm_test_fnames[i];
		f = fopen(fname, "rb");
		if (f == NULL) {
			RM_LOG_PERR("Can't open file [%s]", fname);
		}
		assert_true(f != NULL);
		fd = fileno(f);
		if (fstat(fd, &fs) != 0) {
			RM_LOG_PERR("Can't fstat file [%s]", fname);
			fclose(f);
			assert_true(1 == 0);
		}
		file_sz = fs.st_size; 
		if (file_sz > RM_TEST_L_MAX) {
			RM_LOG_WARN("File [%s] size [%zu] is bigger than testing buffer's size of [%zu], reading only first [%zu] bytes", fname, file_sz, RM_TEST_L_MAX, RM_TEST_L_MAX);
			file_sz = RM_TEST_L_MAX;
		}
		read = fread(buf, 1, file_sz, f);
		if (read != file_sz) {
			RM_LOG_PERR("Error reading file [%s], skipping", fname);
			fclose(f);
			continue;
		}
		fclose(f);
		assert_true(read == file_sz);
		adler1 = rm_adler32_1(buf, file_sz); /* checksum	 */
		adler2 = rm_adler32_2(1, buf, file_sz);
		assert_true(adler1 == adler2);
		adler2_0 = rm_adler32_2(0, buf, file_sz);
		r1_0 = adler2_0 & 0xffff; /* (1 + X) % MOD == (1 % MOD + X % MOD) % MOD */
		r1_1 = (r1_0 + (1 % RM_ADLER32_MODULUS)) % RM_ADLER32_MODULUS;
		assert_true((adler2 & 0xffff) == r1_1);
		r2_0 = (adler2_0 >> 16) & 0xffff;
		r2_1 = (r2_0 + (file_sz % RM_ADLER32_MODULUS)) % RM_ADLER32_MODULUS;
		assert_true(((adler2 >> 16) & 0xffff) == r2_1);
		RM_LOG_INFO("PASS Adler32 (2) checksum [%zu] OK, file [%s], size [%zu]", adler2, fname, file_sz);
	}
    RM_LOG_INFO("%s", "PASSED test #6 (Adler32 (2))");
}

void
test_rm_fast_check_roll(void **state) {
	FILE                    *f;
	int                     fd;
	unsigned char           buf[RM_TEST_L_MAX];
	size_t                  i, j, L, adler1, adler2, tests_n, tests_max;
    size_t                  file_sz, read, read_left, read_now;
	size_t                  idx_min, idx_max, idx, idx_buf;
	struct test_rm_state    *rm_state;
	struct stat             fs;
	const char              *fname;

	rm_state = *state;
	assert_true(rm_state != NULL);

	i = 0; /* test on all files */
	for (; i < RM_TEST_FNAMES_N; ++i) {
		fname = rm_test_fnames[i];
		f = fopen(fname, "rb");
		if (f == NULL) {
			RM_LOG_PERR("Can't open file [%s]", fname);
		}
		assert_true(f != NULL);
		fd = fileno(f); /* get file size */
		if (fstat(fd, &fs) != 0) {
			RM_LOG_PERR("Can't fstat file [%s]", fname);
			fclose(f);
			assert_true(1 == 0);
		}
		file_sz = fs.st_size; 
		j = 0;
		for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
			L = rm_test_L_blocks[j];
			RM_LOG_INFO("Validating testing of fast rolling checksum: file [%s], size [%zu], block size L [%zu], buffer [%zu]", fname, file_sz, L, RM_TEST_L_MAX);
			if (file_sz < 2) {
				RM_LOG_WARN("File [%s] size [%zu] is to small for this test, skipping", fname, file_sz);
				continue;
			}
			if (file_sz < L) {
				RM_LOG_WARN("File [%s] size [%zu] is smaller than block size L [%zu], skipping", fname, file_sz, L);
				continue;
			}
			if (RM_TEST_L_MAX < L + 1) {
				RM_LOG_WARN("Testing buffer [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", RM_TEST_L_MAX, L, fname);
				continue;
			}
			
			RM_LOG_INFO("Testing fast rolling checksum: file [%s], size [%zu], block size L [%zu], buffer [%zu]", fname, file_sz, L, RM_TEST_L_MAX);
			read = fread(buf, 1, L, f); /* read bytes */
			if (read != L) {
				RM_LOG_PERR("Error reading file [%s], skipping", fname);
				fseek(f, 0, SEEK_SET);	/* equivalent to rewind(f) */
				continue;
			}
			assert_true(read == L);

			adler1 = rm_fast_check_block(buf, L); /* initial checksum */
			fseek(f, 0, SEEK_SET);	/* equivalent to rewind(f) */
			tests_max = file_sz - L; /* number of times rolling checksum will be calculated */
			tests_n = 0;

			read_left = file_sz;
			do {
				idx_min = ftell(f);
				read_now = rm_min(RM_TEST_L_MAX, read_left); /* read all up to buffer size */
				read = fread(buf, 1, read_now, f);
				if (read != read_now) {
					RM_LOG_PERR("Error reading file [%s]", fname);
				}
				assert_true(read == read_now);
				idx_max = idx_min + read_now;	/* idx_max is 1 past the end of buf */
				idx = idx_min + 1; /* rolling checksum needs previous byte */
				while (idx < idx_max - L + 1) { /* checksums at offsets [1,idx_max-L] */
					++tests_n; /* count tests */

					idx_buf = idx - idx_min;
					adler2 = rm_fast_check_block(&buf[idx_buf], L);
					adler1 = rm_fast_check_roll(adler1, buf[idx_buf-1], buf[idx_buf+L-1], L); /* rolling checksum for offset [idx] */
					assert_int_equal(adler1, adler2);
					++idx;
				}
				read_left -= read;
				if (read_left > 0) {
					read_left += L; /* there are bytes remaining to read we must start L bytes to the left from idx_max to include a_k */
					fseeko(f, -(long long)L, SEEK_CUR);
				}
			} while (read_left > 0);
			assert_int_equal(tests_n, tests_max);
			
			RM_LOG_INFO("PASSED [%zu] fast rolling checksum tests, file [%s], size [%zu], L [%zu]", tests_n, fname, file_sz, L);
			rewind(f); /* move file pointer back to the beginning */
		}
		fclose(f);
	}
    RM_LOG_INFO("%s", "PASSED test #7 (fast rolling checksum)");
}

/* @brief   Tests number of calculated entries. */
void
test_rm_rx_insert_nonoverlapping_ch_ch_array_1(void **state) {
	FILE                    *f;
	int                     fd;
    int                     res;
	size_t                  i, j, L, file_sz;
	struct test_rm_state    *rm_state;
	struct stat             fs;
	const char              *fname;
	size_t                  blocks_n, entries_n;
    struct rm_ch_ch         *array;

	rm_state = *state;
	assert_true(rm_state != NULL);

	i = 0; /* test on all files */
	for (; i < RM_TEST_FNAMES_N; ++i) {
		fname = rm_test_fnames[i];
		f = fopen(fname, "rb");
		if (f == NULL) {
			RM_LOG_PERR("Can't open file [%s]", fname);
		}
		assert_true(f != NULL);
		fd = fileno(f);
		if (fstat(fd, &fs) != 0) {
			RM_LOG_PERR("Can't fstat file [%s]", fname);
			fclose(f);
			assert_true(1 == 0);
		}
		file_sz = fs.st_size; 
		j = 0;
		for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
			L = rm_test_L_blocks[j];
			RM_LOG_INFO("Validating testing of hashing of non-overlapping blocks: file [%s], size [%zu], block size L [%zu]", fname, file_sz, L);
			if (0 == L) {
				RM_LOG_WARN("Block size [%zu] is too small for this test (should be > [%zu]),  skipping file [%s]", L, 0, fname);
				continue;
			}
			if (file_sz < 2) {
				RM_LOG_WARN("File [%s] size [%zu] is to small for this test, skipping", fname, file_sz);
				continue;
			}
	
			RM_LOG_INFO("Testing of splitting file into non-overlapping blocks: file [%s], size [%zu], block size L [%zu], buffer [%zu]", fname, file_sz, L, RM_TEST_L_MAX);
			blocks_n = file_sz / L + (file_sz % L ? 1 : 0);

            /* reference array (it was allocated in setup function and points to valid memory, no need to free entries as we will
             * simply overwritten them in this test and free array memory in test's teardown function) */
            array = rm_state->array;
            assert(array != NULL && "Assertion failed, array buffer not allocated!\n");
            res = rm_rx_insert_nonoverlapping_ch_ch_array(f, fname, array, L, NULL, blocks_n, &entries_n);
            assert_int_equal(res, 0);
            assert_int_equal(entries_n, blocks_n);

            /* reset array */
            memset(array, 0, entries_n * sizeof(*array));
			
			RM_LOG_INFO("PASSED test of hashing of non-overlapping blocks, file [%s], size [%zu], L [%zu]", fname, file_sz, L);
			/* move file pointer back to the beginning */
			rewind(f);
		}
		fclose(f);
	}
    RM_LOG_INFO("%s", "PASSED test #8 (non-overlapping blocks)");
}

