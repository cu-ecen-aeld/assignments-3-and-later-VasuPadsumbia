/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include "aesd_ioctl.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Vasu Padsumbia"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    struct aesd_dev *dev;
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev; // for other methods to access
    PDEBUG("aesd_open: private_data = %p", filp->private_data);
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    struct aesd_dev *dev = filp->private_data;
    if (dev == NULL) {
        PDEBUG("aesd_release: private_data is NULL");
        return -ENODEV; // Device not found
    }
    PDEBUG("aesd_release: private_data = %p", dev);
    filp->private_data = NULL; // Clear private data
    // Additional cleanup if necessary
    PDEBUG("aesd_release: private_data cleared");
    mutex_unlock(&dev->lock); // Example if you had a lock
    PDEBUG("aesd_release: completed");
    // Return success
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    
    struct aesd_dev *dev = filp->private_data;
    if (dev == NULL) {
        PDEBUG("aesd_read: private_data is NULL");
        return -ENODEV; // Device not found
    }
    PDEBUG("aesd_read: private_data = %p", dev);
    // Lock the device for reading
    mutex_lock(&dev->lock);
    PDEBUG("aesd_read: lock acquired"); 
    // Find the entry for the given file position
    while (count > 0)
    {
        struct aesd_buffer_entry *entry;
        size_t entry_offset_byte;
        entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->_circular_buffer, *f_pos, &entry_offset_byte);
        if (entry == NULL) {
            PDEBUG("aesd_read: no entry found for f_pos %lld", *f_pos);
            mutex_unlock(&dev->lock); // Unlock before returning
            break; // No data available
        }
        PDEBUG("aesd_read: found entry at offset %zu", entry_offset_byte);
        size_t bytes_left_in_entry = entry->size - entry_offset_byte;
        size_t to_copy = (count < bytes_left_in_entry) ? count : bytes_left_in_entry;

        PDEBUG("aesd_read: copying %zu bytes (entry_offset_byte = %zu, entry size = %zu)", to_copy, entry_offset_byte, entry->size);
        // Copy data from the entry to user space
        if (copy_to_user(buf, entry->buffptr + entry_offset_byte, to_copy)) {
            PDEBUG("aesd_read: copy_to_user failed");
            mutex_unlock(&dev->lock); // Unlock before returning
            return -EFAULT; // Failed to copy data to user space
        }
        PDEBUG("aesd_read: copy_to_user succeeded");
        // Update the file position
        *f_pos += to_copy;
        buf += to_copy; // Move the buffer pointer forward
        count -= to_copy; // Decrease the count of bytes left to read
        retval += to_copy; // Update the return value with the number of bytes read
        PDEBUG("aesd_read: updated count = %zu, retval = %zd", count, retval);
    }
    
    mutex_unlock(&dev->lock); // Unlock the device
    PDEBUG("aesd_read: lock released");
    // Return the number of bytes read
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    
    struct aesd_dev *dev = filp->private_data;
    if (dev == NULL) {
        PDEBUG("aesd_write: private_data is NULL");
        return -ENODEV; // Device not found
    }  
    PDEBUG("aesd_write: private_data = %p", dev);
    char *kbuf = kmalloc(count, GFP_KERNEL);
    if (kbuf == NULL) {
        PDEBUG("aesd_write: kmalloc failed");
        return -ENOMEM; // Memory allocation failed
    }
    // Copy data from user space to kernel space
    if (copy_from_user(kbuf, buf, count)) {
        PDEBUG("aesd_write: copy_from_user failed");
        kfree(kbuf); // Free allocated memory
        return -EFAULT; // Failed to copy data from user space
    }
    PDEBUG("aesd_write: copy_from_user succeeded");
    // Lock the device for writing
    mutex_lock(&dev->lock);
    PDEBUG("aesd_write: lock acquired");
    
    size_t new_size = dev->_partial_write_size + count;
    char *combined_buffer = kmalloc(new_size, GFP_KERNEL);
    if (combined_buffer == NULL) {
        PDEBUG("aesd_write: kmalloc for combined_buffer failed");
        mutex_unlock(&dev->lock); // Unlock before returning
        kfree(kbuf); // Free allocated memory
        return -ENOMEM; // Memory allocation failed
    }
    // Copy the existing partial write buffer if it exists
    if (dev->_partial_write_buffer) {
        memcpy(combined_buffer, dev->_partial_write_buffer, dev->_partial_write_size);
        kfree(dev->_partial_write_buffer); // Free the old partial write buffer
    }
    // Copy the new data into the combined buffer
    memcpy(combined_buffer + dev->_partial_write_size, kbuf, count);
    kfree(kbuf); // Free the temporary buffer
    
    dev->_partial_write_buffer = combined_buffer; // Update the partial write buffer
    dev->_partial_write_size = new_size; // Update the size of the partial write buffer
    PDEBUG("aesd_write: partial write buffer updated, size = %zu", dev->_partial_write_size);
    
    char *newline_pos = NULL;
    while ((newline_pos = memchr(dev->_partial_write_buffer, '\n', dev->_partial_write_size))) {
        // Found a newline, we can add the entry to the circular buffer
        size_t entry_size = (newline_pos - dev->_partial_write_buffer) + 1; // Include the newline character
        
        struct aesd_buffer_entry new_entry;
        new_entry.buffptr = kmalloc(entry_size, GFP_KERNEL);
        if (new_entry.buffptr == NULL) {
            PDEBUG("aesd_write: kmalloc for new_entry.buffptr failed");
            kfree(dev->_partial_write_buffer); // Free the partial write buffer
            dev->_partial_write_buffer = NULL; // Reset the partial write buffer
            dev->_partial_write_size = 0; // Reset the size
            mutex_unlock(&dev->lock); // Unlock before returning
            return -ENOMEM; // Memory allocation failed
        }
        
        // Copy the data up to and including the newline character
        memcpy(new_entry.buffptr, dev->_partial_write_buffer, entry_size);
        new_entry.size = entry_size;

        // Add the new entry to the circular buffer
        aesd_circular_buffer_add_entry(&dev->_circular_buffer, &new_entry);
        PDEBUG("aesd_write: new entry added to circular buffer, size = %zu", entry_size);
        int j;
        for (j = 0; j < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; j++) {
            struct aesd_buffer_entry *e = &dev->_circular_buffer.entry[j];
            printk(KERN_DEBUG "aesd_write: circular buffer entry %d: buffptr = %p, size = %zu",
                   j, e->buffptr, e->size);
            if (e->buffptr) {
                printk(KERN_DEBUG "aesd_write: entry %d data: %.*s", j, (int)e->size, e->buffptr);
            } else {
                printk(KERN_DEBUG "aesd_write: entry %d is empty", j);
            }
        }
        // Remove the processed data from the partial write buffer
        size_t remaining_size = dev->_partial_write_size - entry_size;
        if (remaining_size > 0) {
            // Move the remaining data to the start of the buffer
            memmove(dev->_partial_write_buffer, newline_pos + 1, remaining_size);
        }
        // Update the partial write buffer size
        dev->_partial_write_size = remaining_size;
        if (remaining_size <= 0) {
            // If no data remains, free the partial write buffer
            kfree(dev->_partial_write_buffer);
            dev->_partial_write_buffer = NULL; // Reset the partial write buffer
        }
    } 
    mutex_unlock(&dev->lock); // Unlock the device
    PDEBUG("aesd_write: lock released");
    // Return the number of bytes written
    PDEBUG("aesd_write: returning %zd bytes", retval);
    retval = count; // Return the number of bytes written
    return retval;
}

loff_t aesd_llseek(struct file *filp, loff_t offset, int whence)
{
    struct aesd_dev *dev = filp->private_data;
    loff_t new_pos = 0;
    if (dev == NULL) {
        PDEBUG("aesd_llseek: private_data is NULL");
        return -ENODEV; // Device not found
    }
    PDEBUG("aesd_llseek: private_data = %p", dev);
    switch (whence) {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = filp->f_pos + offset;
        break;
    case SEEK_END:
        new_pos = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED + offset;
        break;
    default:
        return -EINVAL;
    }

    if (new_pos < 0)
        return -EINVAL;

    filp->f_pos = new_pos;
    return new_pos;
}
long aesd_ioctl_set_pos(struct file *filp, struct aesd_dev *dev, unsigned long arg)
{
    size_t pos = 0;
    struct aesd_seekto seekto;
    if (copy_from_user(&seekto, (struct aesd_seekto __user *)arg, sizeof(seekto))) {
        PDEBUG("aesd_ioctl_set_pos: copy_from_user failed");
        return -EFAULT; // Failed to copy data from user space
    }
    PDEBUG("aesd_ioctl_set_pos: write_cmd = %u, write_cmd_offset = %u",
           seekto.write_cmd, seekto.write_cmd_offset);
    
    mutex_lock(&dev->lock); // Lock the device for safe access
    if (seekto.write_cmd >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
        PDEBUG("aesd_ioctl_set_pos: write_cmd exceeds max supported");
        mutex_unlock(&dev->lock); // Unlock the device before returning
        return -EINVAL; // Invalid command
    }
    
    size_t cmd_count =dev->_circular_buffer.full ? AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED : dev->_circular_buffer.in_offs;
    if (seekto.write_cmd >= cmd_count) {
        PDEBUG("aesd_ioctl_set_pos: write_cmd exceeds current command count");
        mutex_unlock(&dev->lock); // Unlock the device before returning
        return -EINVAL; // Invalid command
    }

    for (int i=0; i<seekto.write_cmd; i++) {
        size_t idx = (dev->_circular_buffer.out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

        if (dev->_circular_buffer.entry[idx].buffptr == NULL) {
            PDEBUG("aesd_ioctl_set_pos: write_cmd %u is empty", seekto.write_cmd);
            mutex_unlock(&dev->lock); // Unlock the device before returning
            return -EINVAL; // Invalid command
        }
        pos += dev->_circular_buffer.entry[idx].size;
    }

    size_t cmd_idx = (dev->_circular_buffer.out_offs + seekto.write_cmd) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    if (dev->_circular_buffer.entry[cmd_idx].buffptr == NULL || seekto.write_cmd_offset >= dev->_circular_buffer.entry[cmd_idx].size) {
        PDEBUG("aesd_ioctl_set_pos: write_cmd %u is empty", seekto.write_cmd);
        mutex_unlock(&dev->lock); // Unlock the device before returning
        return -EINVAL; // Invalid command
    }
    pos += seekto.write_cmd_offset; // Add the offset within the command
    filp->f_pos = pos; // Set the file position
    mutex_unlock(&dev->lock); // Unlock the device
    PDEBUG("aesd_ioctl_set_pos: file position set to %lld", filp->f_pos);
    return 0; // Success
}

long unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct aesd_dev *dev = filp->private_data;
    if (dev == NULL) {
        PDEBUG("unlocked_ioctl: private_data is NULL");
        return -ENODEV; // Device not found
    }
    PDEBUG("unlocked_ioctl: private_data = %p", dev);
    if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC) {
        PDEBUG("unlocked_ioctl: invalid magic number");
        return -ENOTTY; // Not a valid ioctl command
    }
    if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR) {
        PDEBUG("unlocked_ioctl: command number exceeds max supported");
        return -ENOTTY; // Not a valid ioctl command    
    }
    if (_IOC_NR(cmd) == AESDCHAR_IOCSEEKTO) {
        PDEBUG("unlocked_ioctl: AESDCHAR_IOCSEEKTO command received");
        return aesd_ioctl_set_pos(filp, dev, arg);
    }
    PDEBUG("unlocked_ioctl: unknown command %u", cmd);
    return -ENOTTY; // Not a valid ioctl command
}



struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek =   aesd_llseek,
    .unlocked_ioctl = unlocked_ioctl,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    aesd_circular_buffer_init(&aesd_device._circular_buffer);
    mutex_init(&aesd_device.lock); // Example if you had a lock
    aesd_device._partial_write_buffer = NULL; // Initialize partial write buffer
    aesd_device._partial_write_size = 0; // Initialize partial write size
    aesd_device._device = NULL; // Initialize device structure for sysfs

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    struct aesd_buffer_entry *entry;
    int i;
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device._circular_buffer, i) {
        if (entry->buffptr != NULL) {
            kfree(entry->buffptr); // Free the buffer memory
            entry->buffptr = NULL; // Set pointer to NULL after freeing
        }
    }
    kfree(aesd_device._partial_write_buffer); // Free the partial write buffer
    mutex_destroy(&aesd_device.lock); // Destroy the mutex
    if (aesd_device._device) {
        device_destroy(aesd_device._device->class, MKDEV(aesd_major, aesd_minor));
    }
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
