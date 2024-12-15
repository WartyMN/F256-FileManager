//! @file lib_sys.h

/*
 * lib_sys.h
 *
*  Created on: Mar 22, 2022
 *      Author: micahbly
 */

// THIS IS A CUT-DOWN VERSION OF the OS/f lib_sys.h file, just enough to power Lich King
// adapted for Foenix F256 Jr starting November 29, 2022



#ifndef LIB_SYS_H_
#define LIB_SYS_H_


/* about this library: System
 *
 * This provides overall system level functionality
 *
 *** things this library needs to be able to do
 * Manage global system resources such as fonts, screens, mouse pointer, etc. 
 * Provide event handling
 *
 * STRETCH GOALS
 * 
 *
 * SUPER STRETCH GOALS
 * 
 * 
 */


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#ifdef FEATURE_BITMAP
#include "bitmap.h"
#endif

// C includes
#include <stdbool.h>
#include <stdint.h>

#include "f256.h"


/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/

#define CH_ZERO			48		// this is just so we can do match to convert a char '5' into an integer 5, etc.
#define CH_NINE			57		// for number ranges when converting user input to numbers

#define PARAM_SPRITES_ON		true	// parameter for Sys_SetGraphicMode
#define PARAM_SPRITES_OFF		false	// parameter for Sys_SetGraphicMode
#define PARAM_BITMAP_ON			true	// parameter for Sys_SetGraphicMode
#define PARAM_BITMAP_OFF		false	// parameter for Sys_SetGraphicMode
#define PARAM_TILES_ON			true	// parameter for Sys_SetGraphicMode
#define PARAM_TILES_OFF			false	// parameter for Sys_SetGraphicMode
#define PARAM_TEXT_OVERLAY_ON	true	// parameter for Sys_SetGraphicMode
#define PARAM_TEXT_OVERLAY_OFF	false	// parameter for Sys_SetGraphicMode
#define PARAM_TEXT_ON			true	// parameter for Sys_SetGraphicMode
#define PARAM_TEXT_OFF			false	// parameter for Sys_SetGraphicMode

#define PARAM_DOUBLE_SIZE_TEXT	true	// parameter for Sys_SetTextPixelHeight
#define PARAM_NORMAL_SIZE_TEXT	false	// parameter for Sys_SetTextPixelHeight


/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/



/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/

typedef struct System
{
	uint8_t			model_number_;
	uint8_t			text_cols_vis_;		// accounting for borders, the number of visible columns on screen
	uint8_t			text_rows_vis_;		// accounting for borders, the number of visible rows on screen
	uint8_t			text_mem_cols_;		// for the current resolution, the total number of columns per row in VRAM. Use for plotting x,y 
	uint8_t			text_mem_rows_;		// for the current resolution, the total number of rows per row in VRAM. Use for plotting x,y 
// 	uint8_t*		text_ram_;
// 	uint8_t*		text_attr_ram_;
// 	uint8_t*		text_font_ram_;			// 2K of memory holding font definitions.
// 	uint8_t*		text_color_fore_ram_;	// 64b of memory holding foreground color LUTs for text mode, in BGRA order
// 	uint8_t*		text_color_back_ram_;	// 64b of memory holding background color LUTs for text mode, in BGRA order
#ifdef FEATURE_BITMAP
	int16_t			screen_width_;			// for the current resolution, the max horizontal pixel count 
	int16_t			screen_height_;			// for the current resolution, the max vertical pixel count
	Bitmap*			bitmap_;			//! The foreground (layer0=0) and background (layer1=1) bitmaps associated with this screen, if any. (Text only screens do not have bitmaps available)
#endif
} System;





/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/


// **** CONSTRUCTOR AND DESTRUCTOR *****




// **** System Initialization functions *****


//! Initialize the system (primary entry point for all system initialization activity)
//! Starts up the memory manager, creates the global system object, runs autoconfigure to check the system hardware, loads system and application fonts, allocates a bitmap for the screen.
bool Sys_InitSystem(void);

//! Find out what kind of machine the software is running on, and configure the passed screen accordingly
//! Configures screen settings, RAM addresses, etc. based on known info about machine types
//! Configures screen width, height, total text rows and cols, and visible text rows and cols by checking hardware
//! @return	Returns false if the machine is known to be incompatible with this software. 
bool Sys_AutoConfigure(void);

//! Find out what kind of machine the softw`are is running on, and determine # of screens available
//! @return	Returns false if the machine is known to be incompatible with this software. 
bool Sys_AutoDetectMachine(void);

//! Detect the current screen mode/resolution, and set # of columns, rows, H pixels, V pixels, accordingly
//! @return	returns false on any error/invalid input.
bool Sys_DetectScreenSize(void);



// **** Event-handling functions *****




// **** Screen mode/resolution/size functions *****


// enable or disable double height/width pixels
void Sys_SetTextPixelHeight(bool double_x, bool double_y);

//! Change video mode to the one passed.
//! @param	new_mode - One of the enumerated screen_resolution values. Must correspond to a valid VICKY video mode for the host machine. See VICKY_IIIA_RES_800X600_FLAGS, etc. defined in a2560_platform.h
//! @return	returns false on any error/invalid input.
bool Sys_SetVideoMode(uint8_t new_mode);

//! Switch machine into text mode
//! @param	as_overlay - If true, sets text overlay mode (text over graphics). If false, sets full text mode (no graphics);
void Sys_SetModeText(bool as_overlay);

//! Switch machine into graphics mode, text mode, sprite mode, etc.
//! Use PARAM_SPRITES_ON/OFF, PARAM_BITMAP_ON/OFF, PARAM_TILES_ON/OFF, PARAM_TEXT_OVERLAY_ON/OFF, PARAM_TEXT_ON/OFF
void Sys_SetGraphicMode(bool enable_sprites, bool enable_bitmaps, bool enable_tiles, bool enable_text_overlay, bool enable_text);


//! Enable or disable the hardware cursor in text mode, for the specified screen
//! @param	enable_it - If true, turns the hardware blinking cursor on. If false, hides the hardware cursor;
void Sys_EnableTextModeCursor(bool enable_it);

//! Set the left/right and top/bottom borders
//! This will reset the visible text columns as a side effect
//! @param	border_width - width in pixels of the border on left and right side of the screen. Total border used with be the double of this.
//! @param	border_height - height in pixels of the border on top and bottom of the screen. Total border used with be the double of this.
void Sys_SetBorderSize(uint8_t border_width, uint8_t border_height);




// **** Other GET functions *****




// **** Other SET functions *****



// **** xxx functions *****


// **** xxx functions *****








// **** Font functions *****

// switch to font set 0 or 1
//! @param	use_primary_font - true to switch the primary font, false to switch to the secondary font. Recommend using PARAM_USE_PRIMARY_FONT_SLOT/PARAM_USE_SECONDARY_FONT_SLOT.
void Sys_SwitchFontSet(bool use_primary_font);




// **** Render functions *****




// **** Tiny VICKY I/O page functions *****

// disable the I/O bank to allow RAM to be mapped into it
// current MMU setting is saved to the 6502 stack
void Sys_DisableIOBank(void);

// change the I/O page
void Sys_SwapIOPage(uint8_t the_page_number);

// restore the previous MMU setting, which was saved by Sys_SwapIOPage()
void Sys_RestoreIOPage(void);

// update the system clock with a date/time string in YY/MM/DD HH:MM format
// returns true if format was acceptable (and thus update of RTC has been performed).
bool Sys_UpdateRTC(char* datetime_from_user);



// **** Debug functions *****

// void Sys_Print(System* the_system);

// void Sys_PrintScreen(Screen* the_screen);



#endif /* LIB_SYS_H_ */


