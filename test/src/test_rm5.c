/* @file        test_rm5.c
 * @brief       Test suite #5.
 * @details     Test of rm_rolling_ch_proc.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        06 May 2016 04:00 PM
 * @copyright   LGPLv2.1 */


#include "test_rm5.h"


const char* rm_test_fnames[RM_TEST_FNAMES_N] = { 
    "rm_f_1_ts5", "rm_f_2_ts5","rm_f_4_ts5", "rm_f_8_ts5", "rm_f_65_ts5",
    "rm_f_100_ts5", "rm_f_511_ts5", "rm_f_512_ts5", "rm_f_513_ts5", "rm_f_1023_ts5",
    "rm_f_1024_ts5", "rm_f_1025_ts5", "rm_f_4096_ts5", "rm_f_7787_ts5", "rm_f_20100_ts5"};

size_t  rm_test_fsizes[RM_TEST_FNAMES_N] = { 1, 2, 4, 8, 65,
    100, 511, 512, 513, 1023,
    1024, 1025, 4096, 7787, 20100 };

size_t  rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE] = { 1, 2, 3, 4, 8, 10, 13, 16,
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
        if (f == NULL) {
            /* file doesn't exist, create */
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
        /* create copy */
        strncpy(buf, rm_test_fnames[i], RM_FILE_LEN_MAX);
        strncpy(buf + strlen(buf), postfix, 49);
        buf[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf, "rb+");
        if (f_copy == NULL) { /* if doesn't exist, create */
            RM_LOG_INFO("Creating copy [%s] of file [%s]", buf, rm_test_fnames[i]);
            f_copy = fopen(buf, "wb");
            if (f_copy == NULL) {
                RM_LOG_ERR("Can't open [%s] copy of file [%s]", buf, rm_test_fnames[i]);
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
    unsigned long const seed = time(NULL);

#ifdef DEBUG
    err = rm_util_chdir_umask_openlog("../build/debug", 1, "rsyncme_test_5", 1);
#else
    err = rm_util_chdir_umask_openlog("../build/release", 1, "rsyncme_test_5", 1);
#endif
    if (err != RM_ERR_OK) {
        exit(EXIT_FAILURE);
    }
    rm_state.l = rm_test_L_blocks;
    *state = &rm_state;

    assert_true(RM_TEST_5_9_FILE_IDX >= 0 && RM_TEST_5_9_FILE_IDX < RM_TEST_FNAMES_N);

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
        fclose(f);
    }

    /* find biggest L */
    i = 0;
    j = 0;
    for (; i < RM_TEST_L_BLOCKS_SIZE; ++i) {
        if (rm_test_L_blocks[i] > j) {
            j = rm_test_L_blocks[i];
        }
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

    s = rm_session_create(RM_PUSH_LOCAL); /* session for local push */
    if (s == NULL) {
        RM_LOG_ERR("%s", "Can't allocate session local push");
    }
    assert_true(s != NULL);
    rm_state.s = s;

    rm_get_unique_string(rm_state.f.name);
    f = fopen(rm_state.f.name, "rb+");
    if (f == NULL) {
        RM_LOG_INFO("Creating file [%s]", rm_state.f.name); /* file doesn't exist, create */
        f = fopen(rm_state.f.name, "wb");
        if (f == NULL) {
            RM_LOG_CRIT("Can't create file [%s]!", rm_state.f.name); 
            exit(EXIT_FAILURE);
        }
    }
    rm_state.f.f_created = 1;
    fclose(f);
    rm_get_unique_string(rm_state.f1.name);
    f = fopen(rm_state.f1.name, "rb+");
    if (f == NULL) {
        RM_LOG_INFO("Creating file [%s] (f1)", rm_state.f1.name); /* file doesn't exist, create */
        f = fopen(rm_state.f1.name, "wb");
        if (f == NULL) {
            RM_LOG_CRIT("Can't create file [%s]!", rm_state.f1.name); 
            exit(EXIT_FAILURE);
        }
        j = RM_TEST_5_FILE_X_SZ;
        RM_LOG_INFO("Writing [%zu] random bytes to file [%s]", j, rm_state.f1.name);
        while (j--) {
                fputc(rand(), f);
        }
    }
    rm_state.f1.f_created = 1;
    fclose(f);
    rm_get_unique_string(rm_state.f2.name);
    f = fopen(rm_state.f2.name, "rb+");
    if (f == NULL) {
        RM_LOG_INFO("Creating file [%s] (f2)", rm_state.f2.name); /* file doesn't exist, create */
        f = fopen(rm_state.f2.name, "wb");
        if (f == NULL) {
            RM_LOG_CRIT("Can't create file [%s]!", rm_state.f2.name); 
            exit(EXIT_FAILURE);
        }
        j = RM_TEST_5_FILE_Y_SZ;
        RM_LOG_INFO("Writing [%zu] random bytes to file [%s]", j, rm_state.f2.name);
        while (j--) {
                fputc(rand(), f);
        }
    }
    rm_state.f2.f_created = 1;
    fclose(f);
    rm_get_unique_string(rm_state.f3.name);
    f = fopen(rm_state.f3.name, "rb+");
    if (f != NULL) {
        fclose(f);
        RM_LOG_INFO("File [%s] exists. removing (f3)...", rm_state.f3.name); /* file exists, remove */
        if (unlink(rm_state.f3.name) != 0) {
            RM_LOG_CRIT("Can't unlink file [%s]!", rm_state.f3.name);
            assert_true(1 == 0 && "Can't unlink!");
        }
    }
    return 0;
}

int
test_rm_teardown(void **state) {
    size_t  i;
    FILE    *f;
    struct  test_rm_state *rm_state;

    rm_state = *state;
    assert_true(rm_state != NULL);
    if (RM_TEST_5_DELETE_FILES == 1) { /* delete all test files */
        i = 0;
        for (; i < RM_TEST_FNAMES_N; ++i) {
            f = fopen(rm_test_fnames[i], "rb");
            if (f == NULL) {
                RM_LOG_ERR("Can't open file [%s]", rm_test_fnames[i]);	
            } else {
                RM_LOG_INFO("Removing file [%s]", rm_test_fnames[i]);
                fclose(f);
                if (remove(rm_test_fnames[i]) != 0) {
                    assert_true(1 == 0 && "Can't remove!");
                }
            }
        }
    }
    free(rm_state->buf);
    free(rm_state->buf2);
    rm_session_free(rm_state->s);
    if (rm_state->f.f_created == 1) {
        f = fopen(rm_state->f.name, "rb");
        if (f == NULL) {
            RM_LOG_ERR("Can't open file [%s]", rm_state->f.name);	
        } else {
            RM_LOG_INFO("Removing file [%s]...", rm_state->f.name);
            fclose(f);
            if (unlink(rm_state->f.name) != 0) {
                RM_LOG_CRIT("Can't unlink file [%s]!", rm_state->f.name);
                assert_true(1 == 0 && "Can't unlink!");
            }
        }
    }
    if (rm_state->f1.f_created == 1) {
        f = fopen(rm_state->f1.name, "rb");
        if (f == NULL) {
            RM_LOG_ERR("Can't open file [%s] (f1)", rm_state->f1.name);	
        } else {
            RM_LOG_INFO("Removing file [%s] (f1)...", rm_state->f1.name);
            fclose(f);
            if (unlink(rm_state->f1.name) != 0) {
                RM_LOG_CRIT("Can't unlink file [%s]!", rm_state->f1.name);
                assert_true(1 == 0 && "Can't unlink!");
            }
        }
    }
    if (rm_state->f2.f_created == 1) {
        f = fopen(rm_state->f2.name, "rb");
        if (f == NULL) {
            RM_LOG_ERR("Can't open file [%s] (f2)", rm_state->f2.name);	
        } else {
            RM_LOG_INFO("Removing file [%s] (f2)...", rm_state->f2.name);
            fclose(f);
            if (unlink(rm_state->f2.name) != 0) {
                RM_LOG_CRIT("Can't unlink file [%s]!", rm_state->f2.name);
                assert_true(1 == 0 && "Can't unlink!");
            }
        }
    }
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

/* @brief   Test if number of bytes enqueued as delta elements is correct,
 *          when x file is same as y (file has no changes). */
void
test_rm_rolling_ch_proc_1(void **state) {
    FILE                    *f, *f_x, *f_y;
    int                     fd;
    int                     err;
    size_t                  i, j, L, file_sz, y_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    const char              *fname, *y;
    size_t                  blocks_n_exp, blocks_n;
    struct twhlist_node     *tmp;
    struct rm_session       *s;
    struct rm_session_push_local    *prvt;

    /* hashtable deletion */
    unsigned int            bkt;
    const struct rm_ch_ch_ref_hlink *e;

    /* delta queue's content verification */
    const twfifo_queue          *q;             /* produced queue of delta elements */
    const struct rm_delta_e     *delta_e;       /* iterator over delta elements */
    struct twlist_head          *lh;
    size_t                      rec_by_ref, rec_by_raw, delta_ref_n, delta_raw_n,
                                rec_by_tail, delta_tail_n, rec_by_zero_diff, delta_zero_diff_n;
    size_t                      detail_case_1_n, detail_case_2_n, detail_case_3_n;

    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);
    rm_state = *state;
    assert_true(rm_state != NULL);

    /* test on all files */
    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i) {
        fname = rm_test_fnames[i];
        f = fopen(fname, "rb");
        if (f == NULL) {
            RM_LOG_PERR("Can't open file [%s]", fname);
        }
        assert_true(f != NULL && "Can't fopen file");
        fd = fileno(f);
        if (fstat(fd, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", fname);
            fclose(f);
            assert_true(1 == 0 && "Can't fstat file");
        }
        file_sz = fs.st_size; 
        if (file_sz < 2) {
            RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", fname, file_sz);
            fclose(f);
            continue;
        }
        detail_case_1_n = 0;
        detail_case_2_n = 0;
        detail_case_3_n = 0;
        j = 0;
        for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
            L = rm_test_L_blocks[j];
            RM_LOG_INFO("Validating testing #1 of rolling checksum, file [%s], size [%zu], block size L [%zu]", fname, file_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, fname);
                continue;
            }
            RM_LOG_INFO("Testing rolling checksum procedure #1: file [%s], size [%zu], block size L [%zu]", fname, file_sz, L);

            f_y = f; /* reference file exists, split it and calc checksums */
            f_x = f;
            y_sz = fs.st_size;
            y = fname;

            blocks_n_exp = y_sz / L + (y_sz % L ? 1 : 0); /* split @y file into non-overlapping blocks and calculate checksums on these blocks, expected number of blocks is */
            err = rm_rx_insert_nonoverlapping_ch_ch_ref(f_y, y, h, L, NULL, blocks_n_exp, &blocks_n);
            assert_int_equal(err, RM_ERR_OK);
            assert_int_equal(blocks_n_exp, blocks_n);
            rewind(f_y);

            s = rm_state->s; /* run rolling checksum procedure */
            memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx)); /* init reconstruction context */
            s->rec_ctx.L = L;
            s->rec_ctx.copy_all_threshold = 0;
            s->rec_ctx.copy_tail_threshold = 0;
            s->rec_ctx.send_threshold = L;
            prvt = s->prvt;
            prvt->h = h;
            prvt->f_x = f_x;                        /* run on same file */
            prvt->delta_f = rm_roll_proc_cb_1;
            err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, 0);    /* 1. run rolling checksum procedure */
            assert_int_equal(err, RM_ERR_OK);

            q = &prvt->tx_delta_e_queue; /* verify s->prvt delta queue content */
            assert_true(q != NULL);

            rec_by_ref = rec_by_raw = 0;
            delta_ref_n = delta_raw_n = 0;
            rec_by_tail = delta_tail_n = 0;
            rec_by_zero_diff = delta_zero_diff_n = 0;
            for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh)) {    /* dequeue, so can free later */
                delta_e = tw_container_of(lh, struct rm_delta_e, link);
                switch (delta_e->type) {
                    case RM_DELTA_ELEMENT_REFERENCE:
                        rec_by_ref += L;
                        ++delta_ref_n;
                        break;
                    case RM_DELTA_ELEMENT_RAW_BYTES:
                        rec_by_raw += delta_e->raw_bytes_n;
                        ++delta_raw_n;
                        break;
                    case RM_DELTA_ELEMENT_ZERO_DIFF:
                        rec_by_ref += delta_e->raw_bytes_n; /* delta ZERO_DIFF has raw_bytes_n set to indicate bytes that matched
                                                               (whole file) so we can nevertheless check at receiver that is correct */
                        ++delta_ref_n;

                        rec_by_zero_diff += delta_e->raw_bytes_n;
                        ++delta_zero_diff_n;
                        break;
                    case RM_DELTA_ELEMENT_TAIL:
                        rec_by_ref += delta_e->raw_bytes_n; /* delta TAIL has raw_bytes_n set to indicate bytes that matched
                                                               (that tail) so we can nevertheless check at receiver there is no error */
                        ++delta_ref_n;

                        rec_by_tail += delta_e->raw_bytes_n;
                        ++delta_tail_n;
                        break;
                    default:
                        RM_LOG_ERR("%s", "Unknown delta element type!");
                        assert_true(1 == 0 && "Unknown delta element type!");
                }
                free((void*) delta_e);  /* free delta element */
            }

            /* general tests */
            assert_int_equal(rec_by_raw, 0);
            assert_int_equal(rec_by_ref, file_sz);
            assert_true(delta_tail_n == 0 || delta_tail_n == 1);
            assert_true(delta_zero_diff_n == 0 || (delta_zero_diff_n == 1 && rec_by_ref == y_sz && rec_by_zero_diff == y_sz && delta_tail_n == 0 && delta_raw_n == 0));

            /* detail cases */
            /* 1. if L is >= file size, delta must be ZERO_DIFF */
            if (L >= y_sz) {
                assert_true(delta_zero_diff_n == 1);
                assert_true(delta_ref_n == 1);
                assert_true(delta_tail_n == 0);
                assert_true(delta_raw_n == 0);
                assert_true(rec_by_ref == y_sz);
                assert_true(rec_by_zero_diff == y_sz);
                assert_true(rec_by_tail == 0);
                assert_true(rec_by_raw == 0);
                ++detail_case_1_n;
            }
            /* 2. if L is not equal file_sz and does divide evenly file_sz, there must be file_sz/L delta reference blocks present
             * and zero raw bytes, zero ZERO_DIFF, zero TAIL blocks */
            if ((L != y_sz) && (y_sz % L == 0)) {
                assert_true(delta_zero_diff_n == 0);
                assert_true(delta_ref_n == y_sz / L);
                assert_true(delta_tail_n == 0);
                assert_true(delta_raw_n == 0);
                assert_true(rec_by_ref == y_sz);
                assert_true(rec_by_zero_diff == 0);
                assert_true(rec_by_tail == 0);
                assert_true(rec_by_raw == 0);
                ++detail_case_2_n;
            }
            /* 3. if L is less than file_sz and doesn't divide evenly file_sz, there must be delta TAIL reference block present
             * file_sz/L + 1 reference blocks (includes TAIL block), zero raw bytes, zero ZERO_DIFF */
            if ((L < y_sz) && (y_sz % L != 0)) {
                assert_true(delta_zero_diff_n == 0);
                assert_true(delta_ref_n == y_sz / L + 1);
                assert_true(delta_tail_n == 1);
                assert_true(delta_raw_n == 0);
                assert_true(rec_by_ref == y_sz);
                assert_true(rec_by_zero_diff == 0);
                assert_true(rec_by_tail == y_sz % L);
                assert_true(rec_by_raw == 0);
                ++detail_case_3_n;
            }

            if (delta_tail_n == 0) {
                if (delta_zero_diff_n > 0) {
                    RM_LOG_INFO("PASSED test #1: correct number of bytes sent in delta elements, file [%s], size [%zu], L [%zu], blocks [%zu], DELTA REF [%zu] bytes [%zu], DELTA ZERO DIFF [%zu] bytes [%zu]",
                            fname, y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_zero_diff_n, rec_by_zero_diff);
                } else {
                    RM_LOG_INFO("PASSED test #1: correct number of bytes sent in delta elements, file [%s], size [%zu], L [%zu], blocks [%zu], DELTA REF [%zu] bytes [%zu], DELTA RAW [%zu] bytes [%zu]",
                            fname, y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_raw_n, rec_by_raw);
                }
            } else {
                RM_LOG_INFO("PASSED test #1: correct number of bytes sent in delta elements, file [%s], size [%zu], L [%zu], blocks [%zu], DELTA REF [%zu] bytes [%zu] (DELTA_TAIL [%zu] bytes [%zu]), DELTA RAW [%zu] bytes [%zu]",
                        fname, y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_tail_n, rec_by_tail, delta_raw_n, rec_by_raw);
            }
            rewind(f);
            blocks_n = 0;
            bkt = 0;
            twhash_for_each_safe(h, bkt, tmp, e, hlink) {
                twhash_del((struct twhlist_node*)&e->hlink);
                free((struct rm_ch_ch_ref_hlink*)e);
                ++blocks_n;
            }
            assert_int_equal(blocks_n_exp, blocks_n);
            rewind(f);
        }
        fclose(f);
        RM_LOG_INFO("PASSED test #1 detail cases, file [%s], size [%zu], detail case #1 [%zu] #2 [%zu] #3 [%zu]", fname, y_sz, detail_case_1_n, detail_case_2_n, detail_case_3_n);
    }
    return;
}

/* @brief   Test if number of bytes enqueued as delta elements is correct,
 *          when x is copy of y, but first byte in x is changed. */
void
test_rm_rolling_ch_proc_2(void **state) {
    int                     err;
    char                    buf_x_name[RM_FILE_LEN_MAX + 50];   /* @x (copy of @y with changed single byte at the beginning) */
    const char              *f_y_name;  /* @y name */
    unsigned char           c;
    FILE                    *f_copy, *f_x, *f_y;
    int                     fd_x, fd_y;
    size_t                  i, j, L, f_x_sz, f_y_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    size_t                  blocks_n_exp, blocks_n;
    struct twhlist_node     *tmp;
    struct rm_session       *s;
    struct rm_session_push_local    *prvt;

    /* hashtable deletion */
    unsigned int            bkt;
    const struct rm_ch_ch_ref_hlink *e;

    /* delta queue's content verification */
    const twfifo_queue          *q;             /* produced queue of delta elements */
    const struct rm_delta_e     *delta_e;       /* iterator over delta elements */
    struct twlist_head          *lh;
    size_t                      rec_by_ref, rec_by_raw, delta_ref_n, delta_raw_n,
                                rec_by_tail, delta_tail_n, rec_by_zero_diff, delta_zero_diff_n;
    size_t                      detail_case_1_n, detail_case_2_n, detail_case_3_n;

    err = test_rm_copy_files_and_postfix("_test_2");
    if (err != 0) {
        RM_LOG_ERR("%s", "Error copying files, skipping test");
        return;
    }

    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);
    rm_state = *state;
    assert_true(rm_state != NULL);

    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i) { /* test on all files */
        f_y_name = rm_test_fnames[i];
        f_y = fopen(f_y_name, "rb");
        if (f_y == NULL)
        {
            RM_LOG_PERR("Can't open file [%s]", f_y_name);
        }
        assert_true(f_y != NULL && "Can't fopen file");
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
            fclose(f_y);
            assert_true(1 == 0 && "Can't fstat file");
        }
        f_y_sz = fs.st_size;
        if (f_y_sz < 2) {
            RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
            fclose(f_y);
            continue;
        }
        strncpy(buf_x_name, f_y_name, RM_FILE_LEN_MAX); /* change byte in copy */
        strncpy(buf_x_name + strlen(buf_x_name), "_test_2", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
        }
        f_x = f_copy;
        fd_x = fileno(f_x);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_x, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
            fclose(f_x);
            assert_true(1 == 0);
        }
        f_x_sz = fs.st_size;
        if (rm_fpread(&c, sizeof(unsigned char), 1, 0, f_x) != 1) { /* read first byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }
        c = (c + 1) % 256; /* change first byte, so ZERO_DIFF delta can't happen in this test, this would be an error */
        if (rm_fpwrite(&c, sizeof(unsigned char), 1, 0, f_x) != 1) {
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
            RM_LOG_INFO("Validating testing #2 (first byte changed) of rolling checksum procedure, file [%s], size [%zu], block size L [%zu]", f_y_name, f_y_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            if (f_y_sz < 2) {
                RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
                continue;
            }
            RM_LOG_INFO("Testing rolling checksum procedure #2 (first byte changed): file @x[%s] size [%zu] file @y[%s], size [%zu], block size L [%zu]", buf_x_name, f_x_sz, f_y_name, f_y_sz, L);

            blocks_n_exp = f_y_sz / L + (f_y_sz % L ? 1 : 0); /* split @y file into non-overlapping blocks and calculate checksums on these blocks, expected number of blocks is */
            err = rm_rx_insert_nonoverlapping_ch_ch_ref(f_y, f_y_name, h, L, NULL, blocks_n_exp, &blocks_n);
            assert_int_equal(err, RM_ERR_OK);
            assert_int_equal(blocks_n_exp, blocks_n);
            rewind(f_x);
            rewind(f_y);

            s = rm_state->s; /* run rolling checksum procedure on @x */
            memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx)); /* init reconstruction context */
            s->rec_ctx.L = L;
            s->rec_ctx.copy_all_threshold = 0;
            s->rec_ctx.copy_tail_threshold = 0;
            s->rec_ctx.send_threshold = L;
            prvt = s->prvt; /* setup private session's arguments */
            prvt->h = h;
            prvt->f_x = f_x;                        /* run on @x */
            prvt->delta_f = rm_roll_proc_cb_1;
            err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, 0); /* 1. run rolling checksum procedure */
            assert_int_equal(err, RM_ERR_OK);

            q = &prvt->tx_delta_e_queue; /* verify s->prvt delta queue content */
            assert_true(q != NULL);

            rec_by_ref = rec_by_raw  = 0;
            delta_ref_n = delta_raw_n = 0;
            rec_by_tail = delta_tail_n = 0;
            rec_by_zero_diff = delta_zero_diff_n = 0;
            for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh)) {
                delta_e = tw_container_of(lh, struct rm_delta_e, link);
                switch (delta_e->type) {
                    case RM_DELTA_ELEMENT_REFERENCE:
                        rec_by_ref += delta_e->raw_bytes_n;
                        assert_int_equal(delta_e->raw_bytes_n, L);  /* can't be different here */
                        ++delta_ref_n;
                        break;
                    case RM_DELTA_ELEMENT_RAW_BYTES:
                        rec_by_raw += delta_e->raw_bytes_n;
                        ++delta_raw_n;
                        break;
                    case RM_DELTA_ELEMENT_ZERO_DIFF:
                        assert_true(1 == 0 && "DELTA ELEMENT ZERO DIFF can't happen in this test!");
                        rec_by_ref += delta_e->raw_bytes_n; /* delta ZERO_DIFF has raw_bytes_n set to indicate bytes that matched (whole file) so we can nevertheless check at receiver that is correct */
                        ++delta_ref_n;

                        rec_by_zero_diff += delta_e->raw_bytes_n;
                        ++delta_zero_diff_n;
                        break;
                    case RM_DELTA_ELEMENT_TAIL:
                        rec_by_ref += delta_e->raw_bytes_n; /* delta TAIL has raw_bytes_n set to indicate bytes that matched (that tail) so we can nevertheless check at receiver there is no error */
                        ++delta_ref_n;

                        rec_by_tail += delta_e->raw_bytes_n;
                        ++delta_tail_n;
                        break;
                    default:
                        RM_LOG_ERR("%s", "Unknown delta element type!");
                        assert_true(1 == 0 && "Unknown delta element type!");
                }
                if (delta_e->type == RM_DELTA_ELEMENT_RAW_BYTES) {
                    free(delta_e->raw_bytes);
                }
                free((void*)delta_e);
            }

            assert_int_equal(rec_by_ref + rec_by_raw, f_x_sz); /* general tests */
            assert_true(delta_tail_n == 0 || delta_tail_n == 1);
            assert_true(delta_zero_diff_n == 0 && "DELTA ELEMENT ZERO DIFF can't happen in this test!");
            assert_true(rec_by_zero_diff == 0);

            if (delta_tail_n == 0) {
                RM_LOG_INFO("PASSED test #2 (first byte changed): delta elements cover whole file, file [%s], size [%zu], "
                        "L [%zu], blocks [%zu], DELTA REF [%zu] bytes [%zu], DELTA RAW [%zu] bytes [%zu]",
                        f_y_name, f_y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_raw_n, rec_by_raw);
            } else {
                RM_LOG_INFO("PASSED test #2 (first byte changed): delta elements cover whole file, file [%s], size [%zu], "
                        "L [%zu], blocks [%zu], DELTA REF [%zu] bytes [%zu] (DELTA_TAIL [%zu] bytes [%zu]), DELTA RAW [%zu] bytes [%zu]",
                        f_y_name, f_y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_tail_n, rec_by_tail,
                        delta_raw_n, rec_by_raw);
            }
            /* detail cases */
            /* 1. if L is >= file size, delta must be single RAW element */
            if (L >= f_y_sz) {
                assert_true(delta_ref_n == 0);
                assert_true(delta_tail_n == 0);
                assert_true(delta_raw_n == 1);
                assert_true(rec_by_ref == 0);
                assert_true(rec_by_tail == 0);
                assert_true(rec_by_raw == f_y_sz);
                ++detail_case_1_n;
            }
            /* 2. if L is less than file size and does divide evenly file size, there will usually be f_y_sz/L - 1 delta reference blocks present
             * and 1 raw byte block because the first block in @x doesn't match the first block in @y (all other blocks in @x always match corresponding blocks in @y in this test #2),
             * and none of the checksums computed by rolling checksum procedure starting from second byte in @x up to Lth byte, i.e. on blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will match some of the nonoverlapping checksums from @y.
             * But there is a chance one of nonoverlapping blocks in @y will match first block in @x (which has first byte changed), e.g if L is 1, size is 100 and @y file is 0x1 0x2 0x3 0x78 0x5 ...
             * the @x file is then 0x78 0x2 0x3 0x78 0x5 ... and first block will match 4th block in @y. There is also a chance this won't match BUT some of blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will find a match and rolling proc will move on offsets different that nonoverlapping blocks. All next blocks may match or up to L bytes may be transferred as raw elements in any
             * possible way: L blocks each 1 byte size, or L/2 each 2 bytes or 1 block 1 byte long and one L-1, etc. */
            if ((L < f_y_sz) && (f_y_sz % L == 0)) {
                assert_true(rec_by_raw <= L);
                assert_true(delta_raw_n <= L);
                assert_true((delta_ref_n == f_y_sz/L - 1 && delta_raw_n > 0 && rec_by_raw == L) || (delta_ref_n == f_y_sz/L && delta_raw_n == 0)); /* the first block in @x will not match the first block in @y, but it can match some other block in @y */
                assert_true(delta_ref_n * L == f_y_sz - rec_by_raw);
                assert_true(delta_tail_n == 0);
                assert_true((rec_by_ref == f_y_sz - L && rec_by_raw == L) || (rec_by_ref == f_y_sz && rec_by_raw == 0));
                assert_true(rec_by_tail == 0);
                ++detail_case_2_n;
            }
            /* 3. if L is less than file size and doesn't divide evenly file size, there can be delta TAIL reference block present,
             * regarding delta reference blocks it is the same situation as in test #2 (plus remember that TAIL is also counted as reference). */
            if ((L < f_y_sz) && (f_y_sz % L != 0)) {
                assert_true(rec_by_raw <= L);
                assert_true(delta_raw_n <= L);
                assert_true((delta_ref_n == f_y_sz/L && delta_raw_n > 0 && rec_by_raw == L) || (delta_ref_n == f_y_sz/L + 1 && delta_raw_n == 0)); /* the first block in @x will not match the first block in @y, but it can match some other block in @y */
                assert_true((delta_tail_n == 1 && (rec_by_tail == f_y_sz % L) && ((delta_ref_n - 1) * L + rec_by_tail == f_y_sz - rec_by_raw)) || (delta_tail_n == 0 && delta_ref_n * L == f_y_sz - rec_by_raw));
                assert_true((rec_by_ref == f_y_sz - L && rec_by_raw == L) || (rec_by_ref == f_y_sz && rec_by_raw == 0));
                ++detail_case_3_n;
            }

            /* move file pointer back to the beginning */
            rewind(f_x);
            rewind(f_y);

            blocks_n = 0;
            bkt = 0;
            twhash_for_each_safe(h, bkt, tmp, e, hlink) {
                twhash_del((struct twhlist_node*)&e->hlink);
                free((struct rm_ch_ch_ref_hlink*)e);
                ++blocks_n;
            }
            assert_int_equal(blocks_n_exp, blocks_n);
        }
        fclose(f_x);
        fclose(f_y);
        RM_LOG_INFO("PASSED test #2 (first byte changed) detail cases, file [%s], size [%zu], detail case #1 [%zu] #2 [%zu] #3 [%zu]",
                f_y_name, f_y_sz, detail_case_1_n, detail_case_2_n, detail_case_3_n);
    }

    if (RM_TEST_5_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_2");
        if (err != 0) {
            RM_LOG_ERR("%s", "Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}

/* @brief   Test if number of bytes enqueued as delta elements is correct,
 *          when x is copy of y, but last byte in x is changed. */
void
test_rm_rolling_ch_proc_3(void **state) {
    int                     err;
    char                    buf_x_name[RM_FILE_LEN_MAX + 50];   /* @x (copy of @y with changed single byte at the end) */
    const char              *f_y_name;  /* @y name */
    unsigned char           c;
    FILE                    *f_copy, *f_x, *f_y;
    int                     fd_x, fd_y;
    size_t                  i, j, L, f_x_sz, f_y_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    size_t                  blocks_n_exp, blocks_n;
    struct twhlist_node     *tmp;
    struct rm_session       *s;
    struct rm_session_push_local    *prvt;

    /* hashtable deletion */
    unsigned int            bkt;
    const struct rm_ch_ch_ref_hlink *e;

    /* delta queue's content verification */
    const twfifo_queue          *q;             /* produced queue of delta elements */
    const struct rm_delta_e     *delta_e;       /* iterator over delta elements */
    struct twlist_head          *lh;
    size_t                      rec_by_ref, rec_by_raw, delta_ref_n, delta_raw_n,
                                rec_by_tail, delta_tail_n, rec_by_zero_diff, delta_zero_diff_n;
    size_t                      detail_case_1_n, detail_case_2_n, detail_case_3_n;

    err = test_rm_copy_files_and_postfix("_test_3");
    if (err != 0) {
        RM_LOG_ERR("%s", "Error copying files, skipping test");
        return;
    }

    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);
    rm_state = *state;
    assert_true(rm_state != NULL);

    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i) { /* test on all files */
        f_y_name = rm_test_fnames[i];
        f_y = fopen(f_y_name, "rb");
        if (f_y == NULL)
        {
            RM_LOG_PERR("Can't open file [%s]", f_y_name);
        }
        assert_true(f_y != NULL && "Can't fopen file");
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
            fclose(f_y);
            assert_true(1 == 0 && "Can't fstat file");
        }
        f_y_sz = fs.st_size;
        if (f_y_sz < 2) {
            RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
            fclose(f_y);
            continue;
        }
        strncpy(buf_x_name, f_y_name, RM_FILE_LEN_MAX); /* change byte in copy */
        strncpy(buf_x_name + strlen(buf_x_name), "_test_3", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
        }
        f_x = f_copy;
        fd_x = fileno(f_x);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_x, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
            fclose(f_x);
            assert_true(1 == 0);
        }
        f_x_sz = fs.st_size;
        if (rm_fpread(&c, sizeof(unsigned char), 1, f_x_sz - 1, f_x) != 1) { /* read last byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }
        c = (c + 1) % 256; /* change last byte, so ZERO_DIFF delta and TAIL delta can't happen in this test, this would be an error */
        if (rm_fpwrite(&c, sizeof(unsigned char), 1, f_x_sz - 1, f_x) != 1) {
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
            RM_LOG_INFO("Validating testing #3 (last byte changed) of rolling checksum, file [%s], size [%zu], block size L [%zu]", f_y_name, f_y_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            if (f_y_sz < 2) {
                RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
                continue;
            }
            RM_LOG_INFO("Testing rolling checksum procedure #3 (last byte changed): file @x[%s] size [%zu] file @y[%s], size [%zu], block size L [%zu]", buf_x_name, f_x_sz, f_y_name, f_y_sz, L);

            blocks_n_exp = f_y_sz / L + (f_y_sz % L ? 1 : 0); /* split @y file into non-overlapping blocks and calculate checksums on these blocks, expected number of blocks is */
            err = rm_rx_insert_nonoverlapping_ch_ch_ref(f_y, f_y_name, h, L, NULL, blocks_n_exp, &blocks_n);
            assert_int_equal(err, RM_ERR_OK);
            assert_int_equal(blocks_n_exp, blocks_n);
            rewind(f_x);
            rewind(f_y);

            s = rm_state->s; /* run rolling checksum procedure on @x */
            memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx)); /* init reconstruction context */
            s->rec_ctx.L = L;
            s->rec_ctx.copy_all_threshold = 0;
            s->rec_ctx.copy_tail_threshold = 0;
            s->rec_ctx.send_threshold = L;
            prvt = s->prvt; /* setup private session's arguments */
            prvt->h = h;
            prvt->f_x = f_x;                        /* run on @x */
            prvt->delta_f = rm_roll_proc_cb_1;
            err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, 0); /* 1. run rolling checksum procedure */
            assert_int_equal(err, RM_ERR_OK);

            q = &prvt->tx_delta_e_queue; /* verify s->prvt delta queue content */
            assert_true(q != NULL);

            rec_by_ref = rec_by_raw  = 0;
            delta_ref_n = delta_raw_n = 0;
            rec_by_tail = delta_tail_n = 0;
            rec_by_zero_diff = delta_zero_diff_n = 0;
            for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh)) {
                delta_e = tw_container_of(lh, struct rm_delta_e, link);
                switch (delta_e->type) {
                    case RM_DELTA_ELEMENT_REFERENCE:
                        rec_by_ref += delta_e->raw_bytes_n;
                        assert_int_equal(delta_e->raw_bytes_n, L);  /* can't be different here */
                        ++delta_ref_n;
                        break;
                    case RM_DELTA_ELEMENT_RAW_BYTES:
                        rec_by_raw += delta_e->raw_bytes_n;
                        ++delta_raw_n;
                        break;
                    case RM_DELTA_ELEMENT_ZERO_DIFF:
                        rec_by_ref += delta_e->raw_bytes_n; /* delta ZERO_DIFF has raw_bytes_n set to indicate bytes that matched (whole file) so we can nevertheless check at receiver that is correct */
                        ++delta_ref_n;

                        rec_by_zero_diff += delta_e->raw_bytes_n;
                        ++delta_zero_diff_n;
                        break;
                    case RM_DELTA_ELEMENT_TAIL:
                        rec_by_ref += delta_e->raw_bytes_n; /* delta TAIL has raw_bytes_n set to indicate bytes that matched (that tail) so we can nevertheless check at receiver there is no error */
                        ++delta_ref_n;

                        rec_by_tail += delta_e->raw_bytes_n;
                        ++delta_tail_n;
                        break;
                    default:
                        RM_LOG_ERR("%s", "Unknown delta element type!");
                        assert_true(1 == 0 && "Unknown delta element type!");
                }
                if (delta_e->type == RM_DELTA_ELEMENT_RAW_BYTES) {
                    free(delta_e->raw_bytes);
                }
                free((void*)delta_e);
            }

            assert_int_equal(rec_by_ref + rec_by_raw, f_x_sz); /* general tests */
            assert_true(delta_tail_n == 0 && "DELTA TAIL can't happen in this test!");
            assert_true(delta_zero_diff_n == 0 && "DELTA ELEMENT ZERO DIFF can't happen in this test!");
            assert_true(rec_by_zero_diff == 0);

            if (delta_tail_n == 0) {
                RM_LOG_INFO("PASSED test #3 (last byte changed): delta elements cover whole file, file [%s], size [%zu], "
                        "L [%zu], blocks [%zu], DELTA REF [%zu] bytes [%zu], DELTA RAW [%zu] bytes [%zu]",
                        f_y_name, f_y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_raw_n, rec_by_raw);
            } else {
                RM_LOG_INFO("PASSED test #3 (last byte changed): delta elements cover whole file, file [%s], size [%zu], "
                        "L [%zu], blocks [%zu], DELTA REF [%zu] bytes [%zu] (DELTA_TAIL [%zu] bytes [%zu]), DELTA RAW [%zu] bytes [%zu]",
                        f_y_name, f_y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_tail_n, rec_by_tail,
                        delta_raw_n, rec_by_raw);
            }
            /* detail cases */
            /* 1. if L is >= file size, delta must be single RAW element */
            if (L >= f_y_sz) {
                assert_true(delta_ref_n == 0);
                assert_true(delta_tail_n == 0);
                assert_true(delta_raw_n == 1);
                assert_true(rec_by_ref == 0);
                assert_true(rec_by_tail == 0);
                assert_true(rec_by_raw == f_x_sz);
                ++detail_case_1_n;
            }
            /* 2. if L is less than file size and does divide evenly file size, there will usually be f_y_sz/L - 1 delta reference blocks present
             * and 1 raw byte block because the last block in @x doesn't match the last block in @y (all other blocks in @x always match corresponding blocks in @y in this test #3),
             * and none of the checksums computed by rolling checksum procedure starting from second byte in @x up to Lth byte, i.e. on blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will match some of the nonoverlapping checksums from @y.
             * But there is a chance one of nonoverlapping blocks in @y will match changed block in @x (which has last byte changed), e.g if L is 1, size is 100 and last 5 bytes in @y file are ... 0x6 0x2 0x3 0x04 0x5
             * the corresponding bytes in @x file are then ... 0x6 0x2 0x3 0x4 0x6 and last block will match 96th block in @y. There can't be TAIL element in this test */
            if ((L < f_y_sz) && (f_y_sz % L == 0)) {
                assert_true(rec_by_raw <= L);
                assert_true(delta_raw_n <= L);
                assert_true((delta_ref_n == f_y_sz/L - 1 && delta_raw_n > 0 && rec_by_raw == L) || (delta_ref_n == f_y_sz/L && delta_raw_n == 0)); /* the last block in @x will not match the last block in @y, but it can match some other block in @y */
                assert_true(delta_ref_n * L == f_y_sz - rec_by_raw);
                assert_true(delta_tail_n == 0);
                assert_true((rec_by_ref == f_y_sz - L && rec_by_raw == L) || (rec_by_ref == f_y_sz && rec_by_raw == 0));
                assert_true(rec_by_tail == 0);
                ++detail_case_2_n;
            }
            /* 3. if L is less than file size and doesn't divide evenly file size, the last block will not match and only that block will not match, there can't be delta TAIL reference block present */
            if ((L < f_y_sz) && (f_y_sz % L != 0)) {
                assert_true(rec_by_raw <= L && rec_by_raw > 0);
                assert_true(delta_raw_n == 1);
                assert_true((delta_ref_n == f_y_sz/L && delta_raw_n == 1 && rec_by_raw == f_y_sz % L)); /* the last block in @x will not match the last block in @y */
                assert_true(delta_tail_n == 0 && delta_ref_n * L == f_y_sz - rec_by_raw);
                ++detail_case_3_n;
            }

            /* move file pointer back to the beginning */
            rewind(f_x);
            rewind(f_y);

            blocks_n = 0;
            bkt = 0;
            twhash_for_each_safe(h, bkt, tmp, e, hlink) {
                twhash_del((struct twhlist_node*)&e->hlink);
                free((struct rm_ch_ch_ref_hlink*)e);
                ++blocks_n;
            }
            assert_int_equal(blocks_n_exp, blocks_n);
        }
        fclose(f_x);
        fclose(f_y);
        RM_LOG_INFO("PASSED test #3 (last byte changed) detail cases, file [%s], size [%zu], detail case #1 [%zu] #2 [%zu] #3 [%zu]",
                f_y_name, f_y_sz, detail_case_1_n, detail_case_2_n, detail_case_3_n);
    }

    if (RM_TEST_5_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_3");
        if (err != 0) {
            RM_LOG_ERR("%s", "Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}

/* @brief   Test if number of bytes enqueued as delta elements is correct,
 *          when x is copy of y, but first and last bytes in x are changed. */
void
test_rm_rolling_ch_proc_4(void **state) {
    int                     err;
    char                    buf_x_name[RM_FILE_LEN_MAX + 50];   /* @x (copy of @y with changed bytes) */
    const char              *f_y_name;  /* @y name */
    unsigned char           c;
    FILE                    *f_copy, *f_x, *f_y;
    int                     fd_x, fd_y;
    size_t                  i, j, L, f_x_sz, f_y_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    size_t                  blocks_n_exp, blocks_n;
    struct twhlist_node     *tmp;
    struct rm_session       *s;
    struct rm_session_push_local    *prvt;

    /* hashtable deletion */
    unsigned int            bkt;
    const struct rm_ch_ch_ref_hlink *e;

    /* delta queue's content verification */
    const twfifo_queue          *q;             /* produced queue of delta elements */
    const struct rm_delta_e     *delta_e;       /* iterator over delta elements */
    struct twlist_head          *lh;
    size_t                      rec_by_ref, rec_by_raw, delta_ref_n, delta_raw_n,
                                rec_by_tail, delta_tail_n, rec_by_zero_diff, delta_zero_diff_n;
    size_t                      detail_case_1_n, detail_case_2_n, detail_case_3_n;
    size_t                      send_threshold;

    err = test_rm_copy_files_and_postfix("_test_4");
    if (err != 0) {
        RM_LOG_ERR("%s", "Error copying files, skipping test");
        return;
    }

    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);
    rm_state = *state;
    assert_true(rm_state != NULL);

    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i) { /* test on all files */
        f_y_name = rm_test_fnames[i];
        f_y = fopen(f_y_name, "rb");
        if (f_y == NULL)
        {
            RM_LOG_PERR("Can't open file [%s]", f_y_name);
        }
        assert_true(f_y != NULL && "Can't fopen file");
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
            fclose(f_y);
            assert_true(1 == 0 && "Can't fstat file");
        }
        f_y_sz = fs.st_size;
        if (f_y_sz < 2) {
            RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
            fclose(f_y);
            continue;
        }
        strncpy(buf_x_name, f_y_name, RM_FILE_LEN_MAX); /* change byte in copy */
        strncpy(buf_x_name + strlen(buf_x_name), "_test_4", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
        }
        f_x = f_copy;
        fd_x = fileno(f_x);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_x, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
            fclose(f_x);
            assert_true(1 == 0);
        }
        f_x_sz = fs.st_size;
        if (rm_fpread(&c, sizeof(unsigned char), 1, 0, f_x) != 1) { /* read first byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }
        c = (c + 1) % 256; /* change first byte, so ZERO_DIFF delta can't happen in this test, this would be an error */
        if (rm_fpwrite(&c, sizeof(unsigned char), 1, 0, f_x) != 1) {
            RM_LOG_ERR("Error writing to file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }
        if (rm_fpread(&c, sizeof(unsigned char), 1, f_x_sz - 1, f_x) != 1) { /* read last byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }
        c = (c + 1) % 256; /* change last byte, so TAIL delta can't happen in this test, this would be an error */
        if (rm_fpwrite(&c, sizeof(unsigned char), 1, f_x_sz - 1, f_x) != 1) {
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
            RM_LOG_INFO("Validating testing #4 (2 bytes changed) of rolling checksum, file [%s], size [%zu], block size L [%zu]", f_y_name, f_y_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            if (f_y_sz < 2) {
                RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
                continue;
            }
            RM_LOG_INFO("Testing rolling checksum procedure #4 (2 bytes changed): file @x[%s] size [%zu] file @y[%s], size [%zu], block size L [%zu]", buf_x_name, f_x_sz, f_y_name, f_y_sz, L);

            blocks_n_exp = f_y_sz / L + (f_y_sz % L ? 1 : 0); /* split @y file into non-overlapping blocks and calculate checksums on these blocks, expected number of blocks is */
            err = rm_rx_insert_nonoverlapping_ch_ch_ref(f_y, f_y_name, h, L, NULL, blocks_n_exp, &blocks_n);
            assert_int_equal(err, RM_ERR_OK);
            assert_int_equal(blocks_n_exp, blocks_n);
            rewind(f_x);
            rewind(f_y);

            s = rm_state->s; /* run rolling checksum procedure on @x */
            memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx)); /* init reconstruction context */
            s->rec_ctx.L = L;
            s->rec_ctx.copy_all_threshold = 0;
            s->rec_ctx.copy_tail_threshold = 0;
            send_threshold = L;
            s->rec_ctx.send_threshold = send_threshold;
            prvt = s->prvt; /* setup private session's arguments */
            prvt->h = h;
            prvt->f_x = f_x;                        /* run on @x */
            prvt->delta_f = rm_roll_proc_cb_1;
            err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, 0); /* 1. run rolling checksum procedure */
            assert_int_equal(err, RM_ERR_OK);

            q = &prvt->tx_delta_e_queue; /* verify s->prvt delta queue content */
            assert_true(q != NULL);

            rec_by_ref = rec_by_raw  = 0;
            delta_ref_n = delta_raw_n = 0;
            rec_by_tail = delta_tail_n = 0;
            rec_by_zero_diff = delta_zero_diff_n = 0;
            for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh)) {
                delta_e = tw_container_of(lh, struct rm_delta_e, link);
                switch (delta_e->type) {
                    case RM_DELTA_ELEMENT_REFERENCE:
                        rec_by_ref += delta_e->raw_bytes_n;
                        assert_int_equal(delta_e->raw_bytes_n, L);  /* can't be different here */
                        ++delta_ref_n;
                        break;
                    case RM_DELTA_ELEMENT_RAW_BYTES:
                        rec_by_raw += delta_e->raw_bytes_n;
                        ++delta_raw_n;
                        break;
                    case RM_DELTA_ELEMENT_ZERO_DIFF:
                        rec_by_ref += delta_e->raw_bytes_n; /* delta ZERO_DIFF has raw_bytes_n set to indicate bytes that matched (whole file) so we can nevertheless check at receiver that is correct */
                        ++delta_ref_n;

                        rec_by_zero_diff += delta_e->raw_bytes_n;
                        ++delta_zero_diff_n;
                        break;
                    case RM_DELTA_ELEMENT_TAIL:
                        rec_by_ref += delta_e->raw_bytes_n; /* delta TAIL has raw_bytes_n set to indicate bytes that matched (that tail) so we can nevertheless check at receiver there is no error */
                        ++delta_ref_n;

                        rec_by_tail += delta_e->raw_bytes_n;
                        ++delta_tail_n;
                        break;
                    default:
                        RM_LOG_ERR("%s", "Unknown delta element type!");
                        assert_true(1 == 0 && "Unknown delta element type!");
                }
                if (delta_e->type == RM_DELTA_ELEMENT_RAW_BYTES) {
                    free(delta_e->raw_bytes);
                }
                free((void*)delta_e);
            }

            assert_int_equal(rec_by_ref + rec_by_raw, f_x_sz); /* general tests */
            assert_true(delta_tail_n == 0 && "DELTA TAIL can't happen in this test!");
            assert_true(delta_zero_diff_n == 0 && "DELTA ELEMENT ZERO DIFF can't happen in this test!");
            assert_true(rec_by_zero_diff == 0);

            if (delta_tail_n == 0) {
                RM_LOG_INFO("PASSED test #4 (2 bytes changed): delta elements cover whole file, file [%s], size [%zu], "
                        "L [%zu], blocks [%zu], DELTA REF [%zu] bytes [%zu], DELTA RAW [%zu] bytes [%zu]",
                        f_y_name, f_y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_raw_n, rec_by_raw);
            } else {
                RM_LOG_INFO("PASSED test #4 (2 bytes changed): delta elements cover whole file, file [%s], size [%zu], "
                        "L [%zu], blocks [%zu], DELTA REF [%zu] bytes [%zu] (DELTA_TAIL [%zu] bytes [%zu]), DELTA RAW [%zu] bytes [%zu]",
                        f_y_name, f_y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_tail_n, rec_by_tail,
                        delta_raw_n, rec_by_raw);
            }
            /* detail cases */
            /* 1. if L is >= file size, delta must be single RAW element */
            if (L >= f_y_sz) {
                assert_true(delta_ref_n == 0);
                assert_true(delta_tail_n == 0);
                assert_true(delta_raw_n == 1);
                assert_true(rec_by_ref == 0);
                assert_true(rec_by_tail == 0);
                assert_true(rec_by_raw == f_x_sz);
                ++detail_case_1_n;
            }
            /* 2. if L is less than file size and does divide evenly file size, there will usually be f_y_sz/L - 2 delta reference blocks present
             * and 2 raw byte blocks because the first and/or last block in @x doesn't match corresponding blocks in @y (all other blocks in @x always match corresponding blocks in @y in this test),
             * and none of the checksums computed by rolling checksum procedure starting from second byte in @x up to Lth byte, i.e. on blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will match some of the nonoverlapping checksums from @y.
             * But there is a chance one of nonoverlapping blocks in @y will match first and/or last block in @x (which has first byte changed), e.g if L is 1, size is 100 and @y file is 0x1 0x2 0x3 0x78 0x5 ...
             * the @x file is then 0x78 0x2 0x3 0x78 0x5 ... and first block will match 4th block in @y. There is also a chance this won't match BUT some of blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will find a match and rolling proc will move on offsets different that nonoverlapping blocks. All next f_y_sz/L - 2 blocks may match or not and up to 2*L bytes will be transferred as raw delta elements */
            if ((L < f_y_sz) && (f_y_sz % L == 0)) {
                if (rec_by_raw > 2 * L) {
                    test_rm_dump(s->rec_ctx);
                }
                assert_true(rec_by_raw <= 2 * L);
                if (delta_raw_n > f_y_sz) {
                    test_rm_dump(s->rec_ctx);
                }
                assert_true(delta_raw_n <= f_y_sz);
                if (f_y_sz <= (2 * L)) {
                    if (delta_ref_n == 0) {
                        assert_true(rec_by_ref == 0 && delta_raw_n > 0 && delta_raw_n == (f_y_sz / send_threshold + (f_y_sz % send_threshold ? 1 : 0)) && rec_by_raw == f_y_sz);
                    } else {
                        assert_true(delta_ref_n > 0 && rec_by_ref == (delta_ref_n * L) && delta_raw_n > 0 && delta_raw_n <= 2 * L && rec_by_raw == (f_y_sz - rec_by_ref));
                    }
                } else {
                    if (delta_ref_n == f_y_sz / L - 2) {
                        if (rec_by_ref != f_y_sz - 2 * L) {
                            test_rm_dump(s->rec_ctx);
                        }
                        assert_true(rec_by_ref == f_y_sz - 2 * L);
                        assert_true(delta_raw_n > 0);
                        if (delta_raw_n > 2 * L) {
                            test_rm_dump(s->rec_ctx);
                        }
                        assert_true(delta_raw_n <= 2 * L);
                        if (rec_by_raw != 2 * L) {
                            test_rm_dump(s->rec_ctx);
                        }
                        assert_true(rec_by_raw == 2 * L);
                    } else if (delta_ref_n == (f_y_sz / L - 1)) {
                        assert_true(rec_by_ref == (f_y_sz - L) && delta_raw_n > 0 && delta_raw_n <= L && rec_by_raw == L);
                    } else {
                        assert_true(delta_ref_n == f_y_sz / L && rec_by_ref == f_y_sz && delta_raw_n == 0 && rec_by_raw == 0); /* the last (tail) block in @x will not match the last block in @y, but it can match some other block in @y, same with first block */
                    }
                }
                assert_true(delta_ref_n * L == f_y_sz - rec_by_raw);
                assert_true(delta_tail_n == 0); /* impossible as L divides @y evenly */
                assert_true(rec_by_tail == 0);
                ++detail_case_2_n;
            }
            /* 3. if L is less than file size and doesn't divide evenly file size the last block will not match, but the first still may match */
            if ((L < f_y_sz) && (f_y_sz % L != 0)) {
                assert_true(rec_by_raw <= L + (f_y_sz % L) && rec_by_raw >= f_y_sz % L);
                assert_true(delta_raw_n >= 1);
                assert_true((delta_ref_n == f_y_sz/L - 1 && delta_raw_n > 0 && rec_by_raw == L + f_y_sz % L) || (delta_ref_n == f_y_sz/L && delta_raw_n > 0 && rec_by_raw == f_y_sz % L)); /* the last block in @x will not match the last block in @y but the first may match some block */
                assert_true(delta_tail_n == 0 && delta_ref_n * L == f_y_sz - rec_by_raw);
                ++detail_case_3_n;
            }
            /* 4. sanity check for L == 1 */
            if (f_y_sz >= 3 && L == 1) {
                assert_true(delta_raw_n <= 2 && "L == 1 but matching blocks not found!");
            } 

            /* move file pointer back to the beginning */
            rewind(f_x);
            rewind(f_y);

            blocks_n = 0;
            bkt = 0;
            twhash_for_each_safe(h, bkt, tmp, e, hlink) {
                twhash_del((struct twhlist_node*)&e->hlink);
                free((struct rm_ch_ch_ref_hlink*)e);
                ++blocks_n;
            }
            assert_int_equal(blocks_n_exp, blocks_n);
        }
        fclose(f_x);
        fclose(f_y);
        RM_LOG_INFO("PASSED test #4 (2 bytes changed) detail cases, file [%s], size [%zu], detail case #1 [%zu] #2 [%zu] #3 [%zu]",
                f_y_name, f_y_sz, detail_case_1_n, detail_case_2_n, detail_case_3_n);
    }

    if (RM_TEST_5_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_4");
        if (err != 0) {
            RM_LOG_ERR("%s", "Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}

/* @brief   Test if number of bytes enqueued as delta elements is correct,
 *          when x is copy of y, but first, middle and last bytes in x are changed. */
void
test_rm_rolling_ch_proc_5(void **state) {
    int                     err;
    char                    buf_x_name[RM_FILE_LEN_MAX + 50];   /* @x (copy of @y with changed bytes) */
    const char              *f_y_name;  /* @y name */
    unsigned char           c;
    FILE                    *f_copy, *f_x, *f_y;
    int                     fd_x, fd_y;
    size_t                  i, j, L, f_x_sz, f_y_sz, half_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    size_t                  blocks_n_exp, blocks_n;
    struct twhlist_node     *tmp;
    struct rm_session       *s;
    struct rm_session_push_local    *prvt;

    /* hashtable deletion */
    unsigned int            bkt;
    const struct rm_ch_ch_ref_hlink *e;

    /* delta queue's content verification */
    const twfifo_queue          *q;             /* produced queue of delta elements */
    const struct rm_delta_e     *delta_e;       /* iterator over delta elements */
    struct twlist_head          *lh;
    size_t                      rec_by_ref, rec_by_raw, delta_ref_n, delta_raw_n,
                                rec_by_tail, delta_tail_n, rec_by_zero_diff, delta_zero_diff_n;
    size_t                      detail_case_1_n, detail_case_2_n, detail_case_3_n;
    size_t                      send_threshold;

    err = test_rm_copy_files_and_postfix("_test_5");
    if (err != 0) {
        RM_LOG_ERR("%s", "Error copying files, skipping test");
        return;
    }

    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);
    rm_state = *state;
    assert_true(rm_state != NULL);

    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i) { /* test on all files */
        f_y_name = rm_test_fnames[i];
        f_y = fopen(f_y_name, "rb");
        if (f_y == NULL)
        {
            RM_LOG_PERR("Can't open file [%s]", f_y_name);
        }
        assert_true(f_y != NULL && "Can't fopen file");
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", f_y_name);
            fclose(f_y);
            assert_true(1 == 0 && "Can't fstat file");
        }
        f_y_sz = fs.st_size;
        if (f_y_sz < 2) {
            RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
            fclose(f_y);
            continue;
        }
        strncpy(buf_x_name, f_y_name, RM_FILE_LEN_MAX); /* change byte in copy */
        strncpy(buf_x_name + strlen(buf_x_name), "_test_5", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
        }
        f_x = f_copy;
        fd_x = fileno(f_x);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_x, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
            fclose(f_x);
            assert_true(1 == 0);
        }
        f_x_sz = fs.st_size;
        if (rm_fpread(&c, sizeof(unsigned char), 1, 0, f_x) != 1) { /* read first byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }
        c = (c + 1) % 256; /* change first byte, so ZERO_DIFF delta can't happen in this test, this would be an error */
        if (rm_fpwrite(&c, sizeof(unsigned char), 1, 0, f_x) != 1) {
            RM_LOG_ERR("Error writing to file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }
        half_sz = f_x_sz / 2 + f_x_sz % 2;
        if (rm_fpread(&c, sizeof(unsigned char), 1, half_sz, f_x) != 1) { /* read middle byte */
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
        c = (c + 1) % 256;    /* change middle byte */
        if (rm_fpwrite(&c, sizeof(unsigned char), 1, half_sz, f_x) != 1) {
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
        if (rm_fpread(&c, sizeof(unsigned char), 1, f_x_sz - 1, f_x) != 1) { /* read last byte */
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }
        c = (c + 1) % 256; /* change last byte, so TAIL delta can't happen in this test, this would be an error */
        if (rm_fpwrite(&c, sizeof(unsigned char), 1, f_x_sz - 1, f_x) != 1) {
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
            RM_LOG_INFO("Validating testing #5 (3 bytes changed) of rolling checksum, file [%s], size [%zu], block size L [%zu]", f_y_name, f_y_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            if (f_y_sz < 2) {
                RM_LOG_INFO("File [%s] size [%zu] is too small for this test, skipping", f_y_name, f_y_sz);
                continue;
            }
            RM_LOG_INFO("Testing rolling checksum procedure #3 (3 bytes changed): file @x[%s] size [%zu] file @y[%s], size [%zu], block size L [%zu]", buf_x_name, f_x_sz, f_y_name, f_y_sz, L);

            blocks_n_exp = f_y_sz / L + (f_y_sz % L ? 1 : 0); /* split @y file into non-overlapping blocks and calculate checksums on these blocks, expected number of blocks is */
            err = rm_rx_insert_nonoverlapping_ch_ch_ref(f_y, f_y_name, h, L, NULL, blocks_n_exp, &blocks_n);
            assert_int_equal(err, RM_ERR_OK);
            assert_int_equal(blocks_n_exp, blocks_n);
            rewind(f_x);
            rewind(f_y);

            s = rm_state->s; /* run rolling checksum procedure on @x */
            memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx)); /* init reconstruction context */
            s->rec_ctx.L = L;
            s->rec_ctx.copy_all_threshold = 0;
            s->rec_ctx.copy_tail_threshold = 0;
            send_threshold = L;
            s->rec_ctx.send_threshold = send_threshold;
            prvt = s->prvt; /* setup private session's arguments */
            prvt->h = h;
            prvt->f_x = f_x;                        /* run on @x */
            prvt->delta_f = rm_roll_proc_cb_1;
            err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, 0); /* 1. run rolling checksum procedure */
            assert_int_equal(err, RM_ERR_OK);

            q = &prvt->tx_delta_e_queue; /* verify s->prvt delta queue content */
            assert_true(q != NULL);

            rec_by_ref = rec_by_raw  = 0;
            delta_ref_n = delta_raw_n = 0;
            rec_by_tail = delta_tail_n = 0;
            rec_by_zero_diff = delta_zero_diff_n = 0;
            for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh)) {
                delta_e = tw_container_of(lh, struct rm_delta_e, link);
                switch (delta_e->type) {
                    case RM_DELTA_ELEMENT_REFERENCE:
                        rec_by_ref += delta_e->raw_bytes_n;
                        assert_int_equal(delta_e->raw_bytes_n, L);  /* can't be different here */
                        ++delta_ref_n;
                        break;
                    case RM_DELTA_ELEMENT_RAW_BYTES:
                        rec_by_raw += delta_e->raw_bytes_n;
                        ++delta_raw_n;
                        break;
                    case RM_DELTA_ELEMENT_ZERO_DIFF:
                        rec_by_ref += delta_e->raw_bytes_n; /* delta ZERO_DIFF has raw_bytes_n set to indicate bytes that matched (whole file) so we can nevertheless check at receiver that is correct */
                        ++delta_ref_n;

                        rec_by_zero_diff += delta_e->raw_bytes_n;
                        ++delta_zero_diff_n;
                        break;
                    case RM_DELTA_ELEMENT_TAIL:
                        rec_by_ref += delta_e->raw_bytes_n; /* delta TAIL has raw_bytes_n set to indicate bytes that matched (that tail) so we can nevertheless check at receiver there is no error */
                        ++delta_ref_n;

                        rec_by_tail += delta_e->raw_bytes_n;
                        ++delta_tail_n;
                        break;
                    default:
                        RM_LOG_ERR("%s", "Unknown delta element type!");
                        assert_true(1 == 0 && "Unknown delta element type!");
                }
                if (delta_e->type == RM_DELTA_ELEMENT_RAW_BYTES) {
                    free(delta_e->raw_bytes);
                }
                free((void*)delta_e);
            }

            assert_int_equal(rec_by_ref + rec_by_raw, f_x_sz); /* general tests */
            assert_true(delta_tail_n == 0 && "DELTA TAIL can't happen in this test!");
            assert_true(delta_zero_diff_n == 0 && "DELTA ELEMENT ZERO DIFF can't happen in this test!");
            assert_true(rec_by_zero_diff == 0);

            if (delta_tail_n == 0) {
                RM_LOG_INFO("PASSED test #5 (3 bytes changed): delta elements cover whole file, file [%s], size [%zu], "
                        "L [%zu], blocks [%zu], DELTA REF [%zu] bytes [%zu], DELTA RAW [%zu] bytes [%zu]",
                        f_y_name, f_y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_raw_n, rec_by_raw);
            } else {
                RM_LOG_INFO("PASSED test #5 (3 bytes changed): delta elements cover whole file, file [%s], size [%zu], "
                        "L [%zu], blocks [%zu], DELTA REF [%zu] bytes [%zu] (DELTA_TAIL [%zu] bytes [%zu]), DELTA RAW [%zu] bytes [%zu]",
                        f_y_name, f_y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_tail_n, rec_by_tail,
                        delta_raw_n, rec_by_raw);
            }
            /* detail cases */
            /* 1. if L is >= file size, delta must be single RAW element */
            if (L >= f_y_sz) {
                assert_true(delta_ref_n == 0);
                assert_true(delta_tail_n == 0);
                assert_true(delta_raw_n == 1);
                assert_true(rec_by_ref == 0);
                assert_true(rec_by_tail == 0);
                assert_true(rec_by_raw == f_x_sz);
                ++detail_case_1_n;
            }
            /* 2. if L is less than file size and does divide evenly file size, there will usually be f_y_sz/L - 3 delta reference blocks present
             * and 3 raw byte blocks because the first and/or middle and/or last block in @x doesn't match corresponding blocks in @y (all other blocks in @x always match corresponding blocks in @y in this test),
             * and none of the checksums computed by rolling checksum procedure starting from second byte in @x up to Lth byte, i.e. on blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will match some of the nonoverlapping checksums from @y.
             * But there is a chance one of nonoverlapping blocks in @y will match first and/or middle and/or last block in @x (which has first byte changed), e.g if L is 1, size is 100 and @y file is 0x1 0x2 0x3 0x78 0x5 ...
             * the @x file is then 0x78 0x2 0x3 0x78 0x5 ... and first block will match 4th block in @y. There is also a chance this won't match BUT some of blocks [1,1+L], [2,2+L], ..., [L-1,2L-1]
             * will find a match and rolling proc will move on offsets different that nonoverlapping blocks. All next f_y_sz/L - 2 blocks may match or not and up to 3*L bytes will be transferred as delta raw elements */
            if ((L < f_y_sz) && (f_y_sz % L == 0)) {
                assert_true(delta_tail_n == 0);
                assert_true(rec_by_raw <= 3 * L);
                assert_true(delta_raw_n <= f_y_sz);
                if (f_y_sz <= (3 * L)) {
                    if (delta_ref_n == 0) {
                        assert_true(rec_by_ref == 0 && delta_raw_n > 0 && delta_raw_n == (f_y_sz / send_threshold + (f_y_sz % send_threshold ? 1 : 0)) && rec_by_raw == f_y_sz);
                    } else {
                        assert_true(delta_ref_n > 0 && rec_by_ref == (delta_ref_n * L) && delta_raw_n > 0 && delta_raw_n <= 3 * L && rec_by_raw == (f_y_sz - rec_by_ref));
                    }
                } else {
                    if (delta_ref_n == f_y_sz / L - 3) {
                        if (rec_by_ref != f_y_sz - 3 * L) {
                            test_rm_dump(s->rec_ctx);
                        }
                        assert_true(rec_by_ref == f_y_sz - 3 * L);
                        assert_true(delta_raw_n > 0);
                        if (delta_raw_n > 3 * L) {
                            test_rm_dump(s->rec_ctx);
                        }
                        assert_true(delta_raw_n <= 3 * L);
                        if (rec_by_raw != 3 * L) {
                            test_rm_dump(s->rec_ctx);
                        }
                        assert_true(rec_by_raw == 3 * L);
                    } else if (delta_ref_n == (f_y_sz / L - 2)) {
                        assert_true(rec_by_ref == (f_y_sz - 2 * L) && delta_raw_n > 0 && delta_raw_n <= 2 * L  && rec_by_raw == 2 * L);
                    } else if (delta_ref_n == (f_y_sz / L - 1)) {
                        assert_true(rec_by_ref == (f_y_sz - 1 * L) && delta_raw_n > 0 && delta_raw_n <= 1 * L  && rec_by_raw == 1 * L);
                    } else {
                        assert_true(delta_ref_n == f_y_sz / L && rec_by_ref == f_y_sz && delta_raw_n == 0 && rec_by_raw == 0); /* the last (tail) block in @x will not match the last block in @y, but it can match some other block in @y, same with middle and first block */
                    }
                }
                assert_true(delta_ref_n * L == f_y_sz - rec_by_raw);
                assert_true(delta_tail_n == 0); /* impossible as L divides @y evenly */
                assert_true(rec_by_tail == 0);
                ++detail_case_2_n;
            }
            /* 3. if L is less than file size and doesn't divide evenly file size the last block will not match, but the first and middle may still match */
            if ((L < f_y_sz) && (f_y_sz % L != 0)) {
                assert_true(rec_by_raw <= 3 * L);
                if (f_y_sz <= (3 * L)) {
                    if (delta_ref_n == 0) {
                        assert_true(rec_by_ref == 0 && delta_raw_n > 0 && delta_raw_n == (f_y_sz / send_threshold + (f_y_sz % send_threshold ? 1 : 0)) && rec_by_raw == f_y_sz);
                    } else {
                        assert_true(delta_ref_n > 0 && rec_by_ref == (delta_ref_n * L) && delta_raw_n > 0 && delta_raw_n <= 2 * L && rec_by_raw == (f_y_sz - rec_by_ref));
                    }
                } else {
                    if (delta_ref_n == (f_y_sz / L + 1 - 3)) {
                        assert_true(rec_by_ref == (L * (f_y_sz / L + 1 - 3)));
                        assert_true(delta_raw_n > 0 && delta_raw_n <= 2 * L + (f_y_sz % L));
                        assert_true(rec_by_raw == (2 * L + f_y_sz % L));
                    } else if (delta_ref_n == (f_y_sz / L + 1 - 2)) {
                        assert_true(rec_by_ref == (L * (f_y_sz / L + 1 - 2)));
                        assert_true(delta_raw_n > 0 && delta_raw_n <= L + (f_y_sz % L));
                        assert_true(rec_by_raw == (L + f_y_sz % L));
                    } else if (delta_ref_n == (f_y_sz / L + 1 - 1)) {
                        assert_true((rec_by_ref == (f_y_sz - L) || rec_by_ref == (f_y_sz - f_y_sz % L)) && delta_raw_n > 0 && delta_raw_n <= L && (rec_by_raw == L || rec_by_raw == f_y_sz % L));
                    } else {
                        assert_true(1 == 0 && "Got only delta reference blocks but TAIL can't match in this test!"); 
                    }
                }
                assert_true(rec_by_ref == f_y_sz - rec_by_raw);
                ++detail_case_3_n;
            }

            /* move file pointer back to the beginning */
            rewind(f_x);
            rewind(f_y);

            blocks_n = 0;
            bkt = 0;
            twhash_for_each_safe(h, bkt, tmp, e, hlink) {
                twhash_del((struct twhlist_node*)&e->hlink);
                free((struct rm_ch_ch_ref_hlink*)e);
                ++blocks_n;
            }
            assert_int_equal(blocks_n_exp, blocks_n);
        }
        fclose(f_x);
        fclose(f_y);
        RM_LOG_INFO("PASSED test #5 (3 bytes changed) detail cases, file [%s], size [%zu], detail case #1 [%zu] #2 [%zu] #3 [%zu]",
                f_y_name, f_y_sz, detail_case_1_n, detail_case_2_n, detail_case_3_n);
    }

    if (RM_TEST_5_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_5");
        if (err != 0) {
            RM_LOG_ERR("%s", "Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}

/* @brief   Test error reporting.
 * @details NULL session. */
void
test_rm_rolling_ch_proc_6(void **state) {
    FILE                *f_x;
    struct rm_session   *s;
    enum rm_error       err;
    struct test_rm_state     *rm_state = *state;

    RM_LOG_INFO("%s", "Running test #6 (Test error reporting: NULL session)...");
    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);

    s = NULL;
    f_x = fopen(rm_state->f.name, "rb");
    if (f_x == NULL) {
        RM_LOG_ERR("Can't open file [%s]!", rm_state->f.name);
        assert_true(1 == 0 && "Can't open @x file!");
    }
    err = rm_rolling_ch_proc(s, h, f_x, rm_roll_proc_cb_1, 0); /* 1. run rolling checksum procedure */
    fclose(f_x);
    assert_int_equal(err, RM_ERR_BAD_CALL);
    RM_LOG_INFO("%s", "PASSED test #6 (Test error reporting: NULL session)");
}

/* @brief   Test error reporting.
 * @details NULL file @x pointer. */
void
test_rm_rolling_ch_proc_7(void **state) {
    struct rm_session   *s;
    struct rm_session_push_local *prvt;
    enum rm_error       err;
    struct test_rm_state     *rm_state = *state;

    RM_LOG_INFO("%s", "Running test #7 (Test error reporting: NULL file pointer)...");
    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);

    s = rm_state->s; /* run rolling checksum procedure on @x */
    memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx)); /* init reconstruction context */
    s->rec_ctx.L = 512;
    s->rec_ctx.copy_all_threshold = 0;
    s->rec_ctx.copy_tail_threshold = 0;
    s->rec_ctx.send_threshold = 512;
    prvt = s->prvt; /* set private session's arguments */
    prvt->h = h;
    prvt->f_x = NULL;                        /* run on @x */
    prvt->delta_f = rm_roll_proc_cb_1;
    err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, 0); /* 1. run rolling checksum procedure */
    assert_int_equal(err, RM_ERR_BAD_CALL);
    RM_LOG_INFO("%s", "PASSED test #7 (Test error reporting: NULL file pointer)");
}

/* @brief   Test error reporting.
 * @details Bad request of reading out of range from file @x, file size is 0. */
void
test_rm_rolling_ch_proc_8(void **state) {
    struct rm_session   *s;
    struct rm_session_push_local *prvt;
    enum rm_error       err;
    FILE                *f_x;
    struct test_rm_state     *rm_state = *state;

    RM_LOG_INFO("%s", "Running test #8 (Test error reporting: zero size file)...");
    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);

    s = rm_state->s; /* run rolling checksum procedure on @x */
    memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx)); /* init reconstruction context */
    s->rec_ctx.L = 512;
    s->rec_ctx.copy_all_threshold = 0;
    s->rec_ctx.copy_tail_threshold = 0;
    s->rec_ctx.send_threshold = 512;
    prvt = s->prvt; /* set private session's arguments */
    prvt->h = h;
    f_x = fopen(rm_state->f.name, "rb");
    if (f_x == NULL) {
        RM_LOG_ERR("Can't open file [%s]!", rm_state->f.name);
        assert_true(1 == 0 && "Can't open @x file!");
    }
    prvt->f_x = f_x;                        /* run on @x */
    prvt->delta_f = rm_roll_proc_cb_1;
    err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, 0); /* 1. run rolling checksum procedure */
    fclose(f_x);
    assert_int_equal(err, RM_ERR_TOO_MUCH_REQUESTED);
    RM_LOG_INFO("%s", "PASSED test #8 (Test error reporting: zero size file)");
}

/* @brief   Test error reporting.
 * @details NULL request of reading out of range from file @x, file size is not 0. */
void
test_rm_rolling_ch_proc_9(void **state) {
    struct rm_session   *s;
    struct rm_session_push_local *prvt;
    enum rm_error       err;
    FILE                *f_x;
    int                 fd;
    size_t              file_sz;
    struct stat         fs;
    struct test_rm_state     *rm_state = *state;

    RM_LOG_INFO("%s", "Running test #9 (Test error reporting: reading out of range on nonzero size file)...");
    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);

    s = rm_state->s; /* run rolling checksum procedure on @x */
    memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx)); /* init reconstruction context */
    s->rec_ctx.L = 512;
    s->rec_ctx.copy_all_threshold = 0;
    s->rec_ctx.copy_tail_threshold = 0;
    s->rec_ctx.send_threshold = 512;
    prvt = s->prvt; /* set private session's arguments */
    prvt->h = h;
    f_x = fopen(rm_test_fnames[RM_TEST_5_9_FILE_IDX], "rb");
    if (f_x == NULL) {
        RM_LOG_ERR("Can't open file [%s]!", rm_test_fnames[RM_TEST_5_9_FILE_IDX]);
        assert_true(1 == 0 && "Can't open @x file!");
    }
    fd = fileno(f_x);
    if (fstat(fd, &fs) != 0) {
        RM_LOG_CRIT("Can't fstat file [%s]", rm_test_fnames[RM_TEST_5_9_FILE_IDX]);
        assert_true(1 == 0 && "Can't fstat @x file!");
    }
    file_sz = fs.st_size;
    assert_true(file_sz > 0);
    prvt->f_x = f_x;                        /* run on @x */
    prvt->delta_f = rm_roll_proc_cb_1;
    err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, file_sz); /* 1. run rolling checksum procedure */
    fclose(f_x);
    assert_int_equal(err, RM_ERR_TOO_MUCH_REQUESTED);
    RM_LOG_INFO("%s", "PASSED test #9 (Test error reporting: reading out of range on nonzero size file)");
}

/* @brief   Test send threshold */
void
test_rm_rolling_ch_proc_10(void **state) {
    FILE                    *f_x, *f_y;
    int                     fd;
    int                     err;
    size_t                  j, L, y_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    const char              *y;
    size_t                  blocks_n_exp, blocks_n;
    struct twhlist_node     *tmp;
    struct rm_session       *s;
    struct rm_session_push_local    *prvt;
    size_t                  send_threshold;
    size_t                  delta_ref_n, delta_raw_n, delta_tail_n, delta_zero_diff_n;

    /* hashtable deletion */
    unsigned int            bkt;
    const struct rm_ch_ch_ref_hlink *e;

    /* delta queue's content verification */
    const twfifo_queue          *q;             /* produced queue of delta elements */
    const struct rm_delta_e     *delta_e;       /* iterator over delta elements */
    struct twlist_head          *lh;

    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);
    rm_state = *state;
    assert_true(rm_state != NULL);

    f_x = fopen(rm_state->f1.name, "rb");
    if (f_x == NULL) {
        RM_LOG_PERR("Can't open file [%s]", rm_state->f1.name);
    }
    assert_true(f_x != NULL && "Can't fopen file");
    f_y = fopen(rm_state->f2.name, "rb");
    if (f_y == NULL) {
        RM_LOG_PERR("Can't open file [%s]", rm_state->f2.name);
    }
    assert_true(f_y != NULL && "Can't fopen file");
    y = rm_state->f2.name;
    fd = fileno(f_y);
    if (fstat(fd, &fs) != 0) {
        RM_LOG_PERR("Can't fstat file [%s]", rm_state->f2.name);
        fclose(f_y);
        assert_true(1 == 0 && "Can't fstat file");
    }
    y_sz = fs.st_size;
    
    j = 0;
    for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
        L = rm_test_L_blocks[j];
        send_threshold = L;
        RM_LOG_INFO("Testing rolling checksum procedure (send threshold) #10: block size [%zu], send threshold [%zu]", L, send_threshold);
        blocks_n_exp = y_sz / L + (y_sz % L ? 1 : 0); /* split @y file into non-overlapping blocks and calculate checksums on these blocks, expected number of blocks is */
        err = rm_rx_insert_nonoverlapping_ch_ch_ref(f_y, y, h, L, NULL, blocks_n_exp, &blocks_n);
        assert_int_equal(err, RM_ERR_OK);
        assert_int_equal(blocks_n_exp, blocks_n);
        rewind(f_y);
        
        s = rm_state->s; /* run rolling checksum procedure */
        memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx)); /* reset reconstruction context */
        s->rec_ctx.L = L;
        s->rec_ctx.copy_all_threshold = 0;
        s->rec_ctx.copy_tail_threshold = 0;
        s->rec_ctx.send_threshold = send_threshold;
        prvt = s->prvt;
        prvt->h = h;
        prvt->f_x = f_x;
        prvt->delta_f = rm_roll_proc_cb_1;
        err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, 0);
        assert_int_equal(err, RM_ERR_OK);

        q = &prvt->tx_delta_e_queue; /* check delta elements */
        assert_true(q != NULL);

        delta_ref_n = delta_raw_n = 0;
        delta_tail_n = delta_zero_diff_n = 0;
        for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh)) {    /* dequeue, so can free later */
            delta_e = tw_container_of(lh, struct rm_delta_e, link);
            switch (delta_e->type) {
                case RM_DELTA_ELEMENT_REFERENCE:
                    ++delta_ref_n;
                    break;
                case RM_DELTA_ELEMENT_RAW_BYTES:
                    assert_true(delta_e->raw_bytes_n > 0);
                    assert_true(delta_e->raw_bytes_n <= send_threshold);
                    free(delta_e->raw_bytes);
                    ++delta_raw_n;
                    break;
                case RM_DELTA_ELEMENT_ZERO_DIFF:
                    ++delta_ref_n;
                    ++delta_zero_diff_n;
                    break;
                case RM_DELTA_ELEMENT_TAIL:
                    ++delta_ref_n;
                    ++delta_tail_n;
                    break;
                default:
                    RM_LOG_ERR("%s", "Unknown delta element type!");
                    assert_true(1 == 0 && "Unknown delta element type!");
            }
            free((void*) delta_e);  /* free delta element */
        }

        if (delta_tail_n == 0) {
            if (delta_zero_diff_n > 0) {
                RM_LOG_INFO("PASSED test #10 (send threshold): send threshold [%zu], L [%zu], blocks [%zu], DELTA REF [%zu], DELTA ZERO DIFF [%zu]",
                        send_threshold, L, blocks_n, delta_ref_n, delta_zero_diff_n);
            } else {
                RM_LOG_INFO("PASSED test #10 (send threshold): send threshold [%zu], L [%zu], blocks [%zu], DELTA REF [%zu], DELTA RAW [%zu]",
                        send_threshold, L, blocks_n, delta_ref_n, delta_raw_n);
            }
        } else {
            RM_LOG_INFO("PASSED test #10 (send threshold): send threshold [%zu], L [%zu], blocks [%zu], DELTA REF [%zu], DELTA_TAIL [%zu], DELTA RAW [%zu]",
                    send_threshold, L, blocks_n, delta_ref_n, delta_tail_n, delta_raw_n);
        }

        blocks_n = 0;
        bkt = 0;
        twhash_for_each_safe(h, bkt, tmp, e, hlink) {
            twhash_del((struct twhlist_node*)&e->hlink);
            free((struct rm_ch_ch_ref_hlink*)e);
            ++blocks_n;
        }
        assert_int_equal(blocks_n_exp, blocks_n);
        rewind(f_x);
        rewind(f_y);
    }
    fclose(f_x);
    fclose(f_y);
    RM_LOG_INFO("%s", "PASSED test #10 (send threshold)");
    return;
}

/* @brief   Test copy all threshold. Specify threshold of file size + 1 so copying must happened */
void
test_rm_rolling_ch_proc_11(void **state) {
    FILE                    *f, *f_x, *f_y;
    int                     fd;
    int                     err;
    size_t                  i, j, L, file_sz, y_sz;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    const char              *fname, *y;
    size_t                  blocks_n_exp, blocks_n;
    struct twhlist_node     *tmp;
    struct rm_session       *s;
    struct rm_session_push_local    *prvt;

    /* hashtable deletion */
    unsigned int            bkt;
    const struct rm_ch_ch_ref_hlink *e;

    /* delta queue's content verification */
    const twfifo_queue          *q;             /* produced queue of delta elements */
    const struct rm_delta_e     *delta_e;       /* iterator over delta elements */
    struct twlist_head          *lh;
    size_t                      rec_by_ref, rec_by_raw, delta_ref_n, delta_raw_n,
                                rec_by_tail, delta_tail_n, rec_by_zero_diff, delta_zero_diff_n;

    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    twhash_init(h);
    rm_state = *state;
    assert_true(rm_state != NULL);

    /* test on all files */
    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i) {
        fname = rm_test_fnames[i];
        f = fopen(fname, "rb");
        if (f == NULL) {
            RM_LOG_PERR("Can't open file [%s]", fname);
        }
        assert_true(f != NULL && "Can't fopen file");
        fd = fileno(f);
        if (fstat(fd, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", fname);
            fclose(f);
            assert_true(1 == 0 && "Can't fstat file");
        }
        file_sz = fs.st_size;
        assert_true(rm_state->f2.f_created == 1);
        f_y = fopen(rm_state->f2.name, "wb+"); /* open reference file, split it and calc checksums */
        if (f_y == NULL) {
            RM_LOG_PERR("Can't open file [%s]", rm_state->f2.name);
        }
        assert_true(f_y != NULL && "Can't fopen file");
        j = 0;
        for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
            L = rm_test_L_blocks[j];
            RM_LOG_INFO("Validating testing #11 of rolling checksum, file [%s], size [%zu], block size L [%zu]", fname, file_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%zu] is too small for this test (should be > [%zu]), skipping file [%s]", L, 0, fname);
                continue;
            }
            RM_LOG_INFO("Testing rolling checksum procedure #11: file [%s], size [%zu], block size L [%zu]", fname, file_sz, L);

            f_x = f;
            fd = fileno(f_y);
            if (fstat(fd, &fs) != 0) {
                RM_LOG_PERR("Can't fstat file [%s]", rm_state->f2.name);
                fclose(f_y);
                assert_true(1 == 0 && "Can't fstat file");
            }
            y_sz = fs.st_size;
            y = rm_state->f2.name;

            blocks_n_exp = y_sz / L + (y_sz % L ? 1 : 0); /* split @y file into non-overlapping blocks and calculate checksums on these blocks, expected number of blocks is */
            err = rm_rx_insert_nonoverlapping_ch_ch_ref(f_y, y, h, L, NULL, blocks_n_exp, &blocks_n);
            assert_int_equal(err, RM_ERR_OK);
            assert_int_equal(blocks_n_exp, blocks_n);
            rewind(f_y);

            s = rm_state->s; /* run rolling checksum procedure */
            memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx)); /* init reconstruction context */
            s->rec_ctx.L = L;
            s->rec_ctx.copy_all_threshold = file_sz + 1;
            s->rec_ctx.copy_tail_threshold = 0;
            s->rec_ctx.send_threshold = L;
            prvt = s->prvt;
            prvt->h = h;
            prvt->f_x = f_x;
            prvt->delta_f = rm_roll_proc_cb_1;
            err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, 0);    /* 1. run rolling checksum procedure */
            assert_int_equal(err, RM_ERR_OK);

            q = &prvt->tx_delta_e_queue; /* verify s->prvt delta queue content */
            assert_true(q != NULL);

            rec_by_ref = rec_by_raw = 0;
            delta_ref_n = delta_raw_n = 0;
            rec_by_tail = delta_tail_n = 0;
            rec_by_zero_diff = delta_zero_diff_n = 0;
            for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh)) {    /* dequeue, so can free later */
                delta_e = tw_container_of(lh, struct rm_delta_e, link);
                switch (delta_e->type) {
                    case RM_DELTA_ELEMENT_REFERENCE:
                        rec_by_ref += L;
                        ++delta_ref_n;
                        break;
                    case RM_DELTA_ELEMENT_RAW_BYTES:
                        rec_by_raw += delta_e->raw_bytes_n;
                        ++delta_raw_n;
                        break;
                    case RM_DELTA_ELEMENT_ZERO_DIFF:
                        rec_by_ref += delta_e->raw_bytes_n; /* delta ZERO_DIFF has raw_bytes_n set to indicate bytes that matched
                                                               (whole file) so we can nevertheless check at receiver that is correct */
                        ++delta_ref_n;

                        rec_by_zero_diff += delta_e->raw_bytes_n;
                        ++delta_zero_diff_n;
                        break;
                    case RM_DELTA_ELEMENT_TAIL:
                        rec_by_ref += delta_e->raw_bytes_n; /* delta TAIL has raw_bytes_n set to indicate bytes that matched
                                                               (that tail) so we can nevertheless check at receiver there is no error */
                        ++delta_ref_n;

                        rec_by_tail += delta_e->raw_bytes_n;
                        ++delta_tail_n;
                        break;
                    default:
                        RM_LOG_ERR("%s", "Unknown delta element type!");
                        assert_true(1 == 0 && "Unknown delta element type!");
                }
                free((void*) delta_e);  /* free delta element */
            }

            /* general tests */
            assert_true(delta_raw_n == 0);
            assert_int_equal(rec_by_raw, 0);
            assert_true(delta_ref_n == 1);
            assert_int_equal(rec_by_ref, file_sz);
            assert_true(delta_tail_n == 0);
            assert_true(rec_by_tail == 0);
            assert_true(delta_zero_diff_n == 1);
            assert_true(rec_by_zero_diff == file_sz);
            
            RM_LOG_INFO("PASSED test #11 (copy tail threshold): file [%s], size [%zu], L [%zu], blocks [%zu], DELTA REF [%zu] bytes [%zu], DELTA ZERO DIFF [%zu] bytes [%zu]",
                    fname, file_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_zero_diff_n, rec_by_zero_diff);
            rewind(f);
            blocks_n = 0;
            bkt = 0;
            twhash_for_each_safe(h, bkt, tmp, e, hlink) {
                twhash_del((struct twhlist_node*)&e->hlink);
                free((struct rm_ch_ch_ref_hlink*)e);
                ++blocks_n;
            }
            assert_int_equal(blocks_n_exp, blocks_n);
            rewind(f_x);
            rewind(f_y);
        }
        fclose(f_x);
        fclose(f_y);
    }
    RM_LOG_INFO("%s", "PASSED test #11 (send threshold)");
    return;
}
