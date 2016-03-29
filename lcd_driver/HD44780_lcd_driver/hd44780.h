#ifndef HD44780_H
#define HD44780_H
/*Exported types*/
struct SLcdPins
{
	u8 en;
	u8 rs;
	u8 port[8];
};


typedef enum 
{
	Mode_4_bits=0x00,
	Mode_8_bits
}ELcdMode;

#define CLEAR_DISPLAY (1<<0)
#define RETURN_HOME (1<<1)
#define ENTRY_MODE(cursor_increase,scroll_display) ((1<<2)|((cursor_increase)<<1)|((scroll_display)<<0))
#define DISPLAY_CONTROL(display,cursor,blink) ((1<<3)|((display)<<2)|((cursor)<<1)|((blink)<<0))
#define CURSOR_SHIFT(shift_move,right_left) ((1<<4)|((shift_move)<<3)|((right_left)<<2))
#define FUNCTION_SET(data_length,lines) ((1<<5)|((data_length)<<4)|((lines)<<3))
#define BUSY_FLAG ((((0x01) & (1<<7))==0))
#endif
