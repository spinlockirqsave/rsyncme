/*
 * @file        test_rm_main3.c
 * @brief       Execution of test suite #3 [local].
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        6 Mar 2016 11:13 PM
 * @copyright	LGPLv2.1
 */


#include "rm_defs.h"
#include "test_rm3.h"


int main(void)
{
    const struct CMUnitTest tests[] = {
	    cmocka_unit_test(test_rm_rx_insert_nonoverlapping_ch_ch_local_1),
	    cmocka_unit_test(test_rm_rx_insert_nonoverlapping_ch_ch_local_2)
    };
    return cmocka_run_group_tests(tests,
		test_rm_setup, test_rm_teardown);
}
