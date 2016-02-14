#!/bin/bash

#workaround for a bug in bone-pinmux-helper to enable GPIO modules.
#based on the code from Luigi Rinaldi: https://groups.google.com/forum/#!msg/beagleboard/OYFp4EXawiI/Mq6s3sg14HoJ

#based on code provided by piranha32 at >>https://github.com/piranha32/IOoo/blob/master/examples/bbb_enable_gpio.sh

echo 5 > /sys/class/gpio/export
echo 65 > /sys/class/gpio/export
echo 105 > /sys/class/gpio/export
