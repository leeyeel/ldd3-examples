#include <linux/fs.h>
#include <linux/types.h>    /* size_t */
#include <linux/module.h>   /*module_init */
#include <linux/init.h>
#include <linux/slab.h>     /* kmalloc */
#include <linux/seq_file.h>
#include <linux/proc_fs.h>

#include "scull.h"

int scull_major =   SCULL_MAJOR;
int scull_minor =   0;
int scull_nr_devs = SCULL_NR_DEVS;  /* number of bare scull devices */

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet, leo");
MODULE_LICENSE("Dual BSD/GPL");

struct scull_dev *scull_devices;	/* allocated in scull_init_module */

struct file_operations scull_fops = {
	.owner =    THIS_MODULE,
	.read =     NULL,
	.write =    NULL,
	.unlocked_ioctl = NULL,
	.open =     NULL,
	.release =  NULL,
};

int scull_read_procmem(struct seq_file *s, void *v)
{
    return 0;
}

static int scullmem_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, scull_read_procmem, NULL);
}

static struct file_operations scullmem_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = scullmem_proc_open,
	.read    = NULL,
	.llseek  = NULL,
	.release = NULL 
};

static void scull_create_proc(void)
{
	proc_create("scullmem", 0, NULL, &scullmem_proc_ops);
}

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err, devno = MKDEV(scull_major, scull_minor + index);

	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

void scull_cleanup_module(void)
{

}
//start
int scull_init_module(void)
{
    int result, i;
    dev_t dev = 0;

    if (scull_major) {
        dev = MKDEV(scull_major, scull_minor);
        result = register_chrdev_region(dev, scull_nr_devs, "scull");
    } else {
        result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs,"scull");
        scull_major = MAJOR(dev);
    } 

    if (result < 0) {
        printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
        return result;
    }
    
    scull_devices = kmalloc(scull_nr_devs * sizeof(struct scull_dev), GFP_KERNEL);
	if (!scull_devices) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(scull_devices, 0, scull_nr_devs * sizeof(struct scull_dev));

    for (i = 0; i < scull_nr_devs; i++) {
		scull_setup_cdev(&scull_devices[i], i);
    }

    scull_create_proc();

    return 0;

fail:
    scull_cleanup_module();
	return result;
}

module_init(scull_init_module);
module_exit(scull_cleanup_module);
