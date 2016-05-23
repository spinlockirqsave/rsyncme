/*
 * @file        test_rm_main7.c
 * @brief       Execution of test suite #7
 *              (rm_rx_process_delta_element).
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        20 May 2016 09:08 PM
 * @copyright	LGPLv2.1
 */


#include "rm_defs.h"
#include "test_rm7.h"


int main(void)
{
    const struct CMUnitTest tests[] = {
	    cmocka_unit_test(test_rm_rx_process_delta_element_1),
	    cmocka_unit_test(test_rm_rx_process_delta_element_2)
    };
    return cmocka_run_group_tests(tests,
		test_rm_setup, test_rm_teardown);
}
