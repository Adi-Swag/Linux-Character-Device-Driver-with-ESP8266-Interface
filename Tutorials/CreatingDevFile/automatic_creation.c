#include<linux/module.h>
#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/kdev_t.h>
#include<linux/device.h>

dev_t dev;
static struct class *dev_class;




static int __init auto_creation_init(void){
    if(alloc_chrdev_region(&dev, 0, 2, "my_driver")<0){
        printk(KERN_ERR "No available regions!!\n");
        printk(KERN_ERR "LKM loading aborted!!\n");
        goto r_region;
    }
    printk(KERN_INFO "Region ALlocated\n");
    printk(KERN_INFO "Major = %d, Minor = %d, %d\n", MAJOR(dev), MINOR(dev), MINOR(dev)+1);
    dev_class = class_create("my_class");
    if(IS_ERR(dev_class)){
        printk(KERN_ERR "Class Creation Failed!!\n");
        printk(KERN_ERR "LKM Loading Aborted!!\n");
        goto r_class;
    }
    printk(KERN_INFO "Class Created with name my_class\n");
    if(IS_ERR(device_create(dev_class, NULL, dev, NULL, "my_device"))){
        printk(KERN_ERR "Device File creation Failed!!\n");
        printk(KERN_ERR "LKM Loading Aborted!!\n");
        goto r_device;
    }
    printk(KERN_INFO "Device File created with name my_device\n");
    printk(KERN_INFO "LKM Loaded!!\n");
    return 0;

r_device:
    class_destroy(dev_class);
    printk(KERN_INFO "Class removed\n");
r_class:
    unregister_chrdev_region(dev, 2);
    printk(KERN_INFO "Region Freed\n");
r_region:
    return -1;
}

static void __exit auto_creation_exit(void){
    device_destroy(dev_class, dev);
    printk(KERN_INFO "Device Removed\n");
    class_destroy(dev_class);
    printk(KERN_INFO "Class Removed\n");
    unregister_chrdev_region(dev, 2);
    printk(KERN_INFO "Region Unregistered!!\n");

    printk(KERN_INFO "LKM Unloaded!!\n");
}

module_init(auto_creation_init);
module_exit(auto_creation_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai Adithya");
MODULE_DESCRIPTION("Automated Creation of Device File");
MODULE_VERSION("1.1.1");