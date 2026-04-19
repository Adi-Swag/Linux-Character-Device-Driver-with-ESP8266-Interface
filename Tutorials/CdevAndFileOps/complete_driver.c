#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/cdev.h>  

#define BUF_SIZE 100

char kernel_buffer[2][BUF_SIZE];   // 2 devices
int buffer_size[2];
dev_t dev;
static struct class *dev_class;
static struct cdev my_cdev;

static int __init complete_driver_init(void);
static void __exit complete_driver_exit(void);
static int my_open(struct inode *inode, struct file *file);
static int my_close(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t my_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_close,
};

static int my_open(struct inode *inode, struct file *file){
    int minor;
    pr_info("Driver Open Function Called !!\n");
    minor = iminor(inode);
    if(minor == 0){
        pr_info("1st_device opened\n");
    }else{
        pr_info("2nd_device opened\n");
    }
    return 0;
}

static int my_close(struct inode *inode, struct file *file){
    pr_info("Driver Close Function Called!!\n");
    return 0;
}

static ssize_t my_read(struct file *filp, char __user *buff, size_t len, loff_t *off){
    int minor = iminor(file_inode(filp));
    pr_info("Driver Read Function Called!!\n");

    if(minor == 0){
        pr_info("Read from 1st_device\n");
    }else{
        pr_info("Read from 2nd_device\n");
    }
    if(*off >= buffer_size[minor]){
        return 0;
    }
    size_t to_copy = min(len, (size_t)(buffer_size[minor]-*off));
    if(copy_to_user(buff, kernel_buffer[minor] + *off, to_copy)){
        return -EFAULT;
    }
    *off += to_copy;
    return to_copy;
}

static ssize_t my_write(struct file *filp, const char __user *buff, size_t len, loff_t *off){
    int minor = iminor(file_inode(filp));
    pr_info("Driver Write Function Called!!\n");
    
    if(len > BUF_SIZE) len = BUF_SIZE;
    buffer_size[minor] = len;
    
    if(minor == 0){
        pr_info("Write to 1st_device\n");
    }else{
        pr_info("Write to 2nd_device\n");
    }
    if(copy_from_user(kernel_buffer[minor], buff, len)){
        return -EFAULT;
    }
    return len;
}


static int __init complete_driver_init(void){
    if(alloc_chrdev_region(&dev,0,2,"complete_driver")<0){
        pr_err("Region Not Available!!\n");
        pr_err("Aborting LKM Insertion!!!\n");
        goto r_region;
    }
    pr_info("Region Alloted with Major = %d and Minor = %d,%d\n", MAJOR(dev), MINOR(dev), MINOR(dev+1));

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    pr_info("Created my_cdev Struct!!\n");

    if(cdev_add(&my_cdev, dev, 2)<0){
        pr_err("Cannot Add device to the System!!\n");
        pr_err("Aborting LKM Insertion!!!\n");
        goto r_cdev_add;
    }
    pr_info("Added device to the System!!\n");  

    dev_class = class_create("functioning_drivers");
    if(IS_ERR(dev_class)){
        pr_err("Failed to create Class!!\n");
        pr_err("Aborting LKM Insertion!!!\n");
        goto r_class;
    }
    pr_info("Class created with name functioning_drivers\n");

    if(IS_ERR(device_create(dev_class, NULL, dev, NULL, "1st_device"))) {
        pr_err("Failed to create 1st device\n");
        goto r_device;
    }

    if(IS_ERR(device_create(dev_class, NULL, dev + 1, NULL, "2nd_device"))) {
        pr_err("Failed to create 2nd device\n");
        device_destroy(dev_class, dev);
        goto r_device;
    }
    pr_info("Devices created: 1st_device, 2nd_device\n");
    pr_info("LKM Inserted Sucessfully!!\n");
    return 0;

r_device:
    class_destroy(dev_class);
    pr_info("Deleted functioning_drivers\n");
r_class:
    cdev_del(&my_cdev);
r_cdev_add:
    unregister_chrdev_region(dev,2);
    pr_info("Freed Region\n");
r_region:
    return -EINVAL;
}

static void __exit complete_driver_exit(void){
    device_destroy(dev_class, dev);
    device_destroy(dev_class, dev+1);
    pr_info("Deleted Devices!!\n");
    class_destroy(dev_class);
    pr_info("Deleted Class!!\n");
    cdev_del(&my_cdev);
    pr_info("Deleted my_cdev structure\n");
    unregister_chrdev_region(dev, 2);
    pr_info("Freed Region!!\n");
    pr_info("LKM Unloaded\n");
}

module_init(complete_driver_init);
module_exit(complete_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai Adithya");
MODULE_DESCRIPTION("A complete driver implementation");
MODULE_VERSION("1.1.1");