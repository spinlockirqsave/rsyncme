/*
 * @file        test_rm_main8.c
 * @brief       Execution of test suite #8
 *              (rm_tx_local_push).
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        15 June 2016 10:45 PM
 * @copyright	LGPLv2.1
 */


#include "rm_defs.h"
#include "test_rm8.h"


int main(void) {
    const struct CMUnitTest tests[] = {
	    cmocka_unit_test(test_rm_tx_local_push_1),
	    cmocka_unit_test(test_rm_tx_local_push_2),
	    cmocka_unit_test(test_rm_tx_local_push_3),
	    cmocka_unit_test(test_rm_tx_local_push_4),
	    cmocka_unit_test(test_rm_tx_local_push_5),
	    cmocka_unit_test(test_rm_tx_local_push_6)
    };
    return cmocka_run_group_tests(tests,
		test_rm_setup, test_rm_teardown);
}
