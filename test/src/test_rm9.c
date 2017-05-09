/* @file        test_rm9.c
 * @brief       Test suite #9.
 * @details     Test of local push error reporting.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        20 June 2016 04:48 PM
 * @copyright   LGPLv2.1 */


#include "test_rm9.h"


enum rm_loglevel RM_LOGLEVEL = RM_LOGLEVEL_NORMAL;

const char* rm_test_fnames[RM_TEST_FNAMES_N] = { 
    "rm_f_1_ts9", "rm_f_2_ts9","rm_f_4_ts9", "rm_f_8_ts9", "rm_f_65_ts9",
    "rm_f_100_ts9", "rm_f_511_ts9", "rm_f_512_ts9", "rm_f_513_ts9", "rm_f_1023_ts9",
    "rm_f_1024_ts9", "rm_f_1025_ts9", "rm_f_4096_ts9", "rm_f_7787_ts9", "rm_f_20100_ts9"};

size_t rm_test_fsizes[RM_TEST_FNAMES_N] = { 1, 2, 4, 8, 65,
    100, 511, 512, 513, 1023,
    1024, 1025, 4096, 7787, 20100 };

size_t rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE] = { 1, 2, 3, 4, 8, 10, 13, 16,
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

    f = NULL;
    f_copy = NULL;
    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f = fopen(rm_test_fnames[i], "rb");
        if (f == NULL) { /* if file doesn't exist, create */
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
        if (f_copy == NULL) { /* if doesn't exist, create */
            RM_LOG_INFO("Creating copy [%s] of file [%s]", buf, rm_test_fnames[i]);
            f_copy = fopen(buf, "wb");
            if (f_copy == NULL) {
                if (f != NULL) {
                    fclose(f);
                    f = NULL;
                }
                RM_LOG_ERR("Can't open [%s] copy of file [%s]", buf, rm_test_fnames[i]);
                return -1;
            }
            err = rm_copy_buffered(f, f_copy, rm_test_fsizes[i], NULL);
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
            if (f != NULL) {
                fclose(f);
                f = NULL;
            }
            if (f_copy != NULL) {
                fclose(f_copy);
                f_copy = NULL;
            }
        } else {
            RM_LOG_INFO("Using previously created copy of file [%s]", rm_test_fnames[i]);
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
        /* open copy */
        strncpy(buf, rm_test_fnames[i], RM_FILE_LEN_MAX);
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

int RM_TEST_MOCK_FOPEN      = 0;
int RM_TEST_MOCK_FOPEN64    = 0;

int
test_rm_setup(void **state) {
    int         err = -1;
    size_t      i = 0, j = 0;
    FILE        *f = NULL;
    void        *buf = NULL;
    struct rm_session   *s = NULL;
    unsigned long const seed = time(NULL);
	struct rm_core_options opt = { .loglevel = RM_LOGLEVEL_NORMAL };

#ifdef DEBUG
    err = rm_util_chdir_umask_openlog("../build/debug", 1, "rsyncme_test_9", 1);
#else
    err = rm_util_chdir_umask_openlog("../build/release", 1, "rsyncme_test_9", 1);
#endif
    if (err != RM_ERR_OK) {
        exit(EXIT_FAILURE);
    }
    rm_state.l = rm_test_L_blocks;
    *state = &rm_state;
    f = NULL;

    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f = fopen(rm_test_fnames[i], "rb+");
        if (f == NULL) {
            RM_LOG_INFO("Creating file [%s]", rm_test_fnames[i]); /* file doesn't exist, create */
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
        if (f != NULL) {
            fclose(f);
            f = NULL;
        }
    }

    i = 0; /* find biggest L */
    j = 0;
    for (; i < RM_TEST_L_BLOCKS_SIZE; ++i) {
        if (rm_test_L_blocks[i] > j) {
            j = rm_test_L_blocks[i];
        }
    }
    buf = malloc(j);
    if (buf == NULL) {
        RM_LOG_ERR("Can't allocate 1st memory buffer"
                " of [%zu] bytes, malloc failed", j);
    }
    assert_true(buf != NULL);
    rm_state.buf = buf;
    buf = malloc(j);
    if (buf == NULL) {
        RM_LOG_ERR("Can't allocate 2nd memory buffer of [%zu] bytes, malloc failed", j);
        if (f != NULL) {
            fclose(f);
            f = NULL;
        }
    }
    assert_true(buf != NULL && "Can't malloc buffer");
    rm_state.buf2 = buf;

    s = rm_session_create(RM_PUSH_LOCAL, &opt); /* session for local push */
    if (s == NULL) {
        RM_LOG_ERR("%s", "Can't allocate session local push");
        if (f != NULL) {
            fclose(f);
            f = NULL;
        }
    }
    assert_true(s != NULL && "Can't create session");
    rm_state.s = s;
    rm_get_unique_string(rm_state.unique_name);
    return 0;
}

int
test_rm_teardown(void **state) {
    size_t  i;
    FILE    *f;
    struct  test_rm_state *rm_state;

    rm_state = *state;
    assert_true(rm_state != NULL);
    f = NULL;
    if (RM_TEST_9_DELETE_FILES == 1) { /* delete all test files */
        i = 0;
        for (; i < RM_TEST_FNAMES_N; ++i) {
            f = fopen(rm_test_fnames[i], "wb+");
            if (f == NULL) {
                RM_LOG_ERR("Can't open file [%s]", rm_test_fnames[i]);	
            } else {
                RM_LOG_INFO("Removing file [%s]", rm_test_fnames[i]);
                if (f != NULL) {
                    fclose(f);
                    f = NULL;
                }
                remove(rm_test_fnames[i]);
            }
        }
    }
    free(rm_state->buf);
    free(rm_state->buf2);
    rm_session_free(rm_state->s);
    return 0;
}

FILE*
__wrap_fopen(const char *path, const char *mode) {
    FILE* ret;
    if (RM_TEST_MOCK_FOPEN == 0) {
        return __real_fopen(path, mode);
    }
    ret = mock_type(void*);
    return ret;
}

FILE*
__wrap_fopen64(const char *path, const char *mode) {
    FILE* ret;
    if (RM_TEST_MOCK_FOPEN64 == 0) {
        return __real_fopen64(path, mode);
    }
    ret = mock_type(void*);
    return ret;
}

/* @brief   Test NULL file pointers */
void
test_rm_local_push_err_1(void **state) {
    int                     err;
    enum rm_error           status;
    char                    buf_x_name[RM_FILE_LEN_MAX + 50];   /* @x (copy of @y but with bytes changed) */
    const char              *f_y_name;  /* @y name */
    FILE                    *f_x, *f_copy, *f_y;
    int                     fd_y;
    size_t                  i, j, L, f_y_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    rm_push_flags                   flags;
    size_t                          copy_all_threshold, copy_tail_threshold, send_threshold;
    struct rm_delta_reconstruct_ctx rec_ctx;
	struct rm_tx_options opt = { .loglevel = RM_LOGLEVEL_NORMAL };

    err = test_rm_copy_files_and_postfix("_test_9");
    if (err != 0) {
        RM_LOG_ERR("%s", "Error copying files, skipping test");
        return;
    }

    rm_state = *state;
    assert_true(rm_state != NULL);
    f_x = NULL;
    f_y = NULL;
    f_copy = NULL;

    i = 0; /* test on all files */
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f_y_name = rm_test_fnames[i];
        f_y = fopen(f_y_name, "rb");
        if (f_y == NULL) {
            RM_LOG_PERR("Can't open file [%s]", f_y_name);
        }
        assert_true(f_y != NULL && "Can't open file @y");
        fd_y = fileno(f_y);
        if (fstat(fd_y, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
            if (f_y != NULL) {
                fclose(f_y);
                f_y = NULL;
            }
            assert_true(1 == 0 && "Can't fstat @y file");
        }
        f_y_sz = fs.st_size; 
        if (f_y_sz < 1) {
            RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
            if (f_y != NULL) {
                fclose(f_y);
                f_y = NULL;
            }
            continue;
        }
        if (f_y != NULL) {
            fclose(f_y);
            f_y = NULL;
        }
        strncpy(buf_x_name, f_y_name, RM_FILE_LEN_MAX);
        strncpy(buf_x_name + strlen(buf_x_name), "_test_9", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
        }
        if (f_copy != NULL) {
            fclose(f_copy);
            f_copy = NULL;
        }
        j = 0;
        for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
            L = rm_test_L_blocks[j];
            RM_LOG_INFO("Validating testing #1 (NULL file pointers), file [%s], size [%zu], block size L [%zu]", f_y_name, f_y_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            RM_LOG_INFO("Testing #1 (NULL file pointers): file [%s], size [%zu], block size L [%zu]", f_y_name, f_y_sz, L);

            copy_all_threshold = 0;
            copy_tail_threshold = 0;
            send_threshold = 1;
            flags = 0;

            flags = 0;
            status = rm_tx_local_push(NULL, NULL, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx, &opt);
            assert_int_equal(status, RM_ERR_BAD_CALL);

            flags = 0;
            status = rm_tx_local_push(NULL, f_y_name, rm_state->unique_name, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx, &opt);
            assert_int_equal(status, RM_ERR_BAD_CALL);

            flags = 0;
            status = rm_tx_local_push(rm_state->unique_name, f_y_name, rm_state->unique_name, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, NULL, &opt);
            assert_int_equal(status, RM_ERR_BAD_CALL);

            flags = 0;
            status = rm_tx_local_push(rm_state->unique_name, f_y_name, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx, &opt);
            assert_int_equal(status, RM_ERR_OPEN_X);

            flags = 0;
            status = rm_tx_local_push(NULL, f_y_name, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx, &opt);
            assert_int_equal(status, RM_ERR_BAD_CALL);

            flags = 0;
            status = rm_tx_local_push(buf_x_name, NULL, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx, &opt);
            assert_int_equal(status, RM_ERR_BAD_CALL);

            flags = 0;
            RM_TEST_MOCK_FOPEN64 = 1;
            will_return(__wrap_fopen64, NULL);
            status = rm_tx_local_push(rm_state->unique_name, f_y_name, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx, &opt);
            RM_TEST_MOCK_FOPEN64 = 0;
            assert_int_equal(status, RM_ERR_OPEN_X);

            flags = 0; /* RM_BIT_4 (force creation) bit not set, expected RM_ERR_OPEN_Y */
            f_x = fopen(buf_x_name, "rb");  /* open @x because local push will attempt to close it using returned pointer from mocked fopen64, fclose will SIGSEGV otherwise */
            assert_true(f_x != NULL && "Can't open file @x");
            RM_TEST_MOCK_FOPEN64 = 1;
            will_return(__wrap_fopen64, f_x);
            will_return(__wrap_fopen64, NULL);
            status = rm_tx_local_push(buf_x_name, rm_state->unique_name, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx, &opt);
            RM_TEST_MOCK_FOPEN64 = 0;
            assert_int_equal(status, RM_ERR_OPEN_Y);
            f_x = NULL;

            flags |= RM_BIT_4; /* RM_BIT_4 (force creation) bit set but call to fopen fails, expected RM_ERR_OPEN_Z (need to mock three calls to fopen) */
            f_x = fopen(buf_x_name, "rb");  /* open @x because local push will attempt to close it using returned pointer from mocked fopen64, fclose will SIGSEGV otherwise */
            assert_true(f_x != NULL);
            RM_TEST_MOCK_FOPEN64 = 1;
            will_return(__wrap_fopen64, f_x);
            will_return(__wrap_fopen64, NULL);
            will_return(__wrap_fopen64, NULL);
            status = rm_tx_local_push(buf_x_name, f_y_name, NULL, L, copy_all_threshold, copy_tail_threshold, send_threshold, flags, &rec_ctx, &opt);
            RM_TEST_MOCK_FOPEN64 = 0;
            assert_int_equal(status, RM_ERR_OPEN_Z);
            f_x = NULL;
        }
        RM_LOG_INFO("PASSED test #1 (NULL file pointers), file [%s], size [%zu]", f_y_name, f_y_sz);
    }
    if (RM_TEST_9_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_9");
        if (err != 0) {
            RM_LOG_ERR("%s", "Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
        }
    }
    return;
}
