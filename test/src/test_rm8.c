/* @file        test_rm8.c
 * @brief       Test suite #8.
 * @details     Test of rm_tx_local_push.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        15 June 2016 10:44 PM
 * @copyright   LGPLv2.1 */


#include "test_rm8.h"


const char* rm_test_fnames[RM_TEST_FNAMES_N] = {
    "rm_f_1_ts8", "rm_f_2_ts8","rm_f_4_ts8", "rm_f_8_ts8", "rm_f_65_ts8",
    "rm_f_100_ts8", "rm_f_511_ts8", "rm_f_512_ts8", "rm_f_513_ts8", "rm_f_1023_ts8",
    "rm_f_1024_ts8", "rm_f_1025_ts8", "rm_f_4096_ts8", "rm_f_7787_ts8", "rm_f_20100_ts8"};

size_t	rm_test_fsizes[RM_TEST_FNAMES_N] = { 1, 2, 4, 8, 65,
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
    size_t      i, j;
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
            RM_LOG_INFO("Writing [%zu] random bytes to file [%s]", j, rm_test_fnames[i]);
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
                fclose(f);
                return -1;
            }
            err = rm_copy_buffered(f, f_copy, rm_test_fsizes[i]);
            switch (err) {
                case RM_ERR_OK:
                    break;
                case RM_ERR_FEOF:
                    RM_LOG_ERR("Can't write from [%s] to it's copy [%s], EOF", buf, rm_test_fnames[i]);
                    fclose(f);
                    fclose(f_copy);
                    return -2;
                case RM_ERR_FERROR:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s], error set on @x or @y", buf, rm_test_fnames[i]);
                    fclose(f);
                    fclose(f_copy);
                    return -3;
                case RM_ERR_TOO_MUCH_REQUESTED:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s], too much requested", buf, rm_test_fnames[i]);
                    fclose(f);
                    fclose(f_copy);
                    return -4;
                default:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s], unknown error", buf, rm_test_fnames[i]);
                    fclose(f);
                    fclose(f_copy);
                    return -13;
            }
        } else {
            RM_LOG_INFO("Using previously created copy of file [%s]", rm_test_fnames[i]);
        }
		if (f != NULL) {
			fclose(f);
		}
		if (f_copy != NULL) {
			fclose(f_copy);
		}
    }
    return 0;
}

static int
test_rm_delete_copies_of_files_postfixed(const char *postfix) {
    int         err;
    size_t      i;
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
    size_t      i,j;
    FILE        *f;
    void        *buf;
    struct rm_session   *s;
    unsigned long seed;

#ifdef DEBUG
    err = rm_util_chdir_umask_openlog("../build/debug", 1, "rsyncme_test_8", 1);
#else
    err = rm_util_chdir_umask_openlog("../build/release", 1, "rsyncme_test_8", 1);
#endif
    if (err != RM_ERR_OK) {
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

    i = 0;
    j = 0;
    for (; i < RM_TEST_L_BLOCKS_SIZE; ++i) {    /* find biggest L */
        if (rm_test_L_blocks[i] > j) j = rm_test_L_blocks[i];
    }
    buf = malloc(j);
    if (buf == NULL) {
        RM_LOG_ERR("Can't allocate 1st memory buffer of [%zu] bytes, malloc failed", j);
	}
    assert_true(buf != NULL);
    rm_state.buf = buf;
    buf = malloc(j);
    if (buf == NULL) {
        RM_LOG_ERR("Can't allocate 2nd memory buffer of [%zu] bytes, malloc failed", j);
	}
    assert_true(buf != NULL);
    rm_state.buf2 = buf;

    s = rm_session_create(RM_PUSH_LOCAL, 0);
    if (s == NULL) {
        RM_LOG_ERR("%s", "Can't allocate session local push");
	}
    assert_true(s != NULL);
    rm_state.s = s;
    return 0;
}

int
test_rm_teardown(void **state) {
    size_t  i;
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
                fclose(f);
                remove(rm_test_fnames[i]);
            }
        }
    }
    free(rm_state->buf);
    free(rm_state->buf2);
    rm_session_free(rm_state->s);
    return 0;
}

static void
test_rm_dump(struct rm_delta_reconstruct_ctx rec_ctx) {
    RM_LOG_CRIT("rec_ctx dump:\nmethod [%zu], rec_by_ref [%zu], rec_by_raw [%zu], delta_ref_n [%zu], delta_raw_n [%zu], "
            "rec_by_tail [%zu], rec_by_zero_diff [%zu], delta_tail_n [%zu], delta_zero_diff_n [%zu], L [%zu], "
            "copy_all_threshold [%zu], copy_tail_threshold [%zu], send_threshold [%zu]", rec_ctx.method, rec_ctx.rec_by_ref, rec_ctx.rec_by_raw,
            rec_ctx.delta_ref_n, rec_ctx.delta_raw_n, rec_ctx.rec_by_tail, rec_ctx.rec_by_zero_diff, rec_ctx.delta_tail_n,
            rec_ctx.delta_zero_diff_n, rec_ctx.L, rec_ctx.copy_all_threshold, rec_ctx.copy_tail_threshold, rec_ctx.send_threshold);
}
/* @brief   Test if result file @f_z is reconstructed properly
 *          when x file is same as y (file has no changes). */
void
test_rm_tx_local_push_1(void **state) {
    int                     err;
    enum rm_error           status;
    char                    buf_x_name[RM_FILE_LEN_MAX + 50];   /* @x (copy of @y with changed single byte at the beginning) */
    const char              *f_y_name;  /* @y name */
    unsigned char           cx, cz;
    FILE                    *f_copy, *f_x, *f_y;
    int                     fd_x, fd_y;
    size_t                  i, j, k, L, f_x_sz, f_y_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    rm_push_flags                   flags;
    size_t                          copy_all_threshold, copy_tail_threshold, send_threshold;
    struct rm_delta_reconstruct_ctx rec_ctx;
    size_t                          detail_case_1_n, detail_case_2_n, detail_case_3_n;

    err = test_rm_copy_files_and_postfix("_test_1");
    if (err != 0) {
        RM_LOG_ERR("%s", "Error copying files, skipping test");
        return;
    }

    rm_state = *state;
    assert_true(rm_state != NULL);
    f_x = NULL;
    f_y = NULL;
    f_copy = NULL;

    i = 0;  /* test on all files */
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f_y_name = rm_test_fnames[i];
        f_y = fopen(f_y_name, "rb");
        if (f_y == NULL) {
            RM_LOG_PERR("Can't open file [%s]", f_y_name);
        }
        assert_true(f_y != NULL && "Can't open @y file");
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {    /* get file size */
            RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
            fclose(f_y);
            assert_true(1 == 0);
        }
        f_y_sz = fs.st_size;
        if (f_y_sz < 2) {
            RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
            fclose(f_y);
            continue;
        }

        /* create/open copy */
        strncpy(buf_x_name, f_y_name, RM_FILE_LEN_MAX);
        strncpy(buf_x_name + strlen(buf_x_name), "_test_1", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
            if (f_y != NULL) fclose(f_y);
        }
        assert_true(f_copy != NULL && "Can't open copy");
        f_x = f_copy;
        fd_x = fileno(f_x);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_x, &fs) != 0) {    /* get @x size */
            RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
            if (f_x != NULL) fclose(f_x);
            if (f_y != NULL) fclose(f_y);
            assert_true(1 == 0);
        }
        f_x_sz = fs.st_size;

        detail_case_1_n = 0;
        detail_case_2_n = 0;
        detail_case_3_n = 0;
        j = 0;
        for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
            L = rm_test_L_blocks[j];
            RM_LOG_INFO("Validating testing #1 of local push, file [%s], size [%zu], block size L [%zu]", f_y_name, f_y_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            if (f_y_sz < 2) {
                RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
                continue;
            }
            RM_LOG_INFO("Testing local push #1: file @x[%s] size [%zu] file @y[%s], size [%zu], block size L [%zu]", buf_x_name, f_x_sz, f_y_name, f_y_sz, L);
            copy_all_threshold = 0;
            copy_tail_threshold = 0;
            send_threshold = L;
            flags = 0;

            if (f_x != NULL) fclose(f_x);
            if (f_y != NULL) fclose(f_y);
            memset(&rec_ctx, 0, sizeof (struct rm_delta_reconstruct_ctx));
            status = rm_tx_local_push(buf_x_name, f_y_name, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx);
            assert_int_equal(status, RM_ERR_OK);

            /* general tests */
            assert_int_equal(rec_ctx.rec_by_raw, 0);
            assert_int_equal(rec_ctx.rec_by_ref, f_y_sz); /* in this test y_sz == file_sz is size of both @x and @y */
            assert_true(rec_ctx.delta_tail_n == 0 || rec_ctx.delta_tail_n == 1);
            assert_true(rec_ctx.delta_zero_diff_n == 0 || (rec_ctx.delta_zero_diff_n == 1 && rec_ctx.rec_by_ref == f_y_sz && rec_ctx.rec_by_zero_diff == f_y_sz && rec_ctx.delta_tail_n == 0 && rec_ctx.delta_raw_n == 0));

            f_x = fopen(buf_x_name, "rb+");
            if (f_x == NULL) {
                RM_LOG_PERR("Can't open file [%s]", buf_x_name);
            }
            assert_true(f_x != NULL && "Can't open file @x");
            fd_x = fileno(f_x);
            f_y = fopen(f_y_name, "rb");
            if (f_y == NULL) {
                RM_LOG_PERR("Can't open file [%s]", f_y_name);
                if (f_x != NULL) fclose(f_x);
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
                    RM_LOG_CRIT("Bytes [%zu] differ: cx [%zu], cz [%zu]\n", k, cx, cz);
                }
                assert_true(cx == cz && "Bytes differ!");
                ++k;
            }
            if ((err = rm_file_cmp(f_x, f_y, 0, 0, f_x_sz)) != RM_ERR_OK) {
                RM_LOG_ERR("Bytes differ, err [%d]", err);
                assert_true(1 == 0);
            }
            /* don't unlink/remove result file, as it is just the same as @x and can be reused */

            /* detail cases */
            /* 1. if L is >= file size, delta must be ZERO_DIFF */
            if (L >= f_y_sz) {
                assert_true(rec_ctx.delta_zero_diff_n == 1);
                assert_true(rec_ctx.delta_ref_n == 1);
                assert_true(rec_ctx.delta_tail_n == 0);
                assert_true(rec_ctx.delta_raw_n == 0);
                assert_true(rec_ctx.rec_by_ref == f_y_sz);
                assert_true(rec_ctx.rec_by_zero_diff == f_y_sz);
                assert_true(rec_ctx.rec_by_tail == 0);
                assert_true(rec_ctx.rec_by_raw == 0);
                ++detail_case_1_n;
            }
            /* 2. if L is not equal file_sz and does divide evenly file_sz, there must be file_sz/L delta reference blocks present
             * and zero raw bytes, zero ZERO_DIFF, zero TAIL blocks */
            if ((L != f_y_sz) && (f_y_sz % L == 0)) {
                assert_true(rec_ctx.delta_zero_diff_n == 0);
                assert_true(rec_ctx.delta_ref_n == f_y_sz / L);
                assert_true(rec_ctx.delta_tail_n == 0);
                assert_true(rec_ctx.delta_raw_n == 0);
                assert_true(rec_ctx.rec_by_ref == f_y_sz);
                assert_true(rec_ctx.rec_by_zero_diff == 0);
                assert_true(rec_ctx.rec_by_tail == 0);
                assert_true(rec_ctx.rec_by_raw == 0);
                ++detail_case_2_n;
            }
            /* 3. if L is less than file_sz and doesn't divide evenly file_sz, there must be delta TAIL reference block present
             * file_sz/L + 1 reference blocks (includes TAIL block), zero raw bytes, zero ZERO_DIFF */
            if ((L < f_y_sz) && (f_y_sz % L != 0)) {
                assert_true(rec_ctx.delta_zero_diff_n == 0);
                assert_true(rec_ctx.delta_ref_n == f_y_sz / L + 1);
                assert_true(rec_ctx.delta_tail_n == 1);
                assert_true(rec_ctx.delta_raw_n == 0);
                assert_true(rec_ctx.rec_by_ref == f_y_sz);
                assert_true(rec_ctx.rec_by_zero_diff == 0);
                assert_true(rec_ctx.rec_by_tail == f_y_sz % L);
                assert_true(rec_ctx.rec_by_raw == 0);
                ++detail_case_3_n;
            }
            RM_LOG_INFO("PASSED test #1: files [%s] [%s], block [%zu], passed delta reconstruction, files are the same", buf_x_name, f_y_name, L);
            /* no need to recreate @y file as input to local push in this test, as @y stays the same all the time */
        }
        if (f_x != NULL) fclose(f_x);
        f_x = NULL;
        if (f_y != NULL) fclose(f_y);
        f_y = NULL;
        RM_LOG_INFO("PASSED test #1: files [%s] [%s] passed delta reconstruction for all block sizes, files are the same (detail cases: #1 [%zu] #2 [%zu] #3 [%zu])",
                buf_x_name, f_y_name, detail_case_1_n, detail_case_2_n, detail_case_3_n);
	}

    if (RM_TEST_8_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_1");
        if (err != 0) {
            RM_LOG_ERR("%s", "Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}

/* @brief   Test #2. */
/* @brief   Test if result file @f_z is reconstructed properly
 *          when x is copy of y, but first byte in x is changed. */
void
test_rm_tx_local_push_2(void **state) {
    int                     err;
    enum rm_error           status;
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

    err = test_rm_copy_files_and_postfix("_test_2");
    if (err != 0) {
        RM_LOG_ERR("%s", "Error copying files, skipping test");
        return;
    }

    rm_state = *state;
    assert_true(rm_state != NULL);
    f_x = NULL;
    f_y = NULL;
    f_copy = NULL;

    i = 0;  /* test on all files */
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f_y_name = rm_test_fnames[i];
        f_y = fopen(f_y_name, "rb");
        if (f_y == NULL) {
            RM_LOG_PERR("Can't open file [%s]", f_y_name);
        }
        assert_true(f_y != NULL && "Can't open @y file");
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {    /* get file size */
            RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
            fclose(f_y);
            assert_true(1 == 0);
        }
        f_y_sz = fs.st_size;
        if (f_y_sz < 2) {
            RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
            fclose(f_y);
            continue;
        }

        /* change byte in copy */
        strncpy(buf_x_name, f_y_name, RM_FILE_LEN_MAX);
        strncpy(buf_x_name + strlen(buf_x_name), "_test_2", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
            if (f_y != NULL) fclose(f_y);
        }
        assert_true(f_copy != NULL && "Can't open copy");
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

        detail_case_1_n = 0;
        detail_case_2_n = 0;
        detail_case_3_n = 0;
        j = 0;
        for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
            L = rm_test_L_blocks[j];
            RM_LOG_INFO("Validating testing #2 of local push [first byte changed], file [%s], size [%zu], block size L [%zu]", f_y_name, f_y_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            if (f_y_sz < 2) {
                RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
                continue;
            }
            RM_LOG_INFO("Testing local push #2 [first byte changed]: file @x[%s] size [%zu] file @y[%s], size [%zu], block size L [%zu]", buf_x_name, f_x_sz, f_y_name, f_y_sz, L);
            copy_all_threshold = 0;
            copy_tail_threshold = 0;
            send_threshold = L;
            flags = 0;

            if (f_x != NULL) {
                fclose(f_x);
            }
            if (f_y != NULL) {
                fclose(f_y);
            }
            memset(&rec_ctx, 0, sizeof (struct rm_delta_reconstruct_ctx));
            status = rm_tx_local_push(buf_x_name, f_y_name, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx);
            assert_int_equal(status, RM_ERR_OK);

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
                if (f_x != NULL) {
                    fclose(f_x);
                }
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
                    RM_LOG_CRIT("Bytes [%zu] differ: cx [%zu], cz [%zu]\n", k, cx, cz);
                }
                assert_true(cx == cz && "Bytes differ!");
                ++k;
            }
            if ((err = rm_file_cmp(f_x, f_y, 0, 0, f_x_sz)) != RM_ERR_OK) {
                RM_LOG_ERR("Bytes differ, err [%d]", err);
                assert_true(1 == 0);
            }

            if (RM_TEST_8_DELETE_FILES == 1) { /* and fclose/unlink/remove result file */
				if (f_y) {
					fclose(f_y);
				}
                if (unlink(f_y_name) != 0) {
                    RM_LOG_ERR("Can't unlink result file [%s]", f_y_name);
                    assert_true(1 == 0 && "Can't unlink @y file");
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
            RM_LOG_INFO("PASSED test #2: files [%s] [%s], block [%zu], passed delta reconstruction, files are the same", buf_x_name, f_y_name, L);

            f_y_name = rm_test_fnames[i]; /* recreate @y file as input to local push */
            f_y = fopen(f_y_name, "wb+");
            if (f_y == NULL) {
                RM_LOG_PERR("Can't recreate file [%s]", f_y_name);
            }
            assert_true(f_y != NULL);
            if (rm_copy_buffered(f_x, f_y, rm_test_fsizes[i]) != RM_ERR_OK) {
                RM_LOG_ERR("%s", "Error copying file @x to @y for next test");
                if (f_x != NULL) {
                    fclose(f_x);
                }
                if (f_y != NULL) {
                    fclose(f_y);
                }
                assert_true(1 == 0 && "Error copying file @x to @y for next test");
            }
            if (rm_fpwrite(&cx_copy, sizeof(unsigned char), 1, 0, f_y) != 1) {
                RM_LOG_ERR("Error writing to file [%s], skipping this test", f_y_name);
                fclose(f_x);
                fclose(f_y);
                continue;
            }
		}
		if (f_x) {
			fclose(f_x);
            f_x = NULL;
		}
		if (f_y) {
			fclose(f_y);
            f_y = NULL;
		}
        RM_LOG_INFO("PASSED test #2: files [%s] [%s] passed delta reconstruction for all block sizes, files are the same (detail cases: #1 [%zu] #2 [%zu] #3 [%zu])",
                buf_x_name, f_y_name, detail_case_1_n, detail_case_2_n, detail_case_3_n);
	}

    if (RM_TEST_8_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_2");
        if (err != 0) {
            RM_LOG_ERR("%s", "Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}

/* @brief   Test #3. */
/* @brief   Test if result file @f_z is reconstructed properly
 *          when x is copy of y, but last byte in x is changed. */
void
test_rm_tx_local_push_3(void **state) {
    int                     err;
    enum rm_error           status;
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

    err = test_rm_copy_files_and_postfix("_test_3");
    if (err != 0) {
        RM_LOG_ERR("%s", "Error copying files, skipping test");
        return;
    }

    rm_state = *state;
    assert_true(rm_state != NULL);
    f_x = NULL;
    f_y = NULL;
    f_copy = NULL;

    i = 0;  /* test on all files */
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f_y_name = rm_test_fnames[i];
        f_y = fopen(f_y_name, "rb");
        if (f_y == NULL) {
            RM_LOG_PERR("Can't open file [%s]", f_y_name);
        }
        assert_true(f_y != NULL && "Can't open file @y");
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {    /* get file size */
            RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
            fclose(f_y);
            assert_true(1 == 0);
        }
        f_y_sz = fs.st_size;
        if (f_y_sz < 2) {
            RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
            fclose(f_y);
            continue;
        }

        /* change last byte in copy */
        strncpy(buf_x_name, f_y_name, RM_FILE_LEN_MAX);
        strncpy(buf_x_name + strlen(buf_x_name), "_test_3", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
            if (f_y != NULL) fclose(f_y);
        }
        assert_true(f_copy != NULL && "Can't open copy file");
        f_x = f_copy;
        fd_x = fileno(f_x);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_x, &fs) != 0) {    /* get @x size */
            RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            assert_true(1 == 0);
        }
        f_x_sz = fs.st_size;
        if (rm_fpread(&cx, sizeof(unsigned char), 1, f_x_sz - 1, f_x) != 1) { /* read last byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }
        cx_copy = cx;           /* remember the last byte for recreation */
        cx = (cx + 1) % 256;    /* change last byte, so ZERO_DIFF and TAIL delta can't happen in this test, therefore this would be an error in this test */
        if (rm_fpwrite(&cx, sizeof(unsigned char), 1, f_x_sz - 1, f_x) != 1) {
            RM_LOG_ERR("Error writing to file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }

        detail_case_1_n = 0;
        detail_case_2_n = 0;
        detail_case_3_n = 0;
        j = 0;
        for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
            L = rm_test_L_blocks[j];
            RM_LOG_INFO("Validating testing #3 of local push [last byte changed], file [%s], size [%zu], block size L [%zu]", f_y_name, f_y_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            if (f_y_sz < 2) {
                RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
                continue;
            }
            RM_LOG_INFO("Testing local push #3 [last byte changed]: file @x[%s] size [%zu] file @y[%s], size [%zu], block size L [%zu]", buf_x_name, f_x_sz, f_y_name, f_y_sz, L);
            copy_all_threshold = 0;
            copy_tail_threshold = 0;
            send_threshold = L;
            flags = 0;

            if (f_x != NULL) fclose(f_x);
            if (f_y != NULL) fclose(f_y);
            memset(&rec_ctx, 0, sizeof (struct rm_delta_reconstruct_ctx));
            status = rm_tx_local_push(buf_x_name, f_y_name, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx);
            assert_int_equal(status, RM_ERR_OK);

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
                fclose(f_y);
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
                    RM_LOG_CRIT("Bytes [%zu] differ: cx [%zu], cz [%zu]\n", k, cx, cz);
                }
                assert_true(cx == cz && "Bytes differ!");
                ++k;
            }
            if ((err = rm_file_cmp(f_x, f_y, 0, 0, f_x_sz)) != RM_ERR_OK) {
                RM_LOG_ERR("Bytes differ, err [%d]", err);
                assert_true(1 == 0);
            }

            if (RM_TEST_8_DELETE_FILES == 1) { /* and fclose/unlink/remove result file */
				if (f_y) {
					fclose(f_y);
				}
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
             * possible way: L blocks each 1 byte size, or L/2 each 2 bytes or 1 block 1 byte long and one L-1, etc. The same applies to last block, THEREFORE:
             *  -> if last block is sent by delta ref - all file must be sent by delta ref,
             *  -> if last block doesn't match - there maust be f_y_sz/L - 1 DELTA REFERENCE blocks and f_y_sz % L raw bytes sent by up to f_y_sz % L DELTA RAW elements (depends on send threshold) */
            if ((L < f_y_sz) && (f_y_sz % L == 0)) {
                assert_true(rec_ctx.rec_by_raw <= L);
                assert_true(rec_ctx.delta_raw_n <= L);
                assert_true((rec_ctx.delta_ref_n == f_y_sz/L - 1 && rec_ctx.delta_raw_n > 0 && rec_ctx.rec_by_raw == L) || (rec_ctx.delta_ref_n == f_y_sz/L && rec_ctx.delta_raw_n == 0 && rec_ctx.rec_by_raw == 0)); /* the last (tail) block in @x will not match the last block in @y, but it can match some other block in @y */
                assert_true(rec_ctx.delta_ref_n * L == f_y_sz - rec_ctx.rec_by_raw);
                assert_true(rec_ctx.delta_tail_n == 0); /* impossible as L divides @y evenly */
                assert_true(rec_ctx.rec_by_tail == 0);
                assert_true((rec_ctx.rec_by_ref == f_y_sz - L && rec_ctx.rec_by_raw == L) || (rec_ctx.delta_ref_n == f_y_sz/L && rec_ctx.rec_by_ref == f_y_sz && rec_ctx.rec_by_raw == 0));
                ++detail_case_2_n;
            }
            /* 3. if L is less than file size and doesn't divide evenly file size, there can be delta TAIL reference block present (this cannot happen in this test),
             * regarding delta reference blocks it is the same situation as in test #2 (plus remember that TAIL is also counted as reference). */
            if ((L < f_y_sz) && (f_y_sz % L != 0)) {
                assert_true(rec_ctx.rec_by_raw <= L);
                assert_true(rec_ctx.delta_raw_n <= L);
                assert_true(rec_ctx.delta_ref_n == f_y_sz/L && rec_ctx.delta_raw_n > 0 && rec_ctx.rec_by_raw == f_y_sz % L); /* the last (tail) block in @x will not match the last block in @y and it cannot match any other block in @y (they are of different size) */
                assert_true((rec_ctx.delta_tail_n == 1 && (rec_ctx.rec_by_tail == f_y_sz % L) && ((rec_ctx.delta_ref_n - 1) * L + rec_ctx.rec_by_tail == f_y_sz - rec_ctx.rec_by_raw)) || (rec_ctx.delta_tail_n == 0 && rec_ctx.delta_ref_n * L == f_y_sz - rec_ctx.rec_by_raw));
                assert_true(rec_ctx.rec_by_raw == f_y_sz % L && rec_ctx.rec_by_ref == f_y_sz - rec_ctx.rec_by_raw);
                ++detail_case_3_n;
            }
            /* 4. There can't be DELTA TAIL elements in this test as tail of @x will never match potential tail element in @y as last bytes in files differ */
            assert_true(rec_ctx.rec_by_tail == 0 && rec_ctx.delta_tail_n == 0);
            RM_LOG_INFO("PASSED test #3: files [%s] [%s], block [%zu], passed delta reconstruction, files are the same", buf_x_name, f_y_name, L);

            f_y_name = rm_test_fnames[i]; /* recreate @y file as input to local push */
            f_y = fopen(f_y_name, "wb+");
            if (f_y == NULL) {
                RM_LOG_PERR("Can't recreate file [%s]", f_y_name);
                if (f_x != NULL) fclose(f_x);
            }
            assert_true(f_y != NULL);
            if (rm_copy_buffered(f_x, f_y, rm_test_fsizes[i]) != RM_ERR_OK) {
                RM_LOG_ERR("%s", "Error copying file @x to @y for next test");
                if (f_x != NULL) fclose(f_x);
                if (f_y != NULL) fclose(f_y);
                assert_true(1 == 0 && "Error copying file @x to @y for next test");
            }
            if (rm_fpwrite(&cx_copy, sizeof(unsigned char), 1, f_x_sz - 1, f_y) != 1) {
                RM_LOG_ERR("Error writing to file [%s], skipping this test", f_y_name);
                if (f_x != NULL) fclose(f_x);
                if (f_y != NULL) fclose(f_y);
                continue;
            }
        }
        if (f_x != NULL) fclose(f_x);
        f_x = NULL;
        if (f_y != NULL) fclose(f_y);
        f_y = NULL;
        RM_LOG_INFO("PASSED test #3: files [%s] [%s] passed delta reconstruction for all block sizes, files are the same (detail cases: #1 [%zu] #2 [%zu] #3 [%zu])",
                buf_x_name, f_y_name, detail_case_1_n, detail_case_2_n, detail_case_3_n);
	}

    if (RM_TEST_8_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_3");
        if (err != 0) {
            RM_LOG_ERR("%s", "Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}

/* @brief   Test #4. */
/* @brief   Test if result file @f_z is reconstructed properly
 *          when x is copy of y, but first and last bytes in x are changed. */
void
test_rm_tx_local_push_4(void **state) {
    int                     err;
    enum rm_error           status;
    char                    buf_x_name[RM_FILE_LEN_MAX + 50];   /* @x (copy of @y with bytes changed) */
    const char              *f_y_name;  /* @y name */
    unsigned char           cx, cz, cx_copy_first, cx_copy_last;
    FILE                    *f_copy, *f_x, *f_y;
    int                     fd_x, fd_y;
    size_t                  i, j, k, L, f_x_sz, f_y_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    rm_push_flags                   flags;
    size_t                          copy_all_threshold, copy_tail_threshold, send_threshold;
    struct rm_delta_reconstruct_ctx rec_ctx;
    size_t                      detail_case_1_n, detail_case_2_n, detail_case_3_n;

    err = test_rm_copy_files_and_postfix("_test_4");
    if (err != 0) {
        RM_LOG_ERR("%s", "Error copying files, skipping test");
        return;
    }

    rm_state = *state;
    assert_true(rm_state != NULL);
    f_x = NULL;
    f_y = NULL;
    f_copy = NULL;

    i = 0;  /* test on all files */
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f_y_name = rm_test_fnames[i];
        f_y = fopen(f_y_name, "rb");
        if (f_y == NULL) {
            RM_LOG_PERR("Can't open file [%s]", f_y_name);
        }
        assert_true(f_y != NULL && "Can't open file @y");
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {    /* get file size */
            RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
            fclose(f_y);
            assert_true(1 == 0);
        }
        f_y_sz = fs.st_size;
        if (f_y_sz < 2) {
            RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
            fclose(f_y);
            continue;
        }

        /* change first and last bytes in copy */
        strncpy(buf_x_name, f_y_name, RM_FILE_LEN_MAX);
        strncpy(buf_x_name + strlen(buf_x_name), "_test_4", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
            if (f_y != NULL) fclose(f_y);
        }
        assert_true(f_copy != NULL && "Can't open copy file");
        f_x = f_copy;
        fd_x = fileno(f_x);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_x, &fs) != 0) {    /* get @x size */
            RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
            if (f_x != NULL) fclose(f_x);
            if (f_y != NULL) fclose(f_y);
            assert_true(1 == 0);
        }
        f_x_sz = fs.st_size;
        if (rm_fpread(&cx, sizeof(unsigned char), 1, 0, f_x) != 1) { /* read first byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            if (f_x != NULL) fclose(f_x);
            if (f_y != NULL) fclose(f_y);
            continue;
        }
        cx_copy_first = cx;     /* remember the first byte for recreation */
        cx = (cx + 1) % 256;    /* change first byte, so ZERO_DIFF can't happen in this test */
        if (rm_fpwrite(&cx, sizeof(unsigned char), 1, 0, f_x) != 1) {
            RM_LOG_ERR("Error writing to file [%s], skipping this test", buf_x_name);
            if (f_x != NULL) fclose(f_x);
            if (f_y != NULL) fclose(f_y);
            continue;
        }
        if (rm_fpread(&cx, sizeof(unsigned char), 1, f_x_sz - 1, f_x) != 1) { /* read last byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            if (f_x != NULL) fclose(f_x);
            if (f_y != NULL) fclose(f_y);
            continue;
        }
        cx_copy_last = cx;      /* remember the last byte for recreation */
        cx = (cx + 1) % 256;    /* change last byte, so ZERO_DIFF and TAIL delta can't happen in this test */
        if (rm_fpwrite(&cx, sizeof(unsigned char), 1, f_x_sz - 1, f_x) != 1) {
            RM_LOG_ERR("Error writing to file [%s], skipping this test", buf_x_name);
            if (f_x != NULL) fclose(f_x);
            if (f_y != NULL) fclose(f_y);
            continue;
        }

        detail_case_1_n = 0;
        detail_case_2_n = 0;
        detail_case_3_n = 0;
        j = 0;
        for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
            L = rm_test_L_blocks[j];
            RM_LOG_INFO("Validating testing #4 of local push [first and last bytes changed], file [%s], size [%zu], block size L [%zu]", f_y_name, f_y_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            RM_LOG_INFO("Testing local push #4 [first and last byte changed]: file @x[%s] size [%zu] file @y[%s], size [%zu], block size L [%zu]", buf_x_name, f_x_sz, f_y_name, f_y_sz, L);
            copy_all_threshold = 0;
            copy_tail_threshold = 0;
            send_threshold = L;
            flags = 0;

            if (f_x != NULL) fclose(f_x);
            if (f_y != NULL) fclose(f_y);
            memset(&rec_ctx, 0, sizeof (struct rm_delta_reconstruct_ctx));
            status = rm_tx_local_push(buf_x_name, f_y_name, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx);
            assert_int_equal(status, RM_ERR_OK);

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
                if (f_x != NULL) fclose(f_x);
            }
            assert_true(f_y != NULL && "Can't open file @y");
            fd_y = fileno(f_y);

            /* verify files size */
            memset(&fs, 0, sizeof(fs)); /* get @x size */
            if (fstat(fd_x, &fs) != 0) {
                RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
                if (f_x != NULL) fclose(f_x);
                if (f_y != NULL) fclose(f_y);
                assert_true(1 == 0);
            }
            f_x_sz = fs.st_size;
            memset(&fs, 0, sizeof(fs));
            if (fstat(fd_y, &fs) != 0) {
                RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
                if (f_x != NULL) fclose(f_x);
                if (f_y != NULL) fclose(f_y);
                assert_true(1 == 0);
            }
            f_y_sz = fs.st_size;
            assert_true(f_x_sz == f_y_sz && "File sizes differ!");

            k = 0;
            while (k < f_x_sz) {
                if (rm_fpread(&cx, sizeof(unsigned char), 1, k, f_x) != 1) {
                    RM_LOG_CRIT("Error reading file [%s]!", buf_x_name);
                    if (f_x != NULL) fclose(f_x);
                    if (f_y != NULL) fclose(f_y);
                    assert_true(1 == 0 && "ERROR reading byte in file @x!");
                }
                if (rm_fpread(&cz, sizeof(unsigned char), 1, k, f_y) != 1) {
                    RM_LOG_CRIT("Error reading file [%s]!", f_y_name);
                    if (f_x != NULL) fclose(f_x);
                    if (f_y != NULL) fclose(f_y);
                    assert_true(1 == 0 && "ERROR reading byte in file @z!");
                }
                if (cx != cz) {
                    RM_LOG_CRIT("Bytes [%zu] differ: cx [%zu], cz [%zu]\n", k, cx, cz);
                    if (f_x != NULL) fclose(f_x);
                    if (f_y != NULL) fclose(f_y);
                }
                assert_true(cx == cz && "Bytes differ!");
                ++k;
            }
            if ((err = rm_file_cmp(f_x, f_y, 0, 0, f_x_sz)) != RM_ERR_OK) {
                RM_LOG_ERR("Bytes differ, err [%d]", err);
                assert_true(1 == 0);
            }

            if (RM_TEST_8_DELETE_FILES == 1) { /* and fclose/unlink/remove result file */
				if (f_y) {
					fclose(f_y);
                    f_y = NULL;
				}
                if (unlink(f_y_name) != 0) {
                    RM_LOG_ERR("Can't unlink result file [%s]", f_y_name);
                    if (f_x != NULL) fclose(f_x);
                    if (f_y != NULL) fclose(f_y);
                    assert_true(1 == 0);
                }
            }
            /* detail cases */
            /* 1. if L is >= file size, delta must be single RAW element */
            if (L >= f_x_sz) {
                assert_true(rec_ctx.delta_ref_n == 0);
                assert_true(rec_ctx.delta_tail_n == 0);
                assert_true(rec_ctx.delta_raw_n == f_y_sz / send_threshold + (f_y_sz % send_threshold ? 1 : 0));
                assert_true(rec_ctx.rec_by_ref == 0);
                assert_true(rec_ctx.rec_by_tail == 0);
                assert_true(rec_ctx.rec_by_raw == f_y_sz);
                ++detail_case_1_n;
            }
            /* 2. if L is less than file size and does divide evenly file size, there will usually be f_y_sz/L - 2 delta reference blocks present
             * and 2 raw byte blocks because the first block in @x doesn't match the first block in @y and last doesn't match the last in @y,
             * and none of the checksums computed by rolling checksum procedure starting from second byte in @x up to Lth byte, i.e. on blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will match some of the nonoverlapping checksums from @y.
             * But there is a chance one of nonoverlapping blocks in @y will match first and/or last block in @x (which has first and last bytes changed), e.g if L is 1, size is 100 and @y file is 0x1 0x2 0x3 0x78 0x5 ...
             * the @x file is then 0x78 0x2 0x3 0x78 0x5 ... and first block will match 4th block in @y. There is also a chance this won't match BUT some of blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will find a match and rolling proc will move on offsets different that nonoverlapping blocks. All next f_y_sz/L - 2 blocks may match or not and up to 2*L bytes will be transferred as raw delta elements */
            if ((L < f_y_sz) && (f_y_sz % L == 0)) {
                assert_true(rec_ctx.delta_tail_n == 0);
                assert_true(rec_ctx.rec_by_raw <= 2 * L);
                assert_true(rec_ctx.delta_raw_n <= f_y_sz);
                if (f_y_sz <= (2 * L)) {
                    if (rec_ctx.delta_ref_n == 0) {
                        assert_true(rec_ctx.rec_by_ref == 0 && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n == (f_y_sz / send_threshold + (f_y_sz % send_threshold ? 1 : 0)) && rec_ctx.rec_by_raw == f_y_sz);
                    } else {
                        assert_true(rec_ctx.delta_ref_n > 0 && rec_ctx.rec_by_ref == (rec_ctx.delta_ref_n * L) && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n <= 2 * L && rec_ctx.rec_by_raw == (f_y_sz - rec_ctx.rec_by_ref));
                    }
                } else {
                    if (rec_ctx.delta_ref_n == f_y_sz / L - 2) {
                        if (rec_ctx.rec_by_ref != f_y_sz - 2 * L) {
                            test_rm_dump(rec_ctx);
                        }
                        assert_true(rec_ctx.rec_by_ref == f_y_sz - 2 * L);
                        assert_true(rec_ctx.delta_raw_n > 0);
                        if (rec_ctx.delta_raw_n > 2 * L) {
                            test_rm_dump(rec_ctx);
                        }
                        assert_true(rec_ctx.delta_raw_n <= 2 * L);
                        if (rec_ctx.rec_by_raw != 2 * L) {
                            test_rm_dump(rec_ctx);
                        }
                        assert_true(rec_ctx.rec_by_raw == 2 * L);
                    } else if (rec_ctx.delta_ref_n == (f_y_sz / L - 1)) {
                       assert_true(rec_ctx.rec_by_ref == (f_y_sz - L) && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n <= L && rec_ctx.rec_by_raw == L);
                    } else {
                       assert_true(rec_ctx.delta_ref_n == f_y_sz / L && rec_ctx.rec_by_ref == f_y_sz && rec_ctx.delta_raw_n == 0 && rec_ctx.rec_by_raw == 0); /* the last (tail) block in @x will not match the last block in @y, but it can match some other block in @y, same with first block */
                    }
                }
                assert_true(rec_ctx.delta_ref_n * L == f_y_sz - rec_ctx.rec_by_raw);
                assert_true(rec_ctx.delta_tail_n == 0); /* impossible as L divides @y evenly */
                assert_true(rec_ctx.rec_by_tail == 0);
                ++detail_case_2_n;
            }
            /* 3. if L is less than file size and doesn't divide evenly file size, there can be delta TAIL reference block present (this cannot happen in this test),
             * regarding delta reference blocks it is the same situation as in test #2 (plus remember that TAIL is also counted as reference). */
            if ((L < f_y_sz) && (f_y_sz % L != 0)) {
                assert_true(rec_ctx.rec_by_raw <= 2 * L);
                if (f_y_sz <= (2 * L)) {
                    if (rec_ctx.delta_ref_n == 0) {
                        assert_true(rec_ctx.rec_by_ref == 0 && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n == (f_y_sz / send_threshold + (f_y_sz % send_threshold ? 1 : 0)) && rec_ctx.rec_by_raw == f_y_sz);
                    } else {
                        assert_true(rec_ctx.delta_ref_n > 0 && rec_ctx.rec_by_ref == (rec_ctx.delta_ref_n * L) && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n <= 2 * L && rec_ctx.rec_by_raw == (f_y_sz - rec_ctx.rec_by_ref));
                    }
                } else {
                    if (rec_ctx.delta_ref_n == (f_y_sz / L + 1 - 2)) {
                        assert_true(rec_ctx.rec_by_ref == (L * (f_y_sz / L + 1 - 2)));
                        assert_true(rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n <= L + (f_y_sz % L));
                        assert_true(rec_ctx.rec_by_raw == (L + f_y_sz % L));
                    } else if (rec_ctx.delta_ref_n == (f_y_sz / L + 1 - 1)) {
                       assert_true((rec_ctx.rec_by_ref == (f_y_sz - L) || rec_ctx.rec_by_ref == (f_y_sz - f_y_sz % L)) && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n <= L && (rec_ctx.rec_by_raw == L || rec_ctx.rec_by_raw == f_y_sz % L));
                    } else {
                        assert_true(1 == 0 && "Got only delta reference blocks but TAIL can't match in this test!"); 
                    }
                }
                assert_true(rec_ctx.rec_by_ref == f_y_sz - rec_ctx.rec_by_raw);
                ++detail_case_3_n;
            }
            /* 4. There can't be DELTA TAIL elements in this test as tail of @x will never match potential tail element in @y as last bytes in files differ */
            assert_true(rec_ctx.rec_by_tail == 0 && rec_ctx.delta_tail_n == 0);
            RM_LOG_INFO("PASSED test #4: files [%s] [%s], block [%zu], passed delta reconstruction, files are the same", buf_x_name, f_y_name, L);

            f_y_name = rm_test_fnames[i]; /* recreate @y file as input to local push */
            f_y = fopen(f_y_name, "wb+");
            if (f_y == NULL) {
                RM_LOG_PERR("Can't recreate file [%s]", f_y_name);
                if (f_x != NULL) fclose(f_x);
            }
            assert_true(f_y != NULL);
            if (rm_copy_buffered(f_x, f_y, rm_test_fsizes[i]) != RM_ERR_OK) {
                RM_LOG_ERR("%s", "Error copying file @x to @y for next test");
                if (f_x != NULL) fclose(f_x);
                if (f_y != NULL) fclose(f_y);
                assert_true(1 == 0 && "Error copying file @x to @y for next test");
            }
            if (rm_fpwrite(&cx_copy_first, sizeof(unsigned char), 1, 0, f_y) != 1) {
                RM_LOG_ERR("Error writing to file [%s], skipping this test", f_y_name);
                if (f_x != NULL) fclose(f_x);
                if (f_y != NULL) fclose(f_y);
                continue;
            }
            if (rm_fpwrite(&cx_copy_last, sizeof(unsigned char), 1, f_x_sz - 1, f_y) != 1) {
                RM_LOG_ERR("Error writing to file [%s], skipping this test", f_y_name);
                if (f_x != NULL) fclose(f_x);
                if (f_y != NULL) fclose(f_y);
                continue;
            }
        }
        if (f_x != NULL) fclose(f_x);
        f_x = NULL;
        if (f_y != NULL) fclose(f_y);
        f_y = NULL;
        RM_LOG_INFO("PASSED test #4: files [%s] [%s] passed delta reconstruction for all block sizes, files are the same (detail cases: #1 [%zu] #2 [%zu] #3 [%zu])",
                buf_x_name, f_y_name, detail_case_1_n, detail_case_2_n, detail_case_3_n);
	}

    if (RM_TEST_8_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_4");
        if (err != 0) {
            RM_LOG_ERR("%s", "Error removing files (unlink)");
            if (f_x != NULL) fclose(f_x);
            if (f_y != NULL) fclose(f_y);
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}

/* @brief   Test #5. */
/* @brief   Test if result file @f_z is reconstructed properly
 *          when x is copy of y, but first, last and byte in the middle of x are changed. */
void
test_rm_tx_local_push_5(void **state) {
    int                     err;
    enum rm_error           status;
    char                    buf_x_name[RM_FILE_LEN_MAX + 50];   /* @x (copy of @y but with bytes changed) */
    const char              *f_y_name;  /* @y name */
    unsigned char           cx, cz, cx_copy_first, cx_copy_last, cx_copy_middle;
    FILE                    *f_copy, *f_x, *f_y;
    int                     fd_x, fd_y;
    size_t                  i, j, k, L, f_x_sz, f_y_sz, half_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    rm_push_flags                   flags;
    size_t                          copy_all_threshold, copy_tail_threshold, send_threshold;
    struct rm_delta_reconstruct_ctx rec_ctx;
    size_t                      detail_case_1_n, detail_case_2_n, detail_case_3_n;

    err = test_rm_copy_files_and_postfix("_test_5");
    if (err != 0) {
        RM_LOG_ERR("%s", "Error copying files, skipping test");
        return;
    }

    rm_state = *state;
    assert_true(rm_state != NULL);
    f_x = NULL;
    f_y = NULL;
    f_copy = NULL;

    i = 0;  /* test on all files */
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f_y_name = rm_test_fnames[i];
        f_y = fopen(f_y_name, "rb");
        if (f_y == NULL) {
            RM_LOG_PERR("Can't open file [%s]", f_y_name);
        }
        assert_true(f_y != NULL && "Can't open file @y");
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {    /* get file size */
            RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
            if (f_y != NULL) fclose(f_y);
            assert_true(1 == 0 && "Can't open @y file");
        }
        f_y_sz = fs.st_size;
        if (f_y_sz < 3) {
            RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
            if (f_y != NULL) fclose(f_y);
            continue;
        }

        /* change first and last bytes in copy */
        strncpy(buf_x_name, f_y_name, RM_FILE_LEN_MAX);
        strncpy(buf_x_name + strlen(buf_x_name), "_test_5", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
            if (f_y != NULL) fclose(f_y);
        }
        assert_true(f_copy != NULL && "Can't open copy of file");
        f_x = f_copy;
        fd_x = fileno(f_x);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_x, &fs) != 0) {    /* get @x size */
            RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
            if (f_x != NULL) fclose(f_x);
            if (f_y != NULL) fclose(f_y);
            assert_true(1 == 0);
        }
        f_x_sz = fs.st_size;
        if (rm_fpread(&cx, sizeof(unsigned char), 1, 0, f_x) != 1) { /* read first byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
            if (f_y != NULL) {
                fclose(f_y);
                f_y = NULL;
            }
            continue;
        }
        cx_copy_first = cx;     /* remember the first byte for recreation */
        cx = (cx + 1) % 256;    /* change first byte, so ZERO_DIFF can't happen in this test */
        if (rm_fpwrite(&cx, sizeof(unsigned char), 1, 0, f_x) != 1) {
            RM_LOG_ERR("Error writing to file [%s], skipping this test", buf_x_name);
            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
            if (f_y != NULL) {
                fclose(f_y);
                f_y = NULL;
            }
            continue;
        }
        half_sz = f_x_sz / 2 + f_x_sz % 2;
        if (rm_fpread(&cx, sizeof(unsigned char), 1, half_sz, f_x) != 1) { /* read middle byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
            if (f_y != NULL) {
                fclose(f_y);
                f_y = NULL;
            }
            continue;
        }
        cx_copy_middle = cx;    /* remember the middle byte for recreation */
        cx = (cx + 1) % 256;    /* change middle byte */
        if (rm_fpwrite(&cx, sizeof(unsigned char), 1, half_sz, f_x) != 1) {
            RM_LOG_ERR("Error writing to file [%s], skipping this test", buf_x_name);
            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
            if (f_y != NULL) {
                fclose(f_y);
                f_y = NULL;
            }
            continue;
        }
        if (rm_fpread(&cx, sizeof(unsigned char), 1, f_x_sz - 1, f_x) != 1) { /* read last byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
            if (f_y != NULL) {
                fclose(f_y);
                f_y = NULL;
            }
            continue;
        }
        cx_copy_last = cx;      /* remember the last byte for recreation */
        cx = (cx + 1) % 256;    /* change last byte */
        if (rm_fpwrite(&cx, sizeof(unsigned char), 1, f_x_sz - 1, f_x) != 1) {
            RM_LOG_ERR("Error writing to file [%s], skipping this test", buf_x_name);
            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
            if (f_y != NULL) {
                fclose(f_y);
                f_y = NULL;
            }
            continue;
        }

        detail_case_1_n = 0;
        detail_case_2_n = 0;
        detail_case_3_n = 0;
        j = 0;
        for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
            L = rm_test_L_blocks[j];
            RM_LOG_INFO("Validating testing #5 of local push [first, last, middle bytes changed], file [%s], size [%zu], block size L [%zu], half_sz [%zu]", f_y_name, f_y_sz, L, half_sz);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            RM_LOG_INFO("Testing local push #5 [first, last, middle bytes changed]: file @x[%s] size [%zu] file @y[%s], size [%zu], block size L [%zu], half_sz [%zu]", buf_x_name, f_x_sz, f_y_name, f_y_sz, L, half_sz);
            copy_all_threshold = 0;
            copy_tail_threshold = 0;
            send_threshold = L;
            flags = 0;

            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
            if (f_y != NULL) {
                fclose(f_y);
                f_y = NULL;
            }
            memset(&rec_ctx, 0, sizeof (struct rm_delta_reconstruct_ctx));
            status = rm_tx_local_push(buf_x_name, f_y_name, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx);
            assert_int_equal(status, RM_ERR_OK);

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
                if (f_x != NULL) {
                    fclose(f_x);
                    f_x = NULL;
                }
            }
            assert_true(f_y != NULL && "Can't open file @y");
            fd_y = fileno(f_y);

            /* verify files size */
            memset(&fs, 0, sizeof(fs)); /* get @x size */
            if (fstat(fd_x, &fs) != 0) {
                RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
                if (f_x != NULL) {
                    fclose(f_x);
                    f_x = NULL;
                }
                if (f_y != NULL) {
                    fclose(f_y);
                    f_y = NULL;
                }
                assert_true(1 == 0);
            }
            f_x_sz = fs.st_size;
            memset(&fs, 0, sizeof(fs));
            if (fstat(fd_y, &fs) != 0) {
                RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
                if (f_x != NULL) {
                    fclose(f_x);
                    f_x = NULL;
                }
                if (f_y != NULL) {
                    fclose(f_y);
                    f_y = NULL;
                }
                assert_true(1 == 0);
            }
            f_y_sz = fs.st_size;
            assert_true(f_x_sz == f_y_sz && "File sizes differ!");

            k = 0;
            while (k < f_x_sz) {
                if (rm_fpread(&cx, sizeof(unsigned char), 1, k, f_x) != 1) {
                    RM_LOG_CRIT("Error reading file [%s]!", buf_x_name);
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
                if (rm_fpread(&cz, sizeof(unsigned char), 1, k, f_y) != 1) {
                    RM_LOG_CRIT("Error reading file [%s]!", f_y_name);
                    if (f_x != NULL) {
                        fclose(f_x);
                        f_x = NULL;
                    }
                    if (f_y != NULL) {
                        fclose(f_y);
                        f_y = NULL;
                    }
                    assert_true(1 == 0 && "ERROR reading byte in file @z!");
                }
                if (cx != cz) {
                    RM_LOG_CRIT("Bytes [%zu] differ: cx [%zu], cz [%zu]\n", k, cx, cz);
                }
                assert_true(cx == cz && "Bytes differ!");
                ++k;
            }
            if ((err = rm_file_cmp(f_x, f_y, 0, 0, f_x_sz)) != RM_ERR_OK) {
                RM_LOG_ERR("Bytes differ, err [%d]", err);
                assert_true(1 == 0);
            }

            if (RM_TEST_8_DELETE_FILES == 1) { /* and fclose/unlink/remove result file */
				if (f_y) {
					fclose(f_y);
				}
                if (unlink(f_y_name) != 0) {
                    RM_LOG_ERR("Can't unlink result file [%s]", f_y_name);
                    if (f_x != NULL) {
                        fclose(f_x);
                        f_x = NULL;
                    }
                    if (f_y != NULL) {
                        fclose(f_y);
                        f_y = NULL;
                    }
                    assert_true(1 == 0 && "Can't unlink");
                }
            }
            /* detail cases */
            /* 1. if L is >= file size, delta must be single RAW element */
            if (L >= f_x_sz) {
                assert_true(rec_ctx.delta_ref_n == 0);
                assert_true(rec_ctx.delta_tail_n == 0);
                assert_true(rec_ctx.delta_raw_n == f_y_sz / send_threshold + (f_y_sz % send_threshold ? 1 : 0));
                assert_true(rec_ctx.rec_by_ref == 0);
                assert_true(rec_ctx.rec_by_tail == 0);
                assert_true(rec_ctx.rec_by_raw == f_y_sz);
                ++detail_case_1_n;
            }
            /* 2. if L is less than file size and does divide evenly file size, there will usually be f_y_sz/L - 3 delta reference blocks present
             * and 3 raw bytes blocks because the first, middle and last blocks in @x doesn't match corresponding blocks in @y (all other blocks in @x always match corresponding blocks in @y in this test),
             * and none of the checksums computed by rolling checksum procedure starting from second byte in @x up to Lth byte, i.e. on blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will match some of the nonoverlapping checksums from @y.
             * But there is a chance one of nonoverlapping blocks in @y will match first and/or middle and/or last block in @x (which has first, middle and last bytes changed), e.g if L is 1, size is 100 and @y file is 0x1 0x2 0x3 0x78 0x5 ...
             * the @x file is then 0x78 0x2 0x3 0x78 0x5 ... and first block will match 4th block in @y. There is also a chance this won't match BUT some of blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will find a match and rolling proc will move on offsets different that nonoverlapping blocks. All next f_y_sz/L - 2 blocks may match or not and up to 3*L bytes will be sent by raw elements */
            if ((L < f_y_sz) && (f_y_sz % L == 0)) {
                assert_true(rec_ctx.delta_tail_n == 0);
                assert_true(rec_ctx.rec_by_raw <= 3 * L);
                assert_true(rec_ctx.delta_raw_n <= f_y_sz);
                if (f_y_sz <= (3 * L)) {
                    if (rec_ctx.delta_ref_n == 0) {
                        assert_true(rec_ctx.rec_by_ref == 0 && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n == (f_y_sz / send_threshold + (f_y_sz % send_threshold ? 1 : 0)) && rec_ctx.rec_by_raw == f_y_sz);
                    } else {
                        assert_true(rec_ctx.delta_ref_n > 0 && rec_ctx.rec_by_ref == (rec_ctx.delta_ref_n * L) && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n <= 3 * L && rec_ctx.rec_by_raw == (f_y_sz - rec_ctx.rec_by_ref));
                    }
                } else {
                    if (rec_ctx.delta_ref_n == f_y_sz / L - 3) {
                        if (rec_ctx.rec_by_ref != f_y_sz - 3 * L) {
                            test_rm_dump(rec_ctx);
                        }
                        assert_true(rec_ctx.rec_by_ref == f_y_sz - 3 * L);
                        assert_true(rec_ctx.delta_raw_n > 0);
                        if (rec_ctx.delta_raw_n > 3 * L) {
                            test_rm_dump(rec_ctx);
                        }
                        assert_true(rec_ctx.delta_raw_n <= 3 * L);
                        if (rec_ctx.rec_by_raw != 3 * L) {
                            test_rm_dump(rec_ctx);
                        }
                        assert_true(rec_ctx.rec_by_raw == 3 * L);
                    } else if (rec_ctx.delta_ref_n == (f_y_sz / L - 2)) {
                       assert_true(rec_ctx.rec_by_ref == (f_y_sz - 2 * L) && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n <= 2 * L  && rec_ctx.rec_by_raw == 2 * L);
                    } else if (rec_ctx.delta_ref_n == (f_y_sz / L - 1)) {
                       assert_true(rec_ctx.rec_by_ref == (f_y_sz - 1 * L) && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n <= 1 * L  && rec_ctx.rec_by_raw == 1 * L);
                    } else {
                       assert_true(rec_ctx.delta_ref_n == f_y_sz / L && rec_ctx.rec_by_ref == f_y_sz && rec_ctx.delta_raw_n == 0 && rec_ctx.rec_by_raw == 0); /* the last (tail) block in @x will not match the last block in @y, but it can match some other block in @y, same with first and middle block */
                    }
                }
                assert_true(rec_ctx.delta_ref_n * L == f_y_sz - rec_ctx.rec_by_raw);
                assert_true(rec_ctx.delta_tail_n == 0); /* impossible as L divides @y evenly */
                assert_true(rec_ctx.rec_by_tail == 0);
                ++detail_case_2_n;
            }
            /* 3. if L is less than file size and doesn't divide evenly file size, there can be delta TAIL reference block present (this cannot happen in this test),
             * regarding delta reference blocks it is the same situation as in test #2 (plus remember that TAIL is also counted as reference). */
            if ((L < f_y_sz) && (f_y_sz % L != 0)) {
                assert_true(rec_ctx.rec_by_raw <= 3 * L);
                if (f_y_sz <= (3 * L)) {
                    if (rec_ctx.delta_ref_n == 0) {
                        assert_true(rec_ctx.rec_by_ref == 0 && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n == (f_y_sz / send_threshold + (f_y_sz % send_threshold ? 1 : 0)) && rec_ctx.rec_by_raw == f_y_sz);
                    } else {
                        assert_true(rec_ctx.delta_ref_n > 0 && rec_ctx.rec_by_ref == (rec_ctx.delta_ref_n * L) && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n <= 2 * L && rec_ctx.rec_by_raw == (f_y_sz - rec_ctx.rec_by_ref));
                    }
                } else {
                    if (rec_ctx.delta_ref_n == (f_y_sz / L + 1 - 3)) {
                        assert_true(rec_ctx.rec_by_ref == (L * (f_y_sz / L + 1 - 3)));
                        assert_true(rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n <= 2 * L + (f_y_sz % L));
                        assert_true(rec_ctx.rec_by_raw == (2 * L + f_y_sz % L));
                    } else if (rec_ctx.delta_ref_n == (f_y_sz / L + 1 - 2)) {
                        assert_true(rec_ctx.rec_by_ref == (L * (f_y_sz / L + 1 - 2)));
                        assert_true(rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n <= L + (f_y_sz % L));
                        assert_true(rec_ctx.rec_by_raw == (L + f_y_sz % L));
                    } else if (rec_ctx.delta_ref_n == (f_y_sz / L + 1 - 1)) {
                       assert_true((rec_ctx.rec_by_ref == (f_y_sz - L) || rec_ctx.rec_by_ref == (f_y_sz - f_y_sz % L)) && rec_ctx.delta_raw_n > 0 && rec_ctx.delta_raw_n <= L && (rec_ctx.rec_by_raw == L || rec_ctx.rec_by_raw == f_y_sz % L));
                    } else {
                        assert_true(1 == 0 && "Got only delta reference blocks but TAIL can't match in this test!"); 
                    }
                }
                assert_true(rec_ctx.rec_by_ref == f_y_sz - rec_ctx.rec_by_raw);
                ++detail_case_3_n;
            }
            /* 4. There can't be DELTA TAIL elements in this test as tail of @x will never match potential tail element in @y as last bytes in files differ */
            assert_true(rec_ctx.rec_by_tail == 0 && rec_ctx.delta_tail_n == 0);
            RM_LOG_INFO("PASSED test #5: files [%s] [%s], block [%zu], passed delta reconstruction, files are the same", buf_x_name, f_y_name, L);

            f_y_name = rm_test_fnames[i]; /* recreate @y file as input to local push */
            f_y = fopen(f_y_name, "wb+");
            if (f_y == NULL) {
                RM_LOG_PERR("Can't recreate file [%s]", f_y_name);
            }
            assert_true(f_y != NULL && "Can't recreate @y file");
            if (rm_copy_buffered(f_x, f_y, rm_test_fsizes[i]) != RM_ERR_OK) {
                RM_LOG_ERR("%s", "Error copying file @x to @y for next test");
                if (f_x != NULL) {
                    fclose(f_x);
                    f_x = NULL;
                }
                if (f_y != NULL) {
                    fclose(f_y);
                    f_y = NULL;
                }
                assert_true(1 == 0 && "Error copying file @x to @y for next test");
            }
            if (rm_fpwrite(&cx_copy_first, sizeof(unsigned char), 1, 0, f_y) != 1) {
                RM_LOG_ERR("Error writing to file [%s], skipping this test", f_y_name);
                if (f_x != NULL) {
                    fclose(f_x);
                    f_x = NULL;
                }
                if (f_y != NULL) {
                    fclose(f_y);
                    f_y = NULL;
                }
                continue;
            }
            if (rm_fpwrite(&cx_copy_middle, sizeof(unsigned char), 1, half_sz, f_y) != 1) {
                RM_LOG_ERR("Error writing to file [%s], skipping this test", f_y_name);
                if (f_x != NULL) {
                    fclose(f_x);
                    f_x = NULL;
                }
                if (f_y != NULL) {
                    fclose(f_y);
                    f_y = NULL;
                }
                continue;
            }
            if (rm_fpwrite(&cx_copy_last, sizeof(unsigned char), 1, f_x_sz - 1, f_y) != 1) {
                RM_LOG_ERR("Error writing to file [%s], skipping this test", f_y_name);
                if (f_x != NULL) {
                    fclose(f_x);
                    f_x = NULL;
                }
                if (f_y != NULL) {
                    fclose(f_y);
                    f_y = NULL;
                }
                continue;
            }
        }
        if (f_x != NULL) {
            fclose(f_x);
            f_x = NULL;
        }
        if (f_y != NULL) {
            fclose(f_y);
            f_y = NULL;
        }
        RM_LOG_INFO("PASSED test #5: files [%s] [%s] passed delta reconstruction for all block sizes, files are the same (detail cases: #1 [%zu] #2 [%zu] #3 [%zu])",
                buf_x_name, f_y_name, detail_case_1_n, detail_case_2_n, detail_case_3_n);
	}

    if (RM_TEST_8_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_5");
        if (err != 0) {
            RM_LOG_ERR("%s", "Error removing files (unlink)");
            if (f_x != NULL) {
                fclose(f_x);
                f_x = NULL;
            }
            if (f_y != NULL) {
                fclose(f_y);
                f_y = NULL;
            }
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}

/* @brief   Test #6. */
/* @brief   Test if result file @f_z is reconstructed properly
 *          when @y doesn't exist and copy buffered is used. */
void
test_rm_tx_local_push_6(void **state) {
    int                     err;
    enum rm_error           status;
    char                    buf_x_name[RM_FILE_LEN_MAX + 50];   /* @x (copy of @y with changed single byte at the beginning) */
    unsigned char           cx, cz;
    const char              *f_y_name;  /* @y name */
    FILE                    *f, *f_copy, *f_x, *f_y;
    int                     fd_x, fd_y;
    size_t                  i, j, k, L, f_x_sz, f_y_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    rm_push_flags                   flags;
    size_t                          copy_all_threshold, copy_tail_threshold, send_threshold;
    struct rm_delta_reconstruct_ctx rec_ctx;

    err = test_rm_copy_files_and_postfix("_test_6");
    if (err != 0) {
        RM_LOG_ERR("%s", "Error copying files, skipping test");
        return;
    }
    f_x = NULL;
    f_y = NULL;
    f_copy = NULL;
    i = 0; /* delete all test files */
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f = fopen(rm_test_fnames[i], "wb+");
        if (f == NULL) {
            RM_LOG_ERR("Can't open file [%s]", rm_test_fnames[i]);
        } else {
            RM_LOG_INFO("Removing file [%s]", rm_test_fnames[i]);
			fclose(f);
            remove(rm_test_fnames[i]);
        }
    }

    rm_state = *state;
    assert_true(rm_state != NULL);

    i = 0;  /* test on all files */
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f_y_name = rm_test_fnames[i];
        /* change byte in copy */
        strncpy(buf_x_name, f_y_name, RM_FILE_LEN_MAX);
        strncpy(buf_x_name + strlen(buf_x_name), "_test_6", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
        }
        assert_true(f_copy != NULL && "Can't open copy of file");
        f_x = f_copy;
        fd_x = fileno(f_x);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_x, &fs) != 0) {    /* get @x size */
            RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
            fclose(f_x);
            assert_true(1 == 0);
        }
        f_x_sz = fs.st_size;
        if (f_x_sz < 2) {
            RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", buf_x_name, f_x_sz);
            fclose(f_x);
            continue;
        }
        fclose(f_x);

        j = 0;
        for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
            L = rm_test_L_blocks[j];
            RM_LOG_INFO("Validating testing #6 of local push [copy buffered], file [%s], size [%zu], block size L [%zu]", buf_x_name, f_x_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, buf_x_name);
                continue;
            }
            if (f_x_sz < 2) {
                RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", buf_x_name, f_x_sz);
                continue;
            }
            RM_LOG_INFO("Testing local push #6 [copy buffered]: file @x[%s] size [%zu] file @y[%s], size [%zu], block size L [%zu]", buf_x_name, f_x_sz, f_y_name, f_x_sz, L);
            copy_all_threshold = 0;
            copy_tail_threshold = 0;
            send_threshold = L;
            flags = RM_BIT_4; /* set force creation flag */

            memset(&rec_ctx, 0, sizeof (struct rm_delta_reconstruct_ctx));
            status = rm_tx_local_push(buf_x_name, f_y_name, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx);
            assert_int_equal(status, RM_ERR_OK);

            assert_true(rec_ctx.method == RM_RECONSTRUCT_METHOD_COPY_BUFFERED);

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
                    RM_LOG_CRIT("Bytes [%zu] differ: cx [%zu], cz [%zu]\n", k, cx, cz);
                }
                assert_true(cx == cz && "Bytes differ!");
                ++k;
            }
            if ((err = rm_file_cmp(f_x, f_y, 0, 0, f_x_sz)) != RM_ERR_OK) {
                RM_LOG_ERR("Bytes differ, err [%d]", err);
                assert_true(1 == 0);
            }

            if (RM_TEST_8_DELETE_FILES == 1) { /* and fclose/unlink/remove result file */
				if (f_y != NULL) {
					fclose(f_y);
					f_y = NULL;
				}
                if (unlink(f_y_name) != 0) {
                    RM_LOG_ERR("Can't unlink result file [%s]", f_y_name);
                    assert_true(1 == 0);
                }
            }
			if (f_x != NULL) {
				fclose(f_x);
				f_x = NULL;
			}
            RM_LOG_INFO("PASSED test #6: files [%s] [%s], block [%zu], passed delta reconstruction, files are the same", buf_x_name, f_y_name, L);

		}
		if (f_x != NULL) {
			fclose(f_x);
			f_x = NULL;
		}
        if (f_y != NULL) {
			fclose(f_y);
			f_x = NULL;
		}
        RM_LOG_INFO("PASSED test #6 (copy buffered): files [%s] [%s] passed delta reconstruction for all block sizes, files are the same", buf_x_name, f_y_name);
	}

    if (RM_TEST_8_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_6");
        if (err != 0) {
            RM_LOG_ERR("%s", "Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}
