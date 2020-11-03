#ifndef __RESURRECTOR_H__
#define __RESURRECTOR_H__

#include "crthread.h"

/** Initializes the resurrection system. */
void resurrector_init();

/** Shuts down the resurrection system. */
void resurrector_shutdown();

/** Submits a thread for checkpointing. */
void resurrector_checkpoint(struct crthread *thread);

#endif