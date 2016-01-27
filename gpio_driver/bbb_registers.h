#ifndef BBBREGISTERS_H_
#define BBBREGISTERS_H_

#define GPIO0                 0x44E07000
#define GPIO1                 0x4804C000
#define GPIO2                 0x481AC000
#define GPIO3                 0x481AE000



#define GPIO_REVISION         0x000       /*R;31-31=scheme;27-16=func;15-11=rtl;10-8=major;7-6=custom;5-0=minor*/
#define GPIO_SYSCONFIG        0x010 
#define GPIO_EOI              0x020
#define GPIO_IRQSTATUS_RAW_0  0x024       /*0=no effect;1=IRQ is triggered*/
#define GPIO_IRQSTATUS_RAW_1  0x028       /*same as GPIO_IRQSTATUS_RAW_0*/
#define GPIO_IRQSTATUS_0      0x02C       /*0hW=no effect;0hR=IRQ is not triggered;1hW clears the IRQ;1hR=IRQ is triggered*/
#define GPIO_IRQSTATUS_1      0x030       /*0hW=no effect;0hR=IRQ is not triggered;1hW clears the IRQ;1hR=IRQ is triggered*/
#define GPIO_IRQSTATUS_SET_0  0x034       /*0h=no effect;1h=enable irq*/
#define GPIO_IRQSTATUS_SET_1  0x038       /*same*/
#define GPIO_IRQSTATUS_CLR_0  0x03C       /*0h=no effect;1h=disable irq*/
#define GPIO_IRQSTATUS_CLR_1  0x040       /*0h=no effect;1h=disable irq*/
#define GPIO_IRQWAKEN_0       0x044       /*0h=disable wakeup;1h=enable wakeup*/
#define GPIO_IRQWAKEN_1       0x048       /*0h=disable wakeup;1h=enable wakeup*/
#define GPIO_SYSSTATUS        0x114       /*R;0h=internal reset is ongoing;1h=reset completed*/
#define GPIO_CTRL             0x130       
#define GPIO_OE               0x134       /*R/W;0h=>Port is output;1h=>Port is input*/
#define GPIO_DATAIN           0x138       /*R;0h to reset*/
#define GPIO_DATAOUT          0x13C       /*R/W; data set on output pins*/
#define GPIO_LEVELDETECT0     0x140       /*R/W;low level detect;0h disable IRQ;1h enable IRQ*/
#define GPIO_LEVELDETECT1     0x144       /*R/W;high level detect;0h disable IRQ;1h enable IRQ*/
#define GPIO_RISINGDETECT     0x148       /*R/W;rising edge detect;0h disable IRQ;1h enable IRQ*/
#define GPIO_FALLINGDETECT    0x14C       /*R/W;falling edge; same as above*/
#define GPIO_DEBOUNCEENABLE   0x150       /*0h debounce disable;1h debounce enable*/
#define GPIO_DEBOUNCINGTIME   0x154       /*7-0 debounce time;Debounce Val=(deb time+1)*31 us*/
#define GPIO_CLEARDATAOUT     0x190       /*R/W;0h no effect; 1h clear the bit*/
#define GPIO_SETDATAOUT       0x194       /*R/W;0h no effect; 1h set the bit*/



static inline u32
gpioreg_map(u8 gpio_group)
{
	switch(gpio_group){
	case '0':return GPIO0;
	case '1':return GPIO1;
	case '2':return GPIO2;
	case '3':return GPIO3;
	default: return GPIO0;/*added GPIO0 for out of range address. to prevent a segfault or kernel dump, altough gpio_group will never be out of range*/
	}
}





#endif
