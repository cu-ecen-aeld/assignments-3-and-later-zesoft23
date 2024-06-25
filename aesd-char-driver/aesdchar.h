/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */

#ifndef AESD_CHAR_DRIVER_AESDCHAR_H_
#define AESD_CHAR_DRIVER_AESDCHAR_H_

#define AESD_DEBUG 1  //Remove comment on this line to enable debug

#include "aesd-circular-buffer.h"

// May need to do something about compiling this both for kernel and userspace
#include <linux/mutex.h>

#undef PDEBUG             /* undef it, just in case */
#ifdef AESD_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "aesdchar: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

struct aesd_dev
{
     // Used for buffering until we reach a "\n"
     struct aesd_buffer_entry working_aesd_buffer_entry;
     // Circular buffer structure to use
     struct aesd_circular_buffer aesd_circular_buffer;
     // Mutex that needs to be acquired
     struct mutex *lock;

     struct cdev cdev;     /* Char device structure      */
};


#endif /* AESD_CHAR_DRIVER_AESDCHAR_H_ */
