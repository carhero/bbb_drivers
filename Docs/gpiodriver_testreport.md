                    Template version 0.1 released in "30-01-2016" author "23ars"

## BBB Gpio Driver

| General Test Report for **gpio_driver**-version    |                        |              |                 |
| ------------------------- | ---------------------- | ---------------------- | ----------------------------- |
| **No of tests performed** | **No of failed tests** | **No of passed tests** | **No of not performed tests** |
|                           |                        |                        |                               |


---

For all test, use DEBUG macro defined. In the testing version all debug messages will be available. In release version, DEBUG macro shall be undefined and only debug_err macro will be available.


| gpio_driver Test Report    |                |               |                     |                   |                               |                 |
|----------------------------|----------------|---------------|---------------------| ----------------- | ----------------------------- | --------------- |
| **Tester:**                | tester_name                                          |
| **Software Version:**      |                                                      |
|                                                                                   |
| **Test ID** | **Test Priority** | **Test Name** | **Precondition**                             | **Test Steps**            | **Expected Results**          | **Test Status** |
|  1          |  Critical         | init          | DEBUGGING is defined;                        | Modprobe driver;Execute `ls /dev/ | grep "bbbgpio*"` | In console message:"Driver bbbgpio loaded.Build on Mar  1 2015 21:22:38" should be displayed. Under /dev/ bbbgpio0 should be created | PASS                |
|  1          |  Critical         | exit          | DEBUGGING is defined;                        | Rmmod driver;             | In console message:"Driver bbbgpio unloaded.Build on Mar  1 2015 21:22:38" should be displayed. | PASS |
|  1          |  Critical         | open          | DEBUGGING is defined;                        | open file descriptor      | No errors when file descriptor is opened | PASS                |
|  1          |  Critical         | release       | DEBUGGING is defined;                        | close file descriptor     | No errors when file descriptor is closed | PASS                |
|  1          |  Critical         | ioctl         | DEBUGGING is defined;                        | Perform a write with IOCTL | No errors shall be displayed            | PASS                |
|  1          |  Critical         | read          | DEBUGGING is defined;                        | Read using ioctl & sysfs. | No errors or kernel panic shall appear   | PASS                |
|  1;4        |  Critical         | write         | DEBUGGING is defined;                        | Write a value to pin using write sysfs | No errors shall be displayed | PASS                 |
|  1          |  Critical         | irq_probe     | DEBUGGING is defined;                        | Enable IRQ                | Probe is successfully         | PASS            |
|  1          |  Critical         | irq_disable   | DEBUGGING is defined;                        | Use ioctl IOCBBBGPIOSBW   | In console, message:"Disable IRQ" shall be displayed | PASS    |
|             |                   |               |                                              |                           |                               |                 |



---

##Notes

**Test ID** is id of specification that will be tested or the issue's id in which a new feature was added to software.

**Test Priority** will have one of the following values:
* *Critical*
* *Recommended*
* *Important*

**Test Name** is the name of the feature that is tested or name of the issue in which a new feature was added.

**Test Status** will have one of the following values:
* *PASS*
* *FAIL*
* *NOT TESTED*  
