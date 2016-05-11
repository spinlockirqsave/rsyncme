/*
 * @file        test_rm4.h
 * @brief       Test suite #4.
 * @details     Test of rm_rx_insert_nonoverlapping_ch_ch_ref.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        24 Apr 2016 10:39 AM
 * @copyright   LGPLv2.1
 */


#ifndef RSYNCME_TEST_RM4_H
#define RSYNCME_TEST_RM4_H


#include "rm_defs.h"
#include "rm.h"
#include "rm_rx.h"
#include "rm_error.h"


#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>


#define RM_TEST_DELETE_FILES        1	/* 0 no, 1 yes */
#define RM_TEST_L_BLOCKS_SIZE       26
#define RM_TEST_L_MAX               1024UL
#define RM_TEST_FNAMES_N            13
const char* rm_test_fnames[RM_TEST_FNAMES_N];
uint32_t    rm_test_fsizes[RM_TEST_FNAMES_N];
uint32_t    rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE];

struct test_rm_state
{
	uint32_t	*l;
	void        *buf;
	void        *buf2;
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

int
rm_random_file(char *name, uint32_t len);

/* @brief   Test of checksums calculation on nonoverlapping
 *          blocks.
 * @details Tests number of blocks used (number of computed
 *          checksums and insertions into hashtable). */
void
test_rm_rx_insert_nonoverlapping_ch_ch_ref_1(void **state);

/* @brief   Test the number of callback calls made */
void
test_rm_rx_insert_nonoverlapping_ch_ch_ref_2(void **state);

/* @brief   Test of checksums correctness. */
void
test_rm_rx_insert_nonoverlapping_ch_ch_ref_3(void **state);


#endif	/* RSYNCME_TEST_RM4_H */
