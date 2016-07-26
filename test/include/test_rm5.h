/*
 * @file        test_rm5.h
 * @brief       Test suite #5.
 * @details     Test of rm_rolling_ch_proc.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        06 May 2016 04:00 PM
 * @copyright   LGPLv2.1
 */


#ifndef RSYNCME_TEST_RM5_H
#define RSYNCME_TEST_RM5_H


#include "rm_defs.h"
#include "rm.h"
#include "rm_rx.h"
#include "rm_error.h"


#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>


#define RM_TEST_5_DELETE_FILES      1	/* 0 no, 1 yes */
#define RM_TEST_L_BLOCKS_SIZE       34
#define RM_TEST_L_MAX               1024UL
#define RM_TEST_FNAMES_N            15
const char* rm_test_fnames[RM_TEST_FNAMES_N];
size_t    rm_test_fsizes[RM_TEST_FNAMES_N];
size_t    rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE];

struct test_rm_state
{
	size_t	        *l;
	void                *buf;
	void                *buf2;
    struct rm_session   *s;
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
rm_random_file(char *name, size_t len);


/* @brief   Test if number of bytes enqueued as delta elements is correct,
 *          when x file is same as y (file has no changes). */
void
test_rm_rolling_ch_proc_1(void **state);

/* @brief   Test if number of bytes enqueued as delta elements is correct,
 *          when x is copy of y, but first byte in x is changed. */
void
test_rm_rolling_ch_proc_2(void **state);

/* @brief   Test if number of bytes enqueued as delta elements is correct,
 *          when x is copy of y, but last byte in x is changed. */
void
test_rm_rolling_ch_proc_3(void **state);

/* @brief   Test if number of bytes enqueued as delta elements is correct,
 *          when x is copy of y, but first and last bytes in x are changed. */
void
test_rm_rolling_ch_proc_4(void **state);


#endif	/* RSYNCME_TEST_RM5_H */
