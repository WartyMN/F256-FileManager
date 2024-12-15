/*
 * f256jr.h
 *
 *  Created on: November 29, 2022
 *      Author: micahbly
 */

#ifndef F256JR_H_
#define F256JR_H_



/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

#include "api.h"
#include <stdint.h>


/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/


// adapted from vinz67
#define R8(x)						*((volatile uint8_t* const)(x))			// make sure we read an 8 bit byte; for VICKY registers, etc.
#define P8(x)						(volatile uint8_t* const)(x)			// make sure we read an 8 bit byte; for VICKY registers, etc.
#define R16(x)						*((volatile uint16_t* const)(x))		// make sure we read an 16 bit byte; for RNG etc.


// ** F256jr MMU
#define MMU_MEM_CTRL					0x0000	// bit 0-1: activate LUT (exit editing); 4-5 LUT to be edited; 7: activate edit mode
#define MMU_IO_CTRL						0x0001	// bits 0-1: IO page; bit 2: disable IO

#define BANK_IO							0x06	// 0xc000-0xdfff
#define BANK_KERNAL						0x07	// 0xe000-0xffff


// ** F256jr - Tiny VICKY

#define TEXT_COL_COUNT_FOR_PLOTTING		80	// regardless of visible cols (between borders), VRAM is fixed at 80 cols across.
#define TEXT_ROW_COUNT_FOR_PLOTTING		60	// regardless of visible rows (between borders), VRAM is fixed at 60 rows up/down.

#define TEXT_ROW_COUNT_60HZ			60
#define TEXT_ROW_COUNT_70HZ			50

#define TEXT_FONT_WIDTH				8	// for text mode, the width of the fixed-sized font chars
#define TEXT_FONT_HEIGHT			8	// for text mode, the height of the fixed-sized font chars.
#define TEXT_FONT_BYTE_SIZE			(8*256)

#define VIDEO_MODE_FREQ_BIT			0x01	//!> the bits in the 2nd byte of the system control register that define video mode (resolution). if this bit is set, resolution is 70hz 320x200 (text mode 80*25). if clar, is 60hz 630*240
#define VIDEO_MODE_DOUBLE_X_BIT		0x02	//!> the bits in the 2nd byte of the system control register control text mode doubling in horizontal. if set, text mode chars are doubled in size, producing 40 chars across
#define VIDEO_MODE_DOUBLE_Y_BIT		0x04	//!> the bits in the 3rd byte of the system control register control text mode doubling in vertical. if set, text mode chars are doubled in size, producing 25 or 30 chars down

#define GAMMA_MODE_DIPSWITCH_BIT	0x20	//!>the bits in the 2nd byte of the system control register reflect dip switch setting for control gamma correction on/off
#define GAMMA_MODE_ONOFF_BITS		0b01000000	//!>the bits in the 1st byte of the system control register control gamma correction on/off


// Tiny VICKY I/O pages
#define VICKY_IO_PAGE_REGISTERS			0	// Low level I/O registers
#define VICKY_IO_PAGE_FONT_AND_LUTS		1	// Text display font memory and graphics color LUTs
#define VICKY_IO_PAGE_CHAR_MEM			2	// Text display character matrix
#define VICKY_IO_PAGE_ATTR_MEM			3	// Text display color matrix

// Address	R/W	7 6 5 4 3 2 1 0 
// 0xD000 	R/W X GAMMA SPRITE TILE BITMAP GRAPH OVRLY TEXT 
// 0xD001 	R/W — FON_SET FON_OVLY MON_SLP DBL_Y DBL_X CLK_70

// Address	R/W 7  6 5 4  3  2 1 0 
// 0xD002	R/W —  LAYER1 —  LAYER0 
// 0xD003	R/W —  - - -  -  LAYER2
//Table 4.2: Bitmap and Tile Map Layer Registers

// Tiny VICKY I/O page 0 addresses
#define VICKY_BASE_ADDRESS				0xd000		// Tiny VICKY offset/first register
#define VICKY_MASTER_CTRL_REG_L			0xd000		// Tiny VICKY Master Control Register - low - gamma, sprite, tiles, bitmap, graphics, text modes
#define VICKY_MASTER_CTRL_REG_H			0xd001		// Tiny VICKY Master Control Register - high - font 1/2, screen res, etc.
#define VICKY_LAYER_CTRL_1				0xd002		// Tiny VICKY Bitmap and Tile Map Layer Register 1
#define VICKY_LAYER_CTRL_2				0xd003		// Tiny VICKY Bitmap and Tile Map Layer Register 2
#define VICKY_BORDER_CTRL_REG			0xd004		// Tiny VICKY Border Control Register
#define VICKY_BORDER_COLOR_B			0xd005		// Tiny VICKY Border Color Blue
#define VICKY_BORDER_COLOR_G			0xd006		// Tiny VICKY Border Color Green
#define VICKY_BORDER_COLOR_R			0xd007		// Tiny VICKY Border Color Red
#define VICKY_BORDER_X_SIZE				0xd008		// Tiny VICKY Border X size in pixels
#define VICKY_BORDER_Y_SIZE				0xd009		// Tiny VICKY Border Y size in pixels
#define VICKY_BACKGROUND_COLOR_B		0xd00d		// Tiny VICKY background color Blue
#define VICKY_BACKGROUND_COLOR_G		0xd00e		// Tiny VICKY background color Green
#define VICKY_BACKGROUND_COLOR_R		0xd00f		// Tiny VICKY background color Red
#define VICKY_TEXT_CURSOR_ENABLE		0xd010		// Bit 0=enable cursor; 1-2=rate; 3=flash enable; 4-7 unused
	#define CURSOR_ONOFF_BITS				0b00000001		//!> bit 0 controls whether cursor is displayed or not
	#define CURSOR_FLASH_RATE_BITS			0b00000110		//!> bits 1-2 control rate of cursor flashing (if visible)
	#define CURSOR_FLASH_RATE_1S			0b00000000		//!> bits 1&2 off = 1 blink per second
	#define CURSOR_FLASH_RATE_12S			0b00000010		//!> bits 1 on = 1 blink per 1/2 second
	#define CURSOR_FLASH_RATE_14S			0b00000100		//!> bits 2 on = 1 blink per 1/4 second
	#define CURSOR_FLASH_RATE_15S			0b00000110		//!> bits 1&2 on = 1 blink per 1/5 second
	#define CURSOR_FLASH_ON_BITS			0b00001000		//!> bit 3 controls whether cursor flashes (1) or remains solid (0)
#define VICKY_TEXT_CURSOR_CHAR			0xd012		// 1-byte
#define VICKY_TEXT_X_POS				0xd014		// 2-byte
#define VICKY_TEXT_Y_POS				0xd016		// 2-byte

#define VICKY_LINE_INT_CTRL				0xd018		// Vicky's line interrupt. 1 to enable, 0 to disable. other bit positions not used.
#define VICKY_LINE_INT_LINE_L			0xd019
#define VICKY_LINE_INT_LINE_H			0xd01a
#define VICKY_LINE_INT_RESERVED			0xd01b
#define VICKY_LINE_INT_RAST_COL_L		0xd01c
#define VICKY_LINE_INT_RAST_COL_H		0xd01d
#define VICKY_LINE_INT_RAST_ROW_L		0xd01e
#define VICKY_LINE_INT_RAST_ROW_H		0xd01f

#define BITMAP_L0_CTRL					0xd100		//!> bitmap control register for first bitmap
#define BITMAP_L0_VRAM_ADDR_L			0xd101		//!> bitmap VRAM address pointer)		
#define BITMAP_L0_VRAM_ADDR_M			0xd102		//!> bitmap VRAM address pointer)		
#define BITMAP_L0_VRAM_ADDR_H			0xd103		//!> bitmap VRAM address pointer)
#define BITMAP_L1_CTRL					0xd108		//!> bitmap control register for 2nd bitmap
#define BITMAP_L1_VRAM_ADDR_L			0xd109		//!> bitmap VRAM address pointer)		
#define BITMAP_L1_VRAM_ADDR_M			0xd10a		//!> bitmap VRAM address pointer)		
#define BITMAP_L1_VRAM_ADDR_H			0xd10b		//!> bitmap VRAM address pointer)
#define BITMAP_L2_CTRL					0xd110		//!> bitmap control register for 3rd bitmap
#define BITMAP_L2_VRAM_ADDR_L			0xd111		//!> bitmap VRAM address pointer)		
#define BITMAP_L2_VRAM_ADDR_M			0xd112		//!> bitmap VRAM address pointer)		
#define BITMAP_L2_VRAM_ADDR_H			0xd113		//!> bitmap VRAM address pointer)



// ** tiles and tile maps -- see also VICKY_LAYER_CTRL_1 and VICKY_LAYER_CTRL_2

#define TILE0_CTRL						0xd200		//!> tile control register for tilemap #1
#define TILE1_CTRL						0xd20c		//!> tile control register for tilemap #2
#define TILE2_CTRL						0xd218		//!> tile control register for tilemap #3

#define TILE_REG_LEN					0x0c		// number of bytes between start of one tilemap register and the next
#define TILE_CTRL_OFFSET_ADDR_LO		0x01		// offset from the tilemap control to the low address
#define TILE_CTRL_OFFSET_ADDR_MED		0x02		// offset from the tilemap control to the medium address
#define TILE_CTRL_OFFSET_ADDR_HI		0x03		// offset from the tilemap control to the high address
#define TILE_CTRL_OFFSET_MAP_SIZE_X		0x04		// offset from the tilemap control to num columns in map
#define TILE_CTRL_OFFSET_MAP_SIZE_Y		0x06		// offset from the tilemap control to num rows in map
#define TILE_CTRL_OFFSET_SCROLL_X_LO	0x08		// offset from the tilemap control horizontal scrollowing (lo)
#define TILE_CTRL_OFFSET_SCROLL_X_HI	0x09		// offset from the tilemap control horizontal scrollowing (hi)
#define TILE_CTRL_OFFSET_SCROLL_Y_LO	0x0a		// offset from the tilemap control vertical scrollowing (lo)
#define TILE_CTRL_OFFSET_SCROLL_Y_HI	0x0b		// offset from the tilemap control vertical scrollowing (hi)

#define TILESET0_ADDR_LO				0xd280		// 20-bit address of tileset, lo
#define TILESET0_ADDR_MED				0xd281		// 20-bit address of tileset, medium
#define TILESET0_ADDR_HI				0xd282		// 20-bit address of tileset, hi
#define TILESET0_SHAPE					0xd283		// 0 if tiles are arranged in 1 vertical column, or 8 if in a square
#define TILESET_REG_LEN					0x04		// number of bytes between start of one tileset register and the next


// ** serial comms related

#define UART_BASE						0xd630		// starting point of serial-related registers
#define UART_RBR						(UART_BASE + 0)
#define UART_IER						(UART_BASE + 1)
	// flags for UART interrupt register
	#define FLAG_UART_IER_RXA			0b00000001		// data receive will trigger interrupt if set
	#define FLAG_UART_IER_TXA			0b00000010		// data send will trigger interrupt if set
	#define FLAG_UART_IER_ERR			0b00000100		// data error will trigger interrupt if set
	#define FLAG_UART_IER_STAT			0b00001000		// RS-232 line state change will trigger interrupt if set
	
#define UART_IIR						(UART_BASE + 2)
#define UART_LCR						(UART_BASE + 3)
	#define FLAG_UART_LSR_DR			0b00000001		// data is ready for reading
	#define FLAG_UART_LSR_OE			0b00000010		// overrun error
	#define FLAG_UART_LSR_PE			0b00000100		// parity error: the parity doesn't coincide with the parameters set when the byte is received.
	#define FLAG_UART_LSR_FE			0b00001000		// framing error: This error occurs if the last bit is not a stop bit. This generally happens because of synchronization error. You may face this error when connecting two computers with the help of null modem, if the baud rates of the transmitting computer differs from the baud rates of the receiving computer.
	#define FLAG_UART_LSR_BI			0b00010000		// break interrupt. It happens when the received data line is held in a logic state '0' (Space) for more than it takes to send a full word. That includes the time for the start bit, data bits, parity bits and stop bits.
	#define FLAG_UART_LSR_THRE			0b00100000		// shows that transmitter holding register is empty
	#define FLAG_UART_LSR_TEMT			0b01000000		// transmitter holding register and the shift register are empty
	#define FLAG_UART_LSR_ERR			0b10000000		// error occurred in Received FIFO when receiving data. This bit has high level if one of the following errors occurred when receiving the bytes contained in the FIFO buffers: break, parity or framing error.
	
#define UART_MCR						(UART_BASE + 4)	// Modem Control Register. "Before setting up any interrupts, you must set bit 3 of the MCR (UART register 4) to 1. This toggles the GPO2, which puts the UART out of tri-state, and allows it to service interrupts." -- https://www.activexperts.com/serial-port-component/tutorials/uart/
	// flags for UART interrupt register
	#define FLAG_UART_MCR_DTR			0b00000001		// Is reflected on RS-232 DTR (Data Terminal Ready) line.
	#define FLAG_UART_MCR_RTS			0b00000010		// Reflected on RS-232 RTS (Request to Send) line.
	#define FLAG_UART_MCR_OUT1			0b00000100		// GPO1 (General Purpose Output 1).
	#define FLAG_UART_MCR_OUT2			0b00001000		// GPO2 (General Purpose Output 2). Enables interrupts to be sent from the UART to the PIC.
	#define FLAG_UART_MCR_LOOP			0b00010000		// Echo (loop back) test.  All characters sent will be echoed if set.	
#define UART_LSR						(UART_BASE + 5)
#define UART_MSR						(UART_BASE + 6)
#define UART_SCR						(UART_BASE + 7)

#define UART_THR						(UART_BASE + 0)	// write register when DLAB=0
#define UART_FCR						(UART_BASE + 2)	// write register when DLAB=0
#define UART_DLL						(UART_BASE + 0)	// read/write register when DLAB=1
#define UART_DLM						(UART_BASE + 1)	// read/write register when DLAB=1

#define UART_BAUD_DIV_300		5244	// divisor for 300 baud
#define UART_BAUD_DIV_600		2622	// divisor for 600 baud
#define UART_BAUD_DIV_1200		1311	// divisor for 1200 baud
#define UART_BAUD_DIV_1800		874		// divisor for 1800 baud
#define UART_BAUD_DIV_2000		786		// divisor for 2000 baud
#define UART_BAUD_DIV_2400		655		// divisor for 2400 baud
#define UART_BAUD_DIV_3600		437		// divisor for 3600 baud
#define UART_BAUD_DIV_4800		327		// divisor for 4800 baud
#define UART_BAUD_DIV_9600		163		// divisor for 9600 baud
#define UART_BAUD_DIV_19200		81		// divisor for 19200 baud
#define UART_BAUD_DIV_38400		40		// divisor for 38400 baud
#define UART_BAUD_DIV_57600		27		// divisor for 57600 baud
#define UART_BAUD_DIV_115200	13		// divisor for 115200 baud

#define UART_DATA_BITS			0b00000011	// 8 bits
#define UART_STOP_BITS			0			// 1 stop bit
#define UART_PARITY				0			// no parity
#define UART_BRK_SIG			0b01000000
#define UART_NO_BRK_SIG			0b00000000
#define UART_DLAB_MASK			0b10000000
#define UART_THR_IS_EMPTY		0b00100000
#define UART_THR_EMPTY_IDLE		0b01000000
#define UART_DATA_AVAILABLE		0b00000001
#define UART_ERROR_MASK			0b10011110



#define VICKY_PS2_INTERFACE_BASE		0xd640
#define VICKY_PS2_CTRL					(VICKY_PS2_INTERFACE_BASE)
#define VICKY_PS2_OUT					(VICKY_PS2_CTRL + 1)
#define VICKY_PS2_KDB_IN				(VICKY_PS2_OUT + 1)
#define VICKY_PS2_MOUSE_IN				(VICKY_PS2_KDB_IN + 1)
#define VICKY_PS2_STATUS				(VICKY_PS2_MOUSE_IN + 1)

#define VICKY_PS2_CTRL_FLAG_K_WR		0b00000010		// set to 1 then 0 to send a byte written on PS2_OUT to the keyboard
#define VICKY_PS2_CTRL_FLAG_M_WR		0b00001000		// set to 1 then 0 to send a byte written on PS2_OUT to the mouse
#define VICKY_PS2_CTRL_FLAG_KCLR		0b00010000		// set to 1 then 0 to clear the keyboard input FIFO queue
#define VICKY_PS2_CTRL_FLAG_MCLR		0b00100000		// set to 1 then 0 to clear the mouse input FIFO queue
#define VICKY_PS2_STATUS_FLAG_KEMP		0b00000001		// when 1, the keyboard input FIFO is empty
#define VICKY_PS2_STATUS_FLAG_MEMP		0b00000010		// when 1, the mouse input FIFO is empty
#define VICKY_PS2_STATUS_FLAG_M_NK		0b00010000		// when 1, the code sent to the mouse has resulted in an error
#define VICKY_PS2_STATUS_FLAG_M_AK		0b00100000		// when 1, the code sent to the mouse has been acknowledged
#define VICKY_PS2_STATUS_FLAG_K_NK		0b01000000		// when 1, the code sent to the keyboard has resulted in an error
#define VICKY_PS2_STATUS_FLAG_K_AK		0b10000000		// when 1, the code sent to the keyboard has been acknowledged

#define RTC_SECONDS						0xd690		//  654: second digit, 3210: 1st digit
#define RTC_SECONDS_ALARM				0xd691		//  654: second digit, 3210: 1st digit
#define RTC_MINUTES						0xd692		//  654: second digit, 3210: 1st digit
#define RTC_MINUTES_ALARM				0xd693		//  654: second digit, 3210: 1st digit
#define RTC_HOURS						0xd694		//   54: second digit, 3210: 1st digit
#define RTC_HOURS_ALARM					0xd695		//   54: second digit, 3210: 1st digit
#define RTC_DAY							0xd696		//   54: second digit, 3210: 1st digit
#define RTC_DAY_ALARM					0xd697		//   54: second digit, 3210: 1st digit
#define RTC_DAY_OF_WEEK					0xd698		//  210: day of week digit
#define RTC_MONTH						0xd699		//    4: second digit, 3210: 1st digit
#define RTC_YEAR						0xd69a		// 7654: second digit, 3210: 1st digit
#define RTC_RATES						0xd69b		//  654: WD (watchdog, not really relevant to F256); 3210: RS
	#define FLAG_RTC_RATE_NONE			0b00000000		// applies to bits 3210 of RTC_RATES
	#define FLAG_RTC_RATE_31NS			0b00000001		// applies to bits 3210 of RTC_RATES. See manual for values between 0001 and 1101
	#define FLAG_RTC_RATE_125MS			0b00001101		// applies to bits 3210 of RTC_RATES
	#define FLAG_RTC_RATE_63MS			0b00001100		// applies to bits 3210 of RTC_RATES - 62.5ms
	#define FLAG_RTC_RATE_250MS			0b00001110		// applies to bits 3210 of RTC_RATES
	#define FLAG_RTC_RATE_500MS			0b00001111		// applies to bits 3210 of RTC_RATES 
#define RTC_ENABLES						0xd69c		// Controls various interrupt enables, only some of which apply to an F256
	#define FLAG_RTC_PERIODIC_INT_EN	0b00000100		// set PIE (bit 2) to raise interrupt based on RTC_RATES
	#define FLAG_RTC_ALARM_INT_EN		0b00001000		// Set AEI (bit 3) to raise interrupt based on RTC_SECONDS_ALARM, etc. 
#define RTC_FLAGS						0xd69d		// check to see why an RTC interrupt was raised
	#define FLAG_RTC_PERIODIC_INT		0b00000100		// will be set if interrupt was raised based on RTC_RATES
	#define FLAG_RTC_ALARM_INT			0b00001000		// will be set if interrupt was raised based on alarm clock
#define RTC_CONTROL						0xd69e		// set UTI (bit 3) to disable update of reg, to read secs. 
	#define MASK_RTC_CTRL_DSE			0b00000001		// if set (1), daylight savings is in effect.
	#define MASK_RTC_CTRL_12_24			0b00000010		// sets whether the RTC is using 12 or 24 hour accounting (1 = 24 Hr, 0 = 12 Hr)
	#define MASK_RTC_CTRL_STOP			0b00000100		// If it is clear (0) before the system is powered down, it will avoid draining the battery and may stop tracking the time. If it is set (1), it will keep using the battery as long as possible.
	#define MASK_RTC_CTRL_UTI			0b00001000		// if set (1), the update of the externally facing registers by the internal timers is inhibited. In order to read or write those registers, the program must first set UTI and then clear it when done.
	#define MASK_RTC_CTRL_UNUSED		0b11110000		// the upper 4 bits are not used.
#define RTC_CENTURY						0xd69f		// 7654: century 10s digit, 3210: centurys 1s digit

#define RANDOM_NUM_GEN_LOW				0xd6a4		// both the SEEDL and the RNDL (depends on bit 1 of RND_CTRL)
#define RANDOM_NUM_GEN_HI				0xd6a5		// both the SEEDH and the RNDH (depends on bit 1 of RND_CTRL)
#define RANDOM_NUM_GEN_ENABLE			0xd6a6		// bit 0: enable/disable. bit 1: seed mode on/off. "RND_CTRL"

#define MACHINE_ID_REGISTER				0xd6a7		// will be '2' for F256JR
#define MACHINE_PCB_ID_0				0xd6a8
#define MACHINE_PCB_ID_1				0xd6a9
#define MACHINE_PCB_MAJOR				0xd6eb		// error in manual? this and next 4 all show same addr. changing here to go up by 1.
#define MACHINE_PCB_MINOR				0xd6ec
#define MACHINE_PCB_DAY					0xd6ed
#define MACHINE_PCB_MONTH				0xd6ef
#define MACHINE_PCB_YEAR				0xd6f0
#define MACHINE_FPGA_SUBV_LOW			0xd6aa		// CHSV0 chip subversion in BCD (low)
#define MACHINE_FPGA_SUBV_HI			0xd6ab		// CHSV1 chip subversion in BCD (high)
#define MACHINE_FPGA_VER_LOW			0xd6ac		// CHV0 chip version in BCD (low)
#define MACHINE_FPGA_VER_HI				0xd6ad		// CHV1 chip version in BCD (high)
#define MACHINE_FPGA_NUM_LOW			0xd6ae		// CHN0 chip number in BCD (low)
#define MACHINE_FPGA_NUM_HI				0xd6af		// CHN1 chip number in BCD (high)

#define TEXT_FORE_LUT					0xd800		// FG_CHAR_LUT_PTR	Text Foreground Look-Up Table
#define TEXT_BACK_LUT					0xd840		// BG_CHAR_LUT_PTR	Text Background Look-Up Table

#define SPRITE0_CTRL					0xd900		// Sprint #0 control register
#define SPRITE0_ADDR_LO					0xd901		// Sprite #0 pixel data address register
#define SPRITE0_ADDR_MED				0xd902		// Sprite #0 pixel data address register
#define SPRITE0_ADDR_HI					0xd903		// Sprite #0 pixel data address register
#define SPRITE0_X_LO					0xd904		// Sprite #0 X position
#define SPRITE0_X_HI					0xd905		// Sprite #0 X position
#define SPRITE0_Y_LO					0xd906		// Sprite #0 Y position
#define SPRITE0_Y_HI					0xd907		// Sprite #0 Y position
#define SPRITE_REG_LEN					0x08		// number of bytes between start of one sprite register set and the next

#define VIA1_CTRL						0xdb00		// 6522 VIA #2 Control registers (K only)
#define VIA1_PORT_B_DATA				0xdb00		// VIA #2 Port B data
#define VIA1_PORT_A_DATA				0xdb01		// VIA #2 Port A data
#define VIA1_PORT_B_DIRECTION			0xdb02		// VIA #2 Port B Data Direction register
#define VIA1_PORT_A_DIRECTION			0xdb03		// VIA #2 Port A Data Direction register
#define VIA1_TIMER1_CNT_L				0xdb04		// VIA #2 Timer 1 Counter Low
#define VIA1_TIMER1_CNT_H				0xdb05		// VIA #2 Timer 1 Counter High
#define VIA1_TIMER1_LATCH_L				0xdb06		// VIA #2 Timer 1 Latch Low
#define VIA1_TIMER1_LATCH_H				0xdb07		// VIA #2 Timer 1 Latch High
#define VIA1_TIMER2_CNT_L				0xdb08		// VIA #2 Timer 2 Counter Low
#define VIA1_TIMER2_CNT_H				0xdb09		// VIA #2 Timer 2 Counter High
#define VIA1_SERIAL_DATA				0xdb0a		// VIA #2 Serial Data Register
#define VIA1_AUX_CONTROL				0xdb0b		// VIA #2 Auxiliary Control Register
#define VIA1_PERI_CONTROL				0xdb0c		// VIA #2 Peripheral Control Register
#define VIA1_INT_FLAG					0xdb0d		// VIA #2 Interrupt Flag Register
#define VIA1_INT_ENABLE					0xdb0e		// VIA #2 Interrupt Enable Register
#define VIA1_PORT_A_DATA_NO_HAND		0xdb0f		// VIA #2 Port A data (no handshake)

#define VIA0_CTRL						0xdc00		// 6522 VIA #1 Control registers
#define VIA0_PORT_B_DATA				0xdc00		// VIA #1 Port B data
#define VIA0_PORT_A_DATA				0xdc01		// VIA #1 Port A data
#define VIA0_PORT_B_DIRECTION			0xdc02		// VIA #1 Port B Data Direction register
#define VIA0_PORT_A_DIRECTION			0xdc03		// VIA #1 Port A Data Direction register
#define VIA0_TIMER1_CNT_L				0xdc04		// VIA #1 Timer 1 Counter Low
#define VIA0_TIMER1_CNT_H				0xdc05		// VIA #1 Timer 1 Counter High
#define VIA0_TIMER1_LATCH_L				0xdc06		// VIA #1 Timer 1 Latch Low
#define VIA0_TIMER1_LATCH_H				0xdc07		// VIA #1 Timer 1 Latch High
#define VIA0_TIMER2_CNT_L				0xdc08		// VIA #1 Timer 2 Counter Low
#define VIA0_TIMER2_CNT_H				0xdc09		// VIA #1 Timer 2 Counter High
#define VIA0_SERIAL_DATA				0xdc0a		// VIA #1 Serial Data Register
#define VIA0_AUX_CONTROL				0xdc0b		// VIA #1 Auxiliary Control Register
#define VIA0_PERI_CONTROL				0xdc0c		// VIA #1 Peripheral Control Register
#define VIA0_INT_FLAG					0xdc0d		// VIA #1 Interrupt Flag Register
#define VIA0_INT_ENABLE					0xdc0e		// VIA #1 Interrupt Enable Register
#define VIA0_PORT_A_DATA_NO_HAND		0xdc0f		// VIA #1 Port A data (no handshake)

#define DMA_CTRL						0xdf00		// VICKY's DMA control register
	#define FLAG_DMA_CTRL_ENABLE			0b00000001		// enable / disable DMA
	#define FLAG_DMA_CTRL_2D_OP				0b00000010		// set to enable a 2D operation, clear to do a linear operations
	#define FLAG_DMA_CTRL_FILL				0b00000100		// set to enable fill operation, clear to enable copy operation
	#define FLAG_DMA_CTRL_INTERUPT_EN		0b00001000		// enables triggering an interrupt when DMA is complete
	#define FLAG_DMA_CTRL_START				0b10000000		// set to trigger the DMA operation
#define DMA_STATUS						0xdf01		// DMA status register (Read Only)
	#define FLAG_DMA_STATUS_BUSY			0b10000000		// status bit set when DMA is busy copying data
#define DMA_FILL_VALUE					0xdf01		// 8-bit value to fill with (COPY only)
#define DMA_SRC_ADDR					0xdf04		// Source address (system bus - 3 byte)
#define DMA_SRC_ADDR_L					0xdf04		// Source address (system bus - 3 byte) - LOW
#define DMA_SRC_ADDR_M					0xdf05		// Source address (system bus - 3 byte) - MEDIUM
#define DMA_SRC_ADDR_H					0xdf06		// Source address (system bus - 3 byte) - HIGH
#define DMA_DST_ADDR					0xdf08		// Destination address (system bus - 3 byte)
#define DMA_DST_ADDR_L					0xdf08		// Destination address (system bus - 3 byte) - LOW
#define DMA_DST_ADDR_M					0xdf09		// Destination address (system bus - 3 byte) - MEDIUM
#define DMA_DST_ADDR_H					0xdf0a		// Destination address (system bus - 3 byte) - HIGH
#define DMA_COUNT						0xdf0c		// Number of bytes to fill or copy - must be EVEN - the number of bytes to copy (only available when 2D is clear) - 19 bit
#define DMA_COUNT_L						0xdf0c		// Number of bytes to fill or copy - must be EVEN - the number of bytes to copy (only available when 2D is clear) - 19 bit
#define DMA_COUNT_M						0xdf0d		// Number of bytes to fill or copy - must be EVEN - the number of bytes to copy (only available when 2D is clear) - 19 bit
#define DMA_COUNT_H						0xdf0e		// Number of bytes to fill or copy - must be EVEN - the number of bytes to copy (only available when 2D is clear) - 19 bit
#define DMA_WIDTH						0xdf0c		// Width of 2D operation - 16 bits - only available when 2D is set
#define DMA_WIDTH_L						0xdf0c		// Width of 2D operation - 16 bits - only available when 2D is set
#define DMA_WIDTH_M						0xdf0d		// Width of 2D operation - 16 bits - only available when 2D is set
#define DMA_HEIGHT						0xdf0e		// Height of 2D operation - 16 bits - only available when 2D is set
#define DMA_HEIGHT_L					0xdf0e		// Height of 2D operation - 16 bits - only available when 2D is set
#define DMA_HEIGHT_M					0xdf0f		// Height of 2D operation - 16 bits - only available when 2D is set
#define DMA_SRC_STRIDE					0xdf10		// Source stride for 2D operation - 16 bits - only available when 2D COPY is set
#define DMA_SRC_STRIDE_L				0xdf10		// Source stride for 2D operation - 16 bits - only available when 2D COPY is set
#define DMA_SRC_STRIDE_M				0xdf11		// Source stride for 2D operation - 16 bits - only available when 2D COPY is set
#define DMA_DST_STRIDE					0xdf12		// Destination stride for 2D operation - 16 bits - only available when 2D is set
#define DMA_DST_STRIDE_L				0xdf12		// Destination stride for 2D operation - 16 bits - only available when 2D is set
#define DMA_DST_STRIDE_M				0xdf13		// Destination stride for 2D operation - 16 bits - only available when 2D is set

// Tiny VICKY I/O page 1 addresses
#define FONT_MEMORY_BANK0				0xc000		// FONT_MEMORY_BANK0	FONT Character Graphic Mem (primary)
#define FONT_MEMORY_BANK1				0xc800		// FONT_MEMORY_BANK1	FONT Character Graphic Mem (secondary)
#define VICKY_CLUT0						0xd000		// each addition LUT is 400 offset from here
#define VICKY_CLUT1						(VICKY_CLUT0 + 0x400)	// each addition LUT is 400 offset from here
#define VICKY_CLUT2						(VICKY_CLUT1 + 0x400)	// each addition LUT is 400 offset from here
#define VICKY_CLUT3						(VICKY_CLUT2 + 0x400)	// each addition LUT is 400 offset from here

// Tiny VICKY I/O page 2 addresses
#define VICKY_TEXT_CHAR_RAM					(char*)0xc000			// in I/O page 2

// Tiny VICKY I/O page 3 addresses
#define VICKY_TEXT_ATTR_RAM					(char*)0xc000			// in I/O page 3

#define GRAPHICS_MODE_TEXT		0x01	// 0b00000001	Enable the Text Mode
#define GRAPHICS_MODE_TEXT_OVER	0x02	// 0b00000010	Enable the Overlay of the text mode on top of Graphic Mode (the Background Color is ignored)
#define GRAPHICS_MODE_GRAPHICS	0x04	// 0b00000100	Enable the Graphic Mode
#define GRAPHICS_MODE_EN_BITMAP	0x08	// 0b00001000	Enable the Bitmap Module In Vicky
#define GRAPHICS_MODE_EN_TILE	0x10	// 0b00010000	Enable the Tile Module in Vicky
#define GRAPHICS_MODE_EN_SPRITE	0x20	// 0b00100000	Enable the Sprite Module in Vicky
#define GRAPHICS_MODE_EN_GAMMA	0x40	// 0b01000000	Enable the GAMMA correction - The Analog and DVI have different color values, the GAMMA is great to correct the difference
#define GRAPHICS_MODE_DIS_VIDEO	0x80	// 0b10000000	This will disable the Scanning of the Video information in the 4Meg of VideoRAM hence giving 100% bandwidth to the CPU


// VICKY RESOLUTION FLAGS
#define VICKY_RES_320X240_FLAGS		0x00	// 0b00000000
#define VICKY_PIX_DOUBLER_FLAGS		0x02	// 0b00000001
#define VICKY_RES_320X200_FLAGS		0x03	// 0b00000011

// 0xD001 VICKY resolution control bits
#define VICKY_RES_CLK_70_FLAG		0x01	// 0b00000001 -- 70Hz = 640x480. 60hz if off = 640x400
#define VICKY_RES_X_DOUBLER_FLAG	0x02	// 0b00000010 -- 640 -> 320 pix if set
#define VICKY_RES_Y_DOUBLER_FLAG	0x04	// 0b00000100 -- 480 or 400 -> 240 or 200 pix if set
#define VICKY_RES_MON_SLP			0x08	// 0b00001000 -- if set, the monitor SYNC signal will be turned off, putting the monitor to sleep
#define VICKY_RES_FON_OVLY			0x10	// 0b00010000 -- fclear(0),onlythetextforegroundcolorwillbedisplayedwhentextoverlaysgraphics(allbackground colors will be completely transparent). If set (1), both foreground and background colors will be displayed, except that background color 0 will be transparent.
#define VICKY_RES_FON_SET			0x20	// 0b00100000 -- if set (1), the text font displayed will be font set 1. If clear (0), the text font displayed will be font set 0.
#define VICKY_RES_UNUSED7			0x40	// 0b01000000
#define VICKY_RES_UNUSED8			0x80	// 0b10000000

#define VICKY_BITMAP_MAX_H_RES		320		// VICKY in F256K and Jr supports a max resolution of 320x240, even if text engine displays at 640x480
#define VICKY_BITMAP_MAX_V_RES		240		// VICKY in F256K and Jr supports a max resolution of 320x240, even if text engine displays at 640x480

#define RES_320X200		0
#define RES_320X240		1
#define RES_640X480		3		// currently F256K2 and 68K machines only
#define RES_800X600		4		// currently not supported on F256 platform; 68K only
#define RES_1024X768	5		// currently not supported on F256 platform; 68K only

// machine model numbers - for decoding s_sys_info.model - value read from MACHINE_ID_REGISTER (see above)
#define MACHINE_C256FMX			0x00	///< for s_sys_info.model
#define MACHINE_C256U			0x01	///< for s_sys_info.model
#define MACHINE_F256JR			0x02	///< for s_sys_info.model
#define MACHINE_F256JRE			0x03	///< for s_sys_info.model
#define MACHINE_GENX			0x04	///< for s_sys_info.model
#define MACHINE_C256_UPLUS		0x05	///< for s_sys_info.model
#define MACHINE_UNDEFINED_1		0x06	///< for s_sys_info.model
#define MACHINE_UNDEFINED_2		0x07	///< for s_sys_info.model
#define MACHINE_A2560X			0x08	///< for s_sys_info.model. (GenX 32Bits Side)
#define MACHINE_A2560U_PLUS		0x09	///< for s_sys_info.model. there is no A2560U only in the field
#define MACHINE_A2560M			0x0a	///< for s_sys_info.model. 
#define MACHINE_A2560K			0x0b	///< for s_sys_info.model. "classic" A2560K
#define MACHINE_A2560K40		0x0c	///< for s_sys_info.model
#define MACHINE_A2560K60		0x0d	///< for s_sys_info.model
#define MACHINE_UNDEFINED_3		0x0e	///< for s_sys_info.model
#define MACHINE_UNDEFINED_4		0x0f	///< for s_sys_info.model
#define MACHINE_F256P			0x10	///< for s_sys_info.model
#define MACHINE_F256K2			0x11	///< for s_sys_info.model
#define MACHINE_F256K			0x12	///< for s_sys_info.model
#define MACHINE_F256KE			0x13	///< for s_sys_info.model
#define MACHINE_F256K2E			0x14	///< for s_sys_info.model

#define MACHINE_MODEL_MASK		0x1F		

typedef uint8_t	ColorIdx;


/*****************************************************************************/
/*                    FOENSCII Character Point Definitions                   */
/*****************************************************************************/

// Standard alpha-numeric characters
#define CH_SPACE			32
#define CH_BANG				33
#define CH_DQUOTE			34
#define CH_HASH				35
#define CH_DOLLAR			36
#define CH_PERCENT			37
#define CH_AMP				38
#define CH_SQUOTE			39
#define CH_LPAREN			40
#define CH_RPAREN			41
#define CH_AST				42
#define CH_PLUS				43
#define CH_COMMA			44
#define CH_MINUS			45
#define CH_PERIOD			46
#define CH_FSLASH			47
#define CH_0				48
#define CH_1				49
#define CH_2				50
#define CH_3				51
#define CH_4				52
#define CH_5				53
#define CH_6				54
#define CH_7				55
#define CH_8				56
#define CH_9				57
#define CH_COLON			58
#define CH_SEMIC			59
#define CH_LESS				60
#define CH_EQUAL			61
#define CH_GREATER			62
#define CH_QUESTION			63
#define CH_AT				64
#define CH_UC_A				65
#define CH_UC_B				(CH_UC_A + 1)
#define CH_UC_C				(CH_UC_B + 1)
#define CH_UC_D				(CH_UC_C + 1)
#define CH_UC_E				(CH_UC_D + 1)
#define CH_UC_F				(CH_UC_E + 1)
#define CH_UC_G				(CH_UC_F + 1)
#define CH_UC_H				(CH_UC_G + 1)
#define CH_UC_I				(CH_UC_H + 1)
#define CH_UC_J				(CH_UC_I + 1)
#define CH_UC_K				(CH_UC_J + 1)
#define CH_UC_L				(CH_UC_K + 1)
#define CH_UC_M				(CH_UC_L + 1)
#define CH_UC_N				(CH_UC_M + 1)
#define CH_UC_O				(CH_UC_N + 1)
#define CH_UC_P				(CH_UC_O + 1)
#define CH_UC_Q				(CH_UC_P + 1)
#define CH_UC_R				(CH_UC_Q + 1)
#define CH_UC_S				(CH_UC_R + 1)
#define CH_UC_T				(CH_UC_S + 1)
#define CH_UC_U				(CH_UC_T + 1)
#define CH_UC_V				(CH_UC_U + 1)
#define CH_UC_W				(CH_UC_V + 1)
#define CH_UC_X				(CH_UC_W + 1)
#define CH_UC_Y				(CH_UC_X + 1)
#define CH_UC_Z				(CH_UC_Y + 1)
#define CH_LBRACKET			91
#define CH_BSLASH			92
#define CH_RBRACKET			93
#define CH_CARET			94
#define CH_UNDER			95
#define CH_LSQUOTE			96
#define CH_LC_A				(CH_UC_A + 32)
#define CH_LC_B				(CH_LC_A + 1)
#define CH_LC_C				(CH_LC_B + 1)
#define CH_LC_D				(CH_LC_C + 1)
#define CH_LC_E				(CH_LC_D + 1)
#define CH_LC_F				(CH_LC_E + 1)
#define CH_LC_G				(CH_LC_F + 1)
#define CH_LC_H				(CH_LC_G + 1)
#define CH_LC_I				(CH_LC_H + 1)
#define CH_LC_J				(CH_LC_I + 1)
#define CH_LC_K				(CH_LC_J + 1)
#define CH_LC_L				(CH_LC_K + 1)
#define CH_LC_M				(CH_LC_L + 1)
#define CH_LC_N				(CH_LC_M + 1)
#define CH_LC_O				(CH_LC_N + 1)
#define CH_LC_P				(CH_LC_O + 1)
#define CH_LC_Q				(CH_LC_P + 1)
#define CH_LC_R				(CH_LC_Q + 1)
#define CH_LC_S				(CH_LC_R + 1)
#define CH_LC_T				(CH_LC_S + 1)
#define CH_LC_U				(CH_LC_T + 1)
#define CH_LC_V				(CH_LC_U + 1)
#define CH_LC_W				(CH_LC_V + 1)
#define CH_LC_X				(CH_LC_W + 1)
#define CH_LC_Y				(CH_LC_X + 1)
#define CH_LC_Z				(CH_LC_Y + 1)
#define CH_LCBRACKET		123
#define CH_PIPE				124
#define CH_RCBRACKET		125
#define CH_TILDE			126


// FOENSCII graphic characters
#define CH_VFILL_UP_1		95	// underscore
#define CH_VFILL_UP_2		1
#define CH_VFILL_UP_3		2
#define CH_VFILL_UP_4		3
#define CH_VFILL_UP_5		4
#define CH_VFILL_UP_6		5
#define CH_VFILL_UP_7		6
#define CH_VFILL_UP_8		7	// solid inverse space
#define CH_VFILL_DN_8		7	// solid inverse space
#define CH_VFILL_DN_7		8
#define CH_VFILL_DN_6		9
#define CH_VFILL_DN_5		10
#define CH_VFILL_DN_4		11
#define CH_VFILL_DN_3		12
#define CH_VFILL_DN_2		13
#define CH_VFILL_DN_1		14

#define CH_VFILLC_UP_1		192	// same as solid fill up/down, but checkered
#define CH_VFILLC_UP_2		193
#define CH_VFILLC_UP_3		194
#define CH_VFILLC_UP_4		195
#define CH_VFILLC_UP_5		196
#define CH_VFILLC_UP_6		197
#define CH_VFILLC_UP_7		198
#define CH_VFILLC_UP_8		199	// full checkered block
#define CH_VFILLC_DN_8		199	// full checkered block
#define CH_VFILLC_DN_7		200
#define CH_VFILLC_DN_6		201
#define CH_VFILLC_DN_5		202
#define CH_VFILLC_DN_4		203
#define CH_VFILLC_DN_3		204
#define CH_VFILLC_DN_2		205
#define CH_VFILLC_DN_1		206

#define CH_HFILL_UP_1		134
#define CH_HFILL_UP_2		135
#define CH_HFILL_UP_3		136
#define CH_HFILL_UP_4		137
#define CH_HFILL_UP_5		138
#define CH_HFILL_UP_6		139
#define CH_HFILL_UP_7		140
#define CH_HFILL_UP_8		7	// solid inverse space
#define CH_HFILL_DN_8		7	// solid inverse space
#define CH_HFILL_DN_7		141
#define CH_HFILL_DN_6		142
#define CH_HFILL_DN_5		143
#define CH_HFILL_DN_4		144
#define CH_HFILL_DN_3		145
#define CH_HFILL_DN_2		146
#define CH_HFILL_DN_1		147

#define CH_HFILLC_UP_1		207
#define CH_HFILLC_UP_2		208
#define CH_HFILLC_UP_3		209
#define CH_HFILLC_UP_4		210
#define CH_HFILLC_UP_5		211
#define CH_HFILLC_UP_6		212
#define CH_HFILLC_UP_7		213
#define CH_HFILLC_UP_8		199	// full checkered block
#define CH_HFILLC_DN_8		199	// full checkered block
#define CH_HFILLC_DN_7		214
#define CH_HFILLC_DN_6		216
#define CH_HFILLC_DN_5		217
#define CH_HFILLC_DN_4		218
#define CH_HFILLC_DN_3		219
#define CH_HFILLC_DN_2		220
#define CH_HFILLC_DN_1		221

#define CH_HDITH_1			15	// horizontal dither patterns...
#define CH_HDITH_2			16
#define CH_HDITH_3			17
#define CH_HDITH_4			18
#define CH_HDITH_5			19
#define CH_HDITH_6			20
#define CH_HDITH_7			21
#define CH_HDITH_8			22
#define CH_HDITH_9			23
#define CH_HDITH_10			24

#define CH_DITH_L1			16	// full-block dither patterns
#define CH_DITH_L2			18
#define CH_DITH_L3			199
#define CH_DITH_L4			21
#define CH_DITH_L5			23

#define CH_VDITH_1			25	// vertical dither patterns...
#define CH_VDITH_2			26
#define CH_VDITH_3			27
#define CH_VDITH_4			28
#define CH_VDITH_5			29

#define CH_DIAG_R1			186	// diagonal patterns and lines...
#define CH_DIAG_R2			30

#define CH_DIAG_R3			184
#define CH_DIAG_R4			230
#define CH_DIAG_R5			234
#define CH_DIAG_R6			238

#define CH_DIAG_R7			229
#define CH_DIAG_R8			233
#define CH_DIAG_R9			237
#define CH_DIAG_R10			241

#define CH_DIAG_L1			187
#define CH_DIAG_L2			31

#define CH_DIAG_L3			228
#define CH_DIAG_L4			232
#define CH_DIAG_L5			236
#define CH_DIAG_L6			240

#define CH_DIAG_L7			185
#define CH_DIAG_L8			231
#define CH_DIAG_L9			235
#define CH_DIAG_L10			239

#define CH_DIAG_X			159

#define CH_HLINE_UP_1		95	// underscore
#define CH_HLINE_UP_2		148
#define CH_HLINE_UP_3		149
#define CH_HLINE_UP_4		150
#define CH_HLINE_UP_5		151
#define CH_HLINE_UP_6		152
#define CH_HLINE_UP_7		153
#define CH_HLINE_UP_8		14
#define CH_HLINE_DN_8		CH_HLINE_UP_8
#define CH_HLINE_DN_7		CH_HLINE_UP_7
#define CH_HLINE_DN_6		CH_HLINE_UP_6
#define CH_HLINE_DN_5		CH_HLINE_UP_5
#define CH_HLINE_DN_4		CH_HLINE_UP_4
#define CH_HLINE_DN_3		CH_HLINE_UP_3
#define CH_HLINE_DN_2		CH_HLINE_UP_2
#define CH_HLINE_DN_1		CH_HLINE_UP_1

#define CH_VLINE_UP_1		134
#define CH_VLINE_UP_2		133
#define CH_VLINE_UP_3		132
#define CH_VLINE_UP_4		131
#define CH_VLINE_UP_5		130
#define CH_VLINE_UP_6		129
#define CH_VLINE_UP_7		128
#define CH_VLINE_UP_8		147
#define CH_VLINE_DN_8		CH_VLINE_UP_8
#define CH_VLINE_DN_7		CH_VLINE_UP_7
#define CH_VLINE_DN_6		CH_VLINE_UP_6
#define CH_VLINE_DN_5		CH_VLINE_UP_5
#define CH_VLINE_DN_4		CH_VLINE_UP_4
#define CH_VLINE_DN_3		CH_VLINE_UP_3
#define CH_VLINE_DN_2		CH_VLINE_UP_2
#define CH_VLINE_DN_1		CH_VLINE_UP_1

#define CH_LINE_WE			150	// box-drawing lines. read clockwise from west. N=up-facing line, E=right-facing line, etc.
#define CH_LINE_NS			130
#define CH_LINE_NES			154
#define CH_LINE_WES			155
#define CH_LINE_WNES		156
#define CH_LINE_WNE			157
#define CH_LINE_WNS			158
#define CH_LINE_ES			160	// square edge corners
#define CH_LINE_WS			161
#define CH_LINE_NE			162
#define CH_LINE_WN			163
#define CH_LINE_RND_ES		188	// rounded edge corners
#define CH_LINE_RND_WS		189
#define CH_LINE_RND_NE		190
#define CH_LINE_RND_WN		191

#define CH_LINE_BLD_WE		173	// thick box-drawing lines. read clockwise from west. N=up-facing line, E=right-facing line, etc.
#define CH_LINE_BLD_NS		174
#define CH_LINE_BLD_NES		164
#define CH_LINE_BLD_WES		165
#define CH_LINE_BLD_WNES	166
#define CH_LINE_BLD_WNE		167
#define CH_LINE_BLD_WNS		168
#define CH_LINE_BLD_ES		169	// square edge corners
#define CH_LINE_BLD_WS		170
#define CH_LINE_BLD_NE		171
#define CH_LINE_BLD_WN		172
#define CH_LINE_BLD_RND_ES	175	// rounded edge corners
#define CH_LINE_BLD_RND_WS	176
#define CH_LINE_BLD_RND_NE	177
#define CH_LINE_BLD_RND_WN	178

#define CH_BLOCK_N			11	// half-width/height block characters. read clockwise from west. N=upper, E=right-side, etc.
#define CH_BLOCK_S			3
#define CH_BLOCK_W			137
#define CH_BLOCK_E			144
#define CH_BLOCK_SE			242
#define CH_BLOCK_SW			243
#define CH_BLOCK_NE			244
#define CH_BLOCK_NW			245
#define CH_BLOCK_NWSE		246
#define CH_BLOCK_SWNE		247

#define CH_MISC_GBP			0
#define CH_MISC_VTILDE		127
#define CH_MISC_COPY		215	// copyright
#define CH_MISC_FOENIX		223
#define CH_MISC_CHECKMARK	222
#define CH_MISC_HEART		252
#define CH_MISC_DIA			253
#define CH_MISC_SPADE		254
#define CH_MISC_CLUB		255

// small arrow shapes
#define CH_ARROW_DN			248
#define CH_ARROW_LEFT		249
#define CH_ARROW_RIGHT		250
#define CH_ARROW_UP			251

// circular shapes, from larger to smaller
#define CH_CIRCLE_1			180	// full-size filled circle
#define CH_CIRCLE_2			225	// full-size selected radio button circle
#define CH_CIRCLE_3			179	// full-size empty circle
#define CH_CIRCLE_4			226 // medium-size filled circle
#define CH_CIRCLE_5			182 // small circle (square tho)
#define CH_CIRCLE_6			183 // tiny circle (1 pixel)

// square shapes, from larger to smaller
#define CH_RECT_1			7	// full-size filled square
#define CH_RECT_2			227	// full-size empty square
#define CH_RECT_3			181 // almost-full size filled square
#define CH_RECT_4			224 // medium-size filled square
#define CH_RECT_5			182 // small square
#define CH_RECT_6			183 // tiny square (1 pixel)

// Japanese JIS characters plus 6 custom from PC-8001
#define CH_JA_HOUR			24
#define CH_JA_MIN			25
#define CH_JA_SEC			26
#define CH_JA_YEAR			28
#define CH_JA_MONTH			29
#define CH_JA_DAY			192

#define CH_JIS_FIRST		193
#define CH_JIS_KUTEN		193
#define CH_JIS_OPEN			194
#define CH_JIS_CLOSE		195
#define CH_JIS_DOKUTEN		196
#define CH_JIS_MID			197
#define CH_JIS_WO			198
#define CH_JIS_L_A			199
#define CH_JIS_L_I			200
#define CH_JIS_L_U			201
#define CH_JIS_L_E			202
#define CH_JIS_L_O			203
#define CH_JIS_L_YA			204
#define CH_JIS_L_YU			205
#define CH_JIS_L_YO			206
#define CH_JIS_L_TU			207
#define CH_JIS_BOU			208
#define CH_JIS_A			209
#define CH_JIS_I			210
#define CH_JIS_U			211
#define CH_JIS_E			212
#define CH_JIS_O			213
#define CH_JIS_KA			214
#define CH_JIS_KI			215
#define CH_JIS_KU			216
#define CH_JIS_KE			217
#define CH_JIS_KO			218
#define CH_JIS_SA			219
#define CH_JIS_SHI			220
#define CH_JIS_SU			221
#define CH_JIS_SE			222
#define CH_JIS_SO			223
#define CH_JIS_TA			224
#define CH_JIS_TI			225
#define CH_JIS_TSU			226
#define CH_JIS_TE			227
#define CH_JIS_TO			228
#define CH_JIS_NA			229
#define CH_JIS_NI			230
#define CH_JIS_NU			231
#define CH_JIS_NE			232
#define CH_JIS_NO			233
#define CH_JIS_HA			234
#define CH_JIS_HI			235
#define CH_JIS_HU			236
#define CH_JIS_HE			237
#define CH_JIS_HO			238
#define CH_JIS_MA			239
#define CH_JIS_MI			240
#define CH_JIS_MU			241
#define CH_JIS_ME			242
#define CH_JIS_MO			243
#define CH_JIS_YA			244
#define CH_JIS_YU			245
#define CH_JIS_YO			246
#define CH_JIS_RA			247
#define CH_JIS_RI			248
#define CH_JIS_RU			249
#define CH_JIS_RE			250
#define CH_JIS_RO			251
#define CH_JIS_WA			252
#define CH_JIS_N			253
#define CH_JIS_B			254
#define CH_JIS_P			255
#define CH_JIS_LAST			255

/*****************************************************************************/
/*                             Named Characters                              */
/*****************************************************************************/

#define SC_HLINE        150
#define SC_VLINE        130
#define SC_ULCORNER     160
#define SC_URCORNER     161
#define SC_LLCORNER     162
#define SC_LRCORNER     163
#define SC_ULCORNER_RND 188
#define SC_URCORNER_RND 189
#define SC_LLCORNER_RND 190
#define SC_LRCORNER_RND 191
#define SC_CHECKERED	199
#define SC_T_DOWN		155 // T-shape pointing down
#define SC_T_UP			157 // T-shape pointing up
#define SC_T_LEFT		158 // T-shape pointing left
#define SC_T_RIGHT		154 // T-shape pointing right
#define SC_T_JUNCTION	156 // + shape meeting of 4 lines

/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/


/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/

typedef struct DateTime {
    uint8_t			year;
    uint8_t			month;
    uint8_t			day;
    uint8_t			hour;
    uint8_t			min;
    uint8_t			sec;
} DateTime;

typedef struct VICKY256Header
{
    uint8_t		magic[8];		// must read "VICKY256"
    uint8_t		version;		// first version is 1
    uint8_t		reserved;		// future use
    uint16_t	width;			// in pixels
    uint16_t	height;			// in pixels
    uint32_t	num_pixels;		// equivalent of width*height; if not, there's a problem. this must equal the bytes that follow the palette in the header.
	uint8_t		palette[1024];	// VICKY compatible BGRA format: 4 bytes per color, 256 colors.
} VICKY256Header;


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/





#endif /* F256JR_H_ */
