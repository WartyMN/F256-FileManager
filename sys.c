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






/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

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
	R8(VICKY_TEXT_CURSOR_ENABLE) = (uint8_t)enable_it;	
	Sys_RestoreIOPage();

	//DEBUG_OUT(("%s %d: cursor enabled now=%u", __func__, __LINE__, enable_it));
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
	static uint8_t	string_offsets[5] = {12,9,6,3,0};	// array is order by RTC order of min-hr-day-month-year
	static uint8_t	rtc_offsets[5] = {0,2,2,3,1};	// starting at min=d692
	static uint8_t	bounds[5] = {60,24,31,12,99};	// starting at min=d692
	uint8_t			old_rtc_control;
	uint8_t			i;
	int8_t			this_digit;
	int8_t			tens_digit;
	uint8_t			rtc_array[5];
	uint8_t*		rtc_addr;
	
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
