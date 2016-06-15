/*
 * @file        test_rm5.c
 * @brief       Test suite #5.
 * @details     Test of rm_rolling_ch_proc.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        06 May 2016 04:00 PM
 * @copyright   LGPLv2.1
 */


#include "test_rm5.h"


const char* rm_test_fnames[RM_TEST_FNAMES_N] = { 
    "rm_f_1_ts5", "rm_f_2_ts5","rm_f_4_ts5", "rm_f_8_ts5", "rm_f_65_ts5",
    "rm_f_100_ts5", "rm_f_511_ts5", "rm_f_512_ts5", "rm_f_513_ts5", "rm_f_1023_ts5",
    "rm_f_1024_ts5", "rm_f_1025_ts5", "rm_f_4096_ts5", "rm_f_7787_ts5", "rm_f_20100_ts5"};

uint32_t	rm_test_fsizes[RM_TEST_FNAMES_N] = { 1, 2, 4, 8, 65,
                                                100, 511, 512, 513, 1023,
                                                1024, 1025, 4096, 7787, 20100 };

uint32_t
rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE] = { 1, 2, 3, 4, 8, 10, 13, 16,
                    24, 32, 50, 64, 100, 127, 128, 129,
                    130, 200, 400, 499, 500, 501, 511, 512,
                    513, 600, 800, 1000, 1100, 1123, 1124, 1125,
                    1200, 100000 };

static int
test_rm_copy_files_and_postfix(const char *postfix)
{
    int         err;
    FILE        *f, *f_copy;
    uint32_t    i, j;
    char        buf[RM_FILE_LEN_MAX + 50];
    unsigned long const seed = time(NULL);

    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i)
    {
        f = fopen(rm_test_fnames[i], "rb");
        if (f == NULL)
        {
            /* file doesn't exist, create */
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
        /* create copy */
        strncpy(buf, rm_test_fnames[i], RM_FILE_LEN_MAX);
        strncpy(buf + strlen(buf), postfix, 49);
        buf[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf, "rb+");
        if (f_copy == NULL)
        {
            /* doesn't exist, create */
            RM_LOG_INFO("Creating copy [%s] of file [%s]", buf, rm_test_fnames[i]);
            f_copy = fopen(buf, "wb");
            if (f_copy == NULL)
            {
                RM_LOG_ERR("Can't open [%s] copy of file [%s]", buf, rm_test_fnames[i]);
                return -1;
            }
            err = rm_copy_buffered(f, f_copy, rm_test_fsizes[i]);
            switch (err)
            {
                case 0:
                    break;
                case -2:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s]", buf, rm_test_fnames[i]);
                    fclose(f);
                    fclose(f_copy);
                    return -2;
                case -3:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s],"
                            " error set on original file", buf, rm_test_fnames[i]);
                    fclose(f);
                    fclose(f_copy);
                    return -3;
                case -4:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s],"
                            " error set on copy", buf, rm_test_fnames[i]);
                    fclose(f);
                    fclose(f_copy);
                    return -4;
                default:
                    RM_LOG_ERR("Can't write from [%s] to it's copy  [%s], unknown error", 
                            buf, rm_test_fnames[i]);
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
test_rm_delete_copies_of_files_postfixed(const char *postfix)
{
    int         err;
    uint32_t    i;
    char        buf[RM_FILE_LEN_MAX + 50];

    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i)
    {
        /* open copy */
        strncpy(buf, rm_test_fnames[i], RM_FILE_LEN_MAX);
        strncpy(buf + strlen(buf), postfix, 49);
        buf[RM_FILE_LEN_MAX + 49] = '\0';
        RM_LOG_INFO("Removing (unlink) [%s] copy of file [%s]", buf, rm_test_fnames[i]);
        err = unlink(buf);
        if (err != 0)
        {
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
    unsigned long const seed = time(NULL);

#ifdef DEBUG
    err = rm_util_chdir_umask_openlog("../build/debug", 1, "rsyncme_test_5");
#else
    err = rm_util_chdir_umask_openlog("../build/release", 1, "rsyncme_test_5");
#endif
    if (err != 0) {
        exit(EXIT_FAILURE);
    }
    rm_state.l = rm_test_L_blocks;
    *state = &rm_state;

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
        RM_LOG_ERR("Can't allocate 1st memory buffer"
                " of [%u] bytes, malloc failed", j);
	}
    assert_true(buf != NULL);
    rm_state.buf = buf;
    buf = malloc(j);
    if (buf == NULL) {
        RM_LOG_ERR("Can't allocate 2nd memory buffer"
                " of [%u] bytes, malloc failed", j);
	}
    assert_true(buf != NULL);
    rm_state.buf2 = buf;

    /* session for loccal push */
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
    if (RM_TEST_5_DELETE_FILES == 1) {
        /* delete all test files */
        i = 0;
        for (; i < RM_TEST_FNAMES_N; ++i) {
            f = fopen(rm_test_fnames[i], "wb+");
            if (f == NULL) {
                RM_LOG_ERR("Can't open file [%s]",
                        rm_test_fnames[i]);	
            } else {
                RM_LOG_INFO("Removing file [%s]",
                        rm_test_fnames[i]);
                remove(rm_test_fnames[i]);
            }
        }
    }
    free(rm_state->buf);
    free(rm_state->buf2);
    rm_session_free(rm_state->s);
    return 0;
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
        assert_true(f != NULL);
        fd = fileno(f);
        if (fstat(fd, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", fname);
            fclose(f);
            assert_true(1 == 0);
        }
        file_sz = fs.st_size; 
        if (file_sz < 2) {
            RM_LOG_INFO("File [%s] size [%u] is too small for this test, skipping", fname, file_sz);
            fclose(f);
            continue;
        }
        detail_case_1_n = 0;
        detail_case_2_n = 0;
        detail_case_3_n = 0;
        j = 0;
        for (; j < RM_TEST_L_BLOCKS_SIZE; ++j) {
            L = rm_test_L_blocks[j];
            RM_LOG_INFO("Validating testing #1 of rolling checksum on tail, file [%s], size [%u], block size L [%u]", fname, file_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%u] is too small for this test (should be > [%u]), skipping file [%s]", L, 0, fname);
                continue;
            }
            RM_LOG_INFO("Testing rolling checksum procedure #1: file [%s], size [%u], block size L [%u]", fname, file_sz, L);

            /* reference file exists, split it and calc checksums */
            f_y = f;
            f_x = f;
            y_sz = fs.st_size;
            y = fname;

            /* split @y file into non-overlapping blocks and calculate checksums on these blocks, expected number of blocks is */
            blocks_n_exp = y_sz / L + (y_sz % L ? 1 : 0);
            err = rm_rx_insert_nonoverlapping_ch_ch_ref(f_y, y, h, L, NULL, blocks_n_exp, &blocks_n);
            assert_int_equal(err, 0);
            assert_int_equal(blocks_n_exp, blocks_n);
            rewind(f_y);

            /* run rolling checksum procedure */
            s = rm_state->s;
            /* init reconstruction context */
            memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx));
            s->rec_ctx.L = L;
            s->rec_ctx.copy_all_threshold = 0;
            s->rec_ctx.copy_tail_threshold = 0;
            s->rec_ctx.send_threshold = L;
            prvt = s->prvt;
            prvt->h = h;
            prvt->f_x = f_x;                        /* run on same file */
            prvt->delta_f = rm_roll_proc_cb_1;
            /* 1. run rolling checksum procedure */
            err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, 0);
            assert_int_equal(err, 0);

            /* verify s->prvt delta queue content */
            q = &prvt->tx_delta_e_queue;
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
                        RM_LOG_ERR("Unknown delta element type!");
                        assert_true(1 == 0 && "Unknown delta element type!");
                }
                free((void*) delta_e);  /* free delta element */
            }

            /* general tests */
            assert_int_equal(rec_by_raw, 0);
            assert_int_equal(rec_by_ref, y_sz);
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
                    RM_LOG_INFO("PASSED test #1: correct number of bytes sent in delta elements, file [%s], size [%u], L [%u], blocks [%u], DELTA REF [%u] bytes [%u], DELTA ZERO DIFF [%u] bytes [%u]",
                        fname, y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_zero_diff_n, rec_by_zero_diff);
                } else {
                    RM_LOG_INFO("PASSED test #1: correct number of bytes sent in delta elements, file [%s], size [%u], L [%u], blocks [%u], DELTA REF [%u] bytes [%u], DELTA RAW [%u] bytes [%u]",
                        fname, y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_raw_n, rec_by_raw);
                    }
            } else {
                RM_LOG_INFO("PASSED test #1: correct number of bytes sent in delta elements, file [%s], size [%u], L [%u], blocks [%u], DELTA REF [%u] bytes [%u] (DELTA_TAIL [%u] bytes [%u]), DELTA RAW [%u] bytes [%u]",
                        fname, y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_tail_n, rec_by_tail, delta_raw_n, rec_by_raw);
            }

            /* move file pointer back to the beginning */
            rewind(f);

            blocks_n = 0;
            bkt = 0;
            twhash_for_each_safe(h, bkt, tmp, e, hlink) {
                twhash_del((struct twhlist_node*)&e->hlink);
                free((struct rm_ch_ch_ref_hlink*)e);
                ++blocks_n;
            }
            assert_int_equal(blocks_n_exp, blocks_n);
			
			/* move file pointer back to the beginning */
			rewind(f);
		}
		fclose(f);
        RM_LOG_INFO("PASSED test #1 detail cases, file [%s], size [%u], detail case #1 [%u] #2 [%u] #3 [%u]", fname, y_sz, detail_case_1_n, detail_case_2_n, detail_case_3_n);
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
        RM_LOG_ERR("Error copying files, skipping test");
        return;
    }

    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
    rm_state = *state;
    assert_true(rm_state != NULL);

    /* test on all files */
    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i) {
        f_y_name = rm_test_fnames[i];
        f_y = fopen(f_y_name, "rb");
        if (f_y == NULL)
        {
            RM_LOG_PERR("Can't open file [%s]", f_y_name);
        }
        assert_true(f_y != NULL);
        /* get file size */
        fd_y = fileno(f_y);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_y, &fs) != 0) {
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
        strncpy(buf_x_name + strlen(buf_x_name), "_test_2", 49);
        buf_x_name[RM_FILE_LEN_MAX + 49] = '\0';
        f_copy = fopen(buf_x_name, "rb+");
        if (f_copy == NULL) {
            RM_LOG_PERR("Can't open file [%s]", buf_x_name);
        }
        f_x = f_copy;
        /* get @x size */
        fd_x = fileno(f_x);
        memset(&fs, 0, sizeof(fs));
        if (fstat(fd_x, &fs) != 0) {
            RM_LOG_PERR("Can't fstat file [%s]", buf_x_name);
            fclose(f_x);
            assert_true(1 == 0);
        }
        f_x_sz = fs.st_size;
        /* read first byte */
        if (rm_fpread(&c, sizeof(unsigned char), 1, 0, f_x) != 1) {
            RM_LOG_ERR("Error reading file [%s], skipping this test", buf_x_name);
            fclose(f_x);
            fclose(f_y);
            continue;
        }
        /* change first byte, so ZERO_DIFF delta can't happen in this test,
         * this would be an error */
        c = (c + 1) % 256;
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
            RM_LOG_INFO("Validating testing #2 of rolling checksum on tail, file [%s], size [%u], block size L [%u]", f_y_name, f_y_sz, L);
            if (0 == L) {
                RM_LOG_INFO("Block size [%u] is too small for this test (should be > [%u]), skipping file [%s]", L, 0, f_y_name);
                continue;
            }
            if (f_y_sz < 2) {
                RM_LOG_INFO("File [%s] size [%u] is too small for this test, skipping", f_y_name, f_y_sz);
                continue;
            }
            RM_LOG_INFO("Testing rolling checksum procedure #2: file @x[%s] size [%u] file @y[%s], size [%u], block size L [%u]", buf_x_name, f_x_sz, f_y_name, f_y_sz, L);

            /* split @y file into non-overlapping blocks and calculate checksums on these blocks, expected number of blocks is */
            blocks_n_exp = f_y_sz / L + (f_y_sz % L ? 1 : 0);
            err = rm_rx_insert_nonoverlapping_ch_ch_ref(f_y, f_y_name, h, L, NULL, blocks_n_exp, &blocks_n);
            assert_int_equal(err, 0);
            assert_int_equal(blocks_n_exp, blocks_n);
            rewind(f_x);
            rewind(f_y);

            /* run rolling checksum procedure on @x */
            s = rm_state->s;
            /* init reconstruction context */
            memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx));
            s->rec_ctx.L = L;
            s->rec_ctx.copy_all_threshold = 0;
            s->rec_ctx.copy_tail_threshold = 0;
            s->rec_ctx.send_threshold = L;
            /* init reconstruction context */
            memset(&s->rec_ctx, 0, sizeof(struct rm_delta_reconstruct_ctx));
            s->rec_ctx.L = L;
            /* setup private session's arguments */
            prvt = s->prvt;
            prvt->h = h;
            prvt->f_x = f_x;                        /* run on @x */
            prvt->delta_f = rm_roll_proc_cb_1;
            /* 1. run rolling checksum procedure */
            err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, 0);
            assert_int_equal(err, 0);

            /* verify s->prvt delta queue content */
            q = &prvt->tx_delta_e_queue;
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
                        RM_LOG_ERR("Unknown delta element type!");
                        assert_true(1 == 0 && "Unknown delta element type!");
                }
                if (delta_e->type == RM_DELTA_ELEMENT_RAW_BYTES) {
                    free(delta_e->raw_bytes);
                }
                free((void*)delta_e);
            }

            /* general tests */
            assert_int_equal(rec_by_ref + rec_by_raw, f_y_sz);
            assert_true(delta_tail_n == 0 || delta_tail_n == 1);
            assert_true(delta_zero_diff_n == 0);
            assert_true(rec_by_zero_diff == 0);

            if (delta_tail_n == 0) {
                if (delta_zero_diff_n > 0) {
                    RM_LOG_INFO("PASSED test #2: delta elements cover whole file, file [%s], size [%u], "
                        "L [%u], blocks [%u], DELTA REF [%u] bytes [%u], DELTA ZERO DIFF [%u] bytes [%u]",
                        f_y_name, f_y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_zero_diff_n, rec_by_zero_diff);
                } else {
                    RM_LOG_INFO("PASSED test #2: delta elements cover whole file, file [%s], size [%u], "
                        "L [%u], blocks [%u], DELTA REF [%u] bytes [%u], DELTA RAW [%u] bytes [%u]",
                        f_y_name, f_y_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_raw_n, rec_by_raw);
                    }
            } else {
                RM_LOG_INFO("PASSED test #2: delta elements cover whole file, file [%s], size [%u], "
                        "L [%u], blocks [%u], DELTA REF [%u] bytes [%u] (DELTA_TAIL [%u] bytes [%u]), DELTA RAW [%u] bytes [%u]",
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
        RM_LOG_INFO("PASSED test #2 detail cases, file [%s], size [%u], detail case #1 [%u] #2 [%u] #3 [%u]",
                f_y_name, f_y_sz, detail_case_1_n, detail_case_2_n, detail_case_3_n);
	}

    if (RM_TEST_5_DELETE_FILES == 1) {
        err = test_rm_delete_copies_of_files_postfixed("_test_2");
        if (err != 0) {
            RM_LOG_ERR("Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}
