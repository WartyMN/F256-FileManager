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

#define VIDEO_MODE_FREQ_BIT			0x01	//!> the bits in the 2nd byte of the system control register that define video mode (resolution). if this bit is set, resolution is 70hz 320x200 (text mode 80*25). if clar, is 60hz 630*240
#define VIDEO_MODE_DOUBLE_X_BIT		0x02	//!> the bits in the 2nd byte of the system control register control text mode doubling in horizontal. if set, text mode chars are doubled in size, producing 40 chars across
#define VIDEO_MODE_DOUBLE_Y_BIT		0x04	//!> the bits in the 3rd byte of the system control register control text mode doubling in vertical. if set, text mode chars are doubled in size, producing 25 or 30 chars down

#define GAMMA_MODE_DIPSWITCH_BIT	0x20	//!>the bits in the 2nd byte of the system control register reflect dip switch setting for control gamma correction on/off
#define GAMMA_MODE_ONOFF_BITS		0x03	//!>the bits in the 3rd byte of the system control register control gamma correction on/off


// Tiny VICKY I/O pages
#define VICKY_IO_PAGE_REGISTERS			0	// Low level I/O registers
#define VICKY_IO_PAGE_FONT_AND_LUTS		1	// Text display font memory and graphics color LUTs
#define VICKY_IO_PAGE_CHAR_MEM			2	// Text display character matrix
#define VICKY_IO_PAGE_ATTR_MEM			3	// Text display color matrix

// Tiny VICKY I/O page 0 addresses
#define VICKY_BASE_ADDRESS				0xd000		// Tiny VICKY offset/first register
#define VICKY_MASTER_CTRL_REG_L			0xd000		// Tiny VICKY Master Control Register - low - graphic mode/text mode/etc.
#define VICKY_MASTER_CTRL_REG_H			0xd001		// Tiny VICKY Master Control Register - high - screen res, etc.
#define VICKY_GAMMA_CTRL_REG			0xd002		// Tiny VICKY Gamma Control Register
#define VICKY_BORDER_CTRL_REG			0xd004		// Tiny VICKY Border Control Register
#define VICKY_BORDER_COLOR_B			0xd005		// Tiny VICKY Border Color Blue
#define VICKY_BORDER_COLOR_G			0xd006		// Tiny VICKY Border Color Green
#define VICKY_BORDER_COLOR_R			0xd007		// Tiny VICKY Border Color Red
#define VICKY_BORDER_X_SIZE				0xd008		// Tiny VICKY Border X size in pixels
#define VICKY_BORDER_Y_SIZE				0xd009		// Tiny VICKY Border Y size in pixels
#define VICKY_BACKGROUND_COLOR_B		0xd00d		// Tiny VICKY background color Blue
#define VICKY_BACKGROUND_COLOR_G		0xd00e		// Tiny VICKY background color Green
#define VICKY_BACKGROUND_COLOR_R		0xd00f		// Tiny VICKY background color Red
#define VICKY_TEXT_CURSOR_ENABLE		0xd010		// bit 2 is enable/disable
#define BITMAP_CTRL						0xd100		//!> bitmap control register	
#define BITMAP_L0_VRAM_ADDR_L			0xd101		//!> bitmap VRAM address pointer)		
#define BITMAP_L0_VRAM_ADDR_M			0xd102		//!> bitmap VRAM address pointer)		
#define BITMAP_L0_VRAM_ADDR_H			0xd103		//!> bitmap VRAM address pointer)		
#define TILE_CTRL						0xd200		//!> tile control register		
#define VICKY_PS2_INTERFACE				0xd640
#define RTC_SECONDS						0xd690
#define RTC_MINUTES						0xd692
#define RTC_CONTROL						0xd69e		// set bit 3 to disable update of reg, to read secs. 
#define RANDOM_NUM_GEN_LOW				0xd6a4		// both the SEEDL and the RNDL (depends on bit 1 of RND_CTRL)
#define RANDOM_NUM_GEN_HI				0xd6a5		// both the SEEDH and the RNDH (depends on bit 1 of RND_CTRL)
#define RANDOM_NUM_GEN_ENABLE			0xd6a6		// bit 0: enable/disable. bit 1: seed mode on/off. "RND_CTRL"
#define MACHINE_ID_REGISTER				0xd6a7		// will be '2' for F256JR
#define TEXT_FORE_LUT					0xd800		// FG_CHAR_LUT_PTR	Text Foreground Look-Up Table
#define TEXT_BACK_LUT					0xd840		// BG_CHAR_LUT_PTR	Text Background Look-Up Table

// Tiny VICKY I/O page 1 addresses
#define FONT_MEMORY_BANK				0xc000		// FONT_MEMORY_BANK0	FONT Character Graphic Mem
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

#define RES_320X200		0
#define RES_320X240		1

// machine model numbers - for decoding s_sys_info.model - value read from MACHINE_ID_REGISTER (see above)
#define MACHINE_C256_FMX		0	///< for s_sys_info.model
#define MACHINE_C256_U			1	///< for s_sys_info.model
#define MACHINE_F256_JR			2	///< for s_sys_info.model
#define MACHINE_C256_GENX		4	///< for s_sys_info.model
#define MACHINE_C256_UPLUS		5	///< for s_sys_info.model
#define MACHINE_A2560U_PLUS		6	///< for s_sys_info.model
#define MACHINE_A2560X			7	///< for s_sys_info.model
#define MACHINE_A2560U			9	///< for s_sys_info.model
#define MACHINE_A2560K			13	///< for s_sys_info.model
		

typedef uint8_t	ColorIdx;


/*****************************************************************************/
/*                              Key Definitions                              */
/*****************************************************************************/

#define CH_F1           224
#define CH_F2           225
#define CH_F3           226
#define CH_F4           227
#define CH_F5           228
#define CH_F6           229
#define CH_F7           230
#define CH_F8           231
#define CH_F9           232
#define CH_F10          233
#define CH_CURS_UP      0x10
#define CH_CURS_DOWN    0x0e
#define CH_CURS_LEFT    0x02
#define CH_CURS_RIGHT   0x06
#define CH_DEL          0x08
#define CH_ENTER        13
#define CH_ESC          27
#define CH_TAB          9
#define CH_SPACE		32

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

typedef struct DateStamp {
	uint16_t	ds_Year;
	uint32_t	ds_Days;
	uint32_t	ds_Minute;
	uint32_t	ds_Tick;
} DateStamp;

typedef struct DateTime {
	DateStamp			dat_Stamp;		// DOS DateStamp
	uint8_t				dat_Format;		// controls appearance of dat_StrDate
	uint8_t				dat_Flags;		// see BITDEF's below
	char*				dat_StrDay;		// day of the week string
	char*				dat_StrDate;	// date string
	char*				dat_StrTime;	// time string
} DateTime;

/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/





#endif /* F256JR_H_ */
