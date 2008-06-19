#ifndef __SETUP_H
#define __SETUP_H

/*
 * Interface for CPU specific stuff
 */

/*
 * setup memory protection
 */
void setup_memory();

/*
 * setup interrupt and exception handlers
 */
void setup_interrupts();

/*
 * setup task switching
 */
void setup_task_switching();

#endif
