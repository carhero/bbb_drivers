/*
gcc test.c -o test
*/
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bbbgpio_ioctl.h>
#include <errno.h>
#include <string.h>
/*P9.41 for write and P8.17 for read*/
#define GROUP_WRITE 0
#define PIN_NO_WRITE 20
#define GPIO_NUMBER_WRITE 32*GROUP_WRITE+PIN_NO_WRITE
#define GROUP_READ 0
#define PIN_NO_READ 27
#define NO_OF_WRITES 10
#define NO_OF_READS 10
int main(int argc, char argv)
{
      int fd;
      struct bbbgpio_ioctl_struct ioctl_struct;
      int i;
      int read_value;
      int irq;
      const uint16_t gpio_number_write=32*GROUP_WRITE+PIN_NO_WRITE;
      const uint16_t gpio_number_read=32*GROUP_READ+PIN_NO_READ;
      fd=open("/dev/bbbgpio0",O_RDWR);
      if(fd==-1){
            fprintf(stderr,"Open:%s\n",strerror(errno));
            return -1;
      }
      /* Fill with 0s structure*/
      memset(&ioctl_struct,0,sizeof(struct bbbgpio_ioctl_struct));
      /* Request pin*/
      ioctl_struct.gpio_number=gpio_number_write;
      if(ioctl(fd,IOCBBBGPIORP,&ioctl_struct)!=0){
            fprintf(stderr,"IOCBBBGPIORP:%s\n",strerror(errno));
            goto error;
      }
      ioctl_struct.gpio_number=gpio_number_read;
      if(ioctl(fd,IOCBBBGPIORP,&ioctl_struct)!=0){
            fprintf(stderr,"IOCBBBGPIORP:%s\n",strerror(errno));
            goto error;
      }
      

      /* Set direction output for writes*/
      ioctl_struct.gpio_number=gpio_number_write;
      ioctl_struct.write_buffer=1;
      if(ioctl(fd,IOCBBBGPIOSD,&ioctl_struct)!=0){
            fprintf(stderr,"IOCBBBGPIOSD:%s\n",strerror(errno));
            goto error;
      }
      /*Perform NO_OF_WRITES with toggle*/
      i=0;
      while(i<NO_OF_WRITES){
            ioctl_struct.gpio_number=gpio_number_write;
            ioctl_struct.write_buffer=1;
            if(ioctl(fd,IOCBBBGPIOWR,&ioctl_struct)!=0){
                  fprintf(stderr,"IOCBBBGPIOWR:%s\n",strerror(errno));
                  goto error;
            }
            sleep(1);
            ioctl_struct.gpio_number=gpio_number_write;
            ioctl_struct.write_buffer=0;
            if(ioctl(fd,IOCBBBGPIOWR,&ioctl_struct)!=0){
                  fprintf(stderr,"IOCBBBGPIOWR:%s\n",strerror(errno));
                  goto error;
            }
            sleep(1);
            i++;
      }
      
      /*Perform NO_OF_WRITES without ioctl*/
      i=0;
      while(i<NO_OF_WRITES){
            ioctl_struct.gpio_number=gpio_number_write;
            ioctl_struct.write_buffer=1;
            write(fd,&ioctl_struct,sizeof(struct bbbgpio_ioctl_struct));
            sleep(1);
            ioctl_struct.gpio_number=gpio_number_write;
            ioctl_struct.write_buffer=0;
            write(fd,&ioctl_struct,sizeof(struct bbbgpio_ioctl_struct));
            sleep(1);
            i++;
      }
      
      /*Set direction input (capture)*/
      ioctl_struct.gpio_number=gpio_number_read;
      ioctl_struct.write_buffer=0;
      if(ioctl(fd,IOCBBBGPIOSD,&ioctl_struct)!=0){
            fprintf(stderr,"IOCBBBGPIOSD:%s\n",strerror(errno));
            goto error;
      }
      /*Perform read*/
      i=0;
      while(i<NO_OF_READS){
            
            read(fd,&ioctl_struct,sizeof(struct bbbgpio_ioctl_struct));
            printf("0x%08X\n",ioctl_struct.read_buffer);
            sleep(1);
            i++;
      }
      
      /*Enable interrupts*/
      ioctl_struct.gpio_number=gpio_number_read;
      if(ioctl(fd,IOCBBBGPIOSH1,&ioctl_struct)!=0){
            fprintf(stderr,"IOCBBBGPIOSH1:%s\n",strerror(errno));
            goto error;
      }
     
      ioctl_struct.gpio_number=gpio_number_read;
      if(ioctl(fd,IOCBBBGPIOSIN,&ioctl_struct)!=0){
            fprintf(stderr,"IOCBBBGPIOSD:%s\n",strerror(errno));
            goto error;
      }
      irq=ioctl_struct.irq_number;
      i=0;
      while(i<NO_OF_READS){
            ioctl_struct.gpio_number=gpio_number_read;
            ioctl_struct.irq_number=irq;
            if(ioctl(fd,IOCBBBGPIORD,&ioctl_struct)!=0){
                  fprintf(stderr,"IOCBBBGPIORD:%s\n",strerror(errno));
                  goto error;
            }
            printf("0x%08X\n",ioctl_struct.read_buffer);
            sleep(2);
            i++;
      }
      
      
      ioctl_struct.gpio_number=gpio_number_read;
      ioctl_struct.irq_number=irq;
      printf("Irq number %i",irq);
      if(ioctl(fd,IOCBBBGPIOSBW,&ioctl_struct)!=0){
            fprintf(stderr,"IOCBBBGPIOSD:%s\n",strerror(errno));
            goto error;
      }
      
      /* Release pin*/
      ioctl_struct.gpio_number=gpio_number_read;
      if(ioctl(fd,IOCBBBGPIOUP,&ioctl_struct)!=0){
            fprintf(stderr,"IOCBBBGPIOUP:%s\n",strerror(errno));
            goto error;
      }
      ioctl_struct.gpio_number=gpio_number_write;
      if(ioctl(fd,IOCBBBGPIOUP,&ioctl_struct)!=0){
            fprintf(stderr,"IOCBBBGPIOUP:%s\n",strerror(errno));
            goto error;
      }
      close(fd);
      return 0;
      error:
      {
            close(fd);
            return errno;
      }
}
