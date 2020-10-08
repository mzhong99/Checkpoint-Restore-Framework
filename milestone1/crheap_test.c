#include "crheap.h"

#include "unittest.h"

#include "nvaddrlist_test.h"
#include "nvblock_test.h"


void run_all_tests()
{
    run_test(test_nvaddrlist_init, "nvaddrlist", "Basic initialization");
    run_test(test_nvaddrlist_basic_insertion, "nvaddrlist", "Small Insertions");
    run_test(test_nvaddrlist_large_insertion, "nvaddrlist", "Large Insertions");

    run_test(test_nvblock_basic, "nvblock", "Basic initialization");
    run_test(test_nvblock_advanced, "nvblock", "Advanced usage");
}

int main()
{
    run_all_tests();
}
