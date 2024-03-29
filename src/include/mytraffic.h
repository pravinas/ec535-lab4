/** mytraffic.h
 *  Manipulate traffic lights per lab4 requirements.
 *  @author Megha Shah
 *  @author Pravi Samaratunga
 */

#include <linux/module.h> /* everything */
#include <linux/gpio.h>
#include <linux/delay.h> /* msleep */

#define BUF_SIZE 10

#define RED 67
#define YELLOW 68
#define GREEN 44
#define BTN0 26
#define BTN1 46

#define MODE_NORMAL 0
#define MODE_FLASHING_RED 1
#define MODE_FLASHING_YELLOW 2
#define MODE_PEDESTRIAN 3
#define DEFAULT_MODE MODE_NORMAL

#define CYCLE 500 // Time in ms per flash
#define CYCLE_UPDATE 100 // Time between calls of the update fn

/* Declaration of memory.c functions */
static int mytraffic_init(void);
static void mytraffic_exit(void);
static int mytraffic_open(struct inode *inode, struct file *filp);
static int mytraffic_release(struct inode *inode, struct file *filp);
//static int mytraffic_read(struct inode *inode, struct file *filp, );
//static int mytraffic_write(struct inode *inode, struct file *filp);


/* Structure that declares the usual file */
/* access functions */
struct file_operations mytraffic_fops = 
{
	.open= mytraffic_open,
	.release= mytraffic_release,
//    .read = mytraffic_read(struct inode *inode, struct file *filp);
//    .write = mytraffic_write(struct inode *inode, struct file *filp);
};

static void normal_mode(void);
static void flashing_red_mode(void);
static void flashing_yellow_mode(void);
static void pedestrian_mode(void);

/* Global variables of the driver */
/* Major number */
static int mytraffic_major = 61;

static char output_buffer[BUF_SIZE];
static int button[2];
static int light_mode;

static struct timer_list cycle_timer;
static void update(struct timer_list*);

