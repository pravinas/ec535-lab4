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

    ped_cache = 0;
	button[0]=-1; // set state off from initial state
    timer_setup(&cycle_timer, update, 0);
    mod_timer(&cycle_timer, jiffies + msecs_to_jiffies(CYCLE_UPDATE));
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

    del_timer(&cycle_timer);
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
	int btn0, btn1;
	mod_timer(&cycle_timer, jiffies + msecs_to_jiffies(CYCLE_UPDATE));
    btn0 = gpio_get_value(BTN0);
    btn1 = gpio_get_value(BTN1);

    // only change state if btns are different
    if(btn0!=button[0] || btn1!=button[1]) {     
        button[0] = btn0;
        button[1] = btn1;
        if (btn0>0) {
                light_mode = (light_mode + 1) % 3;
            }
        if (button[1]) pedestrian = 1;
        switch(light_mode) {
            case MODE_NORMAL:
               normal_mode();
               break;
            case MODE_FLASHING_RED:
                flashing_red_mode();
                break;
            case MODE_FLASHING_YELLOW:
                flashing_yellow_mode();
                break;
            default:
                normal_mode();
                printk(KERN_ERR "something went wrong!");
                break;            
        }
    }

}

static void normal_mode() {
    int p, i;
    pedestrian = ped_cache;
    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,0);
    gpio_set_value(GREEN,1);
    msleep(CYCLE); // do 3 times with interrupts
    if (!pedestrian && light_mode != MODE_NORMAL) return;
    msleep(CYCLE); // do 3 times with interrupts
    if (!pedestrian && light_mode != MODE_NORMAL) return;
    msleep(CYCLE); // do 3 times with interrupts
    if (!pedestrian && light_mode != MODE_NORMAL) return;
    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,1);
    gpio_set_value(GREEN,0);
    msleep(CYCLE); // do 3 times with interrupts
    if (!pedestrian && light_mode != MODE_NORMAL) return;
    p = pedestrian; // lock in pedestrian
    pedestrian = 0;
    ped_cache = 0;
    gpio_set_value(RED,1);
    gpio_set_value(YELLOW,p);
    gpio_set_value(GREEN,0);
    for (i = 0; i < 2 + 3*p; i++) {
        msleep(CYCLE); // do 2 or 5 times with interrupts
        if (pedestrian) ped_cache = 1;
        if (!p && light_mode != MODE_NORMAL) return;
    }
    if (p) light_mode = MODE_NORMAL;
    pedestrian = 0;
    button[0] = -1;

}

static void flashing_red_mode() {
    pedestrian = 0;
    gpio_set_value(RED,1);
    gpio_set_value(YELLOW,0);
    gpio_set_value(GREEN,0);
    msleep(CYCLE);
	if (light_mode != MODE_FLASHING_RED) return;
    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,0);
    gpio_set_value(GREEN,0);
    msleep(CYCLE);
	if (light_mode != MODE_FLASHING_RED) return;
    button[0] = -1;
}

static void flashing_yellow_mode() {
    pedestrian = 0;
    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,1);
    gpio_set_value(GREEN,0);
    msleep(CYCLE);
	if (light_mode != MODE_FLASHING_YELLOW) return;
    gpio_set_value(RED,0);
    gpio_set_value(YELLOW,0);
    gpio_set_value(GREEN,0);
    msleep(CYCLE);
	if (light_mode != MODE_FLASHING_YELLOW) return;
    button[0] = -1;

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
