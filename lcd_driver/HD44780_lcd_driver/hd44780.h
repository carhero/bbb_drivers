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
typedef enum
{
	INCREASE_CURSOR=0x01,
	DECREASE_CURSOR=0x00
}ECursorIncrease;

typedef enum
{
	SCROLL_DISPLAY_OFF=0x00,
	SCROLL_DISPLAY_ON=0x01
}EScrollDisplay;

typedef enum
{
	DISPLAY_OFF=0x00,
	DISPLAY_ON=0x01
}EDisplayControl;
typedef enum
{
	CURSOR_OFF=0x00,
	CURSOR_ON=0x01
}ECursorControl;
typedef enum
{
	CURSOR_BLINK_OFF=0x00,
	CURSOR_BLINK_ON=0x01
}ECursorBlink;
typedef enum
{
	CURSOR_MOVE=0x00,
	SHIFT_DISPLAY=0x01
}EShiftCommand;
typedef enum
{
	LEFT_SHIFT=0x00,
	RIGHT_SHIFT=0x01
}EShiftDirection;


typedef enum
{
	DISPLAY_LINE_1=0x00,
	DISPLAY_LINE_2=0x01
}ENumberOfDisplayLines;

#define CLEAR_DISPLAY (1<<0)
#define RETURN_HOME (1<<1)
#define ENTRY_MODE(cursor_increase,scroll_display) ((1<<2)|((cursor_increase)<<1)|((scroll_display)<<0))
#define DISPLAY_CONTROL(display,cursor,blink) ((1<<3)|((display)<<2)|((cursor)<<1)|((blink)<<0))
#define CURSOR_SHIFT(shift_move,right_left) ((1<<4)|((shift_move)<<3)|((right_left)<<2))
#define FUNCTION_SET(data_length,lines) ((1<<5)|((data_length)<<4)|((lines)<<3))
#define BUSY_FLAG ((((0x01) & (1<<7))==0))
#endif
