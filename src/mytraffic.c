/** mytraffic.c
 *  Manipulate traffic lights per lab4 requirements.
 *  @author Megha Shah
 *  @author Pravi Samaratunga
 */

// TODO: get module to compile successfully
// TODO: implement state machine (see process_input())
// TODO: pull header information out into header file

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
#include <linux/delay.h> /* msleep() */

#define BUF_SIZE 10

#define RED 67
#define YELLOW 68
#define GREEN 44
#define BTN0 26
#define BTN1 46

#define MODE_OFF 0
#define MODE_NORMAL 1
#define MODE_NORMAL 1
#define MODE_FLASHING_RED 2
#define MODE_FLASHING_YELLOW 3
#define MODE_PEDESTRIAN 4
#define DEFAULT_MODE MODE_OFF

#define CYCLE 100 // Time in ms per flash

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

static void process_input(void);
static void off_mode(void);
static void normal_mode(void);
static void flashing_red_mode(void);
static void flashing_yellow_mode(void);
static void pedestrian_mode(void);

/* Global variables of the driver */
/* Major number */
static int mytraffic_major = 61;

static char output_buffer[BUF_SIZE];
static char button[2];
static int light_mode;

/* Declaration of the init and exit functions */
module_init(mytraffic_init);
module_exit(mytraffic_exit);

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
    gpio_direction_input(BTN0);

    if(gpio_request(BTN1,"BTN1") < 0) {
        printk(KERN_ALERT "ERROR: GPIO %d request\n",RED);
        goto r_BTN1;
    }
    gpio_direction_input(BTN1);

    // Set state to 0
    light_mode = MODE_OFF;
    printk(KERN_ALERT "Inserting mytraffic module\n"); 

    return 0;
r_RED:
    gpio_free(RED);
r_YELLOW:
    gpio_free(YELLOW);
r_GREEN:
    gpio_free(GREEN);
r_BTN0:
    gpio_free(BTN0);
r_BTN1:
    gpio_free(BTN1);

	
	return -1;

	
}

static void mytraffic_exit(void)
{
	/* Freeing the major number */
	unregister_chrdev(mytraffic_major, "mytraffic");    	
    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,0);
    gpio_set_value(GREEN,0);
    gpio_set_value(BTN0,0);
    gpio_set_value(BTN1,0);
    gpio_free(RED);
    gpio_free(YELLOW);
    gpio_free(GREEN);
    gpio_free(BTN0);
    gpio_free(BTN1);

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
	memset(output_buffer,0,sizeof(BUF_SIZE));
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


static ssize_t mytraffic_write(struct file *filp, const char *buf,
                             size_t count, loff_t *f_pos)
{
    	// Assuming buf contains the expiration time and message separated by a space
	

	if (copy_from_user(output_buffer, buf, count)!=0)
	{
		return -EFAULT;
    }
    

    switch(light_mode) {
        case MODE_OFF:
            off_mode();
            break;
        case MODE_NORMAL:
            normal_mode();
            if(button[0]==1 && button[1]==0)
                light_mode = 1;
            else if (button[0]==0 && button[1]==0)
                light_mode = 0;
            else if (button[0]==0 && button[1]==1)
                light_mode = 3;
            break;
        case MODE_FLASHING_RED:
            flashing_red_mode();
            if(button[0]==1 && button[1]==0)
                light_mode = 2;
            else if (button[0]==0)
                light_mode = 1;
            break;
        case MODE_FLASHING_YELLOW:
            flashing_yellow_mode();
            if(button[0]==1 && button[1]==0)
                light_mode = 0;
            else if (button[0]==0)
                light_mode = 2;
            break;
        case MODE_PEDESTRIAN:
            pedestrian_mode();
            light_mode = 0;
            break;
        default:
            off_mode();
            printk(KERN_ERR "something went wrong!");
            break;            
    }

    process_input();
    return count;
}

/** process_input()
 *  implements light state machine
 */
static void process_input(){
    // TODO: read values from BTN0 and BTN1 and process appropriately
}

static void off_mode() {
    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,0);
    gpio_set_value(GREEN,1);
    msleep(1*CYCLE);

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
MODULE_AUTHOR("Megha Shah and Pravi Samaratunga");
MODULE_DESCRIPTION("my traffic light");


/*
 * Things to work on:
 * cant use msleep- convert to frequency
 * bonus is not implemented
 * procfs file setup
*/
