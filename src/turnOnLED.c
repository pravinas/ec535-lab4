#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <asm/system_misc.h> /* cli(), *_flags */
#include <linux/uaccess.h>
#include <asm/uaccess.h> /* copy_from/to_user */
#include <linux/string.h>
#include <linux/sched.h> /* get user pid */
#include <linux/seq_file.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/cdev.h>


/* Declaration of memory.c functions */
static int mytraffic_open(struct inode *inode, struct file *filp);
static int mytraffic_release(struct inode *inode, struct file *filp);
static ssize_t mytraffic_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t mytraffic_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static void mytraffic_exit(void);
static int mytraffic_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations mytraffic_fops = 
{
	.read= mytraffic_read,
	.write= mytraffic_write,
	.open= mytraffic_open,
	.release= mytraffic_release,
};



/* Declaration of the init and exit functions */
module_init(mytraffic_init);
module_exit(mytraffic_exit);



/* Global variables of the driver */
/* Major number */
static int mytraffic_major = 61;

// dev_t dev = 0;
// static struct class *dev_class;
// static struct cdev mytraffic_cdev;

#define GPIO_67 (67)
#define GPIO_68 (68)
#define GPIO_47 (47)

static int mytraffic_init(void)
{
	
	int result;

	/* Registering device */
	result = register_chrdev(mytraffic_major, "mytraffic", &mytraffic_fops);
	if (result < 0)
	{
		printk(KERN_ALERT
			"mytraffic: cannot obtain major number %d\n", mytraffic_major);
		return result;
	}

    // procfs entry
	// proc_entry = proc_create("mytimer", 0444, NULL, &mytimer_fops);
    // if (!proc_entry) {
    //     printk(KERN_ALERT "mytimer : Proc entry creation failed\n");
    //     return -ENOMEM;
    // }

    // gpio initialization
    if(gpio_request(GPIO_67,"GPIO_67") < 0) {
        printk(KERN_ALERT "ERROR: GPIO %d request\n",GPIO_67);
        goto r_gpio;
    }
    gpio_direction_output(GPIO_67,0);

    if(gpio_request(GPIO_68,"GPIO_68") < 0) {
        printk(KERN_ALERT "ERROR: GPIO %d request\n",GPIO_68);
        goto r_gpio;
    }
    gpio_direction_output(GPIO_68,0);

    if(gpio_request(GPIO_47,"GPIO_47") < 0) {
        printk(KERN_ALERT "ERROR: GPIO %d request\n",GPIO_47);
        goto r_gpio;
    }
    gpio_direction_output(GPIO_47,0);
    printk(KERN_ALERT "Inserting mytraffic module\n"); 

    return 0;
gpio_67:
    gpio_free(67);
gpio_68:
    gpio_free(68);
gpio_47:
    gpio_free(47);

	
	return -1;

	
}

static void mytraffic_exit(void)
{
	/* Freeing the major number */
	unregister_chrdev(mytraffic_major, "mytraffic");    	
    gpio_set_value(67,0);
    gpio_set_value(68,0);
    gpio_set_value(47,0);
    gpio_free(67);
    gpio_free(68);
    gpio_free(47);

    printk(KERN_INFO "Exiting my_module\n");

	// remove_proc_entry("mytraffic", NULL);
}

static int mytraffic_open(struct inode *inode, struct file *filp)
{
	/* Success */
	// return single_open(filp, mytimer_proc_show, NULL);
    return 0;
}


static int mytraffic_release(struct inode *inode, struct file *filp)
{
	//printk(KERN_INFO "release called: process id %d, command %s\n",
		//current->pid, current->comm);
	memset(output_buffer,0,sizeof(BUFFER_SIZE));
	// single_release(inode, filp);
	// Success 
	return 0;
}



static ssize_t mytraffic_read(struct file *filp, char *buf, 
							size_t count, loff_t *f_pos)
{ 
	
	uint8_t gpio_state = 0;
    // gpio_state = gpio_get_value(GPIO_);

    count = 1;
    if(copy_to_user(buf,&gpio_state,len) > 0) {
        printk(KERN_ERR "Error: bytes not copied\n");
    }
    
	return 0; 
}


static ssize_t mytraffic_write(struct file *filp, const char *buf,
                             size_t count, loff_t *f_pos)
{
    	// Assuming buf contains the expiration time and message separated by a space
	uint9_t output_buffer[10] = {0};

	if (copy_from_user(output_buffer, buf, count)!=0)
	{
		return -EFAULT;
    }

    switch(value) {
        case '0':
            gpio_set_value(67,0);
            gpio_set_value(68,0);
            gpio_set_value(47,0);
            break;
        case '1':
            gpio_set_value(67,1);
            gpio_set_value(68,1);
            gpio_set_value(47,1);
            break;
        default:
            printk(KERN_ALERT "Ivalid input\n");
            break;
    }
    return count;
}

// static int mytimer_proc_show(struct seq_file *m, void *v)
// {
//     unsigned long elapsed_time = jiffies_to_msecs(jiffies - start_time);

//     seq_printf(m, "mytimer- name of the module\n");
//     seq_printf(m, "%lu- time since module loaded\n", elapsed_time);

//     if (user_pid != -1) {
//         seq_printf(m, "%d- user pid\n", user_pid);
//         seq_printf(m, "%s- command\n", command);
// 		seq_printf(m,"%s- message\n",timers[0].message);
//         seq_printf(m, "%lu- time remaining\n", get_time_remaining(&timers[0]));
//     }

//     return 0;
// }

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Megha Shah");
MODULE_DESCRIPTION("my traffic light");
