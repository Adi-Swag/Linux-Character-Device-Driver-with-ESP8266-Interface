#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/param.h>

int extValue;
int extArray[4];
int extCb = 0;

module_param(extValue, int, S_IRUSR | S_IWUSR);
module_param_array(extArray, int, NULL, S_IRUSR | S_IWUSR);

static int my_set(const char *val, const struct kernel_param *kp) {
    int ret = param_set_int(val, kp);

    if (ret == 0) {
        printk(KERN_INFO "extCb changed to %d\n", extCb);
    }

    return ret;
}

static int my_get(char *buffer, const struct kernel_param *kp) {
    return param_get_int(buffer, kp);
}

static const struct kernel_param_ops my_ops = {
    .set = my_set,
    .get = my_get,
};

module_param_cb(extCb, &my_ops, &extCb, S_IRUSR | S_IWUSR);

static int __init param_passing_init(void){
    printk(KERN_INFO "extValue = %d\n",extValue);
    for(int i=0;i<4;i++){
        printk(KERN_INFO "extArray[%d] = %d\n",i,extArray[i]);
    }
    printk(KERN_INFO "extCb = %d",extCb);
    printk(KERN_INFO "Module Inserted!!\n");
    return 0;
}

static void __exit param_passing_exit(void){
    printk(KERN_INFO "Module Removed!!\n");
}

module_init(param_passing_init);
module_exit(param_passing_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai Adithya");
MODULE_DESCRIPTION("Simple Params Passing Module");
MODULE_VERSION("1.1.1");
