#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/ioctl.h>
#include<linux/kdev_t.h>
#include<linux/init.h>
#include<linux/cdev.h>
#include<linux/slab.h>
#include<linux/device.h>
#include<linux/uaccess.h>


#define NO_OF_DEVICES 5
#define MAX_BUFFER_SIZE 50

#define MY_IOCTL_BASE 'a'
#define CMD_CLEAR _IO(MY_IOCTL_BASE, 1)
#define CMD_GET_BUFFER_SIZE _IOR(MY_IOCTL_BASE, 2, int)
#define CMD_SET_APPEND_MODE _IOW(MY_IOCTL_BASE, 3, int)
#define CMD_RESET_OFFSET _IO(MY_IOCTL_BASE, 4)


dev_t dev;
static char* kernel_buffer[NO_OF_DEVICES];
static int buffer_size[NO_OF_DEVICES];
static int device_append_state[NO_OF_DEVICES];
static struct class *my_class;
static struct cdev my_cdev;

static int __init IOCTL_driver_init(void);
static void __exit IOCTL_driver_exit(void);
static int my_open(struct inode *inode, struct file *flip);
static ssize_t my_read(struct file *flip,char __user *buff, size_t len, loff_t *off);
static ssize_t my_write(struct file *flip, const char __user *buff, size_t len, loff_t *off);
static int my_release(struct inode *inode, struct file *filp);
static long my_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_release,
    .llseek = default_llseek,
    .unlocked_ioctl = my_ioctl,
};

static int my_open(struct inode *inode, struct file *filp){
    pr_info("Driver Function Called: Open\n");
    pr_info("File Opened: /dev/device_%d\n", iminor(inode));
    return 0;
}

static ssize_t my_read(struct file *flip, char __user *buff, size_t len, loff_t *off){
    int minor = iminor(file_inode(flip));

    if(*off >= buffer_size[minor]){
        pr_info("EOF /dev/device_%d\n", minor);
        return 0;
    }
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

    if((filp->f_flags & O_APPEND)|| device_append_state[minor]) *off = buffer_size[minor];
    pr_info("APPEND CHECK → flag=%d ioctl=%d\n", !!(filp->f_flags & O_APPEND), device_append_state[minor]);

    if(*off >= MAX_BUFFER_SIZE){
        pr_err("ERROR: BUFFER FULL\n");
        return -ENOSPC;
    }

    size_t to_write = min(len, (size_t)(MAX_BUFFER_SIZE-*off));

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
    return to_write;
}

static int my_release(struct inode *inode, struct file *file){
    pr_info("Driver Function Called: Close\n");
    pr_info("File Closed: /dev/device_%d\n", iminor(inode));
    return 0;
}

static long my_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    int minor = iminor(file_inode(filp));
    int size, value;

    if(minor >= NO_OF_DEVICES){
        pr_err("ERROR: Invalid Minor\n");
        return -EINVAL;
    }

    switch(cmd){
        case CMD_CLEAR:
            buffer_size[minor]=0;
            memset(kernel_buffer[minor], 0, MAX_BUFFER_SIZE);
            pr_info("IOCTL: CLEAR buffer\n");
            break;
        case CMD_GET_BUFFER_SIZE:
            size = buffer_size[minor];
            if(copy_to_user((int __user *)arg, &size, sizeof(int))){
                pr_err("ERROR: Could not copy buffer size safly from kernel-space to user-space\n");
                return -EFAULT;
            }
            pr_info("IOCTL: GET_BUFFER_SIZE = %d\n", size);
            break;
        case CMD_SET_APPEND_MODE:
            value;
            if(copy_from_user(&value, (int __user *)arg, sizeof(int))){
                pr_err("ERROR: Could not copy append mode safly from user-space to kernel-space\n");
                return -EFAULT;
            }
            device_append_state[minor] = value;
            pr_info("IOCTL: SET_MODE_APPEND = %d\n", device_append_state[minor]);
            break;
        case CMD_RESET_OFFSET:
            filp->f_pos = 0;
            pr_info("IOCTL: RESET_OFFSET\n");
            break;
        default:
            pr_err("ERROR: Invalid Cmd\n");
            return -EINVAL;
    }
    return 0;
}

static int __init IOCTL_driver_init(void){
    if(alloc_chrdev_region(&dev, 0, NO_OF_DEVICES, "IOCTL_Driver")<0){
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
        cdev_del(&my_cdev);
        pr_info("Deleted: cdev\n");
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
                pr_info("Freed Space from: device_%d", j);
                device_destroy(my_class, dev+j);
                pr_info("Deleted: /dev/device_%d\n", j);
            }
            device_destroy(my_class, dev+i);
            pr_info("Delete: /dev/device_%d\n", i);
            goto r_device;
        }
        buffer_size[i]=0;
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
    return -EFAULT;
}

static void __exit IOCTL_driver_exit(void){
    for(int i=0; i<NO_OF_DEVICES; i++){
        kfree(kernel_buffer[i]);
        pr_info("Freed Space from: device_%d", i);
        device_destroy(my_class, dev + i);
        pr_info("Deleted: /dev/device_%d\n", i);
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

module_init(IOCTL_driver_init);
module_exit(IOCTL_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai Adithya");
MODULE_DESCRIPTION("Driver with IOCTL features");
MODULE_VERSION("1.1.1");
