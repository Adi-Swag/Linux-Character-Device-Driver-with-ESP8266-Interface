#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>

dev_t dev;
static bool region_allocated;

static int __init dynamic_allocation_init(void){
    if(alloc_chrdev_region(&dev, 0, 2, "my_driver")<0){
        printk(KERN_ERR "No region available\n");
        printk(KERN_ERR "LKM load is aborted!!\n");
        return -EINVAL;
    }
    region_allocated = true;
    printk(KERN_INFO "Region allocated!!\n");
    printk(KERN_INFO "Major = %d, Minor = %d,%d\n",MAJOR(dev),MINOR(dev),MINOR(dev)+1);
    printk(KERN_INFO "LKM loaded!!\n");
    return 0;
}

static void __exit dynamic_allocation_exit(void){
    if(region_allocated){
        unregister_chrdev_region(dev, 2);
        printk(KERN_INFO "Released the region\n");
    }
    printk(KERN_INFO "LKM unloaded!!\n");
}

module_init(dynamic_allocation_init);
module_exit(dynamic_allocation_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai Adithya");
MODULE_DESCRIPTION("Dynamic Allocation of Major and Minor Numbers(using for manual creaction of device file)");
MODULE_VERSION("1.1.1");