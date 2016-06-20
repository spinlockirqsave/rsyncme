/*
 * @file        test_rm2.h
 * @brief       Test suite #2.
 * @details     Test of nonoverlapping checksums error reporting.
 * @author      Piotr Gregor piotrek.gregor at gmail.com
 * @version     0.1.2
 * @date        24 Jan 2016 06:19 PM
 * @copyright   LGPLv2.1
 */


#ifndef RSYNCME_TEST_RM2_H
#define RSYNCME_TEST_RM2_H


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
size_t      rm_test_fsizes[RM_TEST_FNAMES_N];
size_t      rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE];

int RM_TEST_MOCK_FSTAT;	
int RM_TEST_MOCK_FSTAT64;	
int RM_TEST_MOCK_MALLOC;	
int RM_TEST_MOCK_FREAD;	

struct test_rm_state
{
    size_t  *l;
    void    *buf;
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
__real_fstat(int fd, struct stat *buf);
int
__real_fstat64(int fd, struct stat *buf);
void *
__real_malloc(size_t size);
size_t
__real_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

/* @brief   Mocked fstat function. */
int
__wrap_fstat(int fd, struct stat *buf);
int
__wrap_fstat64(int fd, struct stat *buf);

/* @brief   Mocked malloc function. */
void *
__wrap_malloc(size_t size);

/* @brief   Mocked fread function. */
size_t
__wrap_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

/* @brief   Test of checksums calculation on nonoverlapping
 *          blocks.
 * @details Tests reporting of error after failed
 *          call to fstat. */
void
test_rm_rx_insert_nonoverlapping_ch_ch_ref_2(void **state);

/* @brief   Test of checksums calculation on nonoverlapping
 *          blocks.
 * @details Tests reporting of error after failed call to malloc. */
void
test_rm_rx_insert_nonoverlapping_ch_ch_ref_3(void **state);

/* @brief   Test of checksums calculation on nonoverlapping
 *          blocks.
 * @details Tests reporting of error after failed call to fread. */
void
test_rm_rx_insert_nonoverlapping_ch_ch_ref_4(void **state);

/* @brief   Test of checksums calculation on nonoverlapping blocks.
 * @details Tests reporting of error after failed second call to malloc
 *          (first call is successfull). */
void
test_rm_rx_insert_nonoverlapping_ch_ch_ref_5(void **state);

/* @brief   Test of checksums calculation on nonoverlapping blocks.
 * @details Tests reporting of error after failed call to function
 *          sending checksums to remote A. */
void
test_rm_rx_insert_nonoverlapping_ch_ch_ref_6(void **state);


#endif	// RSYNCME_TEST_RM2_H
