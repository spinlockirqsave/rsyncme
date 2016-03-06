/*
 * @file        test_rm_main2.c
 * @brief       Execution of test suite #2.
 * @author      Piotr Gregor piotrek.gregor at gmail.com
 * @version     0.1.2
 * @date        24 Jan 2016 06:16 PM
 * @copyright	LGPLv2.1
 */


#include "rm_defs.h"
#include "test_rm2.h"


int main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(test_rm_rx_insert_nonoverlapping_ch_ch_2),
	cmocka_unit_test(test_rm_rx_insert_nonoverlapping_ch_ch_3),
	cmocka_unit_test(test_rm_rx_insert_nonoverlapping_ch_ch_4),
	cmocka_unit_test(test_rm_rx_insert_nonoverlapping_ch_ch_5),
	cmocka_unit_test(test_rm_rx_insert_nonoverlapping_ch_ch_6)
    };
    return cmocka_run_group_tests(tests,
		test_rm_setup, test_rm_teardown);
}
