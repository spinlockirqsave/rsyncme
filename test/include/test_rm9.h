/* @file        test_rm9.h
 * @brief       Test suite #9.
 * @details     Test of local push error reporting.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        20 June 2016 04:48 PM
 * @copyright   LGPLv2.1 */


#ifndef RSYNCME_TEST_RM9_H
#define RSYNCME_TEST_RM9_H


#include "rm_defs.h"
#include "rm.h"
#include "rm_tx.h"
#include "rm_error.h"


#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>


#define RM_TEST_9_DELETE_FILES      1	/* 0 no, 1 yes */
#define RM_TEST_L_BLOCKS_SIZE       34
#define RM_TEST_L_MAX               1024UL
#define RM_TEST_FNAMES_N            15
const char* rm_test_fnames[RM_TEST_FNAMES_N];
size_t      rm_test_fsizes[RM_TEST_FNAMES_N];
size_t      rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE];

int RM_TEST_MOCK_FOPEN;
int RM_TEST_MOCK_FOPEN64;

struct test_rm_state
{
    size_t              *l;
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

FILE*
__real_fopen(const char *path, const char *mode);
FILE*
__real_fopen64(const char *path, const char *mode);
FILE*
__wrap_fopen(const char *path, const char *mode);
FILE*
__wrap_fopen64(const char *path, const char *mode);


/* @brief   Test NULL file pointers */
void
test_rm_local_push_err_1(void **state);


#endif	/* RSYNCME_TEST_RM5_H */
