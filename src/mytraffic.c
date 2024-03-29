/** mytraffic.c
 *  Manipulate traffic lights per lab4 requirements.
 *  @author Megha Shah
 *  @author Pravi Samaratunga
 */

#include "include/mytraffic.h"

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
    light_mode = DEFAULT_MODE;
    printk(KERN_ALERT "Inserting mytraffic module\n"); 

    timer_setup(&cycle_timer, update, 0);
    mod_timer(&cycle_timer, jiffies + msecs_to_jiffies(CYCLE));
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
}

// TODO: see if this is cruft
static int mytraffic_open(struct inode *inode, struct file *filp)
{
	/* Success */
    return 0;
}

// TODO: see if this is cruft
static int mytraffic_release(struct inode *inode, struct file *filp)
{
	memset(output_buffer,0,sizeof(BUF_SIZE));
	// single_release(inode, filp);
	// Success 
	return 0;
}

static void update (struct timer_list* unused) {
    int btn0 = gpio_get_value(BTN0);
    int btn1 = gpio_get_value(BTN1);

    // only change state if btns are different
    if(btn0!=button[0] || btn1!=button[1]) {     
        button[0] = btn0;
        button[1] = btn1;
        if (btn0 && light_mode != MODE_PEDESTRIAN) {
                light_mode = (light_mode + 1) % 3;
            }
    }

    switch(light_mode) {
        case MODE_NORMAL:
            normal_mode();
            if (btn1) {
                light_mode = MODE_PEDESTRIAN;
            }
            break;
        case MODE_FLASHING_RED:
            flashing_red_mode();
            break;
        case MODE_FLASHING_YELLOW:
            flashing_yellow_mode();
            break;
        case MODE_PEDESTRIAN:
            pedestrian_mode();
            if (btn1) {
                light_mode = MODE_PEDESTRIAN;
            } else {
                light_mode = MODE_NORMAL;
            }
            break;
        default:
            normal_mode();
            printk(KERN_ERR "something went wrong!");
            break;            
    }

    mod_timer(&cycle_timer, jiffies + msecs_to_jiffies(CYCLE));
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
    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,0);
    gpio_set_value(GREEN,0);
    msleep(5*CYCLE);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Megha Shah and Pravi Samaratunga");
MODULE_DESCRIPTION("my traffic light");


/*
 * Things to work on:
 * cant use msleep- convert to frequency
 * bonus is not implemented
 * procfs file setup
 * you have to hold button for 1s if timer is 1s long
*/
