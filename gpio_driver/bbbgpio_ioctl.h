#ifndef BBBGPIO_IOCTL_H_
#define BBBGPIO_IOCTL_H_


struct bbbgpio_ioctl_struct
{
	u16 gpio_number;
	u8 write_buffer;
	u8 read_buffer;
	int irq_number; 
};


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


#endif
