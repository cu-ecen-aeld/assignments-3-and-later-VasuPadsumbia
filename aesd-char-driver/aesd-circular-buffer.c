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
#include <linux/kernel.h>
#else
#include <string.h>
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
    /**
    * TODO: implement per description
    */
       if (buffer == NULL || entry_offset_byte_rtn == NULL) {
        return NULL; // Invalid parameters
    }
    size_t total_size = 0;
    size_t i;
    uint8_t index = buffer->out_offs;   
    // Iterate through the circular buffer entries
    for (i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
        // Get the current entry
        struct aesd_buffer_entry *entry = &buffer->entry[index];
        // Check if the entry is valid
        if (entry->buffptr != NULL) {
            // Check if the char_offset is within the current entry
            if (char_offset < total_size + entry->size) {
                // Calculate the byte offset within the entry
                *entry_offset_byte_rtn = char_offset - total_size;
                return entry; // Return the found entry
            }
            // Update the total size with the current entry size
            total_size += entry->size;
        }
        // Move to the next entry in the circular buffer
        index = (index + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        // If we have wrapped around and reached the out_offs, break
        if (!buffer->full && index == buffer->in_offs) {
            break; // No more entries to check
        }
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
    /**
    * TODO: implement per description
    */
   struct aesd_buffer_entry *target = &buffer->entry[buffer->in_offs];
        
    if (!buffer || !add_entry) {
        return; // Invalid parameters
    }
    // Check if the buffer is full
    if (buffer->full && target->buffptr != NULL) {
        // If the buffer is full, free the memory of the oldest entry
        kfree(target->buffptr);
    }
    *target = *add_entry; // Copy the new entry into the target location
    // Move in_offs to the next position
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;  
    if (buffer->full) {
        // If the buffer was full, move out_offs to the next position
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    // Check if we have wrapped around and filled the buffer
    buffer->full = (buffer->in_offs == buffer->out_offs);
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
#ifdef __KERNEL__
    printk(KERN_INFO "aesd_circular_buffer_init: Initializing circular buffer\n");
#endif
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
