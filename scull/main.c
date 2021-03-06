#include <linux/fs.h>       /* struct file */
#include <linux/types.h>    /* size_t */
#include <linux/module.h>   /*module_init */
#include <linux/init.h>
#include <linux/slab.h>     /* kmalloc */
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>   /*printk,container_of*/

#include "scull.h"

int scull_major =   SCULL_MAJOR;
int scull_minor =   0;
int scull_nr_devs = SCULL_NR_DEVS;  /* number of bare scull devices */

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet, leo");
MODULE_LICENSE("Dual BSD/GPL");

struct scull_dev *scull_devices;	/* allocated in scull_init_module */

int scull_open(struct inode *inode, struct file *filp)
{
    struct scull_dev *dev;
    dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = dev;
    return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
    return 0;
}

struct file_operations scull_fops = {
	.owner =    THIS_MODULE,
	.read =     NULL,
	.write =    NULL,
	.unlocked_ioctl = NULL,
	.open =     scull_open,
	.release =  scull_release,
};

static struct file_operations scullmem_proc_ops = {
	.owner   = THIS_MODULE,
	.open    = NULL,
	.read    = NULL,
	.llseek  = NULL,
	.release = NULL 
};

static void scull_create_proc(void)
{
	proc_create("scullmem", 0, NULL, &scullmem_proc_ops);
}

static void scull_remove_proc(void)
{
	remove_proc_entry("scullmem", NULL /* parent dir */);
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
    scull_remove_proc();
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

    // create proc
    scull_create_proc();

    return 0;

fail:
    scull_cleanup_module();
	return result;
}

module_init(scull_init_module);
module_exit(scull_cleanup_module);
