#ifndef HD44780_H
#define HD44780

typedef enum 
{
	Mode_4_bits=0x00,
	Mode_8_bits
}ELcdMode;

union ULcdPins
{
	struct Mode4BitsPins
	{
		u8 data[4];
		u8 rs;
		u8 en;
	};
	struct  Mode8BitsPins
	{
		u8 data[8];
		u8 rs;
		u8 en;	      
	}
};

extern void hd44780_init_lcd(const union ULcdPins *u_lcd_pins);

#endif
