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
#include <linux/time.h>


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


#define BUF_SIZE 10

#define RED (67)
#define YELLOW (68)
#define GREEN (44)
#define BTN0 (26)
#define BTN1 (46)

#define CYCLE 1;

static char output_buffer[BUF_SIZE];
static int outbuf_len;
static char button[2];
int state;

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
    if(gpio_request(RED,"RED") < 0) {
        printk(KERN_ALERT "ERROR: GPIO %d request\n",RED);
        goto r_RED;
    }
    gpio_direction_output(RED,0);

    if(gpio_request(YELLOW,"YELLOW") < 0) {
        printk(KERN_ALERT "ERROR: GPIO %d request\n",YELLOW);
        goto r_YELLOW;
    }
    gpio_direction_output(YELLOW,0);

    if(gpio_request(GREEN,"GREEN") < 0) {
        printk(KERN_ALERT "ERROR: GPIO %d request\n",GREEN);
        goto r_GREEN;
    }
    gpio_direction_output(GREEN,0);

    if(gpio_request(BTN0,"BTN0") < 0) {
        printk(KERN_ALERT "ERROR: GPIO %d request\n",RED);
        goto r_BTN0;
    }
    gpio_direction_input(BTN0,0);

    if(gpio_request(BTN1,"BTN1") < 0) {
        printk(KERN_ALERT "ERROR: GPIO %d request\n",RED);
        goto r_BTN1;
    }
    gpio_direction_input(BTN1,0);

    printk(KERN_ALERT "Inserting mytraffic module\n"); 

    return 0;
r_RED:
    gpio_free(67);
r_YELLOW:
    gpio_free(68);
r_GREEN:
    gpio_free(44);
r_BTN0:
    gpio_free(26);
r_BTN1:
    gpio_free(46);

	
	return -1;

	
}

static void mytraffic_exit(void)
{
	/* Freeing the major number */
	unregister_chrdev(mytraffic_major, "mytraffic");    	
    gpio_set_value(67,0);
    gpio_set_value(68,0);
    gpio_set_value(44,0);
    gpio_set_value(26,0);
    gpio_set_value(46,0);
    gpio_free(67);
    gpio_free(68);
    gpio_free(44);
    gpio_free(26);
    gpio_free(46);

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
    gpio_state = gpio_get_value(BTN0);
    button[0] = (gpio_state == 1) ? '1' : '0';

    gpio_state = gpio_get_value(BTN1);
    button[1] = (gpio_state == 1) ? '1' : '0';

    
    count = 2;
    if(copy_to_user(buf,output_buffer,count) > 0) {
        printk(KERN_ERR "Error: bytes not copied\n");
    }
    
	return count; 
}


static ssize_t mytraffic
_write(struct file *filp, const char *buf,
                             size_t count, loff_t *f_pos)
{
    	// Assuming buf contains the expiration time and message separated by a space
	

	if (copy_from_user(output_buffer, buf, count)!=0)
	{
		return -EFAULT;
    }

    switch(next_state) {
        case 0:
            normal_mode();
            if(button[0]==1 && button[1]==0)
                next_state = 1;
            else if (button[0]==0 && button[1]==0)
                next_state = 0;
            else if (button[0]==0 && button[1]==1)
                next_state = 3;

        case 1:
            flashing_red_mode();
            if(button[0]==1 && button[1]==0)
                next_state = 2;
            else if (button[0]==0)
                next_state = 1;
        case 2:
            flashing_yellow_mode();
            if(button[0]==1 && button[1]==0)
                next_state = 0;
            else if (button[0]==0)
                next_state = 2;
        case 3:
            pedestrian_mode();
            next_state = 0;
        default:
            printk(KERN_ERR "something went wrong!");
            break;            
    }
    return count;
}

static void normal_mode() {

    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,0);
    gpio_set_value(GREEN,1);
    msleep(3*CYCLE);
    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,1);
    gpio_set_value(GREEN,0);
    msleep(1*CYCLE);
    gpio_set_value(RED,1);
    gpio_set_value(YELLOW,0);
    gpio_set_value(GREEN,0);
    msleep(2*CYCLE);

}

static void flashing_red_mode() {

    gpio_set_value(RED,1);
    gpio_set_value(YELLOW,0);
    gpio_set_value(GREEN,0);
    msleep(1*CYCLE);
    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,0);
    gpio_set_value(GREEN,0);
    msleep(1*CYCLE);

}

static void flashing_yellow_mode() {

    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,1);
    gpio_set_value(GREEN,0);
    msleep(1*CYCLE);
    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,0);
    gpio_set_value(GREEN,0);
    msleep(1*CYCLE);

}

static void pedestrian_mode() {

    gpio_set_value(RED,1);
    gpio_set_value(YELLOW,1);
    gpio_set_value(GREEN,0);
    msleep(5*CYCLE);
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


/*
 * Things to work on:
 * cant use msleep- convert to frequency
 * bonus is not implemented
 * procfs file setup
*/