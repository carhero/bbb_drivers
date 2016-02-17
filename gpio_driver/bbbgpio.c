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
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>

/*
  ====================================
  DRIVER's INFO
  ====================================
*/
#define DRIVER_AUTHOR "23ars <ardeleanasm@gmail.com>"
#define DRIVER_DESC "Gpio Driver for BBB"
#define DEVICE_NAME "bbbgpio"
#define GPIO0 "bbbgpio0_group"
#define GPIO1 "bbbgpio1_group"
#define GPIO2 "bbbgpio2_group"
#define GPIO3 "bbbgpio3_group"
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
#define driver_err(format,arg...)	do { printk( KERN_ERR   format, ## arg ); } while(0)

#endif


/*bbbgpio device structure*/
struct bbbgpio_device{
	struct cdev cdev;
	struct device *device_Ptr;
	struct mutex io_mutex;
	u8 is_open;
};

/*Ioctl structure. Gpio group will be 0 to 3. Value that will be read/write will be 1<<PIN_NO*/
struct bbbgpio_ioctl_struct
{
	u16 gpio_number;
	u8 write_buffer;
	u8 read_buffer;
	int irq_number; 
};

enum bbbgpio_direction
{
	INPUT=0x00,
	OUTPUT
};

static struct bbbgpio_device *bbbgpiodev_Ptr=NULL;
static dev_t bbbgpio_dev_no;
static struct class *bbbgpioclass_Ptr=NULL;
static struct bbbgpio_ioctl_struct ioctl_buffer;
unsigned long irq_flags=IRQF_TRIGGER_NONE;
volatile int bbb_irq=-1;



/*
  ====================================
  DRIVER's RING BUFFER API
  ====================================
*/
#define BUF_LEN 8            /* Max length of the message from the device */
struct bbb_data_content
{
	u8 data;
	u16 dev_id;
	
};
struct bbb_ring_buffer
{
	struct bbb_data_content data[BUF_LEN];
	u8 length;
	u8 head;
	u8 tail;
};
static struct bbb_ring_buffer bbb_data_buffer;
static void bbb_buffer_push(struct bbb_ring_buffer *,struct bbb_data_content);
static s8 bbb_buffer_pop(struct bbb_ring_buffer *,struct bbb_data_content *);
static void bbb_buffer_init(struct bbb_ring_buffer *);
static u8 bbb_buffer_empty(struct bbb_ring_buffer *);






/*
  ====================================
  DRIVER's IOCTL OPTIONS
  ====================================
*/
#define _IOCTL_MAGIC 'K'
#define IOCBBBGPIORP       _IOW(_IOCTL_MAGIC,1,struct bbbgpio_ioctl*)     /*IOCTL used for registering PIN*/
#define IOCBBBGPIOUP       _IOW(_IOCTL_MAGIC,2,struct bbbgpio_ioctl*)     /*IOCTL used for unregistering PIN*/
#define IOCBBBGPIOWR       _IOW(_IOCTL_MAGIC,3,struct bbbgpio_ioctl*)      /*write data to register*/
#define IOCBBBGPIORD       _IOR(_IOCTL_MAGIC,4,struct bbbgpio_ioctl*)      /*read from register*/
#define IOCBBBGPIOSD       _IOW(_IOCTL_MAGIC,5,struct bbbgpio_ioctl*)      /*set direction*/
#define IOCBBBGPIOSL0      _IOW(_IOCTL_MAGIC,6,struct bbbgpio_ioctl*)      /*set low detect*/
#define IOCBBBGPIOSH1      _IOW(_IOCTL_MAGIC,7,struct bbbgpio_ioctl*)      /*set high detect*/
#define IOCBBBGPIOSRE      _IOW(_IOCTL_MAGIC,8,struct bbbgpio_ioctl*)      /*set rising edge*/
#define IOCBBBGPIOSFE      _IOW(_IOCTL_MAGIC,9,struct bbbgpio_ioctl*)      /*set falling edge*/
#define IOCBBBGPIOSIN      _IOW(_IOCTL_MAGIC,10,struct bbbgpio_ioctl*)      /*enable gpio interrupt*/
#define IOCBBBGPIOSBW      _IOW(_IOCTL_MAGIC,11,struct bbbgpio_ioctl*)      /*enable gpio busy wait mode*/ 

/*
  ====================================
  DRIVER's SYSFS FUNCTIONS & ISR 
  ====================================
*/
static int bbbgpio_open(struct inode*,struct file*);
static int bbbgpio_release(struct inode*,struct file*);
static long bbbgpio_ioctl(struct file*, unsigned int ,unsigned long );
static ssize_t bbbgpio_read(struct file *,char __user*,size_t,loff_t*);
static ssize_t bbbgpio_write(struct file *, const char __user *, size_t, loff_t *);
static irq_handler_t irq_handler(int,void *,struct pt_regs *);
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
	driver_info("%s:Open\n",DEVICE_NAME);
	if (mutex_trylock(&bbbgpiodev_Ptr->io_mutex) == 0) {
		driver_err("%s:Mutex not free!\n",DEVICE_NAME);
		return -EBUSY;
	}
	if (bbbgpiodev_Ptr->is_open == 1) {
		driver_err("%s:already open\n",DEVICE_NAME);
		return -EBUSY;
	}
	bbbgpiodev_Ptr->is_open=1;
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	driver_info("%s:Driver Open successfully!\n",DEVICE_NAME);
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
bbbgpio_ioctl(struct file *file, unsigned int ioctl_num ,unsigned long ioctl_param)
{
	
	struct bbbgpio_ioctl_struct __user *p_bbbgpio_user_ioctl;
	long error_code;
	struct bbb_data_content data;
	driver_info("%s:Ioctl\n",DEVICE_NAME);
	memset(&data,0,sizeof(struct bbb_data_content));
	memset(&ioctl_buffer,0,sizeof(struct bbbgpio_ioctl_struct));
	if (bbbgpiodev_Ptr == NULL) {
		driver_err("%s:Device not found!\n",DEVICE_NAME);
		return -ENODEV;
	}
	if (mutex_trylock(&bbbgpiodev_Ptr->io_mutex) == 0) {
		driver_err("%s:Mutex not free!\n",DEVICE_NAME);
		return -EBUSY;
	}
	p_bbbgpio_user_ioctl=(struct bbbgpio_ioctl_struct __user*)ioctl_param;
	if (copy_from_user(&ioctl_buffer,p_bbbgpio_user_ioctl,sizeof(struct bbbgpio_ioctl_struct)) != 0) {
		driver_err("%s:Could not copy data from userspace!\n",DEVICE_NAME);
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		return -EINVAL;
	}
	switch (ioctl_num) {
	case IOCBBBGPIORP:
	{
		gpio_request(ioctl_buffer.gpio_number,"sysfs");
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		break;
	}
	case IOCBBBGPIOUP:
	{
		gpio_unexport(ioctl_buffer.gpio_number);
		gpio_free(ioctl_buffer.gpio_number);
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		break;
	}
	case IOCBBBGPIOWR:
	{
		gpio_set_value(ioctl_buffer.gpio_number,ioctl_buffer.write_buffer);
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		break;
	}
	case IOCBBBGPIORD:
	{
		if (bbb_buffer_empty(&bbb_data_buffer) == 1) {
			mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
			return -EAGAIN;
		}
		bbb_buffer_pop(&bbb_data_buffer,&data);
		ioctl_buffer.gpio_number=data.dev_id;
		ioctl_buffer.read_buffer=data.data;
		if (copy_to_user(p_bbbgpio_user_ioctl,&ioctl_buffer,sizeof(struct bbbgpio_ioctl_struct)) != 0) {
			driver_err("\t%s:Cout not write values to user!\n",DEVICE_NAME);
			mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
			return -EINVAL;
		}
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		break;
	}
	case IOCBBBGPIOSD:
	{
		if (ioctl_buffer.write_buffer == OUTPUT)
			error_code=gpio_direction_output(ioctl_buffer.gpio_number,0);
		else 
			error_code=gpio_direction_input(ioctl_buffer.gpio_number);
		
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		if (error_code != 0) 
			return error_code;
		gpio_export(ioctl_buffer.gpio_number,false);
		break;
	}
	case IOCBBBGPIOSL0:
	{
		irq_flags=IRQF_TRIGGER_LOW;
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		break; 
	}
	case IOCBBBGPIOSH1:
	{
		irq_flags=IRQF_TRIGGER_HIGH;
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		break; 
	}
	case IOCBBBGPIOSRE:
	{
		irq_flags=IRQF_TRIGGER_RISING;
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		break; 
	}
	case IOCBBBGPIOSFE:
	{
		irq_flags=IRQF_TRIGGER_FALLING;
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		break; 

	}
	case IOCBBBGPIOSIN:
	{
		bbb_irq=gpio_to_irq(ioctl_buffer.gpio_number);
		if (request_irq(bbb_irq,(irq_handler_t) irq_handler,irq_flags,DEVICE_NAME,(void *)ioctl_buffer.gpio_number)) {
			driver_err("%s:can't get assigned irq %i\n",DEVICE_NAME,bbb_irq);
			bbb_irq=-1;
		}
		ioctl_buffer.irq_number=bbb_irq;
		if (copy_to_user(p_bbbgpio_user_ioctl,&ioctl_buffer,sizeof(struct bbbgpio_ioctl_struct)) != 0) {
			driver_err("\t%s:Cout not write values to user!\n",DEVICE_NAME);
			mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
			return -EINVAL;
		}

		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);/*Unlock mutex after probing*/		
		break;
	}
	case  IOCBBBGPIOSBW:
	{
		free_irq(ioctl_buffer.irq_number,NULL);
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
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

static ssize_t 
bbbgpio_read(struct file *filp,char __user *buffer,size_t length,loff_t *offset)
{

	if (mutex_trylock(&bbbgpiodev_Ptr->io_mutex) == 0) {
		driver_err("%s:Mutex not free!\n",DEVICE_NAME);
		return -EBUSY;  
	}
	if (copy_from_user(&ioctl_buffer,buffer,sizeof(struct bbbgpio_ioctl_struct)) != 0) {
		driver_err("%s:Could not copy data from userspace!\n",DEVICE_NAME);
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		return -EINVAL;
	}
	
	ioctl_buffer.read_buffer=gpio_get_value(ioctl_buffer.gpio_number);
	if (copy_to_user(buffer,&ioctl_buffer,sizeof(struct bbbgpio_ioctl_struct)) !=0 ) {
		driver_err("\t%s:Cout not write values to user!\n",DEVICE_NAME);
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		return -EINVAL;
	}
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	return 0;
	
}
static ssize_t 
bbbgpio_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset)
{
	if (mutex_trylock(&bbbgpiodev_Ptr->io_mutex) == 0) {
		driver_err("%s:Mutex not free!\n",DEVICE_NAME);
		return -EBUSY;  
	}
	
	if (copy_from_user(&ioctl_buffer,buffer,sizeof(struct bbbgpio_ioctl_struct)) != 0) {
		driver_err("%s:Could not copy data from userspace!\n",DEVICE_NAME);
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		return -EINVAL;
	}
	gpio_set_value(ioctl_buffer.gpio_number,ioctl_buffer.write_buffer);
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	return 0;
}

static irq_handler_t 
irq_handler(int irq,void *dev_id,struct pt_regs *regs)
{
	u16 io_number=(u16)dev_id;
	struct bbb_data_content content;
	if (mutex_trylock(&bbbgpiodev_Ptr->io_mutex) == 0) {
		driver_err("%s:Mutex not free!\n",DEVICE_NAME);
		goto exit_interrupt;
	}
	
	content.data=gpio_get_value(io_number);
	content.dev_id=io_number;
	bbb_buffer_push(&bbb_data_buffer,content);
	
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	driver_info("Interrup handler executed!\n");
exit_interrupt:{
		
		return (irq_handler_t) IRQ_HANDLED;
	}
}

static void 
bbb_buffer_init(struct bbb_ring_buffer *buffer)
{
	memset(buffer,0,sizeof(struct bbb_ring_buffer));
}
static void 
bbb_buffer_push(struct bbb_ring_buffer *buffer,struct bbb_data_content data)
{
	buffer->data[buffer->tail]=data;
	buffer->tail=(buffer->tail+1)%BUF_LEN;
	if (buffer->length < BUF_LEN) 
		buffer->length++;
	else 
		buffer->head=(buffer->head+1)%BUF_LEN;
}
static s8 
bbb_buffer_pop(struct bbb_ring_buffer *buffer,struct bbb_data_content *data)
{
	if (!buffer->length) 
		return -1;
	*data=buffer->data[buffer->head];
	buffer->head=(buffer->head+1)%BUF_LEN;
	buffer->length--;
	return 0;
}
static u8
bbb_buffer_empty(struct bbb_ring_buffer *buffer)
{
	return (buffer->length == 0);
}
static int
__init bbbgpio_init(void)
{
	bbbgpiodev_Ptr=kmalloc(sizeof(struct bbbgpio_device),GFP_KERNEL);
	if (bbbgpiodev_Ptr == NULL) {
		driver_err("%s:Failed to alloc memory for p_bbbgpio_device\n",DEVICE_NAME);
		goto failed_alloc;
	}
	memset(bbbgpiodev_Ptr, 0,sizeof(struct bbbgpio_device));
	if (alloc_chrdev_region(&bbbgpio_dev_no,0,1,DEVICE_NAME) < 0) {
		driver_err("%s:Coud not register\n",DEVICE_NAME);
		goto failed_register;
	}
	bbbgpioclass_Ptr=class_create(THIS_MODULE,DEVICE_CLASS_NAME);
	if (IS_ERR(bbbgpioclass_Ptr)) {
		driver_err("%s:Could not create class\n",DEVICE_NAME);
		goto failed_class_create;
	}
	cdev_init(&(bbbgpiodev_Ptr->cdev),&fops);
	bbbgpiodev_Ptr->cdev.owner=THIS_MODULE;
	if (cdev_add(&(bbbgpiodev_Ptr->cdev),bbbgpio_dev_no,1) != 0) {
		driver_err("%s:Could not add device\n",DEVICE_NAME);
		goto failed_add_device;
	}
	bbbgpiodev_Ptr->device_Ptr=device_create(bbbgpioclass_Ptr,NULL,MKDEV(MAJOR(bbbgpio_dev_no),0),NULL,DEVICE_PROCESS,0);
	if (IS_ERR(bbbgpiodev_Ptr->device_Ptr)){
		driver_err("%s:Could not create device\n",DEVICE_NAME);
		goto failed_device_create;
	}
	mutex_init(&(bbbgpiodev_Ptr->io_mutex));
	driver_info("%s:Registered device with (%d,%d)\n",DEVICE_NAME,MAJOR(bbbgpio_dev_no),MINOR(bbbgpio_dev_no));
	
	
	driver_info("Driver %s loaded.Build on %s %s\n",DEVICE_NAME,__DATE__,__TIME__);
	memset(&ioctl_buffer,0,sizeof(struct bbbgpio_ioctl_struct));
	bbb_buffer_init(&bbb_data_buffer);
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
        if (bbbgpiodev_Ptr != NULL) {
                device_destroy(bbbgpioclass_Ptr,MKDEV(MAJOR(bbbgpio_dev_no),0));
                cdev_del(&(bbbgpiodev_Ptr->cdev));
                kfree(bbbgpiodev_Ptr);
                bbbgpiodev_Ptr=NULL;
        }
        unregister_chrdev_region(bbbgpio_dev_no,1);
        if (bbbgpioclass_Ptr != NULL) {
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
