/* @file        test_rm_main10.c
 * @brief       Execution of test suite 10 (commandline utility).
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        08 July 2016 07:08 PM
 * @copyright	LGPLv2.1 */


#include "rm_defs.h"
#include "test_rm10.h"


int main(void) {
    const struct CMUnitTest tests[] = {
	    cmocka_unit_test(test_rm_cmd_1),
	    cmocka_unit_test(test_rm_cmd_2),
	    cmocka_unit_test(test_rm_cmd_3),
	    cmocka_unit_test(test_rm_cmd_4),
	    cmocka_unit_test(test_rm_cmd_5),
	    cmocka_unit_test(test_rm_cmd_6),
	    cmocka_unit_test(test_rm_cmd_7),
	    cmocka_unit_test(test_rm_cmd_8),
	    cmocka_unit_test(test_rm_cmd_9)
    };
    return cmocka_run_group_tests(tests, test_rm_setup, test_rm_teardown);
}
