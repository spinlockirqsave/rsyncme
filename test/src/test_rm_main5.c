/*
 * @file        test_rm_main5.c
 * @brief       Execution of test suite #5 (rm_rolling_ch_proc).
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        06 May 2016 04:00 PM
 * @copyright	LGPLv2.1
 */


#include "rm_defs.h"
#include "test_rm5.h"


int main(void)
{
    const struct CMUnitTest tests[] = {
	    cmocka_unit_test(test_rm_rolling_ch_proc_1),
	    cmocka_unit_test(test_rm_rolling_ch_proc_2),
	    cmocka_unit_test(test_rm_rolling_ch_proc_3),
	    cmocka_unit_test(test_rm_rolling_ch_proc_4),
	    cmocka_unit_test(test_rm_rolling_ch_proc_5),
	    cmocka_unit_test(test_rm_rolling_ch_proc_6),
	    cmocka_unit_test(test_rm_rolling_ch_proc_7),
	    cmocka_unit_test(test_rm_rolling_ch_proc_8),
	    cmocka_unit_test(test_rm_rolling_ch_proc_9),
	    cmocka_unit_test(test_rm_rolling_ch_proc_10),
	    cmocka_unit_test(test_rm_rolling_ch_proc_11)
    };
    return cmocka_run_group_tests(tests,
		test_rm_setup, test_rm_teardown);
}
