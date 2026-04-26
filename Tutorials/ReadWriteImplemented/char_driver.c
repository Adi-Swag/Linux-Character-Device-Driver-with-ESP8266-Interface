#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/cdev.h>
#include<linux/slab.h>
#include<linux/uaccess.h>

#define NO_OF_DEVICES 3
#define MAX_BUFFER_SIZE 50 //per device

dev_t dev;
static char *kernel_buffer[NO_OF_DEVICES];
static int buffer_size[NO_OF_DEVICES];
static struct class *my_class;
static struct cdev my_cdev;

static int __init char_driver_init(void);
static void __exit char_driver_exit(void);
static int my_open(struct inode *inode, struct file *filp);
static int my_close(struct inode *inode, struct file *filp);
static ssize_t my_read(struct file *filp, char __user *buff, size_t len, loff_t *off);
static ssize_t my_write(struct file *file, const char __user *buff, size_t len, loff_t *off);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_close,
    .llseek  = default_llseek,
};

static int my_open(struct inode *inode, struct file *filp){
    pr_info("Driver Function Called: Open\n");
    pr_info("File Opened: /dev/device_%d\n", iminor(inode));
    return 0;
}

static ssize_t my_read(struct file *filp, char __user *buff, size_t len, loff_t *off){
    int minor = iminor(file_inode(filp));

    if(*off >= buffer_size[minor]) return 0;
    pr_info("Reading From File: /dev/device_%d\n", minor);

    size_t to_copy = min(len, (size_t)(buffer_size[minor] - *off));
    
    if(copy_to_user(buff, kernel_buffer[minor] + *off, to_copy)){
        pr_err("ERROR: Could not copy data safly from kernel-space to user-space\n");
        return -EFAULT;
    }
    *off += to_copy;
    return to_copy;
}

static ssize_t my_write(struct file *filp, const char __user *buff, size_t len, loff_t *off){
    int minor = iminor(file_inode(filp));
    pr_info("Writing to File: /dev/device_%d\n", minor);
    if (filp->f_flags & O_APPEND) *off = buffer_size[minor];
    if(*off >= MAX_BUFFER_SIZE){
        pr_err("ERROR: BUFFER FULL\n");
        return -ENOSPC;
    }
    size_t to_write = min(len, (size_t)(MAX_BUFFER_SIZE-*off));
    pr_info("WRITE: off before = %lld\n", *off);
    pr_info("WRITE: to_write = %zu\n", to_write);
    if(copy_from_user(kernel_buffer[minor] + *off, buff, to_write)){
        pr_err("ERROR: Could not copy data safly from user-space to kernel-space\n");
        return -EFAULT;
    }

    *off += to_write;
    buffer_size[minor]=max(buffer_size[minor], (int)*off);
    if(len != to_write){
        pr_info("Partial Write complete!!\n");
    }else{
        pr_info("Write Complete!!\n");
    }
    pr_info("WRITE: off after = %lld\n", *off);
    pr_info("WRITE: buffer_size = %d\n", buffer_size[minor]);
    return to_write;
}

static int my_close(struct inode *inode, struct file *file){
    pr_info("Driver Function Called: Close\n");
    pr_info("File Closed: /dev/device_%d\n", iminor(inode));
    return 0;
}

static int __init char_driver_init(void){
    if(alloc_chrdev_region(&dev, 0, NO_OF_DEVICES, "character_driver")<0){
        pr_err("ERROR: Region Allocation Failed\n");
        pr_err("ERROR: Aborting LKM Insertion\n");
        goto r_region;
    }
    pr_info("Region Allocated: Major = %d, Minor = %d\n", MAJOR(dev), MINOR(dev));

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    pr_info("Intiated: cdev\n");

    if(cdev_add(&my_cdev, dev, NO_OF_DEVICES)<0){
        pr_err("ERROR: Could Not Add cdev");
        pr_err("ERROR: Aborting LKM Insertion\n");
        goto r_cdev;
    }
    pr_info("Added: cdev\n");

    my_class = class_create("characterDeviceDriver");
    if(IS_ERR(my_class)){
        pr_err("ERROR: Class Creation Failed\n");
        pr_err("ERROR: Aborting LKM Insertion\n");
        goto r_class;
    }
    pr_info("Created: /sys/class/characterDeviceDriver\n");

    for(int i=0; i<NO_OF_DEVICES; i++){
        char name[20];
        snprintf(name, sizeof(name), "device_%d", i);
        if(IS_ERR(device_create(my_class, NULL, dev + i, NULL, name))){
            pr_err("ERROR: Could Not Create device_%d\n", i);
            pr_err("ERROR: Aborting LKM Insertion\n");
            for(int j=0; j<i; j++){
                kfree(kernel_buffer[j]);
                pr_info("Freed Space for: device_%d", j);
                device_destroy(my_class, dev+j);
                pr_info("Deleted: /dev/device_%d\n", j);
            }
            goto r_device;
        }
        pr_info("Created: device_%d\n", i);
        kernel_buffer[i] = kmalloc(MAX_BUFFER_SIZE, GFP_KERNEL);
        if(!kernel_buffer[i]){
            pr_err("ERROR: Couldn't Allocate space for device_%d\n", i);
            pr_err("ERROR: Aborting LKM Insertion\n");
            for(int j=0;j<i;j++){
                kfree(kernel_buffer[j]);
                pr_info("Freed Space for: device_%d", j);
                device_destroy(my_class, dev+j);
                pr_info("Deleted: /dev/device_%d\n", j);
            }
            device_destroy(my_class, dev+i);
            pr_info("Delete: /dev/device_%d\n", i);
            goto r_device;
        }
        buffer_size[i] = 0;
        pr_info("Allocated Space for: device_%d\n", i);
    }

    pr_info("LKM Inserted Successfully\n");
    return 0;


r_device:
    class_destroy(my_class);
    pr_info("Deleted: /sys/class/charcterDeviceDriver\n");
r_class:
    cdev_del(&my_cdev);
    pr_info("Deleted: cdev\n");
r_cdev:
    unregister_chrdev_region(dev, NO_OF_DEVICES);
    pr_info("Cleared Region!!\n");
r_region:
    return -EINVAL;
}

static void __exit char_driver_exit(void){
    for(int j=0; j<NO_OF_DEVICES; j++){
        kfree(kernel_buffer[j]);
        pr_info("Freed Space for: device_%d", j);
        device_destroy(my_class, dev+j);
        pr_info("Deleted: /dev/device_%d\n", j);
    }
    pr_info("Deleted: All device_* files from /dev/\n");

    class_destroy(my_class);
    pr_info("Deleted: /sys/class/charcterDeviceDriver\n");

    cdev_del(&my_cdev);
    pr_info("Deleted: cdev\n");

    unregister_chrdev_region(dev, NO_OF_DEVICES);
    pr_info("Cleared Region!!\n");

    pr_info("LKM Unloaded Successfully\n");
}

module_init(char_driver_init);
module_exit(char_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai Adithya");
MODULE_DESCRIPTION("A Completly Working Character Driver with File Operations");
MODULE_VERSION("1.1.1");