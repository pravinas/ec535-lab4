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

    #ifndef GPIO 
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
    #endif

    // Set state to 0
    light_mode = DEFAULT_MODE;
    printk(KERN_ALERT "Inserting mytraffic module\n"); 

    ped_cache = 0;
    outputs[0] = 0;
    outputs[1] = 0;
    outputs[2] = 0;
	button[0]=-1; // set state off from initial state
    cycle_ms = CYCLE;
    timer_setup(&cycle_timer, update, 0);
    mod_timer(&cycle_timer, jiffies + msecs_to_jiffies(CYCLE_UPDATE));
    inc=0;
    return 0;

#ifndef GPIO
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
#endif
	
}

static void mytraffic_exit(void)
{
	/* Freeing the major number */
    unregister_chrdev(mytraffic_major, "mytraffic");    

    del_timer(&cycle_timer);
    #ifndef GPIO
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
    #endif
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

static void set_gpio_vals() {
    #ifndef GPIO
    gpio_set_value(RED,outputs[0]);
    gpio_set_value(YELLOW,outputs[1]);
    gpio_set_value(GREEN,outputs[2]);
    #endif
    
}

static void simulate_button_press() {
    if (inc%5 == 0){
        btn0 = 1;
        btn1 = 0;
        if (inc%10 == 0) {
            btn0 = 0;
            btn1 = 1;
        }
    } else {
        btn0=0;
        btn1=1;
    }
    msleep(50);
    inc++;
}

static void update (struct timer_list* unused) {
	//int btn0, btn1;
	mod_timer(&cycle_timer, jiffies + msecs_to_jiffies(CYCLE_UPDATE));
    #ifndef GPIO
    btn0 = gpio_get_value(BTN0);
    btn1 = gpio_get_value(BTN1);
    #endif
    #ifdef GPIO
    simulate_button_press();
    #endif

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
    outputs[0] = 0;
    outputs[1] = 0;
    outputs[2] = 1;
    set_gpio_vals();
     
    msleep(cycle_ms); // do 3 times with interrupts
    if (!pedestrian && light_mode != MODE_NORMAL) return;
    msleep(cycle_ms); // do 3 times with interrupts
    if (!pedestrian && light_mode != MODE_NORMAL) return;
    msleep(cycle_ms); // do 3 times with interrupts
    if (!pedestrian && light_mode != MODE_NORMAL) return;

    outputs[0] = 0;
    outputs[1] = 1;
    outputs[2] = 0;
    set_gpio_vals();

    msleep(cycle_ms); 
    if (!pedestrian && light_mode != MODE_NORMAL) return;
    p = pedestrian; // lock in pedestrian
    pedestrian = 0;
    ped_cache = 0;

    outputs[0] = 1;
    outputs[1] = p;
    outputs[2] = 0;
    set_gpio_vals();

    for (i = 0; i < 2 + 3*p; i++) {
        msleep(cycle_ms); // do 2 or 5 times with interrupts
        if (pedestrian) ped_cache = 1;
        if (!p && light_mode != MODE_NORMAL) return;
    }
    if (p) light_mode = MODE_NORMAL;
    pedestrian = 0;
    button[0] = -1;
}

static void flashing_red_mode() {
    pedestrian = 0;
    outputs[0] = 1;
    outputs[1] = 0;
    outputs[2] = 0;
    set_gpio_vals();
    msleep(cycle_ms);
	if (light_mode != MODE_FLASHING_RED) return;
    outputs[0] = 0;
    outputs[1] = 0;
    outputs[2] = 0;
    set_gpio_vals();
    msleep(cycle_ms);
	if (light_mode != MODE_FLASHING_RED) return;
    button[0] = -1;
}

static void flashing_yellow_mode() {
    pedestrian = 0;
    outputs[0] = 0;
    outputs[1] = 1;
    outputs[2] = 0;
    set_gpio_vals();
    msleep(cycle_ms);
	if (light_mode != MODE_FLASHING_YELLOW) return;
    outputs[0] = 0;
    outputs[1] = 0;
    outputs[2] = 0;
    set_gpio_vals();
    msleep(cycle_ms);
	if (light_mode != MODE_FLASHING_YELLOW) return;
    button[0] = -1;

}

static ssize_t mytraffic_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
    char input[256];

	if (count > 256) 
	{
		count = 256;
	}

	if (copy_from_user(input, buf, count))
	{
		return -1;
	}

    if ('0' < input[0] && input[0] <= '9') {
        //double freq = input[0];
        //cycle_ms = (double)1000.0/(double)2.0; //(input[0]));
        cycle_ms = 500; //1000/2;
    }
    return count;


}
static ssize_t mytraffic_read(struct file *filp,char __user *buf, size_t count, loff_t *f_pos)
{
	char fbuf[BUF_SIZE], *fptr = fbuf;
    
    fptr += sprintf(fptr, "~~~\nTRAFFIC DIAGNOSTICS\n");

    switch (light_mode) {
        case MODE_NORMAL:
            fptr += sprintf(fptr, "Operational Mode: Normal         \n");
            break;
        case MODE_FLASHING_RED:
            fptr += sprintf(fptr, "Operational Mode: Flashing Red   \n");
            break;
        case MODE_FLASHING_YELLOW:
            fptr += sprintf(fptr, "Operational Mode: Flashing Yellow\n");
            break;
    }

    fptr += sprintf(fptr, "Cycle Rate: %d \n", 500); //1000.0/cycle_ms);

    fptr += sprintf(fptr, "Lights:\tRED\tYELLOW\tGREEN\n");
    fptr += sprintf(fptr, "Values:\t%s\t%s\t%s\n", outputs[0]?"on ":"off",outputs[1]?"on ":"off",outputs[2]?"on ":"off");

    fptr += sprintf(fptr, "Pedestrian: %s\n", pedestrian? "yes": "no ");
    count = strlen(fbuf);

   	if (count > BUF_SIZE) 
   	{
   		count = BUF_SIZE;
   	}

	if (copy_to_user(buf, fbuf, count))
	{
        // do we need to free fptr?
		return -EFAULT;
	}
    return count;

}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Megha Shah and Pravi Samaratunga");
MODULE_DESCRIPTION("my traffic light");


/*
 * Things to work on:
 * cant use msleep- convert to frequency
 * bonus is not implemented
 * you have to hold button for 1s if timer is 1s long
*/
