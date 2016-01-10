/// @file	test_rm.c
/// @author	peterg
/// @version	0.1.1
/// @date	10 Jan 2016 04:13 PM
/// @copyright	LGPLv2.1


#include "test_rm.h"


int
test_rm_setup(void **state)
{
	*state = &rm_state;
	return 0;
}

int
test_rm_teardown(void **state)
{
	struct test_rm_state *rm_state;
	rm_state = *state;
	return 0;
}

void
test_rm_adler32_1(void **state)
{
	(void) state;

	int err;

	assert_true(1 == 1);
}
