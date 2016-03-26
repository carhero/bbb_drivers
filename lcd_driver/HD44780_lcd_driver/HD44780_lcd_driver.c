#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include "hd44780.h"

#define AUTHOR "23ars <ardeleanasm@gmail.com>"
#define DRIVER_DESC "LCD Driver"
#define DEVICE_NAME "hd44780_lcd_driver"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("hd44780");

/*Extern functions*/
void hd44780_init_lcd(const union ULcdPins *u_lcd_pins)
{
}

EXPORT_SYMBOL(hd44780_init_lcd);

static int
__init hd44780_init(void)
{
	printk(KERN_INFO,"Driver %s loaded.Build on %s %s\n",DEVICE_NAME,__DATE__,__TIME__);

	return 0;
}

static void
__exit hd44780_exit(void)
{
	printk(KERN_INFO,"Driver %s unloaded. Build on %s %s\n",DEVICE_NAME,__DATE__,__TIME__);
}


module_init(hd44780_init);
module_exit(hd44780_exit);
