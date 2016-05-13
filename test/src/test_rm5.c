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


const char* rm_test_fnames[RM_TEST_FNAMES_N] = { "rm_f_0_ts5", "rm_f_1_ts5",
    "rm_f_2_ts5","rm_f_65_ts5", "rm_f_100_ts5", "rm_f_511_ts5", "rm_f_512_ts5",
    "rm_f_513_ts5", "rm_f_1023_ts5", "rm_f_1024_ts5", "rm_f_1025_ts5",
    "rm_f_4096_ts5", "rm_f_20100_ts5"};

uint32_t	rm_test_fsizes[RM_TEST_FNAMES_N] = { 0, 1, 2, 65, 100, 511, 512, 513,
						1023, 1024, 1025, 4096, 20100 };

uint32_t
rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE] = { 0, 1, 13, 50, 64, 100, 127, 128, 129,
					200, 400, 499, 500, 501, 511, 512, 513,
					600, 800, 1000, 1100, 1123, 1124, 1125,
					1200, 100000 };

static int
test_rm_copy_files_and_prefix(const char *postfix)
{
    int         err;
    FILE        *f, *f_copy;
    uint32_t    i, j;
    char        buf[RM_FILE_LEN_MAX + 50];

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
                fputc(rand(), f);
        } else {
            RM_LOG_INFO("Using previously created "
                    "file [%s]", rm_test_fnames[i]);
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
            f_copy = fopen(buf, "wb+");
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
test_rm_delete_copies_of_files_prefixed(const char *postfix)
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
        RM_LOG_ERR("Removing (unlink) [%s] copy of file [%s]", buf, rm_test_fnames[i]);
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
test_rm_setup(void **state)
{
    int         err;
    uint32_t    i,j;
    FILE        *f;
    void        *buf;
    struct rm_session   *s;

#ifdef DEBUG
    err = rm_util_chdir_umask_openlog(
            "../build/debug", 1, "rsyncme_test_5");
#else
    err = rm_util_chdir_umask_openlog(
            "../build/release", 1, "rsyncme_test_5");
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
                fputc(rand(), f);
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
    buf = malloc(j);
    if (buf == NULL)	
    {
        RM_LOG_ERR("Can't allocate 1st memory buffer"
                " of [%u] bytes, malloc failed", j);
	}
    assert_true(buf != NULL);
    rm_state.buf = buf;
    buf = malloc(j);
    if (buf == NULL)	
    {
        RM_LOG_ERR("Can't allocate 2nd memory buffer"
                " of [%u] bytes, malloc failed", j);
	}
    assert_true(buf != NULL);
    rm_state.buf2 = buf;

    /* session for loccal push */
    s = rm_session_create(RM_PUSH_LOCAL);
    if (s == NULL)
    {
        RM_LOG_ERR("Can't allocate session local push");
	}
    assert_true(s != NULL);
    rm_state.s = s;
    return 0;
}

int
test_rm_teardown(void **state)
{
    int     i;
    FILE    *f;
    struct  test_rm_state *rm_state;

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
    free(rm_state->buf);
    free(rm_state->buf2);
    rm_session_free(rm_state->s);
    return 0;
}


/* @brief   Test if created delta elements cover all file. */
void
test_rm_rolling_ch_proc_1(void **state)
{
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
    size_t                      rec_by_ref, rec_by_raw, delta_ref_n, delta_raw_n;

    TWDEFINE_HASHTABLE(h, RM_NONOVERLAPPING_HASH_BITS);
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
            RM_LOG_INFO("Validating testing of checksum "
                    "correctness, file [%s], size [%u],"
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
                RM_LOG_INFO("File [%s] size [%u] is too small "
                        "for this test, skipping", fname, file_sz);
                continue;
            }
            RM_LOG_INFO("Testing rolling checksum procedure #1: "
                    "file [%s], size [%u], block size L [%u]",
                    fname, file_sz, L);

            /* reference file exists, split it and calc checksums */
            f_y = f;
            f_x = f;
            y_sz = fs.st_size;
            y = fname;

            /* split @y file into non-overlapping blocks
             * and calculate checksums on these blocks,
             * expected number of blocks is */
            blocks_n_exp = y_sz / L + (y_sz % L ? 1 : 0);
            err = rm_rx_insert_nonoverlapping_ch_ch_ref(
                f_y, y, h, L, NULL, blocks_n_exp, &blocks_n);
            assert_int_equal(err, 0);
            assert_int_equal(blocks_n_exp, blocks_n);
            rewind(f_y);

            /* run rolling checksum procedure */
            s = rm_state->s;
            s->L = L;
            /* setup private session's arguments */
            prvt = s->prvt;
            prvt->h = h;
            prvt->f_x = f_x;                        /* run on same file */
            prvt->delta_f = rm_roll_proc_cb_1;
            /* 1. run rolling checksum procedure */
            err = rm_rolling_ch_proc(s, h, prvt->f_x, prvt->delta_f, s->L, 0);
            assert_int_equal(err, 0);

            /* verify s->prvt delta queue content */
            q = &prvt->tx_delta_e_queue;
            assert_true(q != NULL);

            rec_by_ref = rec_by_raw = 0;
            delta_ref_n = delta_raw_n = 0;
            for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh))
            {
                delta_e = tw_container_of(lh, struct rm_delta_e, link);
                switch (delta_e->type)
                {
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
                        break;
                    case RM_DELTA_ELEMENT_TAIL:
                        rec_by_ref += delta_e->raw_bytes_n; /* delta TAIL has raw_bytes_n set to indicate bytes that matched
                                                               (that tail) so we can nevertheless check at receiver there is no error */
                        ++delta_ref_n;
                        break;
                    default:
                        RM_LOG_ERR("Unknown delta element type!");
                        assert_true(1 == 0 && "Unknown delta element type!");
                }
            }
            assert_int_equal(rec_by_ref + rec_by_raw, y_sz);

            RM_LOG_INFO("PASSED test #1 of rolling checksum procedure: "
                    "delta elements cover whole file, file [%s], size [%u], "
                    "L [%u], blocks [%u], DELTA REF [%u] bytes [%u], DELTA RAW [%u] bytes [%u]",
                    fname, file_sz, L, blocks_n, delta_ref_n, rec_by_ref, delta_raw_n, rec_by_raw);
            /* move file pointer back to the beginning */
            rewind(f);

            blocks_n = 0;
            bkt = 0;
            twhash_for_each_safe(h, bkt, tmp, e, hlink)
            {
                twhash_del((struct twhlist_node*)&e->hlink);
                free((struct rm_ch_ch_ref_hlink*)e);
                ++blocks_n;
            }
            assert_int_equal(blocks_n_exp, blocks_n);
			
			/* move file pointer back to the beginning */
			rewind(f);
		}
		fclose(f);
	}
    return;
}

/* @brief   Test #2. */
void
test_rm_rolling_ch_proc_2(void **state)
{
    int err;
    (void)state;

    err = test_rm_copy_files_and_prefix("_test_2");
    if (err != 0)
    {
        RM_LOG_ERR("Error copying files, skipping test");
        return;
    }
    if (RM_TEST_DELETE_FILES == 1)
    {
        err = test_rm_delete_copies_of_files_prefixed("_test_2");
        if (err != 0)
        {
            RM_LOG_ERR("Error removing files (unlink)");
            assert_true(1 == 0 && "Error removing files (unlink)");
            return;
        }
    }
    return;
}
