/*
 * lib_sys.c
 *
 *  Created on: Mar 22, 2022
 *      Author: micahbly
 */


// THIS IS A CUT-DOWN VERSION OF the OS/f lib_sys.c file, just enough to power Lich King
// adapted for Foenix F256 Jr starting November 29, 2022


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "sys.h"
#include "app.h"
#include "comm_buffer.h"
#include "memory.h"

// C includes
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// cc65 includes
#include "f256.h"
#include "text.h"


/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/

uint8_t			io_bank_value_kernel;	// stores value for the physical bank pointing to C000-DFFF whenever we change it, so we can restore it.
uint8_t			overlay_bank_value_kernel;	// stores value for the physical bank pointing to A000-BFFF whenever we change it, so we can restore it.



/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

extern char*		global_string_buff1;



/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/



/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/






/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/


static System	system_storage;
System*			global_system = &system_storage;


// F256JR/K colors, used for both fore- and background colors in Text mode
// in C256 & F256, these are 8 bit values; in A2560s, they are 32 bit values, and endianness matters
static uint8_t standard_text_color_lut[64] = 
{
	0x00, 0x00, 0x00, 0x00,
	0x66, 0x66, 0x66, 0x00,
	0xAA, 0x00, 0x00, 0x00,
	0x00, 0xAA, 0x00, 0x00,
	0xEA, 0x41, 0xC0, 0x00,
	0x00, 0x48, 0x87, 0x00,
	0x00, 0x9C, 0xFF, 0x00,
	0xFF, 0xDB, 0x57, 0x00,
	0x28, 0x3F, 0x3F, 0x00,
	0x8A, 0xAA, 0xAA, 0x00,
	0xFF, 0x55, 0x55, 0x00,
	0x55, 0xFF, 0x55, 0x00,
	0xED, 0x8D, 0xFF, 0x00,
	0x00, 0x00, 0xFF, 0x00,			
	0x55, 0xFF, 0xFF, 0x00,
	0xFF, 0xFF, 0xFF, 0x00
};



/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

// enable or disable the gamma correction 
void Sys_SetGammaMode(bool enable_it);

// convert a decimal number to BCD format (for use with RTC)
uint8_t Sys_DecimalToBCD(uint8_t dec_number);


/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/

// convert a decimal number to BCD format (for use with RTC)
uint8_t Sys_DecimalToBCD(uint8_t dec_number)
{
	uint8_t		bcd_number = 0;
	
	while (dec_number >= 10)
	{
		++bcd_number;
		dec_number -= 10;
	}
	
	bcd_number = bcd_number << 4;
	
	return (bcd_number | dec_number);
}


// // interrupt 1 is PS2 keyboard, interrupt 2 is A2560K keyboard
// void Sys_InterruptKeyboard(void)
// {
// 	kbd_handle_irq();
// }

// 
// // interrupt 4 is PS2 mouse
// void Sys_InterruptMouse(void);


// **** Debug functions *****

// void Sys_Print(System* the_system)
// {
// 	DEBUG_OUT(("System print out:"));
// 	DEBUG_OUT(("  address: %p", 			the_system));
// 	DEBUG_OUT(("  num_screens_: %i",		the_system->num_screens_));
// 	DEBUG_OUT(("  model_number_: %i",		the_system->model_number_));
// }


// void Sys_PrintScreen(Screen* the_screen)
// {
// 	DEBUG_OUT(("Screen print out:"));
// 	DEBUG_OUT(("  address: %p", 			the_screen));
// 	DEBUG_OUT(("  id_: %i", 				the_screen->id_));
// 	DEBUG_OUT(("  vicky_: %p", 				the_screen->vicky_));
// 	DEBUG_OUT(("  width_: %i", 				the_screen->width_));
// 	DEBUG_OUT(("  height_: %i", 			the_screen->height_));
// 	DEBUG_OUT(("  text_cols_vis_: %i", 		the_screen->text_cols_vis_));
// 	DEBUG_OUT(("  text_rows_vis_: %i", 		the_screen->text_rows_vis_));
// 	DEBUG_OUT(("  text_mem_cols_: %i", 		the_screen->text_mem_cols_));
// 	DEBUG_OUT(("  text_mem_rows_: %i", 		the_screen->text_mem_rows_));
// 	DEBUG_OUT(("  text_ram_: %p", 			the_screen->text_ram_));
// 	DEBUG_OUT(("  text_attr_ram_: %p", 		the_screen->text_attr_ram_));
// 	DEBUG_OUT(("  text_font_ram_: %p", 		the_screen->text_font_ram_));
// 	DEBUG_OUT(("  bitmap_[0]: %p", 			the_screen->bitmap_[0]));
// 	DEBUG_OUT(("  bitmap_[1]: %p", 			the_screen->bitmap_[1]));
// 	DEBUG_OUT(("  text_font_height_: %i",	the_screen->text_font_height_));
// 	DEBUG_OUT(("  text_font_width_: %i",	the_screen->text_font_width_));
// }



/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/

// **** CONSTRUCTOR AND DESTRUCTOR *****





// **** System Initialization functions *****

//! Initialize the system (primary entry point for all system initialization activity)
//! Starts up the memory manager, creates the global system object, runs autoconfigure to check the system hardware, loads system and application fonts, allocates a bitmap for the screen.
bool Sys_InitSystem(void)
{	
	// open log file, if on real hardware, and built with calypsi, and debugging flags were passed
	#ifdef LOG_LEVEL_5
		#ifndef _SIMULATOR_
			#ifdef __CALYPSI__
				if (General_LogInitialize() == false)
				{
					printf("%s %d: failed to open log file for writing \n", __func__, __LINE__);
				}
			#endif
		#endif
	#endif
	
	//DEBUG_OUT(("%s %d: Initializing System...", __func__, __LINE__));
	
	// check what kind of hardware the system is running on
	// LOGIC: we need to know how many screens it has before allocating screen objects
	if (Sys_AutoDetectMachine() == false)
	{
		LOG_ERR(("%s %d: Detected machine hardware is incompatible with this software", __func__ , __LINE__));
		return false;
	}
	
	//DEBUG_OUT(("%s %d: Hardware detected. Running Autoconfigure...", __func__, __LINE__));
	
	if (Sys_AutoConfigure() == false)
	{
		LOG_ERR(("%s %d: Auto configure failed", __func__, __LINE__));
		return false;
	}

	
	// Enable mouse pointer -- no idea if this works, f68 emulator doesn't support mouse yet. 
	//R32(VICKYB_MOUSE_CTRL_A2560K) = 1;
	
	// set interrupt handlers
//	ps2_init();
//	global_old_keyboard_interrupt = sys_int_register(INT_KBD_PS2, &Sys_InterruptKeyboard);
// 	global_old_keyboard_interrupt = sys_int_register(INT_KBD_A2560K, &Sys_InterruptKeyboard);
// 	global_old_mouse_interrupt = sys_int_register(INT_MOUSE, &Sys_InterruptKeyboard);

	//DEBUG_OUT(("%s %d: System initialization complete.", __func__, __LINE__));

	return true;
}





// **** Event-handling functions *****

// see MCP's ps2.c for real examples once real machine available

// // interrupt 1 is PS2 keyboard, interrupt 2 is A2560K keyboard
// void Sys_InterruptKeyboard(void)
// {
// 	printf("keyboard!\n");
// 	return;
// }
// 
// // interrupt 4 is PS2 mouse
// void Sys_InterruptMouse(void)
// {
// 	printf("mouse!\n");
// 	return;
// }




// **** Screen mode/resolution/size functions *****


//! Find out what kind of machine the software is running on, and determine # of screens available
//! @param	the_system: valid pointer to system object
//! @return	Returns false if the machine is known to be incompatible with this software. 
bool Sys_AutoDetectMachine(void)
{
	uint8_t	the_machine_id;
	
	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);
	
	//asm("stp");
// 	the_machine_id = (R8(MACHINE_ID_REGISTER) & MACHINE_MODEL_MASK);
// 	the_machine_id = (R8(MACHINE_ID_REGISTER) & MACHINE_MODEL_MASK);
	the_machine_id = MACHINE_F256_JR; // emulator might not have machine_id_register set, I'm only seeing a 0 there. 
// 	the_machine_id = MACHINE_F256_JR; // emulator might not have machine_id_register set, I'm only seeing a 0 there. 
	DEBUG_OUT(("%s %d: the_machine_id=%u, gabe raw value=%u", __func__, __LINE__, the_machine_id, R8(MACHINE_ID_REGISTER)));
	
	Sys_RestoreIOPage();

// 	sprintf(global_string_buff1, "the_machine_id=%u, gabe raw value=%u", the_machine_id, R8(MACHINE_ID_REGISTER));
// 	Buffer_NewMessage(global_string_buff1);
// 	General_DelayTicks(65534);
// 	General_DelayTicks(65534);
	
// 	if (the_machine_id == MACHINE_F256_JR)
// 	{
// 		DEBUG_OUT(("%s %d: it's a F256jr!", __func__, __LINE__));
// 	}
// 	else if (the_machine_id == MACHINE_C256_FMX)
// 	{
// 		DEBUG_OUT(("%s %d: it's a C256 FMX!", __func__, __LINE__));
// 	}
// 	else if (the_machine_id == MACHINE_C256_U)
// 	{
// 		DEBUG_OUT(("%s %d: it's a C256U", __func__, __LINE__));
// 	}
// 	else if (the_machine_id == MACHINE_C256_GENX)
// 	{
// 		DEBUG_OUT(("%s %d: it's a C256U", __func__, __LINE__));
// 	}
// 	else if (the_machine_id == MACHINE_C256_UPLUS)
// 	{
// 		DEBUG_OUT(("%s %d: it's a C256U+", __func__, __LINE__));
// 	}		
	
	global_system->model_number_ = the_machine_id;
// 	DEBUG_OUT(("%s %d: global_system->model_number_=%u", __func__, __LINE__, global_system->model_number_));
	
	// temp until Calypsi fix for switch on 65816
	if (the_machine_id != MACHINE_F256_JR)
	{
		//DEBUG_OUT(("%s %d: I can't recognize this machine (id=%u). Application will now quit.", __func__, __LINE__, the_machine_id));
		return false;
	}
		
	return true;
}


//! Find out what kind of machine the software is running on, and configure the passed screen accordingly
//! Configures screen settings, RAM addresses, etc. based on known info about machine types
//! Configures screen width, height, total text rows and cols, and visible text rows and cols by checking hardware
//! @param	the_system: valid pointer to system object
//! @return	Returns false if the machine is known to be incompatible with this software. 
bool Sys_AutoConfigure(void)
{
// 	sprintf(global_string_buff1, "global_system->model_number_=%u", global_system->model_number_);
// 	Buffer_NewMessage(global_string_buff1);
// 	General_DelayTicks(40000);

	if (global_system->model_number_ == MACHINE_F256_JR)
	{
		//DEBUG_OUT(("%s %d: Configuring screens for an F256jr (1 screen)", __func__, __LINE__));
// 		global_system->screen_[ID_CHANNEL_A]->vicky_ = P32(VICKY_C256);
// 		global_system->screen_[ID_CHANNEL_A]->text_ram_ = TEXT_RAM_C256;
// 		global_system->screen_[ID_CHANNEL_A]->text_attr_ram_ = TEXT_ATTR_C256;
// 		global_system->screen_[ID_CHANNEL_A]->text_font_ram_ = FONT_MEMORY_BANK_C256;
// 		global_system->screen_[ID_CHANNEL_A]->text_color_fore_ram_ = (char*)TEXT_FORE_LUT_C256;
// 		global_system->screen_[ID_CHANNEL_A]->text_color_back_ram_ = (char*)TEXT_BACK_LUT_C256;
	
		// use auto configure to set resolution, text cols, margins, etc
		if (Sys_DetectScreenSize() == false)
		{
			LOG_ERR(("%s %d: Unable to auto-configure screen resolution", __func__, __LINE__));
			return false;
		}

		Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);
	
		// set standard color LUTs for text mode
		memcpy((uint8_t*)(TEXT_FORE_LUT), &standard_text_color_lut, 64);
		memcpy((uint8_t*)(TEXT_BACK_LUT), &standard_text_color_lut, 64);
	
		Sys_RestoreIOPage();
	
// 		DEBUG_OUT(("%s %d: This screen has %i x %i text (%i x %i visible)", __func__, __LINE__, 
// 			global_system->text_mem_cols_, 
// 			global_system->text_mem_rows_, 
// 			global_system->text_cols_vis_, 
// 			global_system->text_rows_vis_
// 			));
	}
	else
	{
		//DEBUG_OUT(("%s %d: this system %i not supported!", __func__, __LINE__, global_system->model_number_));
		return false;
	}
		
	// always enable gamma correction
	Sys_SetGammaMode(true);
	
	return true;
}


// //! Switch machine into text mode
// //! @param	the_system: valid pointer to system object
// //! @param as_overlay: If true, sets text overlay mode (text over graphics). If false, sets full text mode (no graphics);
// void Sys_SetModeText(bool as_overlay)
// {	
// 	// LOGIC:
// 	//   On an A2560K or X, the only screen that has a text/graphics mode is the Channel B screen
// 	
// 	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);
// 	
// 	if (as_overlay)
// 	{
// 		// switch to text mode with overlay by setting graphics mode bit, setting bitmap engine enable bit, and setting graphics mode overlay		
// 		R8(VICKY_MASTER_CTRL_REG_L) = (GRAPHICS_MODE_TEXT | GRAPHICS_MODE_TEXT_OVER | GRAPHICS_MODE_GRAPHICS | GRAPHICS_MODE_EN_BITMAP);
// 		R8(BITMAP_CTRL) = 0x01;
// 		
// 		// c256foenix, discord 2022/03/10
// 		// Normally, for example, if you setup everything to be in bitmap mode, and you download an image in VRAM and you can see it properly... If you turn on overlay, then you will see on top of that same image, your text that you had before.
// 		// Mstr_Ctrl_Text_Mode_En  = $01       ; Enable the Text Mode
// 		// Mstr_Ctrl_Text_Overlay  = $02       ; Enable the Overlay of the text mode on top of Graphic Mode (the Background Color is ignored)
// 		// Mstr_Ctrl_Graph_Mode_En = $04       ; Enable the Graphic Mode
// 		// Mstr_Ctrl_Bitmap_En     = $08       ; Enable the Bitmap Module In Vicky
// 		// all of these should be ON
// 	}
// 	else
// 	{
// 		R8(VICKY_MASTER_CTRL_REG_L) = (GRAPHICS_MODE_TEXT);
// 		// disable bitmap
// 		R8(BITMAP_CTRL) = 0x00;
// 	}
// 	
// 	Sys_RestoreIOPage();
// }


//! Detect the current screen mode/resolution, and set # of columns, rows, H pixels, V pixels, accordingly
bool Sys_DetectScreenSize(void)
{
	uint8_t			new_mode;
	uint8_t			the_video_mode_bits;
	uint8_t			border_x_cols;
	uint8_t			border_y_cols;
	int16_t			border_x_pixels;
	int16_t			border_y_pixels;
	
	// detect the video mode and set resolution based on it
	
	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);

	the_video_mode_bits = R8(VICKY_MASTER_CTRL_REG_H);
	//DEBUG_OUT(("%s %d: 8bit vicky ptr 2nd byte=%p, video mode bits=%x", __func__, __LINE__, vicky_8bit_ptr, the_video_mode_bits));
	
	//   F256JR has 1 channel with 2 video modes, 70hz=640x400 (graphics doubled to 320x200) and 60hz=640x480

	if (the_video_mode_bits & VIDEO_MODE_FREQ_BIT)
	{
		new_mode = RES_320X200;
		global_system->text_mem_rows_ = TEXT_ROW_COUNT_70HZ; // 2 options in JR. the_screen->height_ / TEXT_FONT_HEIGHT;
	}
	else
	{
		new_mode = RES_320X240;
		global_system->text_mem_rows_ = TEXT_ROW_COUNT_60HZ; // 2 options in JR. the_screen->height_ / TEXT_FONT_HEIGHT;
	}

// we don't really care about pixels in Lich King... 
// 	switch (new_mode)
// 	{
// 		case RES_320X200:
// 			the_screen->width_ = 320;	
// 			the_screen->height_ = 200;
// 			DEBUG_OUT(("%s %d: set to RES_320X200", __func__, __LINE__));
// 			break;
// 			
// 		case RES_320X240:
// 			the_screen->width_ = 320;	
// 			the_screen->height_ = 240;
// 			DEBUG_OUT(("%s %d: set to RES_320X200", __func__, __LINE__));
// 			break;
// 	}
	
	// detect borders, and set text cols/rows based on resolution modified by borders (if any)
	border_x_pixels = R8(VICKY_BORDER_X_SIZE);
	border_y_pixels = R8(VICKY_BORDER_Y_SIZE);
	//DEBUG_OUT(("%s %d: border x,y=%i,%i", __func__, __LINE__, R8(VICKY_BORDER_X_SIZE), R8(VICKY_BORDER_Y_SIZE)));
	
	Sys_RestoreIOPage();
	
	border_x_cols = (border_x_pixels * 2) / TEXT_FONT_WIDTH;
	border_y_cols = (border_y_pixels * 2) / TEXT_FONT_HEIGHT;
	global_system->text_mem_cols_ = TEXT_COL_COUNT_FOR_PLOTTING; // only 1 option in JR. the_screen->width_ / TEXT_FONT_WIDTH;
	global_system->text_cols_vis_ = global_system->text_mem_cols_ - border_x_cols;
	global_system->text_rows_vis_ = global_system->text_mem_rows_ - border_y_cols;
// 	global_system->rect_.MaxX = the_screen->width_;
// 	global_system->rect_.MaxY = the_screen->height_;	
	//Sys_PrintScreen(the_screen);
	
	return true;
}


// //! Change video mode to the one passed.
// //! @param	the_screen: valid pointer to the target screen to operate on
// //! @param	new_mode: One of the enumerated screen_resolution values. Must correspond to a valid VICKY video mode for the host machine. See VICKY_IIIA_RES_800X600_FLAGS, etc. defined in a2560_platform.h
// //! @return	returns false on any error/invalid input.
// bool Sys_SetVideoMode(uint8_t new_mode)
// {
// 	uint8_t		new_mode_flag;
// 	
// 	if (new_mode == RES_320X240)
// 	{
// 		new_mode_flag = VICKY_RES_320X240_FLAGS;
// 	}
// 	else if (new_mode == RES_320X200)
// 	{
// 		new_mode_flag = VICKY_RES_320X200_FLAGS;
// 	}
// 	else
// 	{
// 		LOG_ERR(("%s %d: specified video mode is not legal for this screen %u", __func__, __LINE__, new_mode));
// 		return false;
// 	}
// 	
//  	//DEBUG_OUT(("%s %d: specified video mode = %u, flag=%u", __func__, __LINE__, new_mode, new_mode_flag));
// 		
// 	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);
// 	
//  	//DEBUG_OUT(("%s %d: vicky before = %x", __func__, __LINE__, *the_screen->vicky_ ));
// 	R8(VICKY_MASTER_CTRL_REG_H) = R8(VICKY_MASTER_CTRL_REG_H) & new_mode_flag;
//  	//DEBUG_OUT(("%s %d: vicky after = %x", __func__, __LINE__, *the_screen->vicky_ ));
// 	
// 	Sys_RestoreIOPage();
// 	
// 	// teach screen about the new settings
// 	if (Sys_DetectScreenSize() == false)
// 	{
// 		LOG_ERR(("%s %d: Changed screen resolution, but the selected resolution could not be handled", __func__, __LINE__, new_mode));
// 		return false;
// 	}
// 
// 	// tell the MCP that we changed res so it can update it's internal col sizes, etc.  - this function is not exposed in MCP headers yet
// 	//sys_text_setsizes();
// 	
// 	return true;
// }


// enable or disable the gamma correction 
void Sys_SetGammaMode(bool enable_it)
{
	uint8_t		the_gamma_mode_bits = R8(VICKY_GAMMA_CTRL_REG);
	uint8_t		new_mode_flag;

	// LOGIC:
	//   both C256s and A2560s have a gamma correction mode
	//   It needs to be hardware enabled by turning DIP switch 7 on the motherboard to ON (I believe)
	//     bit 5 (0x20) of the video mode byte in vicky master control reflects the DIP switch setting, but doesn't change anything if you write to it 
	//   byte 3 of the vicky master control appears to be dedicated to Gamma correction, but not all bits are documented. Stay away from all but the first 2!
	//     gamma correction can be activated by setting the first and 2nd bits of byte 3
	
	if (enable_it)
	{
		new_mode_flag = 0xFF;
	}
	else
	{
		new_mode_flag = 0x00;
	}

	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);
	
	//DEBUG_OUT(("%s %d: vicky byte 3 before gamma change = %x", __func__, __LINE__, the_gamma_mode_bits));
	the_gamma_mode_bits |= (GAMMA_MODE_ONOFF_BITS & new_mode_flag);
	R8(VICKY_GAMMA_CTRL_REG) = the_gamma_mode_bits;
	//DEBUG_OUT(("%s %d: vicky byte 3 after gamma change = %x, %x", __func__, __LINE__, the_gamma_mode_bits, R8(VICKY_GAMMA_CTRL_REG)));
	//DEBUG_OUT(("%s %d: wrote to %x to register at %p", __func__, __LINE__, the_gamma_mode_bits, P8(VICKY_GAMMA_CTRL_REG)));
	
	Sys_RestoreIOPage();
}


//! Enable or disable the hardware cursor in text mode, for the specified screen
//! @param	the_system: valid pointer to system object
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param enable_it: If true, turns the hardware blinking cursor on. If false, hides the hardware cursor;
void Sys_EnableTextModeCursor(bool enable_it)
{
	// LOGIC:
	//   bit 0 is enable/disable
	//   bit 1-2 are the speed of flashing
	//   bit 3 is solid (0) or flashing (1)
	
	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);

	if (enable_it)
	{
		R8(VICKY_TEXT_CURSOR_ENABLE) = 1;
	}
	else
	{
		R8(VICKY_TEXT_CURSOR_ENABLE) = 0;
	}
	
	Sys_RestoreIOPage();

	//DEBUG_OUT(("%s %d: cursor enabled now=%u", __func__, __LINE__, enable_it));
}


//! Set the left/right and top/bottom borders
//! This will reset the visible text columns as a side effect
//! Grotesquely large values will be accepted as is: use at your own risk!
//! @param	border_width: width in pixels of the border on left and right side of the screen. Total border used with be the double of this.
//! @param	border_height: height in pixels of the border on top and bottom of the screen. Total border used with be the double of this.
//! @return	returns false on any error/invalid input.
void Sys_SetBorderSize(uint8_t border_width, uint8_t border_height)
{
	uint8_t		border_x_cols;
	uint8_t		border_y_cols;

	// LOGIC: 
	//   borders are set in pixels, from 0 to 31 max. 
	//   borders have no effect unless the border is enabled!
	
	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);
	
	// set borders
	R8(VICKY_BORDER_X_SIZE) = border_width;
	R8(VICKY_BORDER_Y_SIZE) = border_height;
	
	// enable borders or disable
	if (border_width > 0 || border_height > 0)
	{
		R8(VICKY_BORDER_CTRL_REG) = 1;
	}
	else
	{
		R8(VICKY_BORDER_CTRL_REG) = 0;
	}
	
	Sys_RestoreIOPage();

	border_x_cols = (border_width * 2) / TEXT_FONT_WIDTH;
	border_y_cols = (border_height * 2) / TEXT_FONT_HEIGHT;
	//DEBUG_OUT(("%s %d: x and y borders set to %u, %u", __func__, __LINE__, border_width, border_height));
	//DEBUG_OUT(("%s %d: x and y borders cols/rows now %u, %u", __func__, __LINE__, border_x_cols, border_y_cols));
	
	// now we need to recalculate how many text cols/rows are visible, because it might have changed
	global_system->text_cols_vis_ = global_system->text_mem_cols_ - border_x_cols;
	global_system->text_rows_vis_ = global_system->text_mem_rows_ - border_y_cols;
	//DEBUG_OUT(("%s %d: visible cols,rows now %u, %u", __func__, __LINE__, global_system->text_cols_vis_, global_system->text_rows_vis_));
}


// disable the I/O bank to allow RAM to be mapped into it
// current MMU setting is saved to the 6502 stack
void Sys_DisableIOBank(void)
{
	asm("lda $01");	// Stash the current IO page at ZP_OLD_IO_PAGE
	asm("sta %b", ZP_OLD_IO_PAGE);
	R8(MMU_IO_CTRL) = 4; // set only bit 2
}


// change the I/O page
// current IO setting is saved for later restoration
void Sys_SwapIOPage(uint8_t the_page_number)
{
	asm("lda $01");	// Stash the current IO page at ZP_OLD_IO_PAGE
	asm("sta %b", ZP_OLD_IO_PAGE);
	R8(MMU_IO_CTRL) = the_page_number;
}


// restore the previous IO page setting, which was saved by Sys_SwapIOPage()
void Sys_RestoreIOPage(void)
{
	asm("lda %b", ZP_OLD_IO_PAGE);	// we stashed the previous IO page at ZP_OLD_IO_PAGE
	asm("sta $01");	// switch back to the previous IO setting
}	
	

// update the system clock with a date/time string in YY/MM/DD HH:MM format
// returns true if format was acceptable (and thus update of RTC has been performed).
bool Sys_UpdateRTC(char* datetime_from_user)
{
	uint8_t*		rtc_addr;
	uint8_t			old_rtc_control;
	uint8_t			i;
	int8_t			this_digit;
	int8_t			tens_digit;
	uint8_t			rtc_array[5];
	static uint8_t	string_offsets[5] = {12,9,6,3,0};	// array is order by RTC order of min-hr-day-month-year
	static uint8_t	rtc_offsets[5] = {0,2,2,3,1};	// starting at min=d692
	static uint8_t	bounds[5] = {60,24,31,12,99};	// starting at min=d692
	
	for (i = 0; i < 5; i++)
	{
		this_digit = datetime_from_user[string_offsets[i]];
		
		if (this_digit < CH_ZERO || this_digit > CH_NINE)
		{
			return false;
		}
		
		tens_digit = this_digit - CH_ZERO;
		this_digit = datetime_from_user[string_offsets[i] + 1];
		
		if (this_digit < CH_ZERO || this_digit > CH_NINE)
		{
			return false;
		}
		
		rtc_array[i] = (this_digit - CH_ZERO) + (tens_digit * 10);	
	}

	//sprintf(global_string_buff1, "%02X %02X %02X %02X %02X", rtc_array[4], rtc_array[3], rtc_array[2], rtc_array[1], rtc_array[0]);
	//Text_DrawStringAtXY(0, 3, global_string_buff1, COLOR_BRIGHT_YELLOW, COLOR_BLACK);	

	// check if any of the numbers are too high
	for (i = 0; i < 5; i++)
	{
		if (rtc_array[i] > bounds[i])
		{
			return false;
		}
	}

	// numbers are all good, convert to BCD
	for (i = 0; i < 5; i++)
	{
		rtc_array[i] = Sys_DecimalToBCD(rtc_array[i]);
	}

	//sprintf(global_string_buff1, "20%02X-%02X-%02X %02X:%02X", rtc_array[4], rtc_array[3], rtc_array[2], rtc_array[1], rtc_array[0]);
	//Text_DrawStringAtXY(25, 3, global_string_buff1, COLOR_BRIGHT_YELLOW, COLOR_BLACK);	

	asm("SEI"); // disable interrupts in case some other process has a role here
	
	// need to have vicky registers available
	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);
	asm("SEI"); // disable interrupts in case some other process has a role here
	
	// stop RTC from updating external registers. Required!
	old_rtc_control = R8(RTC_CONTROL);
	R8(RTC_CONTROL) = old_rtc_control | 0x08; // stop it from updating external registers

	rtc_addr = (uint8_t*)RTC_MINUTES;
	
	for (i = 0; i < 5; i++)
	{
		R8(MMU_IO_CTRL) = VICKY_IO_PAGE_REGISTERS; // just make sure i/o page is still up. 
		rtc_addr += rtc_offsets[i];		
		R8(rtc_addr) = rtc_array[i];
	}
	
	// restore timer control to what it had been
	R8(RTC_CONTROL) = old_rtc_control;

	Sys_RestoreIOPage();	
	asm("CLI"); // restore interrupts

	return true;
}
