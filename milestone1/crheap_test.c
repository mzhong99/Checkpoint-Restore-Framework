#include "crheap.h"

#include "unittest.h"

#include "nvaddrlist_test.h"
#include "nvblock_test.h"
#include "nvaddrtable_test.h"

void run_all_tests()
{
    run_test(test_nvaddrlist_init, 
             "nvaddrlist", "Basic initialization");
    run_test(test_nvaddrlist_basic_insertion, 
             "nvaddrlist", "Small insertions");
    run_test(test_nvaddrlist_large_insertion, 
             "nvaddrlist", "Large insertions expand the list");

    run_test(test_nvblock_basic, 
             "nvblock", "Basic initialization with both prevaddr and NULL");
    run_test(test_nvblock_advanced, 
             "nvblock", "Advanced usage with varying page demands");

    run_test(test_nvaddrtable_init, 
             "nvaddrtable", "Basic initialization");
    run_test(test_nvaddrtable_basic_insertion, 
             "nvaddrtable", "Basic insertion for one block");
    run_test(test_nvaddrtable_expansion,
             "nvaddrtable", "More insertions expand the table");
}

int main()
{
    run_all_tests();
}
