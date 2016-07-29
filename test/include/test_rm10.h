/* @file        test_rm10.h
 * @brief       Test suite #10.
 * @details     Black box testing of local push (using commandline utility).
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        8 July 2016 07:08 PM
 * @copyright   LGPLv2.1 */


#ifndef RSYNCME_TEST_RM10_H
#define RSYNCME_TEST_RM10_H


#include "rm_defs.h"
#include "rm.h"
#include "rm_rx.h"
#include "rm_tx.h"
#include "rm_error.h"


#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>


#define RM_TEST_10_DELETE_FILES     1	/* 0 no, 1 yes */
#define RM_TEST_L_BLOCKS_SIZE       34
#define RM_TEST_L_MAX               1024UL
#define RM_TEST_FNAMES_N            15
#define RM_TEST_10_CMD_LEN_MAX      300
#define RM_TEST_10_5_FILE_IDX       3u

const char* rm_test_fnames[RM_TEST_FNAMES_N];
size_t    rm_test_fsizes[RM_TEST_FNAMES_N];
size_t    rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE];

struct test_rm_file {
    char    name[RM_FILE_LEN_MAX + 100];
};

struct test_rm_state
{
    struct test_rm_file f_z;
    struct test_rm_file f1, f2; /* used as non-existing @x and @y */
    uint8_t             tmp_dir_created, f1_created, f2_created;
    char                tmp_dir_name[38]; /* used as dir for target file @z */
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


/* @brief   Test if result file @f_z is reconstructed properly
 *          when x file is same as y (file has no changes),
 *          block size -l is not set (use default setting) */
void
test_rm_cmd_1(void **state);

/* @brief   Test if result file @f_z is reconstructed properly
 *          when x file is same as y (file has no changes),
 *          block size -l is set */
void
test_rm_cmd_2(void **state);

/* @brief   Test if result file @f_z is reconstructed in proper path
 *          when x file is same as y (file has no changes),
 *          -z flag is set. @x and @y are in same folder, target file @z
 *          is created in different folder */
void
test_rm_cmd_3(void **state);

/* @brief   Test --leave flag: if result file @f_z is reconstructed in proper path
 *          and if @y has been left in it's folder. @x is same as @y,
 *          -z flag  and --leave flag are set. @x and @y are in same folder,
 *          target file @z is created in different folder */
void
test_rm_cmd_4(void **state);

/* @brief   Test error reporting: @x not specified */
void
test_rm_cmd_5(void **state);


#endif	/* RSYNCME_TEST_RM10_H */
