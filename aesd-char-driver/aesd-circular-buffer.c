/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/slab.h>
#define FREE(ptr) kfree(ptr)
#else
#include <string.h>
#include <stdlib.h>
#define FREE(ptr) free(ptr)
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    // Start with the buffer, read out_fs, if not full, and char offset > in_fs, return NULL, otherwise
    if (buffer == NULL) {
        return NULL;
    }

    struct aesd_buffer_entry *entry;
    uint8_t index;
    uint8_t read_index;

    size_t current_offset = 0;

    while (index < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {

        read_index = (buffer->out_offs + index) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

        entry=&((buffer)->entry[read_index]);

        if (entry->size + current_offset > char_offset) {
            // They want the actual offset! Set the entry_offset_byte_rtn pointer value to that
            *entry_offset_byte_rtn = char_offset - current_offset;
            return entry;
        }
        else {
            // Still looking
            current_offset += entry->size;
        }

        index++;
    }

    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{

    // if (buffer->full) {
    //     FREE(&buffer->entry[buffer->in_offs % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED]);
    // }
    buffer->entry[buffer->in_offs % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED] = *add_entry;
    if (!buffer->full) {
        buffer->in_offs += 1;
    }
    else if(buffer->full) {
        buffer->in_offs += 1;
        buffer->out_offs += 1;
    }

    // Mark buffer as full if the size of the buffer is reached
    if (!buffer->full && buffer->in_offs == (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)) {
        buffer->full = true;
        // buffer->out_offs += 1;
    }

}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
