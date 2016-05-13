/*
 * @file        test_rm_main6.c
 * @brief       Execution of test suite #6.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version	    0.1.2
 * @date	    13 May 2016 01:53 PM
 * @copyright   LGPLv2.1
 */


#include "rm_defs.h"
#include "test_rm6.h"


int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_rm_fast_check_roll_tail)
    };
    return cmocka_run_group_tests(tests,
		test_rm_setup, test_rm_teardown);
}
