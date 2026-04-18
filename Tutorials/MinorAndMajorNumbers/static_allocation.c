#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/types.h>

int major;
static bool region_allocated;
module_param(major, int, 0);
dev_t dev;

static int __init static_allocation_init(void){
    if(major <= 0){
        printk(KERN_ERR "Enter a valid major!!\n");
        printk(KERN_ERR "LKM load is aborted!!\n");
        return -EINVAL;
    }
    dev = MKDEV(major,0);
    int ret = register_chrdev_region(dev, 2, "my_driver");
    if(ret != 0){
        printk(KERN_ERR "Region Not Available\n");
        printk(KERN_ERR "Major = %d already occupied\n",MAJOR(dev));
        printk(KERN_ERR "LKM load is aborted!!\n");
        return ret;
    }
    region_allocated = true;
    printk(KERN_INFO "Region allocated!!\n");
    printk(KERN_INFO "Major = %d, Minor = %d,%d\n",MAJOR(dev),MINOR(dev),MINOR(dev)+1);
    printk(KERN_INFO "LKM loaded!!\n");
    return 0;
}

static void __exit static_allocation_exit(void){
    if(region_allocated){
        unregister_chrdev_region(dev, 2);
        printk(KERN_INFO "Released the region\n");
    }
    printk(KERN_INFO "LKM unloaded!!\n");
}

module_init(static_allocation_init);
module_exit(static_allocation_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai Adithya");
MODULE_DESCRIPTION("Static Allocation of Major and Minor Numbers with major as param");
MODULE_VERSION("1.1.1");
