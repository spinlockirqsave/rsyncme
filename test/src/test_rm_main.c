/// @file	test_rm_main.c
/// @author	peterg
/// @version	0.1.1
/// @date	10 Jan 2016 04:02 PM
/// @copyright	LGPLv2.1


#include "rm_defs.h"
#include "test_rm.h"


int main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(test_rm_adler32_1),
	cmocka_unit_test(test_rm_adler32_2),
	cmocka_unit_test(test_rm_fast_check_roll),
	cmocka_unit_test(test_rm_rx_insert_nonoverlapping_ch_ch)
    };
    return cmocka_run_group_tests(tests,
		test_rm_setup, test_rm_teardown);
}
