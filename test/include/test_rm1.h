/*
 * @file        test_rm1.h
 * @brief       Test suite #1.
 * @details     Test of rolling checksums and of nonoverlapping
 *              checksums calculation correctness.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        10 Jan 2016 04:07 PM
 * @copyright   LGPLv2.1
 */


#ifndef RSYNCME_TEST_RM1_H
#define RSYNCME_TEST_RM1_H


#include "rm_defs.h"
#include "rm.h"
#include "rm_rx.h"
#include "rm_error.h"


#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>


#define RM_TEST_DELETE_FILES        1	/* 0 no, 1 yes */
#define RM_TEST_L_BLOCKS_SIZE       26U
#define RM_TEST_L_MAX               1024UL
#define RM_TEST_FNAMES_N            13U
#define RM_TEST_1_2_BUF_SZ          10u
const char* rm_test_fnames[RM_TEST_FNAMES_N];
size_t      rm_test_fsizes[RM_TEST_FNAMES_N];
size_t      rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE];
unsigned char file_content_payload[RM_TEST_1_2_BUF_SZ];

struct test_rm_file {
    FILE    *f;
    char    name[RM_FILE_LEN_MAX + 100];
};

struct test_rm_state
{
	size_t  *l;
    size_t  array_entries;
    struct rm_ch_ch *array; /* will be big enough to serve as storage
                               for checksums for each test file */
    struct test_rm_file f_2;
};

struct test_rm_state	rm_state;	/* global tests state */

/* @brief   The setup function which is called before
 *          all unit tests are executed.
 * @details Handles all side-effects: allocates memory needed
 *          by tests, makes IO system calls, cancels test suite
 *          run if preconditions can't be met. */
int
test_rm_setup(void **state);

/* @brief   The teardown function  called after all
 *          tests have finished. */
int
test_rm_teardown(void **state);

void
test_rm_copy_buffered(void **state);

void
test_rm_copy_buffered_2_1(void **state);

void
test_rm_copy_buffered_2_2(void **state);

void
test_rm_copy_buffered_offset(void **state);

void
test_rm_adler32_1(void **state);

void
test_rm_adler32_2(void **state);

void
test_rm_fast_check_roll(void **state);

/* @brief   Test of checksums calculation on nonoverlapping blocks.
 * @details Tests number of blocks used (and insertions into array). */
void
test_rm_rx_insert_nonoverlapping_ch_ch_array_1(void **state);


#endif	/* RSYNCME_TEST_RM1_H */
