#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <asm/barrier.h>
#include <linux/interrupt.h>
#include "bbb_registers.h"

/*
====================================
DRIVER's INFO
====================================
*/
#define DRIVER_AUTHOR "23ars <ardeleanasm@gmail.com>"
#define DRIVER_DESC "Gpio Driver for BBB"
#define DEVICE_NAME "bbbgpio"
#define DEVICE_CLASS_NAME "bbbgpio_class"
#define DEVICE_PROCESS "bbbgpio%d"

/*
====================================
DRIVER's DEBUGGING MACROS
====================================
*/
#define DEBUGGING 1
#ifdef DEBUGGING
#define DEBUGGING_DEBUG KERN_DEBUG
#define DEBUGGING_INFO KERN_INFO
#define DEBUGGING_WARNING KERN_WARNING
#define DEBUGGING_ERROR KERN_ERR

#define driver_dbg(format,arg...)	do { printk( DEBUGGING_DEBUG   format, ## arg ); } while(0)
#define driver_info(format,arg...)	do { printk( DEBUGGING_INFO   format, ## arg ); } while(0)
#define driver_warn(format,arg...)	do { printk( DEBUGGING_WARNING   format, ## arg ); } while(0)
#define driver_err(format,arg...)	do { printk( DEBUGGING_ERROR   format, ## arg ); } while(0)
#else

#define driver_dbg(format,arg...)	
#define driver_info(format,arg...)	
#define driver_warn(format,arg...)	
#define driver_err(format,arg...)	

#endif


struct bbbgpio_device{
	struct cdev cdev;
	struct device *device_Ptr;
	struct mutex io_mutex;
	u8 is_open;
};

struct bbbgpio_ioctl_struct
{
	u8 gpio_group;
	u32 write_buffer;
	u32 read_buffer;
      
};

static struct bbbgpio_device *bbbgpiodev_Ptr=NULL;
static dev_t bbbgpio_dev_no;
static struct class *bbbgpioclass_Ptr=NULL;
static struct bbbgpio_ioctl_struct ioctl_buffer;
#define BUF_LEN 8            /* Max length of the message from the device */
static u8 msg[BUF_LEN];    /* The msg the device will give when asked    */
static u8 *msg_Ptr;

#define _IOCTL_MAGIC 'K'
#define BBBGPIOWR       _IOW(_IOCTL_MAGIC,1,struct bbbgpio_ioctl*)      /*write data to register*/
#define BBBGPIORD       _IOR(_IOCTL_MAGIC,2,struct bbbgpio_ioctl*)      /*red from register*/
#define BBBGPIOSD       _IOW(_IOCTL_MAGIC,3,struct bbbgpio_ioctl*)      /*set direction*/
#define BBBGPIOGD       _IOR(_IOCTL_MAGIC,4,struct bbbgpio_ioctl*)      /*read direction*/
#define BBBGPIOSL0      _IOW(_IOCTL_MAGIC,5,struct bbbgpio_ioctl*)      /*set low detect*/
#define BBBGPIOSH1      _IOW(_IOCTL_MAGIC,6,struct bbbgpio_ioctl*)      /*set high detect*/
#define BBBGPIOSRE      _IOW(_IOCTL_MAGIC,7,struct bbbgpio_ioctl*)      /*set rising edge*/
#define BBBGPIOSFE      _IOW(_IOCTL_MAGIC,8,struct bbbgpio_ioctl*)      /*set falling edge*/
#define BBBGPIOGL0      _IOR(_IOCTL_MAGIC,9,struct bbbgpio_ioctl*)      /*get low detect*/
#define BBBGPIOGH1      _IOR(_IOCTL_MAGIC,10,struct bbbgpio_ioctl*)      /*get low detect*/
#define BBBGPIOGRE      _IOR(_IOCTL_MAGIC,11,struct bbbgpio_ioctl*)      /*get low detect*/
#define BBBGPIOGFE      _IOR(_IOCTL_MAGIC,12,struct bbbgpio_ioctl*)      /*get low detect*/
#define BBBGPIOSIN      _IOW(_IOCTL_MAGIC,9,struct bbbgpio_ioctl*)      /*enable gpio interrupt*/
#define BBBGPIOGIN      _IOR(_IOCTL_MAGIN,10,struct bbbgpio_ioctl*)     /*read gpio interrupt flag*/



static int bbbgpio_open(struct inode*,struct file*);
static void kernel_probe_interrupt(void);
static int bbbgpio_release(struct inode*,struct file*);
static long bbbgpio_ioctl(struct file*, unsigned int ,unsigned long );
static ssize_t bbbgpio_read(struct file *,char __user*,size_t,loff_t*);
static ssize_t bbbgpio_write(struct file *, const char __user *, size_t, loff_t *);
static long bbbgpio_read_buffer(struct bbbgpio_ioctl_struct __user *,u32 );
static long bbbgpio_write_buffer(struct bbbgpio_ioctl_struct __user *,u32 );

struct file_operations fops=
{
	.open=bbbgpio_open,
	.release=bbbgpio_release,
	.unlocked_ioctl=bbbgpio_ioctl,
	.read=bbbgpio_read,
	.write=bbbgpio_write
};


static int 
bbbgpio_open(struct inode *inode,struct file *file)
{
	msg_Ptr = msg;
	driver_info("%s:Open\n",DEVICE_NAME);
	if(mutex_trylock(&bbbgpiodev_Ptr->io_mutex)==0){
		driver_err("%s:Mutex not free!\n",DEVICE_NAME);
		return -EBUSY;
	}
	if(bbbgpiodev_Ptr->is_open==1){
		driver_err("%s:already open\n",DEVICE_NAME);
		return -EBUSY;
	}
	bbbgpiodev_Ptr->is_open=1;
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	return 0;     
}

static int
bbbgpio_release(struct inode *inode,struct file *file)
{
	driver_info("%s:Close\n",DEVICE_NAME);
	mutex_lock(&bbbgpiodev_Ptr->io_mutex);
	bbbgpiodev_Ptr->is_open=0;
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	return 0;
}

static long
bbbgpio_write_buffer(struct bbbgpio_ioctl_struct __user *p_ioctl,u32 gpio_register)
{
	volatile u32 *memory_Ptr=NULL;
	if(copy_from_user(&ioctl_buffer,p_ioctl,sizeof(struct bbbgpio_ioctl_struct))!=0){
		driver_err("%s:Could not copy data from userspace!\n",DEVICE_NAME);
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		return -EINVAL;
	}
	wmb();
	driver_info("%s:Write at address 0x%08X value 0x%08X\n",DEVICE_NAME,ioctl_buffer.gpio_group,ioctl_buffer.write_buffer);
	if(ioctl_buffer.gpio_group>=0 && ioctl_buffer.gpio_group<=3){
		memory_Ptr=(u32*)(gpioreg_map(ioctl_buffer.gpio_group)|gpio_register);
		*memory_Ptr=ioctl_buffer.write_buffer;
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		return 0;
	}
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	driver_err("%s:Invalid value for gpio group number\n",DEVICE_NAME);
	return -EBADSLT;
}
static long
bbbgpio_read_buffer(struct bbbgpio_ioctl_struct __user *p_ioctl,u32 gpio_register)
{
	volatile u32 *memory_Ptr=NULL;
	if(copy_from_user(&ioctl_buffer,p_ioctl,sizeof(struct bbbgpio_ioctl_struct))!=0){
		driver_err("%s:Could not copy data from userspace!\n",DEVICE_NAME);
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		return -EINVAL;
	}
	driver_info("%s:Read from at address 0x%08X\n",DEVICE_NAME,ioctl_buffer.gpio_group);
	rmb();
	if(ioctl_buffer.gpio_group>=0 && ioctl_buffer.gpio_group<=3){
		memory_Ptr=(u32*)(gpioreg_map(ioctl_buffer.gpio_group)|gpio_register);
		ioctl_buffer.read_buffer=*memory_Ptr;
		if(copy_to_user(p_ioctl,&ioctl_buffer,sizeof(struct bbbgpio_ioctl_struct))!=0){
			driver_err("\t%s:Cout not write values to user!\n",DEVICE_NAME);
			mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
			return -EINVAL;
		}
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		return 0;
	}
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	driver_err("%s:Invalid value for gpio group number\n",DEVICE_NAME);
	return -EBADSLT;
}
static long 
bbbgpio_ioctl(struct file *file, unsigned int ioctl_num ,unsigned long ioctl_param)
{
	driver_info("%s:Ioctl\n",DEVICE_NAME);
	struct bbbgpio_ioctl_struct __user *p_bbbgpio_user_ioctl;
	
	memset(&ioctl_buffer,0,sizeof(struct bbbgpio_ioctl_struct));
	if(bbbgpiodev_Ptr==NULL){
		driver_err("%s:Device not found!\n",DEVICE_NAME);
		return -ENODEV;
	}
	if(mutex_trylock(&bbbgpiodev_Ptr->io_mutex)==0){
		driver_err("%s:Mutex not free!\n",DEVICE_NAME);
		return -EBUSY;
	}
	p_bbbgpio_user_ioctl=(struct bbbgpio_ioctl_struct __user*)ioctl_param;
	long error_code;
	switch(ioctl_num){
	case BBBGPIOWR:
	{
		if((error_code=bbbgpio_write_buffer(p_bbbgpio_user_ioctl,GPIO_DATAOUT))!=0){
                        return error_code;
		}
                
		break;
	}
	case BBBGPIORD:
	{
		if((error_code=bbbgpio_read_buffer(p_bbbgpio_user_ioctl,GPIO_DATAIN))!=0){
                        return error_code;
		}
		break;
	}
	case BBBGPIOSD:
	{
		if((error_code=bbbgpio_write_buffer(p_bbbgpio_user_ioctl,GPIO_OE))!=0){
                        return error_code;
		}
		break;
	}
	case BBBGPIOGD:
	{
		
		if((error_code=bbbgpio_read_buffer(p_bbbgpio_user_ioctl,GPIO_OE))!=0){
                        return error_code;
		}
		break;
	}
	case BBBGPIOSL0:
	{
		if((error_code=bbbgpio_write_buffer(p_bbbgpio_user_ioctl,GPIO_LEVELDETECT0))!=0){
                        return error_code;
		}
		break;
	}
	case BBBGPIOSH1:
	{
		if((error_code=bbbgpio_write_buffer(p_bbbgpio_user_ioctl,GPIO_LEVELDETECT1))!=0){
                        return error_code;
		}
		break;
	}
	case BBBGPIOSRE:
	{
		if((error_code=bbbgpio_write_buffer(p_bbbgpio_user_ioctl,GPIO_RISINGDETECT))!=0){
                        return error_code;
		}
		break;
	}
	case BBBGPIOSFE:
	{
		if((error_code=bbbgpio_write_buffer(p_bbbgpio_user_ioctl,GPIO_FALLINGDETECT))!=0){
                        return error_code;
		}                  
		break;
	}
	case BBBGPIOGL0:
	{
		if((error_code=bbbgpio_read_buffer(p_bbbgpio_user_ioctl,GPIO_LEVELDETECT0))!=0){
                        return error_code;
		}
		
		break;
	}
	case BBBGPIOGH1:
	{
		if((error_code=bbbgpio_read_buffer(p_bbbgpio_user_ioctl,GPIO_LEVELDETECT1))!=0){
                        return error_code;
		}
		
		break;
	}
	case BBBGPIOGRE:
	{
		if((error_code=bbbgpio_read_buffer(p_bbbgpio_user_ioctl,GPIO_RISINGDETECT))!=0){
                        return error_code;
		}
		break;
	}
	case BBBGPIOGFE:
	{
		if((error_code=bbbgpio_read_buffer(p_bbbgpio_user_ioctl,GPIO_FALLINGDETECT))!=0){
                        return error_code;
		}                  
		break;
	}
	
	case BBBGPIOSIN:
	{
		/*do kernel probing probing! :)*/
		/*TODO: Use a buffer for storing data in interrupt mode. PS: use enum for working mode- interrupt base or busy-wait*/
		break;
	}
	case BBBGPIOGIN:
	{
		break;
	}
	default:
	{
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		return -ENOTTY;
	}
	
	}
	
	return 0;     
}

static ssize_t bbbgpio_read(struct file *filp,char __user *buffer,size_t length,loff_t *offset)
{
	
	int bytes_read = 0;
	
	/* If we're at the end of the message, return 0 signifying end of file */
	if (*msg_Ptr == 0) return 0;
	if(mutex_trylock(&bbbgpiodev_Ptr->io_mutex)==0){
		driver_err("%s:Mutex not free!\n",DEVICE_NAME);
		return -EBUSY;  
	}
	/* Actually put the data into the buffer */
	while (length && *msg_Ptr)  {
		/* The buffer is in the user data segment, not the kernel segment;
		 * assignment won't work.  We have to use put_user which copies data from
		 * the kernel data segment to the user data segment. */
		put_user(*(msg_Ptr++), buffer++);
		length--;
		bytes_read++;
	}
	
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	/* Most read functions return the number of bytes put into the buffer */
	return bytes_read;
	
}
static ssize_t bbbgpio_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset)
{
	u8 bytes_not_wrote;
	if(mutex_trylock(&bbbgpiodev_Ptr->io_mutex)==0){
		driver_err("%s:Mutex not free!\n",DEVICE_NAME);
		return -EBUSY;  
	}
	bytes_not_wrote=copy_from_user(msg,buffer,length);
	*offset+=length-bytes_not_wrote;
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	if(bytes_not_wrote){
		driver_err("Could not write %d\n",bytes_not_wrote);
		return -EFAULT;
	}
	return length-bytes_not_wrote;
}


static void 
kernel_probe_interrupt(void)
{
	u8 count=0;
	int errno=0;
	while(errno<0 && count<5){
		unsigned long maks;
		mask=probe_irq_on();
		
	}
}

static int
__init bbbgpio_init(void)
{
	bbbgpiodev_Ptr=kmalloc(sizeof(struct bbbgpio_device),GFP_KERNEL);
	if(bbbgpiodev_Ptr==NULL){
		driver_err("%s:Failed to alloc memory for p_bbbgpio_device\n",DEVICE_NAME);
		goto failed_alloc;
	}
	memset(bbbgpiodev_Ptr, 0,sizeof(struct bbbgpio_device));
	if(alloc_chrdev_region(&bbbgpio_dev_no,0,1,DEVICE_NAME)<0){
		driver_err("%s:Coud not register\n",DEVICE_NAME);
		goto failed_register;
	}
	bbbgpioclass_Ptr=class_create(THIS_MODULE,DEVICE_CLASS_NAME);
	if(IS_ERR(bbbgpioclass_Ptr)){
		driver_err("%s:Could not create class\n",DEVICE_NAME);
		goto failed_class_create;
	}
	cdev_init(&(bbbgpiodev_Ptr->cdev),&fops);
	bbbgpiodev_Ptr->cdev.owner=THIS_MODULE;
	if(cdev_add(&(bbbgpiodev_Ptr->cdev),bbbgpio_dev_no,1)!=0){
		driver_err("%s:Could not add device\n",DEVICE_NAME);
		goto failed_add_device;
	}
	bbbgpiodev_Ptr->device_Ptr=device_create(bbbgpioclass_Ptr,NULL,MKDEV(MAJOR(bbbgpio_dev_no),0),NULL,DEVICE_PROCESS,0);
	if(IS_ERR(bbbgpiodev_Ptr->device_Ptr)){
		driver_err("%s:Could not create device\n",DEVICE_NAME);
		goto failed_device_create;
	}
	mutex_init(&(bbbgpiodev_Ptr->io_mutex));
	driver_info("%s:Registered device with (%d,%d)\n",DEVICE_NAME,MAJOR(bbbgpio_dev_no),MINOR(bbbgpio_dev_no));
	
	
	driver_info("Driver %s loaded.Build on %s %s\n",DEVICE_NAME,__DATE__,__TIME__);
	memset(&ioctl_buffer,0,sizeof(struct bbbgpio_ioctl_struct));
	
	
	return 0;
failed_device_create:
	{
		device_destroy(bbbgpioclass_Ptr,MKDEV(MAJOR(bbbgpio_dev_no),0));
		cdev_del(&(bbbgpiodev_Ptr->cdev));
	}
failed_add_device:
	{
		class_destroy(bbbgpioclass_Ptr);
            bbbgpioclass_Ptr=NULL;
	}
failed_class_create:
	{
		unregister_chrdev_region(bbbgpio_dev_no,1);
	}
	
failed_register:
	{
		kfree(bbbgpiodev_Ptr);
		bbbgpiodev_Ptr=NULL;
	}
	
failed_alloc:
	{
		return -EBUSY;
	}
}

static void 
__exit bbbgpio_exit(void){
        driver_info("%s:Unregister...",DEVICE_NAME);
        if(bbbgpiodev_Ptr!=NULL){
                device_destroy(bbbgpioclass_Ptr,MKDEV(MAJOR(bbbgpio_dev_no),0));
                cdev_del(&(bbbgpiodev_Ptr->cdev));
                kfree(bbbgpiodev_Ptr);
                bbbgpiodev_Ptr=NULL;
        }
        unregister_chrdev_region(bbbgpio_dev_no,1);
        if(bbbgpioclass_Ptr!=NULL){
                class_destroy(bbbgpioclass_Ptr);
                bbbgpioclass_Ptr=NULL;
        }
        driver_info("Driver %s unloaded.Build on %s %s\n",DEVICE_NAME,__DATE__,__TIME__);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("device")


module_init(bbbgpio_init);
module_exit(bbbgpio_exit);
