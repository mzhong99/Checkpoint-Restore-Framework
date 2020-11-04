#include "crheap.h"

#include "unittest.h"
#include "memcheck.h"

#include "vaddrlist_test.h"
#include "vblock_test.h"
#include "vtsaddrtable_test.h"
#include "vtslist_test.h"
#include "nvstore_test.h"
#include "memcheck_test.h"
#include "crmalloc_test.h"
#include "crthread_test.h"
#include "checkpoint_test.h"

/**
 * Usually, I try to keep a strict line limit of 80 characters. This is the only
 * place where I'm going to break that rule, simply because the formatting looks
 * really stupid when you actually break each function into two lines.
 */
void run_all_tests()
{
    /**************************************************************************/
    /** Tests: vaddrlist ---------------------------------------------------- */
    /**************************************************************************/
    run_test(test_vaddrlist_init, "vaddrlist", "Basic initialization");
    run_test(test_vaddrlist_basic_insertion, "vaddrlist", "Small insertions");
    run_test(test_vaddrlist_large_insertion, "vaddrlist", "Large insertions expand the list");

    /**************************************************************************/
    /** Tests: vblock ------------------------------------------------------ */
    /**************************************************************************/
    run_test(test_vblock_basic, "vblock", "Basic initialization with both prevaddr and NULL");
    run_test(test_vblock_advanced, "vblock", "Advanced usage with varying page demands");

    /**************************************************************************/
    /** Tests: vtsaddrtable --------------------------------------------------- */
    /**************************************************************************/
    run_test(test_vtsaddrtable_init, "vtsaddrtable", "Basic initialization");
    run_test(test_vtsaddrtable_basic_insertion, "vtsaddrtable", "Basic insertion for one block");
    run_test(test_vtsaddrtable_expansion, "vtsaddrtable", "More insertions expand the table");
    run_test(test_vtsaddrtable_large_entries, "vtsaddrtable", "Insertions of larger than one page");

    /**************************************************************************/
    /** Tests: nvstore ------------------------------------------------------ */
    /**************************************************************************/
    run_test(test_nvstore_init, "nvstore", "Basic initialization and shutdown");
    run_test(test_nvstore_alloc_simple, "nvstore", "Allocation and accessing a single page");
    run_test(test_nvstore_alloc_complex, "nvstore", "Allocation and accessing many pages");
    run_test(test_nvstore_checkpoint_simple, "nvstore", "Simple data checkpointing and restoration");
    run_test(test_nvstore_checkpoint_complex, "nvstore", "Complex data checkpointing and restoration");
    run_test(test_nvstore_checkpoint_without_shutdown, "nvstore", "Checkpoint twice before shutdown");

    /**************************************************************************/
    /** Tests: memcheck ----------------------------------------------------- */
    /**************************************************************************/
    run_test(test_memcheck_malloc_simple, "memcheck", "Basic mcmalloc() usage");
    run_test(test_memcheck_malloc_complex, "memcheck", "Complex mcmalloc() usage");
    run_test(test_memcheck_mmap_simple, "memcheck", "mcmmap() and mcmunmap() usage");
    run_test(test_memcheck_complex, "memcheck", "Allocations with mcmalloc() and mcmmap()");

    /**************************************************************************/
    /** Tests: crmalloc ----------------------------------------------------- */
    /**************************************************************************/
    run_test(test_crmalloc_simple, "crmalloc", "Basic crmalloc() and crfree()");
    run_test(test_crmalloc_complex, "crmalloc", "Complex crmalloc() and crfree()");
    run_test(test_crmalloc_recovery, "crmalloc", "Heap checkpointing and restoration");
    run_test(test_crmalloc_integration, "crmalloc", "Integration of crmalloc(), crfree(), and crrealloc()");

    /**************************************************************************/
    /** Tests: vtslist ------------------------------------------------------ */
    /**************************************************************************/
    run_test(test_vtslist_message_passing, "vtslist", "Thread-safe message passing");

    /**************************************************************************/
    /** Tests: checkpoint --------------------------------------------------- */
    /**************************************************************************/
    run_test(test_checkpoint_basic, "checkpoint", "Basic sequential checkpointing capacity");
    run_test(test_checkpoint_stack, "checkpoint", "Tests the ability to checkpoint a thread's stack");

    /**************************************************************************/
    /** Tests: crthread ----------------------------------------------------- */
    /**************************************************************************/
    run_test(test_crthread_basic, "crthread", "Basic crthread test");


    mcreport();
}

int main()
{
    run_all_tests();
}
