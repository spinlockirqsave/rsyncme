/*
 * @file        test_rm3.c
 * @brief       Test suite #3.
 * @details     Test of local nonoverlapping
 *              checksums calculation correctness.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        6 Mar 2016 11:29 PM
 * @copyright   LGPLv2.1
 */


#include "test_rm3.h"


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
    int         err;
    uint32_t    i,j;
    FILE        *f;
    void		*buf;

#ifdef DEBUG
    err = rm_util_chdir_umask_openlog(
            "../build/debug", 1, "rsyncme_test_3");
#else
    err = rm_util_chdir_umask_openlog(
            "../build/release", 1, "rsyncme_test_3");
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
        RM_LOG_ERR("Can't allocate memory buffer"
                " of [%u] bytes, malloc failed", j);
	}
    assert_true(buf != NULL);
    rm_state.buf = buf;
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
    return 0;
}

void
test_rm_rx_insert_nonoverlapping_ch_ch_local_1(void **state)
{
    FILE                    *f;
    int                     fd;
    int                     res;
    uint32_t                i, j, L, file_sz, blocks_n;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    char                    *fname;
    size_t                  entries_n;
    struct rm_ch_ch_ref_link    *e;
    struct twlist_head      *pos, *tmp, l = TWLIST_HEAD_INIT(l);

    rm_state = *state;
    assert_true(rm_state != NULL);

    /* test on all files */
    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i)
    {
        fname = rm_test_fnames[i];
        f = fopen(fname, "rb");
        if (f == NULL)
            RM_LOG_PERR("Can't open file [%s]", fname);
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

            RM_LOG_INFO("Testing of splitting file into non-overlapping "
                    "blocks: file [%s], size [%u], block size L [%u], buffer"
                    " [%u]", fname, file_sz, L, RM_TEST_L_MAX);
            /* number of blocks */
            blocks_n = file_sz / L + (file_sz % L ? 1 : 0);
            TWINIT_LIST_HEAD(&l);
            res = rm_rx_insert_nonoverlapping_ch_ch_local(f, fname, &l, L, &entries_n);
            assert_int_equal(res, 0u);
            assert_int_equal(entries_n, blocks_n);

            /* free list entries */
            blocks_n = 0;
            twlist_for_each_safe(pos, tmp, &l)
            {
                e = tw_container_of(pos, struct rm_ch_ch_ref_link, link); 
                free(e);
                ++blocks_n;
            }
            assert_int_equal(entries_n, blocks_n);

            RM_LOG_INFO("PASSED test of hashing of non-overlapping"
                    " blocks, file [%s], size [%u], L [%u], blocks [%u]",
                    fname, file_sz, L, blocks_n);
            /* move file pointer back to the beginning */
            rewind(f);
        }
        fclose(f);
    }
}

void
test_rm_rx_insert_nonoverlapping_ch_ch_local_2(void **state)
{
    FILE                    *f;
    int                     fd;
    int                     res;
    uint32_t                i, j, L, file_sz, blocks_n;
    uint32_t                read_left, read_now, read;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    char                    *fname;
    size_t                  entries_n;
    struct rm_ch_ch_ref_link    *e;
    struct rm_md5           s_ch;
    unsigned char           *buf;
    struct twlist_head      *pos, l = TWLIST_HEAD_INIT(l);

    rm_state = *state;
    assert_true(rm_state != NULL);

    /* test on all files */
    i = 0;
    for (; i < RM_TEST_FNAMES_N; ++i)
    {
        fname = rm_test_fnames[i];
        f = fopen(fname, "rb");
        if (f == NULL)
            RM_LOG_PERR("Can't open file [%s]", fname);
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

            RM_LOG_INFO("Testing of splitting file into non-overlapping "
                    "blocks: file [%s], size [%u], block size L [%u], buffer"
                    " [%u]", fname, file_sz, L, RM_TEST_L_MAX);
            /* number of blocks */
            blocks_n = file_sz / L + (file_sz % L ? 1 : 0);
            TWINIT_LIST_HEAD(&l);
            res = rm_rx_insert_nonoverlapping_ch_ch_local(f, fname, &l, L, &entries_n);
            assert_int_equal(res, 0u);
            assert_int_equal(entries_n, blocks_n);
            rewind(f);

            /* check hashes and free list entries */
            blocks_n = 0;
            /* read L bytes chunks */
            read_left = file_sz;
            read_now = rm_min(L, read_left);
            buf = rm_state->buf;
            pos = l.next;
            do
            {
                read = fread(buf, 1, read_now, f);
                if (read != read_now)
                {
                    RM_LOG_PERR("Error reading file [%s] "
                    "(THIS IS SYSTEM ERROR NOT RELATED TO OUR METHOD"
                    " BEING TESTED ! [AND IT SHOULDN'T HAPPEN!]", fname);
                    assert_true(1 == 0);
                }

                assert_true(pos != &l);
                e = tw_container_of(pos, struct rm_ch_ch_ref_link, link); 
                /* check fast checksum */
                assert_true(e->f_ch == rm_fast_check_block(buf, read));
                /* check strong checksum */
                rm_md5(buf, read, s_ch.data);
                assert_true(memcmp(e->s_ch.data,
                            s_ch.data, RM_STRONG_CHECK_BITS) == 0);

                ++blocks_n;

                /* next list entry */
                pos = pos->next;
                free(e);
                read_left -= read;
                read_now = rm_min(L, read_left);
            } while (read_now > 0 && pos != &l);

            assert_int_equal(entries_n, blocks_n);

            RM_LOG_INFO("PASSED test of hashing of non-overlapping"
                    " blocks, file [%s], size [%u], L [%u], blocks [%u]",
                    fname, file_sz, L, blocks_n);
            /* move file pointer back to the beginning */
            rewind(f);
        }
        fclose(f);
    }
}
