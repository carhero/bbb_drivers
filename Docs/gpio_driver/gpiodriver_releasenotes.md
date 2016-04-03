                    Template version 0.1 released in "30-01-2016" author "23ars"

# BBB Gpio Driver


## Release notes

| gpio_driver-version |               |                          |               |           |            |
| ------------ | -------------- | ------------------------- | ------------- | --------- | ---------- |
| **Issue Id** | **Issue Name** | **Description**           | **Milestone** | **Label** | **Author** |
|  1           | BBB Gpio Driver #1                                   | (see issue description)                                            | BBB Gpio Driver | feature        | 23ars |
|  2           | BBB Gpio Driver Read Buffer #2                       | Implement a simple ring buffer for interrupt read.                 | BBB Gpio Driver | feature        | 23ars |
|  3           | BBB Gpio driver test #3                              | (see issue description)                                            | BBB Gpio Driver | feature        | 23ars |
|  4           | Fix read and write barriers #4                       | No description provided.                                           | BBB Gpio Driver | bug            | 23ars |
|  5           | Code Review and Comments #5                          | (see issue description)                                            | BBB Gpio Driver | feature        | 23ars |
|  6           | Add templates for code review, test and release #6   | No description provided.                                           | BBB Gpio Driver | feature        | 23ars |
|  7           | Add prefix IOC to all ioctls #7                      | No description provided.                                           | BBB Gpio Driver | feature        | 23ars |
|  8           | Prepare driver for testing #8                        | (see issue description)                                            | BBB Gpio Driver | general issues | 23ars | 
|  9           | Map memory for IOs #9                                | (see issue description)                                            | BBB Gpio Driver | bug            | 23ars |
| 10           | fix small bugs bbbgpio #10                           | (see issue description)                                            | BBB Gpio Driver | bug            | 23ars |
| 11           | Add example app #11                                  | No description provided.                                           | BBB Gpio Driver | feature        | 23ars |
| 12           | Add ioctl interface and provide dts example #12      | No description provided.                                           | BBB Gpio Driver | feature        | 23ars |
| 13           | Add modifications to driver and BBB_SRM doc file #13 | No description provided.                                           | BBB Gpio Driver | general issues | 23ars |
| 14           | Coding style #14                                     | Modify code according to Linux Coding style                        | BBB Gpio Driver | general issues | 23ars |
| 15           | Fix read bug #15                                     | No description provided.                                           | BBB Gpio Driver | bug            | 23ars |
| 16           | Fix mutex unlock for setting events on interrupts #16| No description provided.                                           | BBB Gpio Driver | bug            | 23ars |
| 17           | Refactor code and use kernel gpio interface #17      | No description provided.                                           | BBB Gpio Driver | feature        | 23ars |
| 18           | Fix Compiler Warnings #18                            | (see issue description)                                            | BBB Gpio Driver | general issues | 23ars |
| 19           | Fix interfaces #19                                   | Number of IOCTLs is modified and also the data structure for ioctl | BBB Gpio Driver | general issues | 23ars |
| 20           | Make irq number global variable #20                  | No description provided.                                           | BBB Gpio Driver | bug            | 23ars |

---

## Compiler flags
```make
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
```

## Compiler Messages 
```
make -C /lib/modules/3.16.0-4-686-pae/build M=/home/mihai/Git/bbb_drivers/gpio_driver modules
make[1]: Entering directory '/usr/src/linux-headers-3.16.0-4-686-pae'
Makefile:10: *** mixed implicit and normal rules: deprecated syntax
make[1]: Entering directory `/usr/src/linux-headers-3.16.0-4-686-pae'
  CC [M]  /home/mihai/Git/bbb_drivers/gpio_driver/bbbgpio.o
/home/mihai/Git/bbb_drivers/gpio_driver/bbbgpio.c: In function ‘bbbgpio_ioctl’:
/home/mihai/Git/bbb_drivers/gpio_driver/bbbgpio.c:287:77: warning: cast to pointer from integer of different size [-Wint-to-pointer-cast]
   if (request_irq(bbb_irq,(irq_handler_t) irq_handler,irq_flags,DEVICE_NAME,(void *)ioctl_buffer.gpio_number)) {
                                                                             ^
/home/mihai/Git/bbb_drivers/gpio_driver/bbbgpio.c: In function ‘irq_handler’:
/home/mihai/Git/bbb_drivers/gpio_driver/bbbgpio.c:363:16: warning: cast from pointer to integer of different size [-Wpointer-to-int-cast]
  u16 io_number=(u16)dev_id;
                ^
  Building modules, stage 2.
  MODPOST 1 modules
  CC      /home/mihai/Git/bbb_drivers/gpio_driver/bbbgpio.mod.o
  LD [M]  /home/mihai/Git/bbb_drivers/gpio_driver/bbbgpio.ko
make[1]: Leaving directory '/usr/src/linux-headers-3.16.0-4-686-pae'
```





