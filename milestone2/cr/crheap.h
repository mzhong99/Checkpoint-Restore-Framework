/**
 * =============================================================================
 * Team SIGSYS(31): Persistent User Programs
 * Developers: Savannah Amos and Matthew Zhong
 *
 * Implementation Notes
 * Milestone One: A Non-Volatile Heap
 * =============================================================================
 *
 *  - Auto-callback upon dereference is actually pretty doable - it requires a
 *    user fault handler which runs in a separate thread. Upon dereferencing, if
 *    a page was written to, we need to checkpoint the page that was targeted.
 *
 *  - In order to update any memory, we designate that the user must accurately
 *    use a transaction-model. Any dereference-write on an array would require a
 *    transaction lock first.
 *
 *  - When we checkpoint the memory, we need to temporarily lock access and send
 *    the data to a file. Probably going to use an mmap()'d file for this? Or 
 *    maybe we could potentially use just a file to dump everything into?
 *
 *  - The API we will develop for Milestone One is specified in the functions
 *    below. Each function's purpose is documented because of specific 
 *    challenges unique to implementing a user-level non-volatile memory system.
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#include "macros.h"
#include "crmalloc.h"

/** 
 * Used to initialize the non-volatile heap system. This initializes any 
 * important heap data structures and the file-backing and manual memory map for
 * inevitable memory dumping. 
 * 
 * @param filename:     The filename which will store the heap upon checkpoint.
 *                      If the file is not empty, we will restore the heap using
 *                      the contents of the file.
 * @return:
 *      0     -> Init successful
 *     -EBADF -> The file presented could not be opened
 *     <more error codes go here>
 */
int crheap_init(const char *filename);

/**
 * Used to uninitalize and cleanup any resources consumed by the crheap 
 * subsystem. This should close the non-volatile memory file, among other 
 * things.
 *
 * @return:
 *      0     -> Cleanup successful
 *     -EBADF -> Non-volatile file could not be closed
 *     <more error codes go here>
 */
int crheap_shutdown();

int crheap_shutdown_nosave();

/** The classic printf() function, without the embedded dynamic allocation. */
int crprintf(const char * __restrict fmt, ...);

/**
 * Checkpoints the current heap into a file.
 *
 * @return:
 *     0 -> checkpoint was successful
 *     <error codes go here>
 */
int crheap_checkpoint_everything();

