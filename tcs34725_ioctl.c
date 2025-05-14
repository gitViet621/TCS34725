#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "tcs34725_driver"
#define CLASS_NAME "tcs34725"
#define DEVICE_NAME "tcs34725"

#define ENABLE_REG 0x00
#define RGBC_TIMING_REG 0x01
#define WAIT_TIME_REG 0x03
#define RGBC_INTERUPT_THRESHOLD_REG 0x04
#define PERSISTENCE_REG 0x0C
#define CONFIG_REG 0x0D
#define CONTROL_REG 0x0F
#define ID_REG 0x12
#define STATUS_REG 0x13
#define RGBC_CHANNEL_DATA_REG 0x14

#define CMD_BIT 0x80
#define TYPE_BIT 0x20

// IOCTL commands
#define TCS34725_IOCTL_MAGIC 't'
#define TCS34725_IOCTL_RGBC_DATA _IOR(TCS34725_IOCTL_MAGIC, 1, struct tcs34725_color)
#define TCS34725_IOCTL_ENABLE _IOW(TCS34725_IOCTL_MAGIC, 2, u8)
#define TCS34725_IOCTL_TIMING _IOW(TCS34725_IOCTL_MAGIC, 3, u8)
#define TCS34725_IOCTL_WAIT_TIME _IOW(TCS34725_IOCTL_MAGIC, 4, u8)
#define TCS34725_IOCTL_INTERRUPT_THRESHOLD _IOW(TCS34725_IOCTL_MAGIC, 5, struct tcs34725_interrupt_threshold)
#define TCS34725_IOCTL_PERSISTENCE _IOW(TCS34725_IOCTL_MAGIC, 6, u8)
#define TCS34725_IOCTL_CONFIG _IOW(TCS34725_IOCTL_MAGIC, 7, u8)
#define TCS34725_IOCTL_CONTROL _IOW(TCS34725_IOCTL_MAGIC, 8, u8)
#define TCS34725_IOCTL_STATUS _IOR(TCS34725_IOCTL_MAGIC, 9, u8)

static struct i2c_client *tcs34725_client;
static struct class* tcs34725_class = NULL;
static struct device* tcs34725_device = NULL;
static int major_number;

/* note when passing a struct as parameter to function. Must declared the struct first for the compiler to understand
what type of struct you pass in the function, how many element it have, the compiler distinguish structs by their name. 
So passing by their correct name you want */
struct tcs34725_color{
    u16 clear;
    u16 red;
    u16 green;
    u16 blue;
};  

struct tcs34725_interrupt_threshold{
    u16 high_threshold;
    u16 low_threshold;
};

static int tcs34725_read_color(struct i2c_client *client, struct tcs34725_color *color)
{
    u8 buf[8];

    if (i2c_smbus_read_i2c_block_data(client, CMD_BIT|TYPE_BIT|RGBC_CHANNEL_DATA_REG, sizeof(buf), buf) < 0) {
        printk(KERN_ERR "Failed to read color data register\n");
        return -EIO;
    }

    // Combine high and low bytes to form 16-bit values
    color->clear = (buf[0] << 8) | buf[1]; // clear
    color->red = (buf[2] << 8) | buf[3]; // red
    color->green = (buf[4] << 8) | buf[5]; // green
	color->blue = (buf[6] << 8) | buf[7]; // blue

    return 0;
}

static int tcs34725_write(struct i2c_client *client, u8 reg_addr, u8 value)
{
    if (i2c_smbus_write_byte_data(client, CMD_BIT|TYPE_BIT|reg_addr, value) < 0) {
        printk(KERN_ERR "Failed to config register\n");
        return -EIO;
    }

    return 0;
}

static long tcs34725_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct tcs34725_color tcs_color;
    u8 value;
    u8 data;

    if(_IOC_DIR(cmd) & _IOC_WRITE){
        if(copy_from_user((void __user *)arg, &value, sizeof(value))){
            return -EFAULT;
        }
    }

    switch (cmd) {
    	case TCS34725_IOCTL_RGBC_DATA:
            tcs34725_read_color(tcs34725_client, &tcs_color);
            break;
        case TCS34725_IOCTL_ENABLE:
            tcs34725_write(tcs34725_client, ENABLE_REG, value);
            break;
        case TCS34725_IOCTL_TIMING:
            tcs34725_write(tcs34725_client, RGBC_TIMING_REG, value);
            break;
        case TCS34725_IOCTL_WAIT_TIME:
            tcs34725_write(tcs34725_client, WAIT_TIME_REG, value);
            break;
        case TCS34725_IOCTL_INTERRUPT_THRESHOLD:
            tcs34725_write(tcs34725_client, RGBC_INTERUPT_THRESHOLD_REG, value);
            break;
        case TCS34725_IOCTL_PERSISTENCE:
            tcs34725_write(tcs34725_client, PERSISTENCE_REG, value);
            break;
        case TCS34725_IOCTL_CONFIG:
            tcs34725_write(tcs34725_client, CONFIG_REG, value);
            break;
        case TCS34725_IOCTL_CONTROL:
            tcs34725_write(tcs34725_client, CONFIG_REG, value);
            break;
        case TCS34725_IOCTL_STATUS:
            if(i2c_smbus_read_byte_data(tcs34725_client,STATUS_REG)<0){
                printk(KERN_ERR "Failed to read register 0x%02x\n", STATUS_REG);
                return -EIO;
            }
            data = i2c_smbus_read_byte_data(tcs34725_client,STATUS_REG);
            if(_IOC_DIR(cmd) & _IOC_READ){
                if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
                    return -EFAULT;
                }
            }
            break;
        default:
            return -EINVAL;
    }

    if(_IOC_DIR(cmd) & _IOC_READ){
        if (copy_to_user((void __user *)arg, &tcs_color, sizeof(tcs_color))) {
            return -EFAULT;
        }
    }
    
    return 0;
}

static int tcs34725_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "TCS34725 device opened\n");
    return 0;
}

static int tcs34725_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "TCS34725 device closed\n");
    return 0;
}

static struct file_operations fops = {
    .open = tcs34725_open,
    .unlocked_ioctl = tcs34725_ioctl,
    .release = tcs34725_release,
};

static int tcs34725_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    tcs34725_client = client;

    // Create a char device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "Failed to register a major number\n");
        return major_number;
    }

    tcs34725_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(tcs34725_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to register device class\n");
        return PTR_ERR(tcs34725_class);
    }

    tcs34725_device = device_create(tcs34725_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(tcs34725_device)) {
        class_destroy(tcs34725_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to create the device\n");
        return PTR_ERR(tcs34725_device);
    }

    // Step 4: power enable TCS34725 (important step)
    if (i2c_smbus_write_byte_data(client, CMD_BIT|TYPE_BIT|ENABLE_REG, 0x1B) < 0) {
        printk(KERN_ERR "Failed to enable TCS34725\n");
        device_destroy(tcs34725_class, MKDEV(major_number, 0));
        class_destroy(tcs34725_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return -EIO;
    }

    printk(KERN_INFO "TCS34725 driver installed\n");
    return 0;
}

static void tcs34725_remove(struct i2c_client *client)
{
    device_destroy(tcs34725_class, MKDEV(major_number, 0));
    class_unregister(tcs34725_class);
    class_destroy(tcs34725_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    printk(KERN_INFO "MPU6050 driver removed\n");
}

static const struct of_device_id tcs34725_of_match[] = {
    { .compatible = "ams,tcs34725", },
    { },
};

MODULE_DEVICE_TABLE(of, tcs34725_of_match);

static struct i2c_driver tcs34725_driver = {
    .driver = {
        .name   = DRIVER_NAME,
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(tcs34725_of_match),
    },
    .probe      = tcs34725_probe,
    .remove     = tcs34725_remove,
};

static int __init tcs34725_init(void)
{
    printk(KERN_INFO "Initializing TCS34725 driver\n");
    return i2c_add_driver(&tcs34725_driver);
}

static void __exit tcs34725_exit(void)
{
    printk(KERN_INFO "Exiting TCS34725 driver\n");
    i2c_del_driver(&tcs34725_driver);
}

module_init(tcs34725_init);
module_exit(tcs34725_exit);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("TCS34725 I2C Client Driver with IOCTL Interface");
MODULE_LICENSE("GPL");
