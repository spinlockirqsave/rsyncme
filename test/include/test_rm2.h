/// @file	test_rm2.h
/// @author	peterg
/// @version	0.1.1
/// @date	24 Jan 2016 06:19 PM
/// @copyright	LGPLv2.1


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


#define RM_TEST_DELETE_FILES		1	// 0 no, 1 yes
#define RM_TEST_L_BLOCKS_SIZE		26
#define RM_TEST_L_MAX			1024UL
#define RM_TEST_FNAMES_N		13
char*		rm_test_fnames[RM_TEST_FNAMES_N];
uint32_t	rm_test_fsizes[RM_TEST_FNAMES_N];
uint32_t	rm_test_L_blocks[RM_TEST_L_BLOCKS_SIZE];

struct test_rm_state
{
	uint32_t	*l;
};

struct test_rm_state	rm_state;	// global tests state

/// The setup function which is called before
/// all unit tests are executed.
/// Handles all side-effects: allocates memory needed
/// by tests, makes IO system calls, cancels test suite
/// run if preconditions can't be met. 
int
test_rm_setup(void **state);

/// The teardown function  called after all
/// tests have finished.
int
test_rm_teardown(void **state);

int
__wrap_fread(void);

/// @brief	Test of checksums calculation on nonoverlapping
///		blocks of. Tests reporting of error after failed
///		call to fread.
void
test_rm_rx_insert_nonoverlapping_ch_ch_2(void **state);


#endif	// RSYNCME_TEST_RM2_H
