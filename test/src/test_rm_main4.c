/*
 * @file        test_rm_main4.c
 * @brief       Execution of test suite #4
 *              (rm_rx_insert_nonoverlapping_ch_ch_ref).
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        24 Apr 2016 10:39 AM
 * @copyright	LGPLv2.1
 */


#include "rm_defs.h"
#include "test_rm4.h"


int main(void)
{
    const struct CMUnitTest tests[] = {
	    cmocka_unit_test(test_rm_rx_insert_nonoverlapping_ch_ch_ref_1),
	    cmocka_unit_test(test_rm_rx_insert_nonoverlapping_ch_ch_ref_2),
	    cmocka_unit_test(test_rm_rx_insert_nonoverlapping_ch_ch_ref_3)
    };
    return cmocka_run_group_tests(tests,
		test_rm_setup, test_rm_teardown);
}
