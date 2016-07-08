/* @file        test_rm10.c
 * @brief       Test suite #10.
 * @details     Test of commandline utility.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        8 July 2016 07:08 PM
 * @copyright   LGPLv2.1 */


#include "test_rm10.h"


const char* rm_test_fnames[RM_TEST_FNAMES_N] = {
    "rm_f_1_ts10", "rm_f_2_ts10","rm_f_4_ts10", "rm_f_8_ts10", "rm_f_65_ts10",
    "rm_f_100_ts10", "rm_f_511_ts10", "rm_f_512_ts10", "rm_f_513_ts10", "rm_f_1023_ts10",
    "rm_f_1024_ts10", "rm_f_1025_ts10", "rm_f_4096_ts10", "rm_f_7787_ts10", "rm_f_20100_ts10"};

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
                case 0:
                    break;
                case -2:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s]", buf, rm_test_fnames[i]);
                    if (f != NULL) {
						fclose(f);
					}
                    if (f_copy != NULL) {
						fclose(f_copy);
					}
                    return -2;
                case -3:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s], error set on original file", buf, rm_test_fnames[i]);
                    if (f != NULL) {
						fclose(f);
					}
                    if (f_copy != NULL) {
						fclose(f_copy);
					}
                    return -3;
                case -4:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s], error set on copy", buf, rm_test_fnames[i]);
                    if (f != NULL) {
						fclose(f);
					}
                    if (f_copy != NULL) {
						fclose(f_copy);
					}
                    return -4;
                default:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s], unknown error", buf, rm_test_fnames[i]);
                    if (f != NULL) {
						fclose(f);
					}
                    if (f_copy != NULL) {
						fclose(f_copy);
					}
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
    int         err = 0;
    size_t      i,j;
    FILE        *f;
    unsigned long seed;

    (void) state;
   
#ifdef DEBUG
    err = rm_util_chdir_umask_openlog("../build/debug", 1, "rsyncme_test_10", 0);
#else
    err = rm_util_chdir_umask_openlog("../build/release", 1, "rsyncme_test_10", 0);
#endif
    
    if (err != 0) {
        exit(EXIT_FAILURE);
    }

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
    return 0;
}

int
test_rm_teardown(void **state) {
    size_t  i;
    FILE    *f;
    (void) state;

    if (RM_TEST_10_DELETE_FILES == 1) {
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
    return 0;
}

/* @brief   Test if result file @f_z is reconstructed properly
 *          when x file is same as y (file has no changes),
 *          block size -l is not set (use default setting) */
void
test_rm_cmd_1(void **state) {
    (void) state;
    return;
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
 *          when x file is same as y (file has no changes),
 *          block size -l is set */
void
test_rm_cmd_2(void **state) {
    int                     err;
    enum rm_error           status = RM_ERR_OK;
    char                    buf_x_name[RM_FILE_LEN_MAX + 50];   /* @x (copy of @y with changed single byte at the beginning) */
    char                    cmd[RM_TEST_10_CMD_LEN_MAX];        /* command to execute in shell */
    const char              *f_y_name; /* @y name */
    unsigned char           cx, cz;
    FILE                    *f_copy, *f_x, *f_y;
    int                     fd_x, fd_y;
    size_t                  i, j, k, L, f_x_sz, f_y_sz;
    struct stat             fs;

    (void) state;

    err = test_rm_copy_files_and_postfix("_test_1");
    if (err != 0) {
        RM_LOG_ERR("%s", "Error copying files, skipping test");
        return;
    }

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

        j = 0;
        for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
            L = rm_test_L_blocks[j];
            RM_LOG_INFO("Validating testing #1(commandline utility - local push), file [%s], size [%zu], block size L [%zu]", f_y_name, f_y_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            if (f_y_sz < 2) {
                RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
                continue;
            }
            RM_LOG_INFO("Testing local push via commandline utility #1: file @x[%s] size [%zu] file @y[%s], size [%zu], block size L [%zu]", buf_x_name, f_x_sz, f_y_name, f_y_sz, L);

            if (f_x != NULL) fclose(f_x);
            if (f_y != NULL) fclose(f_y);
            snprintf(cmd, RM_TEST_10_CMD_LEN_MAX, "./rsyncme push -x %s -y %s -l %zu", buf_x_name, f_y_name, L); /* execute built image of rsyncme from debug/release build folder and not from system global path */
            status = system(cmd);
            assert_int_equal(status, RM_ERR_OK);

            /* general tests */
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

            k = 0; /* verify files content */
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
            /* don't unlink/remove result file, as it is just the same as @x and can be reused */
            RM_LOG_INFO("PASSED test #1 (commandline utility - local push): files [%s] [%s], block [%zu], files are the same", buf_x_name, f_y_name, L);
            /* no need to recreate @y file as input to local push in this test, as @y stays the same all the time */
        }
        if (f_x != NULL) fclose(f_x);
        f_x = NULL;
        if (f_y != NULL) fclose(f_y);
        f_y = NULL;
        RM_LOG_INFO("PASSED test #1 (commandline utility - local push): files [%s] [%s] passed delta reconstruction for all block sizes, files are the same", buf_x_name, f_y_name);
	}

    if (RM_TEST_10_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_1");
        if (err != 0) {
            RM_LOG_ERR("%s", "Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}
