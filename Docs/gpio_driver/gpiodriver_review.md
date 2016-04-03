                    Template version 0.1 released in "30-01-2016" author "23ars" 

# BBB Gpio Driver Code Review Document


| bbbgpio Driver Code Review |                |               |                     |
|----------------------------|----------------|---------------|---------------------|
| **Reviewer:**              | 23ars                                                |
| **Software Version:**      | 0.1                                                     |
| **Issue**                         | **Location**                          | **Severity**                          | **Recommendation**                            | 
| u32 data should be dinamycally allocated  | bbbgpio.c, line 932           | Recommended                           | dynamycally alloc memory                      |
| Debugging macros should be removed | bbbgpio.c, line 35                   | Recommended                           | Replace debug messages with printk where needed |
| Poll is not implemented           | no implemented                        | Required                              | Since interrupts are used, poll function should be implemented |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |
|                                   |                                       |                                       |                                               |


---

##Notes

**Severity** can be:

* Critical
* Required
* Recommended
* Informational
