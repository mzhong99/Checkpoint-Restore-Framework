#include "crheap.h"

#include "unittest.h"
#include "memcheck.h"

#include "nvaddrlist_test.h"
#include "nvblock_test.h"
#include "nvaddrtable_test.h"
#include "nvstore_test.h"
#include "memcheck_test.h"
#include "crmalloc_test.h"

/**
 * Usually, I try to keep a strict line limit of 80 characters. This is the only
 * place where I'm going to break that rule, simply because the formatting looks
 * really stupid when you actually break each function into two lines.
 */
void run_all_tests()
{
    /**************************************************************************/
    /** Tests: nvaddrlist --------------------------------------------------- */
    /**************************************************************************/
    run_test(test_nvaddrlist_init, "nvaddrlist", "Basic initialization");
    run_test(test_nvaddrlist_basic_insertion, "nvaddrlist", "Small insertions");
    run_test(test_nvaddrlist_large_insertion, "nvaddrlist", "Large insertions expand the list");

    /**************************************************************************/
    /** Tests: nvblock ------------------------------------------------------ */
    /**************************************************************************/
    run_test(test_nvblock_basic, "nvblock", "Basic initialization with both prevaddr and NULL");
    run_test(test_nvblock_advanced, "nvblock", "Advanced usage with varying page demands");

    /**************************************************************************/
    /** Tests: nvaddrtable -------------------------------------------------- */
    /**************************************************************************/
    run_test(test_nvaddrtable_init, "nvaddrtable", "Basic initialization");
    run_test(test_nvaddrtable_basic_insertion, "nvaddrtable", "Basic insertion for one block");
    run_test(test_nvaddrtable_expansion, "nvaddrtable", "More insertions expand the table");
    run_test(test_nvaddrtable_large_entries, "nvaddrtable", "Insertions of larger than one page");

    /**************************************************************************/
    /** Tests: nvstore ------------------------------------------------------ */
    /**************************************************************************/
    run_test(test_nvstore_init, "nvstore", "Basic initialization and shutdown");
    run_test(test_nvstore_alloc_simple, "nvstore", "Allocation and accessing a single page");
    run_test(test_nvstore_alloc_complex, "nvstore", "Allocation and accessing many pages");
    run_test(test_nvstore_checkpoint_simple, "nvstore", "Simple data checkpointing and restoration");
    run_test(test_nvstore_checkpoint_complex, "nvstore", "Complex data checkpointing and restoration");

    /**************************************************************************/
    /** Tests: memcheck ----------------------------------------------------- */
    /**************************************************************************/
    run_test(test_memcheck_malloc_simple, "memcheck", "Basic mc_malloc() usage");
    run_test(test_memcheck_malloc_complex, "memcheck", "Complex mc_malloc() usage");
    run_test(test_memcheck_mmap_simple, "memcheck", "mc_mmap() and mc_munmap() usage");
    run_test(test_memcheck_complex, "memcheck", "Allocations with mc_malloc() and mc_mmap()");

    /**************************************************************************/
    /** Tests: crmalloc ----------------------------------------------------- */
    /**************************************************************************/
    run_test(test_crmalloc_simple, "crmalloc", "Basic crmalloc() and crfree()");
    run_test(test_crmalloc_complex, "crmalloc", "Complex crmalloc() and crfree()");
    run_test(test_crmalloc_integration, "crmalloc", "Integration of crmalloc(), crfree(), and crrealloc()");

    mc_report();
}

int main()
{
    run_all_tests();
}
