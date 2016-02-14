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
#include <linux/delay.h>
#include <asm/io.h>/*used for ioremap*/
#include <linux/ioport.h> /* used for request_mem_region*/
#include "bbb_registers.h"

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

/*Enum with work modes available in bbbgpio.*/
enum EBbbWorkingMode
{
	BUSY_WAIT=0x00,
	INT_DRIVEN=0x01
};
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
	u8 gpio_group;
	u32 write_buffer;
	u32 read_buffer;
      
};



static struct bbbgpio_device *bbbgpiodev_Ptr=NULL;
static dev_t bbbgpio_dev_no;
static struct class *bbbgpioclass_Ptr=NULL;
static struct bbbgpio_ioctl_struct ioctl_buffer;
volatile int bbb_irq=-1;
volatile u32 gpio_group0_mem_address;
volatile u32 gpio_group1_mem_address;
volatile u32 gpio_group2_mem_address;
volatile u32 gpio_group3_mem_address;
static enum EBbbWorkingMode bbb_working_mode=BUSY_WAIT;/*by default, the driver will be in BUSY_WAIT mode*/
static u8 bbb_is_memory_mapped;

/*
====================================
DRIVER's RING BUFFER API
====================================
*/
#define BUF_LEN 8            /* Max length of the message from the device */
struct bbb_ring_buffer
{
	u32 data[BUF_LEN];
	u8 length;
	u8 head;
	u8 tail;
};
static struct bbb_ring_buffer bbb_data_buffer;
static void bbb_buffer_push(struct bbb_ring_buffer *,u32);
static s8 bbb_buffer_pop(struct bbb_ring_buffer *,u32 *);
static void bbb_buffer_init(struct bbb_ring_buffer *);
static u8 bbb_buffer_empty(struct bbb_ring_buffer *);






/*
====================================
DRIVER's IOCTL OPTIONS
====================================
*/
#define _IOCTL_MAGIC 'K'
#define IOCBBBGPIOWR       _IOW(_IOCTL_MAGIC,1,struct bbbgpio_ioctl*)      /*write data to register*/
#define IOCBBBGPIORD       _IOR(_IOCTL_MAGIC,2,struct bbbgpio_ioctl*)      /*read from register*/
#define IOCBBBGPIOSD       _IOW(_IOCTL_MAGIC,3,struct bbbgpio_ioctl*)      /*set direction*/
#define IOCBBBGPIOGD       _IOR(_IOCTL_MAGIC,4,struct bbbgpio_ioctl*)      /*read direction*/
#define IOCBBBGPIOSL0      _IOW(_IOCTL_MAGIC,5,struct bbbgpio_ioctl*)      /*set low detect*/
#define IOCBBBGPIOSH1      _IOW(_IOCTL_MAGIC,6,struct bbbgpio_ioctl*)      /*set high detect*/
#define IOCBBBGPIOSRE      _IOW(_IOCTL_MAGIC,7,struct bbbgpio_ioctl*)      /*set rising edge*/
#define IOCBBBGPIOSFE      _IOW(_IOCTL_MAGIC,8,struct bbbgpio_ioctl*)      /*set falling edge*/
#define IOCBBBGPIOGL0      _IOR(_IOCTL_MAGIC,9,struct bbbgpio_ioctl*)      /*get low detect*/
#define IOCBBBGPIOGH1      _IOR(_IOCTL_MAGIC,10,struct bbbgpio_ioctl*)      /*get low detect*/
#define IOCBBBGPIOGRE      _IOR(_IOCTL_MAGIC,11,struct bbbgpio_ioctl*)      /*get low detect*/
#define IOCBBBGPIOGFE      _IOR(_IOCTL_MAGIC,12,struct bbbgpio_ioctl*)      /*get low detect*/
#define IOCBBBGPIOSIN      _IOW(_IOCTL_MAGIC,13,struct bbbgpio_ioctl*)      /*enable gpio interrupt*/
#define IOCBBBGPIOGWM      _IOR(_IOCTL_MAGIC,14,struct bbbgpio_ioctl*)     /*read gpio work mode*/
#define IOCBBBGPIOSBW      _IOW(_IOCTL_MAGIC,15,struct bbbgpio_ioctl*)      /*enable gpio busy wait mode*/ /*TODO:Not implemented*/

/*
====================================
DRIVER's SYSFS FUNCTIONS & ISR 
====================================
*/
static int bbbgpio_open(struct inode*,struct file*);
static void kernel_probe_interrupt(void);
static void irq_disable(void);
static irq_handler_t irq_handler(int,void *,struct pt_regs *);
static int bbbgpio_release(struct inode*,struct file*);
static long bbbgpio_ioctl(struct file*, unsigned int ,unsigned long );
static ssize_t bbbgpio_read(struct file *,char __user*,size_t,loff_t*);
static ssize_t bbbgpio_write(struct file *, const char __user *, size_t, loff_t *);
static long bbbgpio_read_buffer(u32 );
static long bbbgpio_write_buffer(u32 );
static u32 gpioreg_map(u8);
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

static u32
gpioreg_map(u8 gpio_group)
{
	switch(gpio_group){
	case 0:return gpio_group0_mem_address;
	case 1:return gpio_group1_mem_address;
	case 2:return gpio_group2_mem_address;
	case 3:return gpio_group3_mem_address;
	default: return gpio_group0_mem_address;/*added GPIO0 for out of range address. to prevent a segfault or kernel dump, altough gpio_group will never be out of range*/
	}
}

/*write_buffer and read_buffer are just to generic functions to 
read and write data when a ioctl option is passed to driver*/
static long
bbbgpio_write_buffer(u32 gpio_register)
{
#ifdef DEBUGGING
	driver_info("%s:Write at address 0x%08X value 0x%08X\n",DEVICE_NAME,ioctl_buffer.gpio_group,ioctl_buffer.write_buffer);
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	return 0;
#else
	volatile u32 *memory_Ptr=NULL;
	if(ioctl_buffer.gpio_group>=0 && ioctl_buffer.gpio_group<=3){
		memory_Ptr=(u32*)(gpioreg_map(ioctl_buffer.gpio_group)|gpio_register);
		*memory_Ptr=ioctl_buffer.write_buffer;
		wmb();
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		return 0;
	}
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	driver_err("%s:Invalid value for gpio group number\n",DEVICE_NAME);
	return -EBADSLT;
#endif
}
static long
bbbgpio_read_buffer(u32 gpio_register)
{
#ifdef DEBUGGING
	driver_info("%s:Read from at address 0x%08X\n",DEVICE_NAME,ioctl_buffer.gpio_group);
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	return 0;
#else
	volatile u32 *memory_Ptr=NULL;
	if(ioctl_buffer.gpio_group>=0 && ioctl_buffer.gpio_group<=3){
		memory_Ptr=(u32*)(gpioreg_map(ioctl_buffer.gpio_group)|gpio_register);
		ioctl_buffer.read_buffer=*memory_Ptr;
		rmb();
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
#endif
}
static long 
bbbgpio_ioctl(struct file *file, unsigned int ioctl_num ,unsigned long ioctl_param)
{

	struct bbbgpio_ioctl_struct __user *p_bbbgpio_user_ioctl;
	long error_code;
	driver_info("%s:Ioctl\n",DEVICE_NAME);	
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
	if(copy_from_user(&ioctl_buffer,p_bbbgpio_user_ioctl,sizeof(struct bbbgpio_ioctl_struct))!=0){
		driver_err("%s:Could not copy data from userspace!\n",DEVICE_NAME);
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		return -EINVAL;
	}
	switch(ioctl_num){
	case IOCBBBGPIOWR:
	{
		if((error_code=bbbgpio_write_buffer(GPIO_DATAOUT))!=0){
                        return error_code;
		}
                
		break;
	}
	case IOCBBBGPIORD:
	{
		if((error_code=bbbgpio_read_buffer(GPIO_DATAIN))!=0){
                        return error_code;
		}
		break;
	}
	case IOCBBBGPIOSD:
	{
		if((error_code=bbbgpio_write_buffer(GPIO_OE))!=0){
                        return error_code;
		}
		break;
	}
	case IOCBBBGPIOGD:
	{
		if((error_code=bbbgpio_read_buffer(GPIO_OE))!=0){
                        return error_code;
		}
		break;
	}
	case IOCBBBGPIOSL0:
	{
		if(bbb_working_mode==BUSY_WAIT){
			break;
		}
		if((error_code=bbbgpio_write_buffer(GPIO_LEVELDETECT0))!=0){
                        return error_code;
		}
		break;
	}
	case IOCBBBGPIOSH1:
	{
		if(bbb_working_mode==BUSY_WAIT){
			break;
		}
		if((error_code=bbbgpio_write_buffer(GPIO_LEVELDETECT1))!=0){
                        return error_code;
		}
		break;
	}
	case IOCBBBGPIOSRE:
	{
		if(bbb_working_mode==BUSY_WAIT){
			break;
		}
		if((error_code=bbbgpio_write_buffer(GPIO_RISINGDETECT))!=0){
                        return error_code;
		}
		break;
	}
	case IOCBBBGPIOSFE:
	{
		if(bbb_working_mode==BUSY_WAIT){
			break;
		}
		if((error_code=bbbgpio_write_buffer(GPIO_FALLINGDETECT))!=0){
                        return error_code;
		}                  
		break;
	}
	case IOCBBBGPIOGL0:
	{
		if((error_code=bbbgpio_read_buffer(GPIO_LEVELDETECT0))!=0){
                        return error_code;
		}
		
		break;
	}
	case IOCBBBGPIOGH1:
	{
		if((error_code=bbbgpio_read_buffer(GPIO_LEVELDETECT1))!=0){
                        return error_code;
		}
		
		break;
	}
	case IOCBBBGPIOGRE:
	{
		if((error_code=bbbgpio_read_buffer(GPIO_RISINGDETECT))!=0){
                        return error_code;
		}
		break;
	}
	case IOCBBBGPIOGFE:
	{
		if((error_code=bbbgpio_read_buffer(GPIO_FALLINGDETECT))!=0){
                        return error_code;
		}                  
		break;
	}
	
	case IOCBBBGPIOSIN:
	{
		if(bbb_working_mode==INT_DRIVEN){
			break;
		}
		kernel_probe_interrupt();
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);/*Unlock mutex after probing*/
		if(bbb_irq!=-1){
			
			if(request_irq(bbb_irq,(irq_handler_t) irq_handler,IRQF_DISABLED,DEVICE_NAME,NULL)){
 	                        driver_err("%s:can't get assigned irq %i\n",DEVICE_NAME,bbb_irq);
 	                        bbb_irq=-1;
			}
		}
		if(bbb_irq!=-1){
			/*If kernel probe successfully and an irq was assigned, change to INT_DRIVEN working mode*/
			bbb_working_mode=INT_DRIVEN;
		}
		break;
	}
	case IOCBBBGPIOGWM:
	{
		ioctl_buffer.read_buffer=bbb_working_mode;
		if(copy_to_user(p_bbbgpio_user_ioctl,&ioctl_buffer,sizeof(struct bbbgpio_ioctl_struct))!=0){
			driver_err("\t%s:Cout not write values to user!\n",DEVICE_NAME);
			mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
			return -EINVAL;
		}
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		break;
	}
	case  IOCBBBGPIOSBW:
	{
		irq_disable();
		bbb_working_mode=BUSY_WAIT;
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
	
	volatile u32* memory_Ptr=NULL;
	if(mutex_trylock(&bbbgpiodev_Ptr->io_mutex)==0){
		driver_err("%s:Mutex not free!\n",DEVICE_NAME);
		return -EBUSY;  
	}
	if(bbb_working_mode==BUSY_WAIT){
		if(copy_from_user(&ioctl_buffer,buffer,sizeof(struct bbbgpio_ioctl_struct))!=0){
			driver_err("%s:Could not copy data from userspace!\n",DEVICE_NAME);
			mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
			return -EINVAL;
		}
		if(ioctl_buffer.gpio_group>=0 && ioctl_buffer.gpio_group<=3){
			memory_Ptr=(u32*)(gpioreg_map(ioctl_buffer.gpio_group)|GPIO_DATAIN);
			ioctl_buffer.read_buffer=*memory_Ptr;
			rmb();
		}
		
	}
	else{
		if(bbb_buffer_pop(&bbb_data_buffer,&ioctl_buffer.read_buffer)!=0){
			driver_err("\t%s:No data storred!\n",DEVICE_NAME);
			mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
			return length;
		}
		
		
	}
	if(copy_to_user(buffer,&ioctl_buffer,sizeof(data))!=0){
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
	volatile u32* memory_Ptr=NULL;
	if(mutex_trylock(&bbbgpiodev_Ptr->io_mutex)==0){
		driver_err("%s:Mutex not free!\n",DEVICE_NAME);
		return -EBUSY;  
	}
	
	if(copy_from_user(&ioctl_buffer,buffer,sizeof(struct bbbgpio_ioctl_struct))!=0){
		driver_err("%s:Could not copy data from userspace!\n",DEVICE_NAME);
		mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
		return -EINVAL;
	}
	driver_info("%s:Write at address 0x%08X value 0x%08X\n",DEVICE_NAME,ioctl_buffer.gpio_group,ioctl_buffer.write_buffer);
	if(ioctl_buffer.gpio_group>=0 && ioctl_buffer.gpio_group<=3){
		memory_Ptr=(u32*)(gpioreg_map(ioctl_buffer.gpio_group)|GPIO_DATAOUT);
		*memory_Ptr=ioctl_buffer.write_buffer;
		wmb();
	}
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	return 0;
}

static irq_handler_t 
irq_handler(int irq,void *dev_id,struct pt_regs *regs)
{
	volatile u32 *memory_Ptr=NULL;
	if(mutex_trylock(&bbbgpiodev_Ptr->io_mutex)==0){
		driver_err("%s:Mutex not free!\n",DEVICE_NAME);
		goto exit_interrupt;
	}
	
	if(ioctl_buffer.gpio_group>=0 && ioctl_buffer.gpio_group<=3){
		memory_Ptr=(u32*)(gpioreg_map(ioctl_buffer.gpio_group)|GPIO_DATAIN);
		ioctl_buffer.read_buffer=*memory_Ptr;
		rmb();
		bbb_buffer_push(&bbb_data_buffer,ioctl_buffer.read_buffer);
	}
	mutex_unlock(&bbbgpiodev_Ptr->io_mutex);
	driver_info("Interrup handler executed!\n");
exit_interrupt:{

	return (irq_handler_t) IRQ_HANDLED;
	}
}

static void 
irq_disable(void)
{
#ifdef DEBUGGING
	driver_info("%s:Disable IRQ\n",DEVICE_NAME);
#else
	volatile u32 *memory_Ptr=NULL;
	memory_Ptr=(u32*)(gpioreg_map(ioctl_buffer.gpio_group)|GPIO_IRQSTATUS_CLR_0);/*calculate address of GPIO_IRQSTATUS_SET_0*/
	*memory_Ptr=ioctl_buffer.write_buffer;/*Enable interrupt. Value from write buffer will be 1<<PIN_NUMBER*/
	wmb();
	memory_Ptr=(u32*)(gpioreg_map(ioctl_buffer.gpio_group)|GPIO_IRQSTATUS_CLR_1);/*calculate address of GPIO_IRQSTATUS_SET_0*/
	*memory_Ptr=ioctl_buffer.write_buffer;/*Enable interrupt. Value from write buffer will be 1<<PIN_NUMBER*/
	wmb();
#endif
}


static void 
kernel_probe_interrupt(void)
{
#ifdef DEBUGGING
	driver_info("%s:Enable IRQ debug!\n",DEVICE_NAME);
#else
	u8 count=0;
	unsigned long mask;
	volatile u32 *memory_Ptr=NULL;
	while(bbb_irq<0 && count<5){
		mask=probe_irq_on();
		memory_Ptr=(u32*)(gpioreg_map(ioctl_buffer.gpio_group)|GPIO_IRQSTATUS_SET_0);/*calculate address of GPIO_IRQSTATUS_SET_0*/
		*memory_Ptr=ioctl_buffer.write_buffer;/*Enable interrupt. Value from write buffer will be 1<<PIN_NUMBER*/
		wmb();
		memory_Ptr=(u32*)(gpioreg_map(ioctl_buffer.gpio_group)|GPIO_IRQSTATUS_SET_1);/*calculate address of GPIO_IRQSTATUS_SET_1*/
		*memory_Ptr=ioctl_buffer.write_buffer;/*Enable interrupt. Value from write buffer will be 1<<PIN_NUMBER*/
		wmb();
		udelay(5);/*wait some time*/
		count++;
		if ((bbb_irq = probe_irq_off(mask)) == 0) { /* none of them? */
 			driver_err(KERN_INFO "%s: no irq reported by probe\n",DEVICE_NAME);
 			bbb_irq=-1;
 		}
 		
	}
	if(bbb_irq==-1){
		driver_err(KERN_INFO "%s: no irq reported by probe after %d attempts\n",DEVICE_NAME,count);
	}
#endif
}
static void 
bbb_buffer_init(struct bbb_ring_buffer *buffer)
{
	memset(buffer,0,sizeof(struct bbb_ring_buffer));
}
static void 
bbb_buffer_push(struct bbb_ring_buffer *buffer,u32 data)
{
	buffer->data[buffer->tail]=data;
	buffer->tail=(buffer->tail+1)%BUF_LEN;
	if(buffer->length<BUF_LEN){
		buffer->length++;
	}else{
		buffer->head=(buffer->head+1)%BUF_LEN;
	}
}
static s8 
bbb_buffer_pop(struct bbb_ring_buffer *buffer,u32 *data)
{
	if(!buffer->length){
		return -1;
	}
	*data=buffer->data[buffer->head];
	buffer->head=(buffer->head+1)%BUF_LEN;
	buffer->length--;
	return 0;
}
static u8
bbb_buffer_empty(struct bbb_ring_buffer *buffer)
{
	if(!buffer->length){
		return 0;
	}
	return 1;
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
	bbb_buffer_init(&bbb_data_buffer);

/*Just to be sure that memory was requested by kernel*/
	if(
		request_mem_region(GPIO0_START,(GPIO0_END-GPIO0_START),GPIO0)!=NULL ||
		request_mem_region(GPIO1_START,(GPIO1_END-GPIO1_START),GPIO1)!=NULL ||
		request_mem_region(GPIO2_START,(GPIO2_END-GPIO2_START),GPIO2)!=NULL ||
		request_mem_region(GPIO3_START,(GPIO3_END-GPIO3_START),GPIO3)!=NULL){
		bbb_is_memory_mapped=1;
		driver_info("%s: Memory regions are already mapped!\n",DEVICE_NAME);
	}

	if((gpio_group0_mem_address=(u32)ioremap(GPIO0_START,(GPIO0_END-GPIO0_START)))==NULL){
		driver_err("%s:Could not map address %0x%08X...\n",DEVICE_NAME,GPIO0_START);
	}
	if((gpio_group1_mem_address=(u32)ioremap(GPIO1_START,(GPIO1_END-GPIO1_START)))==NULL){
		driver_err("%s:Could not map address %0x%08X...\n",DEVICE_NAME,GPIO1_START);
	}
	if((gpio_group2_mem_address=(u32)ioremap(GPIO2_START,(GPIO2_END-GPIO2_START)))==NULL){
		driver_err("%s:Could not map address %0x%08X...\n",DEVICE_NAME,GPIO2_START);
	}
	if((gpio_group3_mem_address=(u32)ioremap(GPIO3_START,(GPIO3_END-GPIO3_START)))==NULL){
		driver_err("%s:Could not map address %0x%08X...\n",DEVICE_NAME,GPIO2_START);
	}
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
	if(bbb_is_memory_mapped==1){
		bbb_is_memory_mapped=0;
		release_mem_region(GPIO0_START,(GPIO0_END-GPIO0_START));
		release_mem_region(GPIO1_START,(GPIO1_END-GPIO1_START));
		release_mem_region(GPIO2_START,(GPIO2_END-GPIO2_START));
		release_mem_region(GPIO3_START,(GPIO3_END-GPIO3_START));

	}


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
