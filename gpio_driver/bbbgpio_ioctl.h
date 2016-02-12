#ifndef BBBGPIO_IOCTL_H_
#define BBBGPIO_IOCTL_H_


struct bbbgpio_ioctl_struct
{
	uint8_t gpio_group;
	uint32_t write_buffer;
	uint32_t read_buffer;
      
};

/*
====================================
DRIVER's IOCTL OPTIONS
====================================
*/
#define _IOCTL_MAGIC 'K'
#define IOCBBBGPIOWR       _IOW(_IOCTL_MAGIC,1,struct bbbgpio_ioctl*)      /*write data to register*/
#define IOCBBBGPIORD       _IOR(_IOCTL_MAGIC,2,struct bbbgpio_ioctl*)      /*red from register*/
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


#endif