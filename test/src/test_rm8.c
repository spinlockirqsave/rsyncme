/*
 * @file        test_rm7.c
 * @brief       Test suite #8.
 * @details     Test of rm_tx_local_push.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        15 June 2016 10:44 PM
 * @copyright   LGPLv2.1
 */


#include "test_rm8.h"


const char* rm_test_fnames[RM_TEST_FNAMES_N] = {
    "rm_f_1_ts8", "rm_f_2_ts8","rm_f_4_ts8", "rm_f_8_ts8", "rm_f_65_ts8",
    "rm_f_100_ts8", "rm_f_511_ts8", "rm_f_512_ts8", "rm_f_513_ts8", "rm_f_1023_ts8",
    "rm_f_1024_ts8", "rm_f_1025_ts8", "rm_f_4096_ts8", "rm_f_7787_ts8", "rm_f_20100_ts8"};

uint32_t	rm_test_fsizes[RM_TEST_FNAMES_N] = { 1, 2, 4, 8, 65,
                                                100, 511, 512, 513, 1023,
                                                1024, 1025, 4096, 7787, 20100 };

size_t
rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE] = { 1, 2, 3, 4, 8, 10, 13, 16,
                    24, 32, 50, 64, 100, 127, 128, 129,
                    130, 200, 400, 499, 500, 501, 511, 512,
                    513, 600, 800, 1000, 1100, 1123, 1124, 1125,
                    1200, 100000 };

static int
test_rm_copy_files_and_postfix(const char *postfix) {
    int         err;
    FILE        *f, *f_copy;
    uint32_t    i, j;
    char        buf[RM_FILE_LEN_MAX + 50];
    unsigned long const seed = time(NULL);

    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f = fopen(rm_test_fnames[i], "rb");
        if (f == NULL) {    /* if file doesn't exist, create */
            RM_LOG_INFO("Creating file [%s]", rm_test_fnames[i]);
            f = fopen(rm_test_fnames[i], "wb+");
            if (f == NULL) {
                exit(EXIT_FAILURE);
            }
            j = rm_test_fsizes[i];
            RM_LOG_INFO("Writing [%u] random bytes to file [%s]", j, rm_test_fnames[i]);
            srand(seed);
            while (j--) {
                fputc(rand(), f);
            }
        } else {
            RM_LOG_INFO("Using previously created file [%s]", rm_test_fnames[i]);
        }
        strncpy(buf, rm_test_fnames[i], RM_FILE_LEN_MAX); /* create copy */
        strncpy(buf + strlen(buf), postfix, 49);
        buf[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf, "rb+");
        if (f_copy == NULL) { /* if doesn't exist then create */
            RM_LOG_INFO("Creating copy [%s] of file [%s]", buf, rm_test_fnames[i]);
            f_copy = fopen(buf, "wb");
            if (f_copy == NULL) {
                RM_LOG_ERR("Can't open [%s] copy of file [%s]", buf, rm_test_fnames[i]);
                return -1;
            }
            err = rm_copy_buffered(f, f_copy, rm_test_fsizes[i]);
            switch (err) {
                case 0:
                    break;
                case -2:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s]", buf, rm_test_fnames[i]);
                    fclose(f);
                    fclose(f_copy);
                    return -2;
                case -3:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s], error set on original file", buf, rm_test_fnames[i]);
                    fclose(f);
                    fclose(f_copy);
                    return -3;
                case -4:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s], error set on copy", buf, rm_test_fnames[i]);
                    fclose(f);
                    fclose(f_copy);
                    return -4;
                default:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s], unknown error", buf, rm_test_fnames[i]);
                    fclose(f);
                    fclose(f_copy);
                    return -13;
            }
            fclose(f);
            fclose(f_copy);
        } else {
            RM_LOG_INFO("Using previously created copy of file [%s]", rm_test_fnames[i]);
        }
    }
    return 0;
}

static int
test_rm_delete_copies_of_files_postfixed(const char *postfix) {
    int         err;
    uint32_t    i;
    char        buf[RM_FILE_LEN_MAX + 50];

    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i) {
        strncpy(buf, rm_test_fnames[i], RM_FILE_LEN_MAX); /* open copy */
        strncpy(buf + strlen(buf), postfix, 49);
        buf[RM_FILE_LEN_MAX + 49] = '\0';
        RM_LOG_INFO("Removing (unlink) [%s] copy of file [%s]", buf, rm_test_fnames[i]);
        err = unlink(buf);
        if (err != 0) {
            RM_LOG_ERR("Can't unlink [%s] copy of file [%s]", buf, rm_test_fnames[i]);
            return -1;
        }
    }
    return 0;
}

int
test_rm_setup(void **state) {
    int         err;
    uint32_t    i,j;
    FILE        *f;
    void        *buf;
    struct rm_session   *s;
    unsigned long seed;

#ifdef DEBUG
    err = rm_util_chdir_umask_openlog("../build/debug", 1, "rsyncme_test_8");
#else
    err = rm_util_chdir_umask_openlog("../build/release", 1, "rsyncme_test_8");
#endif
    if (err != 0) {
        exit(EXIT_FAILURE);
    }
    rm_state.l = rm_test_L_blocks;
    *state = &rm_state;

    i = 0;
    seed = time(NULL);
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f = fopen(rm_test_fnames[i], "rb+");
        if (f == NULL) {    /* if file doesn't exist, create */
            RM_LOG_INFO("Creating file [%s]", rm_test_fnames[i]);
            f = fopen(rm_test_fnames[i], "wb");
            if (f == NULL) {
                exit(EXIT_FAILURE);
            }
            j = rm_test_fsizes[i];
            RM_LOG_INFO("Writing [%u] random bytes to file [%s]", j, rm_test_fnames[i]);
            srand(seed);
            while (j--) {
                fputc(rand(), f);
            }
        } else {
            RM_LOG_INFO("Using previously created file [%s]", rm_test_fnames[i]);
        }
        fclose(f);
    }

    i = 0;
    j = 0;
    for (; i < RM_TEST_L_BLOCKS_SIZE; ++i) {    /* find biggest L */
        if (rm_test_L_blocks[i] > j) j = rm_test_L_blocks[i];
    }
    buf = malloc(j);
    if (buf == NULL) {
        RM_LOG_ERR("Can't allocate 1st memory buffer of [%u] bytes, malloc failed", j);
	}
    assert_true(buf != NULL);
    rm_state.buf = buf;
    buf = malloc(j);
    if (buf == NULL) {
        RM_LOG_ERR("Can't allocate 2nd memory buffer of [%u] bytes, malloc failed", j);
	}
    assert_true(buf != NULL);
    rm_state.buf2 = buf;

    s = rm_session_create(RM_PUSH_LOCAL, 0);
    if (s == NULL) {
        RM_LOG_ERR("Can't allocate session local push");
	}
    assert_true(s != NULL);
    rm_state.s = s;
    return 0;
}

int
test_rm_teardown(void **state) {
    int     i;
    FILE    *f;
    struct  test_rm_state *rm_state;

    rm_state = *state;
    assert_true(rm_state != NULL);
    if (RM_TEST_8_DELETE_FILES == 1) {
        i = 0; /* delete all test files */
        for (; i < RM_TEST_FNAMES_N; ++i) {
            f = fopen(rm_test_fnames[i], "wb+");
            if (f == NULL) {
                RM_LOG_ERR("Can't open file [%s]", rm_test_fnames[i]);	
            } else {
                RM_LOG_INFO("Removing file [%s]", rm_test_fnames[i]);
                remove(rm_test_fnames[i]);
            }
        }
    }
    free(rm_state->buf);
    free(rm_state->buf2);
    rm_session_free(rm_state->s);
    return 0;
}

/* @brief   Test if result file @f_z is reconstructed properly
 *          when x file is same as y (file has no changes). */
void
test_rm_tx_local_push_1(void **state) {
    (void)state;
    return;
}

/* @brief   Test #2. */
/* @brief   Test if result file @f_z is reconstructed properly
 *          when x is copy of y, but first byte in x is changed. */
void
test_rm_tx_local_push_2(void **state) {
    int                     err;
    char                    buf_x_name[RM_FILE_LEN_MAX + 50];   /* @x (copy of @y with changed single byte at the beginning) */
    const char              *f_y_name;  /* @y name */
    unsigned char           cx, cz, cx_copy;
    FILE                    *f_copy, *f_x, *f_y;
    int                     fd_x, fd_y;
    size_t                  i, j, k, L, f_x_sz, f_y_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    rm_push_flags                   flags;
    size_t                          copy_all_threshold, copy_tail_threshold, send_threshold;
    struct rm_delta_reconstruct_ctx rec_ctx;
    size_t                      detail_case_1_n, detail_case_2_n, detail_case_3_n;

    err = test_rm_copy_files_and_postfix("_test_1");
    if (err != 0) {
        RM_LOG_ERR("Error copying files, skipping test");
        return;
    }

    rm_state = *state;
    assert_true(rm_state != NULL);

    i = 0;  /* test on all files */
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f_y_name = rm_test_fnames[i];
        f_y = fopen(f_y_name, "rb");
        if (f_y == NULL) {
            RM_LOG_PERR("Can't open file [%s]", f_y_name);
        }
        assert_true(f_y != NULL);
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {    /* get file size */
            RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
            fclose(f_y);
            assert_true(1 == 0);
        }
        f_y_sz = fs.st_size;
        if (f_y_sz < 2) {
            RM_LOG_INFO("File [%s] size [%u] is too small for this test, skipping", f_y_name, f_y_sz);
            fclose(f_y);
            continue;
        }

        /* change byte in copy */
        strncpy(buf_x_name, f_y_name, RM_FILE_LEN_MAX);
        strncpy(buf_x_name + strlen(buf_x_name), "_test_1", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
        }
        f_x = f_copy;
        fd_x = fileno(f_x);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_x, &fs) != 0) {    /* get @x size */
            RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
            fclose(f_x);
            assert_true(1 == 0);
        }
        f_x_sz = fs.st_size;
        if (rm_fpread(&cx, sizeof(unsigned char), 1, 0, f_x) != 1) { /* read first byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }
        cx_copy = cx;   /* remember first byte for recreation */
        cx = (cx + 1) % 256; /* change first byte, so ZERO_DIFF delta can't happen in this test, therefore this would be an error in this test */
        if (rm_fpwrite(&cx, sizeof(unsigned char), 1, 0, f_x) != 1) {
            RM_LOG_ERR("Error writing to file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }

        j = 0;
        for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
            L = rm_test_L_blocks[j];
            RM_LOG_INFO("Validating testing #1 of local push, file [%s], size [%u], block size L [%u]", f_y_name, f_y_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%u] is too small for this test (should be > [%u]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            if (f_y_sz < 2) {
                RM_LOG_INFO("File [%s] size [%u] is too small for this test, skipping", f_y_name, f_y_sz);
                continue;
            }
            RM_LOG_INFO("Testing local push #1: file @x[%s] size [%u] file @y[%s], size [%u], block size L [%u]", buf_x_name, f_x_sz, f_y_name, f_y_sz, L);
            copy_all_threshold = 0;
            copy_tail_threshold = 0;
            send_threshold = L;
            flags = 0;

            fclose(f_x);
            fclose(f_y);
            memset(&rec_ctx, 0, sizeof (struct rm_delta_reconstruct_ctx));
            err = rm_tx_local_push(buf_x_name, f_y_name, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx);
            assert_int_equal(err, 0);

            assert_int_equal(rec_ctx.rec_by_ref + rec_ctx.rec_by_raw, f_x_sz);  /* validate reconstruction ctx */
            assert_true(rec_ctx.delta_tail_n == 0 || rec_ctx.delta_tail_n == 1);
            assert_true(rec_ctx.delta_zero_diff_n == 0);
            assert_true(rec_ctx.rec_by_zero_diff == 0);

            f_x = fopen(buf_x_name, "rb+");
            if (f_x == NULL) {
                RM_LOG_PERR("Can't open file [%s]", buf_x_name);
            }
            assert_true(f_x != NULL && "Can't open file @x");
            fd_x = fileno(f_x);
            f_y = fopen(f_y_name, "rb");
            if (f_y == NULL) {
                RM_LOG_PERR("Can't open file [%s]", f_y_name);
            }
            assert_true(f_y != NULL && "Can't open file @y");
            fd_y = fileno(f_y);

            /* verify files size */
            memset(&fs, 0, sizeof(fs)); /* get @x size */
            if (fstat(fd_x, &fs) != 0) {
                RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
                fclose(f_x);
                fclose(f_y);
                assert_true(1 == 0);
            }
            f_x_sz = fs.st_size;
            memset(&fs, 0, sizeof(fs));
            if (fstat(fd_y, &fs) != 0) {
                RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
                fclose(f_x);
                fclose(f_y);
                assert_true(1 == 0);
            }
            f_y_sz = fs.st_size;
            assert_true(f_x_sz == f_y_sz && "File sizes differ!");

            k = 0;
            while (k < f_x_sz) {
                if (rm_fpread(&cx, sizeof(unsigned char), 1, k, f_x) != 1) {
                    RM_LOG_CRIT("Error reading file [%s]!", buf_x_name);
                    fclose(f_x);
                    fclose(f_y);
                    assert_true(1 == 0 && "ERROR reading byte in file @x!");
                }
                if (rm_fpread(&cz, sizeof(unsigned char), 1, k, f_y) != 1) {
                    RM_LOG_CRIT("Error reading file [%s]!", f_y_name);
                    fclose(f_x);
                    fclose(f_y);
                    assert_true(1 == 0 && "ERROR reading byte in file @z!");
                }
                if (cx != cz) {
                    RM_LOG_CRIT("Bytes [%u] differ: cx [%u], cz [%u]\n", k, cx, cz);
                }
                assert_true(cx == cz && "Bytes differ!");
                ++k;
            }

            if (RM_TEST_8_DELETE_FILES == 1) { /* and unlink/remove result file */
                if (unlink(f_y_name) != 0) {
                    RM_LOG_ERR("Can't unlink result file [%s]", f_y_name);
                    assert_true(1 == 0);
                }
            }
            /* detail cases */
            /* 1. if L is >= file size, delta must be single RAW element */
            if (L >= f_x_sz) {
                assert_true(rec_ctx.delta_ref_n == 0);
                assert_true(rec_ctx.delta_tail_n == 0);
                assert_true(rec_ctx.delta_raw_n == 1);
                assert_true(rec_ctx.rec_by_ref == 0);
                assert_true(rec_ctx.rec_by_tail == 0);
                assert_true(rec_ctx.rec_by_raw == f_y_sz);
                ++detail_case_1_n;
            }
            /* 2. if L is less than file size and does divide evenly file size, there will usually be f_y_sz/L - 1 delta reference blocks present
             * and 1 raw byte block because the first block in @x doesn't match the first block in @y (all other blocks in @x always match corresponding blocks in @y in this test #2),
             * and none of the checksums computed by rolling checksum procedure starting from second byte in @x up to Lth byte, i.e. on blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will match some of the nonoverlapping checksums from @y.
             * But there is a chance one of nonoverlapping blocks in @y will match first block in @x (which has first byte changed), e.g if L is 1, size is 100 and @y file is 0x1 0x2 0x3 0x78 0x5 ...
             * the @x file is then 0x78 0x2 0x3 0x78 0x5 ... and first block will match 4th block in @y. There is also a chance this won't match BUT some of blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will find a match and rolling proc will move on offsets different that nonoverlapping blocks. All next f_y_sz/L - 2 blocks may match or not and up to L bytes will be transferred as raw elements in any
             * possible way: L blocks each 1 byte size, or L/2 each 2 bytes or 1 block 1 byte long and one L-1, etc, THEREFORE:
             *  -> if first block is sent by delta ref - all file must be sent by delta ref,
             *  -> if first block doesn't match - there maust be f_y_sz/L - 1 DELTA REFERENCE blocks and L raw bytes sent by up to L DELTA RAW elements (depends on send threshold) */
            if ((L < f_y_sz) && (f_y_sz % L == 0)) {
                assert_true(rec_ctx.rec_by_raw <= L);
                assert_true(rec_ctx.delta_raw_n <= L);
                assert_true((rec_ctx.delta_ref_n == f_y_sz/L - 1 && rec_ctx.delta_raw_n > 0 && rec_ctx.rec_by_raw == L) || (rec_ctx.delta_ref_n == f_y_sz/L && rec_ctx.delta_raw_n == 0 && rec_ctx.rec_by_raw == 0)); /* the first block in @x will not match the first block in @y, but it can match some other block in @y */
                assert_true(rec_ctx.delta_ref_n * L == f_y_sz - rec_ctx.rec_by_raw);
                assert_true(rec_ctx.delta_tail_n == 0); /* impossible as L divides @y evenly */
                assert_true((rec_ctx.rec_by_ref == f_y_sz - L && rec_ctx.rec_by_raw == L) || (rec_ctx.delta_ref_n == f_y_sz/L && rec_ctx.rec_by_ref == f_y_sz && rec_ctx.rec_by_raw == 0));
                assert_true(rec_ctx.rec_by_tail == 0);
                ++detail_case_2_n;
            }
            /* 3. if L is less than file size and doesn't divide evenly file size, there can be delta TAIL reference block present,
             * regarding delta reference blocks it is the same situation as in test #2 (plus remember that TAIL is also counted as reference). */
            if ((L < f_y_sz) && (f_y_sz % L != 0)) {
                assert_true(rec_ctx.rec_by_raw <= L);
                assert_true(rec_ctx.delta_raw_n <= L);
                assert_true((rec_ctx.delta_ref_n == f_y_sz/L && rec_ctx.delta_raw_n > 0 && rec_ctx.rec_by_raw == L) || (rec_ctx.delta_ref_n == f_y_sz/L + 1 && rec_ctx.delta_raw_n == 0)); /* the first block in @x will not match the first block in @y, but it can match some other block in @y */
                assert_true((rec_ctx.delta_tail_n == 1 && (rec_ctx.rec_by_tail == f_y_sz % L) && ((rec_ctx.delta_ref_n - 1) * L + rec_ctx.rec_by_tail == f_y_sz - rec_ctx.rec_by_raw)) || (rec_ctx.delta_tail_n == 0 && rec_ctx.delta_ref_n * L == f_y_sz - rec_ctx.rec_by_raw));
                assert_true((rec_ctx.rec_by_ref == f_y_sz - L && rec_ctx.rec_by_raw == L) || (rec_ctx.rec_by_ref == f_y_sz && rec_ctx.rec_by_raw == 0));
                ++detail_case_3_n;
            }
            RM_LOG_INFO("PASSED test #1: files [%s] [%s], block [%u], passed delta reconstruction, files are the same", buf_x_name, f_y_name, L);

            f_y_name = rm_test_fnames[i]; /* recreate @y file as input to local push */
            f_y = fopen(f_y_name, "wb+");
            if (f_y == NULL) {
                RM_LOG_PERR("Can't recreate file [%s]", f_y_name);
            }
            assert_true(f_y != NULL);
            err = rm_copy_buffered(f_x, f_y, rm_test_fsizes[i]);
            if (err != 0) {
                RM_LOG_ERR("Error copying file @x to @y for next test");
                assert_true(1 == 0 && "Error copying file @x to @y for next test");
            }
            if (rm_fpwrite(&cx_copy, sizeof(unsigned char), 1, 0, f_y) != 1) {
                RM_LOG_ERR("Error writing to file [%s], skipping this test", f_y_name);
                fclose(f_x);
                fclose(f_y);
                continue;
            }
		}
		fclose(f_x);
        fclose(f_y);
        RM_LOG_INFO("PASSED test #1: files [%s] [%s] passed delta reconstruction for all block sizes, files are the same (detail cases: #1 [%u] #2 [%u] #3 [%u])",
                buf_x_name, f_y_name, detail_case_1_n, detail_case_2_n, detail_case_3_n);
	}

    if (RM_TEST_8_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_1");
        if (err != 0) {
            RM_LOG_ERR("Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}
