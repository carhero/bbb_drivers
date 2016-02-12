FILE=bbbgpio
obj-m += $(FILE).o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	sudo insmod ./$(FILE).ko
	cp bbbgpio_ioctl.h /usr/include/
run:
	dmesg
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	sudo rmmod $(FILE) 
