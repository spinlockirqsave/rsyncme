/*
 * @file        test_rm4.c
 * @brief       Test suite #4.
 * @details     Test of rm_rx_insert_nonoverlapping_ch_ch_ref.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        24 Apr 2016 10:40 AM
 * @copyright   LGPLv2.1
 */


#include "test_rm4.h"


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
    void        *buf;

#ifdef DEBUG
    err = rm_util_chdir_umask_openlog(
            "../build/debug", 1, "rsyncme_test_4");
#else
    err = rm_util_chdir_umask_openlog(
            "../build/release", 1, "rsyncme_test_4");
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
    return 0;
}

/* #1 */

/* @brief   Tests number of calculated entries. */
void
test_rm_rx_insert_nonoverlapping_ch_ch_ref_1(void **state)
{
	FILE                    *f;
	int                     fd;
    int                     res;
	uint32_t	i, j, L, file_sz;
	struct test_rm_state    *rm_state;
	struct stat             fs;
	char                    *fname;
	size_t                  blocks_n, entries_n;
    size_t                  bkt;    /* hashtable bucket index */
    const struct rm_ch_ch_ref_hlink *e;
    struct twhlist_node     *tmp;

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
			res = rm_rx_insert_nonoverlapping_ch_ch_ref(
					f, fname, h, L, NULL, blocks_n, &entries_n);
            assert_int_equal(res, 0);
			assert_int_equal(entries_n, blocks_n);

            blocks_n = 0;
            bkt = 0;
            twhash_for_each_safe(h, bkt, tmp, e, hlink)
            {
                twhash_del((struct twhlist_node*)&e->hlink);
                free((struct rm_ch_ch_ref_hlink*)e);
                ++blocks_n;
            }
            assert_int_equal(entries_n, blocks_n);
			
			RM_LOG_INFO("PASSED test of hashing of non-overlapping"
				" blocks, file [%s], size [%u], L [%u]", fname,
				file_sz, L);
			/* move file pointer back to the beginning */
			rewind(f);
		}
		fclose(f);
	}
    return;
}

/* #2 */

/* @brief   Artificial function sending checksums to remote A,
 *          here it will simply count the number of times
 *          it was called. */
size_t  f_tx_ch_ch_ref_2_callback_count;
int
f_tx_ch_ch_ref_test_2(const struct f_tx_ch_ch_ref_arg arg)
{
	(void) arg;
    f_tx_ch_ch_ref_2_callback_count++;
	return 0;
}
/* @brief   Tests number of callback calls made. */
void
test_rm_rx_insert_nonoverlapping_ch_ch_ref_2(void **state)
{
	FILE                    *f;
	int                     fd;
	uint32_t	i, j, L, file_sz;
	struct test_rm_state    *rm_state;
	struct stat             fs;
	char                    *fname;
	size_t                  blocks_n, entries_n;
    size_t                  bkt;    /* hashtable bucket index */
    const struct rm_ch_ch_ref_hlink *e;
    struct twhlist_node     *tmp;

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
            /* reset callback counter */
            f_tx_ch_ch_ref_2_callback_count = 0;
			rm_rx_insert_nonoverlapping_ch_ch_ref(
					f, fname, h, L, f_tx_ch_ch_ref_test_2, blocks_n, &entries_n);
            assert_int_equal(f_tx_ch_ch_ref_2_callback_count, blocks_n);

            blocks_n = 0;
            bkt = 0;
            twhash_for_each_safe(h, bkt, tmp, e, hlink)
            {
                twhash_del((struct twhlist_node*)&e->hlink);
                free((struct rm_ch_ch_ref_hlink*)e);
                ++blocks_n;
            }
            assert_int_equal(entries_n, blocks_n);
			
			RM_LOG_INFO("PASSED test of hashing of non-overlapping"
				" blocks, file [%s], size [%u], L [%u]", fname,
				file_sz, L);
			/* move file pointer back to the beginning */
			rewind(f);
		}
		fclose(f);
	}
    return;
}

/* @brief   Test of checksums correctness. */
void
test_rm_rx_insert_nonoverlapping_ch_ch_ref_3(void **state)
{
    FILE                    *f;
    int                     fd;
    int                     res;
    size_t                  i, j, L, file_sz;
    size_t                  read_left, read_now, read;
    unsigned char           *buf, *buf2;
    struct test_rm_state    *rm_state;
    struct stat             fs;
    char                    *fname;
    size_t                  blocks_n, entries_n;
    size_t                  collisions_1st_level, collisions_2nd_level;
    struct rm_ch_ch_ref_hlink e_reference;
    struct twhlist_node     *tmp;

    /* hashtable search */
    unsigned int            bkt;                /* hashtable bucket index */
    unsigned int            hash;               /* iterator over the buckets */
    const struct rm_ch_ch_ref_hlink *e, *e_prev;

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
                RM_LOG_INFO("File [%s] size [%u] is to small "
                        "for this test, skipping", fname, file_sz);
                continue;
            }
            RM_LOG_INFO("Testing checksum correctness: "
                    "file [%s], size [%u], block size L [%u], buffer"
                    " [%u]", fname, file_sz, L, RM_TEST_L_MAX);
            /* number of blocks */
            blocks_n = file_sz / L + (file_sz % L ? 1 : 0);
            res = rm_rx_insert_nonoverlapping_ch_ch_ref(
                    f, fname, h, L, NULL, blocks_n, &entries_n);
            assert_int_equal(res, 0);
            assert_int_equal(entries_n, blocks_n);
            rewind(f);

            /* check hashes and free hashtable entries */
            entries_n = 0;
            /* read L bytes chunks */
            read_left = file_sz;
            read_now = rm_min(L, read_left);
            buf = rm_state->buf;
            buf2 = rm_state->buf2;
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
                
                /* fill in reference data */
                /* compute fast checksum on the block*/
                e_reference.data.ch_ch.f_ch = rm_fast_check_block(buf, read);

                /* compute strong checksum */
                rm_md5(buf, read, e_reference.data.ch_ch.s_ch.data);

                /* assign offset */
                e_reference.data.ref = entries_n;

                /* assert this is also present in the hashtable */
                hash = twhash_min(e_reference.data.ch_ch.f_ch,
                                    RM_NONOVERLAPPING_HASH_BITS);
                e = NULL;
                collisions_1st_level = 0;
                collisions_2nd_level = 0;
                twhlist_for_each_entry(e, &h[hash], hlink)
                {
                    /* hit 1, 1st Level match, hashtable hash match */
                    if (e->data.ch_ch.f_ch == e_reference.data.ch_ch.f_ch)
                    {
                        /* hit 2, 2nd Level match, fast rolling checksum match */
                        if (0 == memcmp(&e->data.ch_ch.s_ch.data,
                                    &e_reference.data.ch_ch.s_ch.data,
                                    RM_STRONG_CHECK_BYTES))
                        {
                            /* hit 3, 3rd Level match, strong checksum match */
                            if (e->data.ref == e_reference.data.ref)
                            {
                                /* OK, FOUND */
                                break;
                            } else {
                                /* references differ but blocks must be the same,
                                 * read at @e offset and compare with current block */
                                read = rm_fpread(buf2, read_now, sizeof(char), e->data.ref, f);
                                if (read != read_now)
                                {
                                    RM_LOG_PERR("Error reading file [%s] "
                                            "(THIS IS SYSTEM ERROR NOT RELATED TO OUR METHOD"
                                            " BEING TESTED ! [AND IT SHOULDN'T HAPPEN!]", fname);
                                    assert_true(1 == 0);
                                }
                                if (0 == memcmp(buf, buf2, read))
                                {
                                    /* OK, blocks are same, just reset the file pointer nad go ahead */
                                    if (fseek(f, entries_n * L + read_now, SEEK_SET) != 0)
                                    {
                                        RM_LOG_PERR("Error reseting file pointer, file [%s] "
                                                "(THIS IS SYSTEM ERROR NOT RELATED TO OUR METHOD"
                                                " BEING TESTED ! [AND IT SHOULDN'T HAPPEN!]", fname);
                                        assert_true(1 == 0);
                                    }
                                    break;
                                }
                                /* collision on 3rd Level, fast checksums are same, strong checksums
                                 * too, but blocks differ, THIS REALLY SHOULDN'T HAPPEN */
                                RM_LOG_ERR("WTF COLLISION 3d Level ref [%u], reference ref [%u]:"
                                        " THIS REALLY SHOULDN'T HAPPEN, please REPORT",
                                        e->data.ref, e_reference.data.ref);
                                assert_true(0 == 1);
                            }
                        } else {
                            /* collision on 2nd Level, strong checksum different
                             * but fast checksums are same (and properly hashed
                             * to the same bucket) */
                            ++collisions_2nd_level;
                        }
                    } else {
                        /* collision on 1st Level, fast checksums are different
                         * but hashed to the same bucket */
                        ++collisions_1st_level;
                    }
                }
                assert_true(e != NULL);
                if ((collisions_1st_level > 0) || (collisions_2nd_level > 0))
                {
                    e_prev = tw_container_of(*e->hlink.pprev, struct rm_ch_ch_ref_hlink, hlink);
                    assert_true(e_prev != NULL);
                    RM_LOG_INFO("COLLISIONS 1st Level [%u], 2nd Level [%u], file [%s], size [%u],"
                            " L [%u], entry [%u], f_ch [%u], prev f_ch [%u], hash [%u]", collisions_1st_level,
                            collisions_2nd_level, fname, file_sz, L, entries_n, e_reference.data.ch_ch.f_ch,
                            e_prev->data.ch_ch.f_ch, hash);
                }

                assert_true(memcmp(&e->data.ch_ch.s_ch.data,
                            &e_reference.data.ch_ch.s_ch.data,
                            RM_STRONG_CHECK_BYTES) == 0 && "WTF f_ch hashed not found!");
                ++entries_n;

                /* next list entry */
                read_left -= read;
                read_now = rm_min(L, read_left);
            } while (read_now > 0);

            assert_int_equal(entries_n, blocks_n);

            RM_LOG_INFO("PASSED test of checksum correctness on non-overlapping"
                    " blocks, file [%s], size [%u], L [%u], blocks [%u]",
                    fname, file_sz, L, blocks_n);
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
            assert_int_equal(entries_n, blocks_n);
			
			/* move file pointer back to the beginning */
			rewind(f);
		}
		fclose(f);
	}
    return;
}
