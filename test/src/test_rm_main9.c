/* @file        test_rm_main9.c
 * @brief       Execution of test suite #9 (checking of local push error checking).
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        20 June 2016 04:47 PM
 * @copyright	LGPLv2.1 */


#include "rm_defs.h"
#include "test_rm9.h"


int main(void) {
    const struct CMUnitTest tests[] = {
	    cmocka_unit_test(test_rm_local_push_err_1)
    };
    return cmocka_run_group_tests(tests, test_rm_setup, test_rm_teardown);
}
