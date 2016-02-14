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
#define GROUP_READ 2//0
#define PIN_NO_READ 2//27
#define NO_OF_WRITES 10
#define NO_OF_READS 10
int main(int argc, char argv)
{
      int fd;
      struct bbbgpio_ioctl_struct ioctl_struct;
      int i;
      int read_value;
      fd=open("/dev/bbbgpio0",O_RDWR);
      if(fd==-1){
            fprintf(stderr,"Open:%s\n",strerror(errno));
            return -1;
      }
      /* Fill with 0s structure*/
      memset(&ioctl_struct,0,sizeof(struct bbbgpio_ioctl_struct));
      
      /*Enable busy wait mode*/
      if(ioctl(fd,IOCBBBGPIOSBW,&ioctl_struct)!=0){
            fprintf(stderr,"IOCBBBGPIOSBW:%s\n",strerror(errno));
            goto error;
      }
      
      // /* Set direction output for writes*/
      // ioctl_struct.gpio_group=GROUP_WRITE;
      // ioctl_struct.write_buffer=(0xFFFFFFFF-(1<<PIN_NO_WRITE));
      // if(ioctl(fd,IOCBBBGPIOSD,&ioctl_struct)!=0){
      //       fprintf(stderr,"IOCBBBGPIOSD:%s\n",strerror(errno));
      //       goto error;
      // }
      // /*Perform NO_OF_WRITES with toggle*/
      // i=0;
      // while(i<NO_OF_WRITES){
      //       ioctl_struct.write_buffer=(1<<PIN_NO_WRITE);
      //       if(ioctl(fd,IOCBBBGPIOWR,&ioctl_struct)!=0){
      //             fprintf(stderr,"IOCBBBGPIOWR:%s\n",strerror(errno));
      //             goto error;
      //       }
      //       sleep(1);
      //       ioctl_struct.write_buffer=(0xFFFFFFFF-(1<<PIN_NO_WRITE));
      //       if(ioctl(fd,IOCBBBGPIOWR,&ioctl_struct)!=0){
      //             fprintf(stderr,"IOCBBBGPIOWR:%s\n",strerror(errno));
      //             goto error;
      //       }
      //       sleep(1);
      //       i++;
      // }
      
      /*Perform NO_OF_WRITES without ioctl*/
      // i=0;
      // while(i<NO_OF_WRITES){
      //       ioctl_struct.write_buffer=(1<<PIN_NO_WRITE);
      //       write(fd,&ioctl_struct,sizeof(struct bbbgpio_ioctl_struct));
      //       sleep(1);
      //       ioctl_struct.write_buffer=(0xFFFFFFFF-(1<<PIN_NO_WRITE));
      //       write(fd,&ioctl_struct,sizeof(struct bbbgpio_ioctl_struct));
      //       sleep(1);
      //       i++;
      // }
      
      /*Set direction input (capture)*/
      ioctl_struct.gpio_group=GROUP_READ;
      ioctl_struct.write_buffer=(1<<PIN_NO_READ);
      if(ioctl(fd,IOCBBBGPIOSD,&ioctl_struct)!=0){
            fprintf(stderr,"IOCBBBGPIOSD:%s\n",strerror(errno));
            goto error;
      }
      
      /*Perform read with ioctl*/
      i=0;
      while(i<NO_OF_READS){
            
            // read(fd,&ioctl_struct,sizeof(struct bbbgpio_ioctl_struct));
            if(ioctl(fd,IOCBBBGPIORD,&ioctl_struct)!=0){
                  fprintf(stderr,"IOCBBBGPIOWR:%s\n",strerror(errno));
                  goto error;
            }
            printf("0x%08X\n",ioctl_struct.read_buffer);
            sleep(1);
            i++;
      }
      close(fd);
      return 0;
      error:
      {
            close(fd);
            return errno;
      }
}