/*
 * text.c
 *
 *  Created on: Feb 19, 2022
 *      Author: micahbly
 */


// This is a cut-down, semi-API-compatible version of the OS/f text.c file from Lich King (Foenix)
// adapted for Foenix F256 Jr starting November 29, 2022



/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "debug.h"
#include "general.h"
#include "keyboard.h"
#include "text.h"
#include "sys.h"

// C includes
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// F256 includes
#include "f256.h"



/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/

// hide __fastcall_ from everything but CC65 (to squash some warnings in LSP/BBEdit)
#ifndef __CC65__
	#define __fastcall__ 
#endif


/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/



/*****************************************************************************/
/*                          File-scoped Variables                            */
/*****************************************************************************/


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

extern System*			global_system;

extern uint8_t*		zp_ptr;
extern uint8_t*		zp_vram_ptr;
extern uint16_t		zp_len;
extern uint8_t		zp_x;
extern uint8_t		zp_y;
extern uint8_t		zp_char;

#pragma zpsym ("zp_ptr");
#pragma zpsym ("zp_vram_ptr");
#pragma zpsym ("zp_len");
#pragma zpsym ("zp_x");
#pragma zpsym ("zp_y");
#pragma zpsym ("zp_char");


/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

// (in text_ml.asm)
//! Calculate the VRAM location of the specified coordinate, update VICKY cursor pos
//!  Set zp_x, zp_y before calling
void __fastcall__ Text_SetMemLocForXY(void);

//! Validate the coordinates are within the bounds of the specified screen. 
//! @param	x - the horizontal position to validate. Must be between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position to validate. Must be between 0 and the screen's text_rows_vis_ - 1
bool Text_ValidateXY(int8_t x, int8_t y);

// Fill attribute or text char memory. Writes to char memory if for_attr is false.
// calling function must validate the screen ID before passing!
//! @return	Returns false on any error/invalid input.
bool Text_FillMemory(bool for_attr, uint8_t the_fill);

//! Fill character and attribute memory for a specific box area
//! calling function must validate coords, attribute value before passing!
//! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	width - width, in character cells, of the rectangle to be filled
//! @param	height - height, in character cells, of the rectangle to be filled
//! @param	the_attribute_value - a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_FillMemoryBoxBoth(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t the_char, uint8_t the_attribute_value);

//! Fill character OR attribute memory for a specific box area
//! calling function must validate screen id, coords, attribute value before passing!
//! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	width - width, in character cells, of the rectangle to be filled
//! @param	height - height, in character cells, of the rectangle to be filled
//! @param	for_attr - true to work with attribute data, false to work character data. Recommend using PARAM_FOR_TEXT_ATTR/PARAM_FOR_TEXT_CHAR.
//! @param	the_fill - either a 1-byte character code, or a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_FillMemoryBox(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool for_attr, uint8_t the_fill);

/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/

// **** NOTE: all functions in private section REQUIRE pre-validated parameters. 
// **** NEVER call these from your own functions. Always use the public interface. You have been warned!


//! Fill attribute or text char memory. 
//! @param	for_attr - true to work with attribute data, false to work character data. Recommend using PARAM_FOR_TEXT_ATTR/PARAM_FOR_TEXT_CHAR.
//! @param	the_fill - either a 1-byte character code, or a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_FillMemory(bool for_attr, uint8_t the_fill)
{
	uint8_t*	the_write_loc;
	
	// LOGIC: 
	//   On F256-classic, the write locs are same for char and attr memory, difference is IO page 2 or 3
	//   On F256-extended, the write locs are different for char and attr memory, as E loads use flat memory map

	if (for_attr)
	{
		Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
	}
	else
	{
		Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	}

	the_write_loc = (uint8_t*)SCREEN_TEXT_MEMORY_LOC;
	memset(the_write_loc, the_fill, SCREEN_TOTAL_BYTES);
		
	Sys_RestoreIOPage();

	//printf("Text_FillMemory: done \n");
	//DEBUG_OUT(("%s %d: done (for_attr=%u, the_fill=%u)", __func__, __LINE__, for_attr, the_fill));

	return true;
}


//! Fill character and attribute memory for a specific box area
//! calling function must validate screen id, coords, attribute value before passing!
//! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	width - width, in character cells, of the rectangle to be filled
//! @param	height - height, in character cells, of the rectangle to be filled
//! @param	the_attribute_value - a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_FillMemoryBoxBoth(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t the_char, uint8_t the_attribute_value)
{
	uint8_t		max_row;

	// set up initial loc
	Text_SetXY(x,y);

	// LOGIC: 
	//   On F256-classic, the write locs are same for char and attr memory, difference is IO page 2 or 3
	//   On F256-extended, the write locs are different for char and attr memory, as E loads use flat memory map
	
	max_row = (y + height) - 1; // we are passing '1' for a single h row
	
	for (; y <= max_row; y++)
	{
		Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
		memset(zp_vram_ptr, the_attribute_value, width);
		Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
		memset(zp_vram_ptr, the_char, width);

		zp_vram_ptr += SCREEN_NUM_COLS;
	}

	Sys_RestoreIOPage();

	// reset current vram loc to match x,y
	Text_SetXY(x,y);		

	return true;
}


//! Fill character OR attribute memory for a specific box area
//! calling function must validate coords, attribute value before passing!
//! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	width - width, in character cells, of the rectangle to be filled
//! @param	height - height, in character cells, of the rectangle to be filled
//! @param	for_attr - true to work with attribute data, false to work character data. Recommend using PARAM_FOR_TEXT_ATTR/PARAM_FOR_TEXT_CHAR.
//! @param	the_fill - either a 1-byte character code, or a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_FillMemoryBox(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool for_attr, uint8_t the_fill)
{
	uint8_t		max_row;

	// set up initial loc
	Text_SetXY(x,y);

	// LOGIC: 
	//   On F256-classic, the write locs are same for char and attr memory, difference is IO page 2 or 3
	//   On F256-extended, the write locs are different for char and attr memory, as E loads use flat memory map

	if (for_attr)
	{
		Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
	}
	else
	{
		Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	}

	max_row = (y + height) - 1; // we are passing '1' for a single h row
	
	for (; y <= max_row; y++)
	{
		memset(zp_vram_ptr, the_fill, width);
		zp_vram_ptr += SCREEN_NUM_COLS;
	}
		
	Sys_RestoreIOPage();
		
	// reset current vram loc to match x,y
	Text_SetXY(x,y);		
			
	return true;
}




/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/

// ** NOTE: there is no destructor or constructor for this library, as it does not track any allocated memory. It works on the basis of a screen ID, which corresponds to the text memory for Vicky's Channel A and Channel B video memory.


// **** Block copy functions ****


// //! Copies characters and attributes from the left, to the right, for the passed length, backfilling with the char and attr passed
// //!   Shift never extends beyond the current row of text. 
// //! @param	working_buffer - valid pointer to a block of memory at least SCREEN_NUM_COLS in size, to act as a temporary line buffer for the operation. 
// //! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	shift_count - the number of character positions text will be shifted. eg, '1' will shift everything to the right of x by 1 character.
// //! @param	backfill_char - the character to place in the space freed up by copy. eg, if you shift 10 chars at positions 60-69 to 70-79, this char will be used to fill the slots from 60-69. 
// //! @param	backfill_fore_color - foreground color that will be applied to the space opened up by the copy
// //! @param	backfill_back_color - background color that will be applied to the space opened up by the copy
// //! @return	Returns false on any error/invalid input.
// bool Text_ShiftTextAndAttrRight(uint8_t* working_buffer, uint8_t x, uint8_t y, uint8_t shift_count, uint8_t backfill_char, uint8_t backfill_fore_color, uint8_t backfill_back_color)
// {
// 	uint8_t*		vram_to_loc;
// 	uint8_t*		vram_from_loc;
// 	int16_t			initial_offset;
// 	uint8_t			the_length;
// 	uint8_t			the_attribute_value;
// 	
// 	// calculate attribute value from passed fore and back colors
// 	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
// 	the_attribute_value = ((backfill_fore_color << 4) | backfill_back_color);
// 
// 	// LOGIC: 
//	//   On F256-classic, the write locs are same for char and attr memory, difference is IO page 2 or 3
//	//   On F256-extended, the write locs are different for char and attr memory, as E loads use flat memory map
// 
// 	// check for valid inputs to ensure we are only adjusting 1 valid line worth of text
// 	if (y > SCREEN_LAST_ROW)
// 	{
// 		return false;
// 	}
// 	if (x > SCREEN_LAST_COL)	// allow position 79 (eg) to be shifted off the screen
// 	{
// 		return false;
// 	}
// 	if ((x + shift_count) > SCREEN_LAST_COL)
// 	{
// 		// can't shift more right than the end of the row
// 		shift_count = (SCREEN_LAST_COL - x) + 1;
// 	}
// 		
// 	// get initial read/write locs - copy from right most character, not left-most.
// 	the_length = (SCREEN_NUM_COLS - x) - shift_count;	// if x=70, and want to shift 5 chars to right, len can't be 10, len must be 5 or we'll overwrite next line
// 	initial_offset = (SCREEN_NUM_COLS * y) + x;
// 
// 	// copy text
// 	vram_from_loc = (uint8_t*)VICKY_TEXT_CHAR_RAM + initial_offset;
// 	vram_to_loc = vram_from_loc + shift_count;
// 	memcpy(working_buffer, vram_from_loc, the_length);
// 	memcpy(vram_to_loc, working_buffer, the_length);
// 	// backfill text
// 	memset(vram_from_loc, backfill_char, shift_count);
// 
// 	
// 	// copy attr
// 	vram_from_loc = (uint8_t*)VICKY_TEXT_ATTR_RAM + initial_offset;
// 	vram_to_loc = vram_from_loc + shift_count;
// 	memcpy(working_buffer, vram_from_loc, the_length);
// 	memcpy(vram_to_loc, working_buffer, the_length);
// 	// backfill attr
// 	memset(vram_from_loc, the_attribute_value, shift_count);
// 	
// 	return true;
// }


// //! Copies characters and attributes from the right, to the left, for the passed length, backfilling with the char and attr passed
// //!   Shift never extends beyond the current row of text. 
// //! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	shift_count - the number of character positions text will be shifted. eg, '1' will shift everything to the left of x by 1 character.
// //! @param	backfill_char - the character to place in the space freed up by copy. eg, if you shift 10 chars at positions 70-79 to 60-69, this char will be used to fill the slots from 70-79. 
// //! @param	backfill_fore_color - foreground color that will be applied to the space opened up by the copy
// //! @param	backfill_back_color - background color that will be applied to the space opened up by the copy
// //! @return	Returns false on any error/invalid input.
// bool Text_ShiftTextAndAttrLeft(uint8_t x, uint8_t y, uint8_t shift_count, uint8_t backfill_char, uint8_t backfill_fore_color, uint8_t backfill_back_color)
// {
// 	uint8_t*		vram_to_loc;
// 	uint8_t*		vram_from_loc;
// 	int16_t			initial_offset;
// 	uint8_t			the_length;
// 	uint8_t			the_attribute_value;
// 
// 	// calculate attribute value from passed fore and back colors
// 	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
// 	the_attribute_value = ((backfill_fore_color << 4) | backfill_back_color);
// 
// 	// LOGIC: 
// 	//   On F256jr, the write len and write locs are same for char and attr memory, difference is IO page 2 or 3
// 
// 	// check for valid inputs to ensure we are only adjusting 1 valid line worth of text
// 	if (y > SCREEN_LAST_ROW)
// 	{
// 		return false;
// 	}
// 	if (x == 0 || x > SCREEN_LAST_COL)
// 	{
// 		return false;
// 	}
// 	if (x < shift_count)
// 	{
// 		// can't shift more left than the start of the row
// 		shift_count = x;
// 	}
// 		
// 	// get initial read/write locs
// 	the_length = SCREEN_NUM_COLS - x;
// 	initial_offset = (SCREEN_NUM_COLS * y) + x;
// 
// 	// copy text
// 	vram_from_loc = (uint8_t*)VICKY_TEXT_CHAR_RAM + initial_offset;
// 	vram_to_loc = vram_from_loc - shift_count;	
// 	memcpy(vram_to_loc, vram_from_loc, the_length);
// 	// backfill text
// 	vram_from_loc += (the_length - 1);
// 	memset(vram_from_loc, backfill_char, shift_count);
// 	
// 	// copy attr
// 	vram_from_loc = (uint8_t*)VICKY_TEXT_ATTR_RAM + initial_offset;
// 	vram_to_loc = vram_from_loc - shift_count;
// 	memcpy(vram_to_loc, vram_from_loc, the_length);
// 	// backfill attr
// 	vram_from_loc += (the_length - 1);
// 	memset(vram_from_loc, the_attribute_value, shift_count);
// 	
// 	return true;
// }


// //! scrolls the text and attribute memory up ONE row.
// //!   e.g, row 0 is lost. row 1 becomes row 0, row 49 becomes row 48, row 49 is cleared.
// //! @param	y1 - the first row to scroll up
// //! @param	y2 - the last row to scroll up
// //! @return	Returns false on any error/invalid input.
// bool Text_ScrollTextAndAttrRowsUp(uint8_t y1, uint8_t y2)
// {
// 	uint8_t*		vram_to_loc;
// 	uint8_t*		vram_from_loc;
// 	int16_t			initial_offset;
// 	uint8_t			num_rows;
// 	uint8_t			i;
// 
// 	// LOGIC: 
// 	//   On F256-classic, the write locs are same for char and attr memory, difference is IO page 2 or 3
// 	//   On F256-extended, the write locs are different for char and attr memory, as E loads use flat memory map
// 
// 	// adjust the x, y, x2, y2, so that we are never trying to copy out of the physical screen box
// 	if (y1 < 1)
// 	{
// 		y1 = 1;	// can't scroll row 0 anywhere useful.
// 	}
// 	else if (y1 > SCREEN_LAST_ROW)
// 	{
// 		y1 = SCREEN_LAST_ROW;
// 	}
// 	if (y2 < y1)
// 	{
// 		y2 = y1; // ok to scroll 1 row, so this is compromise for bad data.
// 	}
// 	else if (y2 > SCREEN_LAST_ROW)
// 	{
// 		y2 = SCREEN_LAST_ROW;
// 	}
// 		
// 	// get initial read/write locs
// 	initial_offset = (SCREEN_NUM_COLS * y1);
// 	num_rows = y2 - y1 + 1;
// 
// 	vram_from_loc = (uint8_t*)SCREEN_TEXT_MEMORY_LOC + initial_offset;
// 	vram_to_loc = vram_from_loc - SCREEN_NUM_COLS;
// 	
// 	for (i = 0; i < num_rows; i++)
// 	{
// 		Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
// 		memcpy(vram_to_loc, vram_from_loc, SCREEN_NUM_COLS);
// 		Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
// 		memcpy(vram_to_loc, vram_from_loc, SCREEN_NUM_COLS);
// 		
// 		vram_to_loc = vram_from_loc;
// 		vram_from_loc += SCREEN_NUM_COLS;
// 	}
// 		
// 	Sys_RestoreIOPage();
// 
// 	return true;
// }


// //! scrolls the text and attribute memory down ONE row.
// //!   e.g, row 59 is lost. row 58 becomes row 59, row 0 becomes row 1, row 0 is cleared.
// //! @param	y1 - the first row to scroll down
// //! @param	y2 - the last row to scroll down. y2+1 is overwritten, y2+2 and beyond are left as is.
// //! @return	Returns false on any error/invalid input.
// bool Text_ScrollTextAndAttrRowsDown(uint8_t y1, uint8_t y2)
// {
// 	uint8_t*		vram_to_loc;
// 	uint8_t*		vram_from_loc;
// 	int16_t			initial_offset;
// 	uint8_t			num_rows;
// 	uint8_t			i;
// 
// 	// LOGIC: 
// 	//   On F256jr, the write len and write locs are same for char and attr memory, difference is IO page 2 or 3
// 
// 	// adjust the x, y, x2, y2, so that we are never trying to copy out of the physical screen box
// 	if (y2 == SCREEN_LAST_ROW)
// 	{
// 		y2 = SCREEN_LAST_ROW - 1;	// can't scroll row 59 anywhere useful.
// 	}
// 	else if (y2 < 1)
// 	{
// 		y2 = 1;
// 	}
// 	if (y2 < y1)
// 	{
// 		y2 = y1; // ok to scroll 1 row, so this is compromise for bad data.
// 	}
// 		
// 	// get initial read/write locs
// 	initial_offset = (SCREEN_NUM_COLS * y2);
// 	num_rows = y2 - y1 + 1;
// 
// 	vram_from_loc = (uint8_t*)VICKY_TEXT_CHAR_RAM + initial_offset;
// 	vram_to_loc = vram_from_loc + SCREEN_NUM_COLS;
// 	
// 	for (i = 0; i < num_rows; i++)
// 	{
// 		memcpy(vram_to_loc, vram_from_loc, SCREEN_NUM_COLS);
// 		
// 		vram_to_loc = vram_from_loc;
// 		vram_from_loc -= SCREEN_NUM_COLS;
// 	}
// 	
// 	vram_from_loc = (uint8_t*)VICKY_TEXT_ATTR_RAM + initial_offset;
// 	vram_to_loc = vram_from_loc + SCREEN_NUM_COLS;
// 
// 	for (i = 0; i < num_rows; i++)
// 	{
// 		memcpy(vram_to_loc, vram_from_loc, SCREEN_NUM_COLS);
// 		
// 		vram_to_loc = vram_from_loc;
// 		vram_from_loc -= SCREEN_NUM_COLS;
// 	}
// 	
// 	return true;
// }


#if defined DO_NOT_HIDE_ME_BRO

//! Copy a linear run of text or attr to or from a linear memory buffer.
//!   Use this if you do not have a full-sized (screen-size) off-screen buffer, and do not have a rectangular area 
//!   of the screen to copy to/from, but instead want to copy a single linear stream to/from a particular cursor position. 
//! @param	the_buffer - valid pointer to a block of memory to hold (or alternatively act as the source of) the character or attribute data for the specified screen memory. This will be read from first byte to last byte, without skipping. e.g., if you want to copy a 227 characters of text from the middle of the screen to this buffer, the buffer must be 227 bytes in length, and data will be written contiguously to it. 
//! @param	x - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	to_screen - true to copy to the screen from the buffer, false to copy from the screen to the buffer. Recommend using PARAM_COPY_TO_SCREEN/PARAM_COPY_FROM_SCREEN.
//! @param	for_attr - true to work with attribute data, false to work character data. Recommend using PARAM_FOR_TEXT_ATTR/PARAM_FOR_TEXT_CHAR.
//! @return	Returns false on any error/invalid input.
bool Text_CopyMemLinearBuffer(uint8_t* the_buffer, uint8_t x, uint8_t y, uint16_t the_len, bool to_screen, bool for_attr)
{
	uint8_t*	the_vram_loc;
	uint8_t*	the_buffer_loc;
	int16_t		initial_offset;

	// LOGIC: 
	//   On F256jr, the write len and write locs are same for char and attr memory, difference is IO page 2 or 3

	// adjust the x, y to ensure we have legitimate positions
	if (x > SCREEN_LAST_COL)
	{
		x = SCREEN_LAST_COL;
	}
	if (y > SCREEN_LAST_ROW)
	{
		y = SCREEN_LAST_ROW;
	}
		
	// get initial read/write locs
	initial_offset = (SCREEN_NUM_COLS * y) + x;
	
	// prevent writing past end of screen memory
	if (initial_offset + the_len > 2000)
	{
		the_len = 2000 - initial_offset;
	}
	

	// LOGIC: 
	//   On F256jr/k, the write locs are same for char and attr memory, difference is IO page 2 or 3
	//   On F256k2, the write locs are different for char and attr memory, as k2 uses flat memory map

	if (for_attr)
	{
		the_vram_loc = (uint8_t*)(VICKY_TEXT_ATTR_RAM + initial_offset);
	}
	else
	{
		the_vram_loc = (uint8_t*)(VICKY_TEXT_CHAR_RAM + initial_offset);
	}

	the_buffer_loc = the_buffer;

	// do copy one line at a time	

//DEBUG_OUT(("%s %d: vramloc=%p, buffer=%p, bufferloc=%p, to_screen=%i, the_write_len=%i", the_vram_loc, the_buffer, the_buffer_loc, to_screen, the_write_len));

	if (to_screen)
	{
		memcpy(the_vram_loc, the_buffer_loc, the_len);
	}
	else
	{
		memcpy(the_buffer_loc, the_vram_loc, the_len);
	}

	return true;
}

#endif


//! Copy a rectangular area of text or attr to or from a linear memory buffer.
//!   Use this if you do not have a full-sized (screen-size) off-screen buffer, but instead have a block perhaps just big enough to hold the rect.
//! @param	the_buffer - valid pointer to a block of memory to hold (or alternatively act as the source of) the character or attribute data for the specified rectangle of screen memory. This will be read from first byte to last byte, without skipping. e.g., if you want to copy a 40x5 rectangle of text from the middle of the screen to this buffer, the buffer must be 40*5=200 bytes in length, and data will be written contiguously to it. 
//! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	to_screen - true to copy to the screen from the buffer, false to copy from the screen to the buffer. Recommend using PARAM_COPY_TO_SCREEN/PARAM_COPY_FROM_SCREEN.
//! @param	for_attr - true to work with attribute data, false to work character data. Recommend using PARAM_FOR_TEXT_ATTR/PARAM_FOR_TEXT_CHAR.
//! @return	Returns false on any error/invalid input.
bool Text_CopyMemBoxLinearBuffer(uint8_t* the_buffer, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool to_screen, bool for_attr)
{
	uint8_t*	orig_vram_loc;
	uint8_t*	the_buffer_loc;
	uint8_t		the_write_len;

	// adjust the x, y, x2, y2, so that we are never trying to copy out of the physical screen box
	if (x2 < x1)
	{
		x2 = x1;
	}
	if (y2 < y1)
	{
		y2 = y1;
	}
	if (x1 > SCREEN_LAST_COL)
	{
		return false;
	}
	if (x2 > SCREEN_LAST_COL)
	{
		x2 = SCREEN_LAST_COL;
	}
	if (y1 > SCREEN_LAST_ROW)
	{
		return false;
	}
	if (y2 > SCREEN_LAST_ROW)
	{
		y2 = SCREEN_LAST_ROW;
	}

	// set up initial loc
	orig_vram_loc = zp_vram_ptr;
	Text_SetXY(x1,y1);

	// LOGIC: 
	//   On F256-classic, the write locs are same for char and attr memory, difference is IO page 2 or 3
	//   On F256-extended, the write locs are different for char and attr memory, as E loads use flat memory map

	if (for_attr)
	{
		Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
	}
	else
	{
		Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	}
		
	the_buffer_loc = the_buffer;
	the_write_len = x2 - x1 + 1;
	
	// do copy one line at a time	

//DEBUG_OUT(("%s %d: vramloc=%p, buffer=%p, bufferloc=%p, to_screen=%i, the_write_len=%i", the_vram_loc, the_buffer, the_buffer_loc, to_screen, the_write_len));

	for (; y1 <= y2; y1++)
	{
		if (to_screen)
		{
			memcpy(zp_vram_ptr, the_buffer_loc, the_write_len);
		}
		else
		{
			memcpy(the_buffer_loc, zp_vram_ptr, the_write_len);
		}

		the_buffer_loc += the_write_len;
		zp_vram_ptr += SCREEN_NUM_COLS;
	}
		
	Sys_RestoreIOPage();

	// restore screen addr
	zp_vram_ptr = orig_vram_loc;

	return true;
}


#if defined DO_NOT_HIDE_ME_BRO

//! Copy a rectangular area of text or attr to or from an off-screen buffer of the same size as the physical screen buffer
//! @param	the_buffer - valid pointer to a block of memory to hold (or alternatively act as the source of) the character or attribute data for the specified rectangle of screen memory. This buffer must be the same size as the physical screen!
//! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	to_screen - true to copy to the screen from the buffer, false to copy from the screen to the buffer. Recommend using PARAM_COPY_TO_SCREEN/PARAM_COPY_FROM_SCREEN.
//! @param	for_attr - true to work with attribute data, false to work character data. Recommend using PARAM_FOR_TEXT_ATTR/PARAM_FOR_TEXT_CHAR.
//! @return	Returns false on any error/invalid input.
bool Text_CopyMemBox(uint8_t* the_buffer, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool to_screen, bool for_attr)
{
	uint8_t*		the_vram_loc;
	uint8_t*		the_buffer_loc;
	uint8_t			the_write_len;
	int16_t			initial_offset;

	// LOGIC: 
	//   On F256jr, the write len and write locs are same for char and attr memory, difference is IO page 2 or 3

	if (for_attr)
	{
		Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
	}
	else
	{
		Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	}
		
	// get initial read/write locs
	initial_offset = (SCREEN_NUM_COLS * y1) + x1;
	the_buffer_loc = the_buffer + initial_offset;
	the_write_len = x2 - x1 + 1;

	the_vram_loc = (uint8_t*)SCREEN_TEXT_MEMORY_LOC + initial_offset;
	
	// do copy one line at a time	

//DEBUG_OUT(("%s %d: vramloc=%p, buffer=%p, bufferloc=%p, to_screen=%i, the_write_len=%i", the_vram_loc, the_buffer, the_buffer_loc, to_screen, the_write_len));

	for (; y1 <= y2; y1++)
	{
		if (to_screen)
		{
			memcpy(the_vram_loc, the_buffer_loc, the_write_len);
		}
		else
		{
			memcpy(the_buffer_loc, the_vram_loc, the_write_len);
		}
		
		the_buffer_loc += SCREEN_NUM_COLS;
		the_vram_loc += SCREEN_NUM_COLS;
	}
		
	Sys_RestoreIOPage();

	return true;
}

#endif



// **** Block fill functions ****



//! Clear the text screen and reset foreground and background colors
void Text_ClearScreen(uint8_t fore_color, uint8_t back_color)
{
	uint8_t			the_attribute_value;

	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);
	//DEBUG_OUT(("%s %d: the_attribute_value=%u", __func__, __LINE__, the_attribute_value));

	Text_FillMemory(PARAM_FOR_TEXT_CHAR, ' ');
	Text_FillMemory(PARAM_FOR_TEXT_ATTR, the_attribute_value);
}


//! Fill character and attribute memory for a specific box area
//! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_char - the character to be used for the fill operation
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_FillBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t the_char, uint8_t fore_color, uint8_t back_color)
{
	uint8_t		dy;
	uint8_t		dx;
	uint8_t		the_attribute_value;

 	// add 1 to H line len, because dx becomes width, and if width = 0, then memset gets 0, and nothing happens.
	// same for dy, as we account for that in the next function called
	dx = x2 - x1 + 1;
	dy = y2 - y1 + 1;

	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);

	return Text_FillMemoryBoxBoth(x1, y1, dx, dy, the_char, the_attribute_value);
}


// //! Fill character memory for a specific box area
// //! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	the_char - the character to be used for the fill operation
// //! @return	Returns false on any error/invalid input.
// bool Text_FillBoxCharOnly(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t the_char)
// {
// 	uint8_t		dy;
// 	uint8_t		dx;
// 	
// 	if (x1 > x2 || y1 > y2)
// 	{
// 		LOG_ERR(("%s %d: illegal coordinates", __func__, __LINE__));
// 		return false;
// 	}
// 
//  	// add 1 to H line len, because dx becomes width, and if width = 0, then memset gets 0, and nothing happens.
// 	// same for dy, as we account for that in the next function called
// 	dx = x2 - x1 + 1;
// 	dy = y2 - y1 + 1;
// 
// 	return Text_FillMemoryBox(x1, y1, dx, dy, PARAM_FOR_TEXT_CHAR, the_char);
// }


//! Fill attribute memory for a specific box area
//! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_FillBoxAttrOnly(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fore_color, uint8_t back_color)
{
	uint8_t			dy;
	uint8_t			dx;
	uint8_t			the_attribute_value;
	
	if (x1 > x2 || y1 > y2)
	{
		LOG_ERR(("%s %d: illegal coordinates", __func__, __LINE__));
		return false;
	}

 	// add 1 to H line len, because dx becomes width, and if width = 0, then memset gets 0, and nothing happens.
	// same for dy, as we account for that in the next function called
	dx = x2 - x1 + 1;
	dy = y2 - y1 + 1;

	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);

	return Text_FillMemoryBox(x1, y1, dx, dy, PARAM_FOR_TEXT_ATTR, the_attribute_value);
}


// //! Invert the colors of a rectangular block.
// //! As this requires sampling each character cell, it is no faster (per cell) to do for entire screen as opposed to a subset box
// //! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @return	Returns false on any error/invalid input.
// bool Text_InvertBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
// {
// 	uint8_t			the_attribute_value;
// 	uint8_t			the_inversed_value;
// 	uint8_t			the_col;
// 	uint8_t			skip_len;
// 	uint8_t			back_nibble;
// 	uint8_t			fore_nibble;
// 	uint8_t*		the_write_loc;
// 	
// 	// get initial read/write loc
// 	Text_SetXY(x1,y1);
// 	the_write_loc = zp_vram_ptr;
// 	
// 	// amount of cells to skip past once we have written the specified line len
// 	skip_len = SCREEN_NUM_COLS - (x2 - x1) - 1;
// 
// 	Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
// 	
// 	for (; y1 <= y2; y1++)
// 	{
// 		for (the_col = x1; the_col <= x2; the_col++)
// 		{
// 			the_attribute_value = R8(the_write_loc);
// 			
// 			// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
// 			back_nibble = ((the_attribute_value & 0xF0) >> 4);
// 			fore_nibble = ((the_attribute_value & 0x0F) << 4);
// 			the_inversed_value = (fore_nibble | back_nibble);
// 			
// 			*the_write_loc++ = the_inversed_value;
// 		}
// 
// 		the_write_loc += skip_len;
// 	}
// 		
// 	Sys_RestoreIOPage();
// 	
// 	// note: for this function, we will not update the next write VRAM address to the point after the lower right corner.
// 
// 	return true;
// }






// **** Cursor positioning functions *****

//! Move cursor to the specified x, y coord
//! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
void Text_SetXY(uint8_t x, uint8_t y)
{
// 	uint16_t	initial_offset;
// 	
// 	// LOGIC:
// 	//   For plotting the VRAM, VICKY uses the full width, regardless of borders. 
// 	//   So even if only 72 are showing, the screen is arranged from 0-71 for row 1, then 80-151 for row 2, etc. 
// 	//   For F256K/JR non-flat memory loads, char ram and attr ram are at same address, and only difference is which I/O bank is being used
// 	//   For F256K/JR/K2 with flat memory loads, char and ram are at different addresses
// 	//   the file-scoped current x/y are always set when this function is called, regardless if for attr or char
// 	//   the file-scoped current memory address is also always set, but only ever points to the char memory, not attr memory. 
// 	
// 	initial_offset = (SCREEN_NUM_COLS * y) + x;
// 	
// 	// save the new current address, x, y position, and also tell VICKY where the cursor should be
// 	zp_vram_ptr = (uint8_t*)SCREEN_TEXT_MEMORY_LOC + initial_offset;
// 	zp_x = x;
// 	zp_y = y;
// 
// 	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);
// 	R8(VICKY_TEXT_X_POS) = zp_x;
// 	R8(VICKY_TEXT_Y_POS) = zp_y;
// 	Sys_RestoreIOPage();

	zp_x = x;
	zp_y = y;
	Text_SetMemLocForXY();
}


#if defined DO_NOT_HIDE_ME_BRO

//! Return the current cursor location's X coordinate
//! @return	the horizontal position, between 0 and the screen's text_cols_vis_ - 1
uint8_t Text_GetX(void)
{
	return zp_x;
}

#endif


#if defined DO_NOT_HIDE_ME_BRO

//! Return the current cursor location's Y coordinate
//! @return	the vertical position, between 0 and the screen's text_rows_vis_ - 1
uint8_t Text_GetY(void)
{
	return zp_y;
}

#endif


// **** Set char/attr functions *****

// NOTE: all functions from here lower that pass an x/y will update the zp_x/zp_y parameters.

//! Set a char at a specified x, y coord
//! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_char - the character to be used
//! @return	Returns false on any error/invalid input.
bool Text_SetCharAtXY(uint8_t x, uint8_t y, uint8_t the_char)
{
	Text_SetXY(x, y);
	Text_SetChar(the_char);
		
	return true;
}


#if defined DO_NOT_HIDE_ME_BRO

//! Set the attribute value at a specified x, y coord
//! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_attribute_value - a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_SetAttrAtXY(uint8_t x, uint8_t y, uint8_t the_attribute_value)
{
	Text_SetXY(x, y);
	Text_SetAttr(the_attribute_value);
		
	return true;
}

#endif


//! Set the attribute value at a specified x, y coord based on the foreground and background colors passed
//! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_SetColorAtXY(uint8_t x, uint8_t y, uint8_t fore_color, uint8_t back_color)
{
	uint8_t			the_attribute_value;

	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);

	Text_SetXY(x, y);
	Text_SetAttr(the_attribute_value);

	return true;
}


#if defined DO_NOT_HIDE_ME_BRO

//! Draw a char at a specified x, y coord, also setting the color attributes
//! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_char - the character to be used
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_SetCharAndAttrAtXY(uint8_t x, uint8_t y, uint8_t the_char, uint8_t the_attribute_value)
{
	uint8_t*		the_write_loc;

	the_write_loc = Text_GetMemLocForXY(x, y);	
	
	Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
	*the_write_loc = the_attribute_value;
	Sys_RestoreIOPage();

	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	*the_write_loc = the_char;
	Sys_RestoreIOPage();

	zp_x = x;
	zp_y = y;
	
	return true;
}

#endif



//! Draw a char at a specified x, y coord, also setting the color attributes, and advance cursor position by 1
//! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_char - the character to be used
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_SetCharAndColorAtXY(uint8_t x, uint8_t y, uint8_t the_char, uint8_t fore_color, uint8_t back_color)
{
	Text_SetXY(x, y);
	Text_SetCharAndColor(the_char, fore_color, back_color);

	return true;
}


//! Copy n-bytes into display memory, at the X/Y position specified
void Text_DrawCharsAtXY(uint8_t x, uint8_t y, uint8_t* the_buffer, uint16_t the_len)
{
	Text_SetXY(x, y);
	zp_ptr = the_buffer;
	Text_DrawChars(the_len);
}


// //! Set a char at the current X/Y position, and advance cursor position by 1
// //! @param	the_char - the character to be used
// //! @return	Returns false on any error/invalid input.
// bool Text_SetCharC(uint8_t the_char)
// {
// 	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
// 	*zp_vram_ptr = the_char;
// 	Sys_RestoreIOPage();
// 
// 	zp_vram_ptr++;
// 	zp_x++;
// 	
// 	// bounds check. would be nicer to JSR to this but that's expensive in C, so just copying this everywhere...
// 	
// 	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);
// 
// 	if (zp_x > SCREEN_LAST_COL && zp_y < SCREEN_LAST_ROW)
// 	{
// 		zp_x = 0;
// 		zp_y++;
// 		R8(VICKY_TEXT_Y_POS) = zp_y;
// 	}
// 	
// 	R8(VICKY_TEXT_X_POS) = zp_x;
// 
// 	Sys_RestoreIOPage();
// 		
// 	return true;
// }


//! Set the attribute value at the current X/Y position, and advance cursor position by 1
//! @param	the_attribute_value - a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_SetAttr(uint8_t the_attribute_value)
{
	Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
	*zp_vram_ptr = the_attribute_value;
	Sys_RestoreIOPage();

	zp_vram_ptr++;
	zp_x++;
	
	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);

	// bounds check. would be nicer to JSR to this but that's expensive in C, so just copying this everywhere...
	if (zp_x > SCREEN_LAST_COL && zp_y < SCREEN_LAST_ROW)
	{
		zp_x = 0;
		zp_y++;
		R8(VICKY_TEXT_Y_POS) = zp_y;
	}
	
	R8(VICKY_TEXT_X_POS) = zp_x;

	Sys_RestoreIOPage();
	
	return true;
}


#if defined DO_NOT_HIDE_ME_BRO

//! Set the attribute value at the current X/Y position based on the foreground and background colors passed, and advance cursor position by 1
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_SetColor(uint8_t fore_color, uint8_t back_color)
{
	uint8_t			the_attribute_value;

	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);

	return Text_SetAttr(the_attribute_value);
}

#endif


//! Draw a char at the current X/Y position, also setting the color attributes, and advance cursor position by 1
//! @param	the_char - the character to be used
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_SetCharAndColor(uint8_t the_char, uint8_t fore_color, uint8_t back_color)
{
	uint8_t			the_attribute_value;
			
	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);

	Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
	*zp_vram_ptr = the_attribute_value;
	Sys_RestoreIOPage();

	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	*zp_vram_ptr = the_char;
	Sys_RestoreIOPage();

	zp_vram_ptr++;
	zp_x++;
	
	// bounds check. would be nicer to JSR to this but that's expensive in C, so just copying this everywhere...
	
	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);

	if (zp_x > SCREEN_LAST_COL && zp_y < SCREEN_LAST_ROW)
	{
		zp_x = 0;
		zp_y++;
		R8(VICKY_TEXT_Y_POS) = zp_y;
	}
	
	R8(VICKY_TEXT_X_POS) = zp_x;

	Sys_RestoreIOPage();
	
	return true;
}


// // copy n-bytes into display memory, at the current X/Y position
// bool Text_DrawCharsOLD(uint8_t* the_buffer, uint16_t the_len)
// {
// 	// draw the string
// 	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
// 	memcpy(zp_vram_ptr, the_buffer, the_len);
// 	Sys_RestoreIOPage();
// 
// 	zp_y = zp_y + (uint8_t)(the_len / SCREEN_NUM_COLS);
// 	zp_x = zp_x + (uint8_t)(the_len - ((the_len / SCREEN_NUM_COLS) * SCREEN_NUM_COLS));
// 	zp_vram_ptr += the_len;
// 	
// 	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);
// 	R8(VICKY_TEXT_X_POS) = zp_x;
// 	R8(VICKY_TEXT_Y_POS) = zp_y;
// 	Sys_RestoreIOPage();
// 
// 	return true;
// }




// **** FONT RELATED *****


#if defined DO_NOT_HIDE_ME_BRO

//! replace the current font data with the data at the passed memory buffer
//! @param	new_font_data - Pointer to 2K (256 characters x 8 lines/bytes each) of font data. Each byte represents one line of an 8x8 font glyph.
//! @param	for_primary_font - true to update the primary font, false to update the secondary font. Recommend using PARAM_USE_PRIMARY_FONT_SLOT/PARAM_USE_SECONDARY_FONT_SLOT.
//! @return	Returns false on any error/invalid input.
bool Text_UpdateFontData(char* new_font_data, bool for_primary_font)
{
	Sys_SwapIOPage(VICKY_IO_PAGE_FONT_AND_LUTS);

	if (for_primary_font == PARAM_USE_PRIMARY_FONT_SLOT)
	{
		memcpy((uint8_t*)FONT_MEMORY_BANK0, new_font_data, 2048);
	}
	else
	{
		memcpy((uint8_t*)FONT_MEMORY_BANK1, new_font_data, 2048);
	}
		
	Sys_RestoreIOPage();

	return true;
}

#endif




// **** Get char/attr functions *****


#if defined DO_NOT_HIDE_ME_BRO

//! Get the char at the current X/Y position
//! @return	Returns the character code at the current screen location
uint8_t Text_GetChar(void)
{
	uint8_t		the_value;
	
	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	the_value = *zp_vram_ptr;
	Sys_RestoreIOPage();
	
	return the_value;
}

#endif


#if defined DO_NOT_HIDE_ME_BRO

//! Get the char at the current X/Y position - 1. Does not change what the current X/Y pos is.
//! @return	Returns the character code at the char preceeding the current screen location
uint8_t Text_GetPrevChar(void)
{
	uint8_t		the_value;
	
	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	the_value = *(zp_vram_ptr - 1);
	Sys_RestoreIOPage();
	
	return the_value;
}

#endif


#if defined DO_NOT_HIDE_ME_BRO

//! Get the char at the current X/Y position + 1. Does not change what the current X/Y pos is.
//! @return	Returns the character code at the char following the current screen location
uint8_t Text_GetNextChar(void)
{
	uint8_t		the_value;
	
	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	the_value =  *(zp_vram_ptr + 1);
	Sys_RestoreIOPage();
	
	return the_value;
}

#endif


#if defined DO_NOT_HIDE_ME_BRO

//! Get the char at the specified X/Y position, without changing the text engine's current position marker
//! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @return	Returns the character code at the specified screen location
uint8_t Text_GetCharAtXY(uint8_t x, uint8_t y)
{
	uint8_t	tempx;
	uint8_t	tempy;
	uint8_t	the_char;
	
	// LOGIC:
	//   stash previous x, y so we can restore it afterwards. we don't want GET functions to change current x,y info.
	
	tempx = zp_x;
	tempy = zp_y;
	
	Text_SetXY(x, y);

	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	the_char = *zp_vram_ptr;
	Sys_RestoreIOPage();

	Text_SetXY(tempx, tempy);

	return the_char;
}

#endif




// **** Drawing functions *****


//! Draws a horizontal line from specified coords, for n characters, using the specified char and/or attribute
//! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_line_len - The total length of the line, in characters, including the start and end character.
//! @param	the_char - the character to be used when drawing
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	the_draw_choice - controls the scope of the action, and is one of CHAR_ONLY, ATTR_ONLY, or CHAR_AND_ATTR. See the text_draw_choice enum.
void Text_DrawHLine(uint8_t x, uint8_t y, uint8_t the_line_len, uint8_t the_char, uint8_t fore_color, uint8_t back_color, uint8_t the_draw_choice)
{
	uint8_t			the_attribute_value;
	
	// LOGIC: 
	//   an H line is just a box with 1 row, so we can re-use Text_FillMemoryBox(Both)(). These routines use memset, so are quicker than for loops. 
	
	if (the_draw_choice == CHAR_ONLY)
	{
		Text_FillMemoryBox(x, y, the_line_len, 1, PARAM_FOR_TEXT_CHAR, the_char);
	}
	else
	{
		// calculate attribute value from passed fore and back colors
		// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground

		the_attribute_value = ((fore_color << 4) | back_color);
	
		if (the_draw_choice == ATTR_ONLY)
		{
			Text_FillMemoryBox(x, y, the_line_len, 1, PARAM_FOR_TEXT_ATTR, the_attribute_value);
		}
		else
		{
			Text_FillMemoryBoxBoth(x, y, the_line_len, 1, the_char, the_attribute_value);
		}
	}
}


//! Draws a vertical line from specified coords, for n characters, using the specified char and/or attribute
//! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_line_len - The total length of the line, in characters, including the start and end character.
//! @param	the_char - the character to be used when drawing
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	the_draw_choice - controls the scope of the action, and is one of CHAR_ONLY, ATTR_ONLY, or CHAR_AND_ATTR. See the text_draw_choice enum.
void Text_DrawVLine(uint8_t x, uint8_t y, uint8_t the_line_len, uint8_t the_char, uint8_t fore_color, uint8_t back_color, uint8_t the_draw_choice)
{
	uint8_t		dy;
	
	switch (the_draw_choice)
	{
		case CHAR_ONLY:
			for (dy = 0; dy < the_line_len; dy++)
			{
				Text_SetCharAtXY(x, y + dy, the_char);
			}
			break;
			
		case ATTR_ONLY:
			for (dy = 0; dy < the_line_len; dy++)
			{
				Text_SetColorAtXY(x, y + dy, fore_color, back_color);
			}
			break;
			
		case CHAR_AND_ATTR:
		default:
			for (dy = 0; dy < the_line_len; dy++)
			{
				Text_SetCharAndColorAtXY(x, y + dy, the_char, fore_color, back_color);		
			}
			break;			
	}
}


//! Draws a box based on 2 sets of coords, using the predetermined line and corner "graphics", and the passed colors
//! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
void Text_DrawBoxCoordsFancy(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fore_color, uint8_t back_color)
{
	uint8_t		dy;
	uint8_t		dx;
	
	//DEBUG_OUT(("%s %d: %u, %u x %u, %u", __func__, __LINE__, x1, y1, x2, y2));
	
// 	// add 1 to H line len, because dx becomes width, and if width = 0, then memset gets 0, and nothing happens.
	// dy can be 0 and you still get at least one row done.
	// but, for this, because of how we draw H line, do NOT add 1 to x1. see "x1+1" below... 
	dx = x2 - x1 + 0;
	dy = y2 - y1 + 0;
	
	// draw all lines one char shorter on each end so that we don't overdraw when we do corners
	
	Text_DrawHLine(x1+1, y1, dx, SC_HLINE, fore_color, back_color, CHAR_AND_ATTR);
	Text_DrawHLine(x1+1, y2, dx, SC_HLINE, fore_color, back_color, CHAR_AND_ATTR);
	Text_DrawVLine(x2, y1+1, dy, SC_VLINE, fore_color, back_color, CHAR_AND_ATTR);
	Text_DrawVLine(x1, y1+1, dy, SC_VLINE, fore_color, back_color, CHAR_AND_ATTR);
	
	// draw the 4 corners with dedicated corner pieces
	Text_SetCharAndColorAtXY(x1, y1, SC_ULCORNER, fore_color, back_color);		
	Text_SetCharAndColorAtXY(x2, y1, SC_URCORNER, fore_color, back_color);		
	Text_SetCharAndColorAtXY(x2, y2, SC_LRCORNER, fore_color, back_color);		
	Text_SetCharAndColorAtXY(x1, y2, SC_LLCORNER, fore_color, back_color);		

	// move cursor one past bottom,right corner of box
	zp_x++;
	
	// bounds check. would be nicer to JSR to this but that's expensive in C, so just copying this everywhere...
	
	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);

	if (zp_x > SCREEN_LAST_COL && zp_y < SCREEN_LAST_ROW)
	{
		zp_x = 0;
		zp_y++;
		R8(VICKY_TEXT_Y_POS) = zp_y;
	}
	
	R8(VICKY_TEXT_X_POS) = zp_x;

	Sys_RestoreIOPage();
}



// **** Draw string functions *****


//! Draw a string at a specified x, y coord, also setting the color attributes.
//! If it is too long to display on the line it started, it will be truncated at the right edge of the screen.
//! No word wrap is performed. 
//! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_string - the null-terminated string to be drawn
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_DrawStringAtXY(uint8_t x, uint8_t y, char* the_string, uint8_t fore_color, uint8_t back_color)
{
	Text_SetXY(x, y);
	Text_DrawString(the_string, fore_color, back_color);

	return true;
}


//! Draw a string at the current X/Y position, also setting the color attributes.
//! If it is too long to display on the line it started, it will be truncated at the right edge of the screen.
//! No word wrap is performed. 
//! @param	the_string - the null-terminated string to be drawn
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_DrawString(char* the_string, uint8_t fore_color, uint8_t back_color)
{
	uint8_t			the_attribute_value;
	uint8_t			max_col;
	uint8_t			the_len;
	
	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);
	
	the_len = (uint8_t)strlen(the_string); // can't be wider than the screen anyway
	max_col = SCREEN_NUM_COLS - 1;
	
	if (zp_x + the_len > max_col)
	{
		the_len = (max_col - zp_x) + 1;
	}
	
	//DEBUG_OUT(("%s %d: draw_len=%i, max_col=%i, x=%i", __func__, __LINE__, draw_len, max_col, x));
	//printf("%s %d: draw_len=%i, max_col=%i, x=%i \n", __func__, __LINE__, draw_len, max_col, x);

	//printf("%s %d: the_char_loc=%p, *charloc=%u \n", __func__, __LINE__, the_char_loc, *the_char_loc);
	//printf("%s %d: string=%s \n", __func__, __LINE__, the_string);
	
	// draw the string
	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	memcpy(zp_vram_ptr, the_string, the_len);
	Sys_RestoreIOPage();
	
	// draw the attributes
	Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
	memset(zp_vram_ptr, the_attribute_value, the_len);
	Sys_RestoreIOPage();

	// set x,y to end of string+1
	Text_SetXY(zp_x + (the_len - ((the_len / SCREEN_NUM_COLS) * SCREEN_NUM_COLS)), zp_y + (the_len / SCREEN_NUM_COLS));
	
	return true;
}



// **** "Text Window" Functions ****
// **** Move these back into OS/f Text Library in the future!


// general function for drawing a "window"-like text object using draw chars
// can supply a title, and specify if it should optionally draw another row under the title
// can supply background color, line color, and text color. 
// can say if you want background cleared
// can pass a pointer to a buffer where the text/color under the window will be saved before drawing (for easy restore later)
//! @param	accent_color - Index to the desired accent color (0-15). Window frame, etc.
//! @param	fore_color - Index to the desired foreground text color (0-15).
//! @param	back_color - Index to the desired background color (0-15).
bool Text_DrawWindow(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t accent_color, uint8_t fore_color, uint8_t back_color, char* the_header_text, char* char_save_mem, char* attr_save_mem, bool clear_first, bool enclose_header)
{
	//DEBUG_OUT(("%s %d: char save=%p, attr save=%p", __func__, __LINE__, char_save_mem, attr_save_mem));

	// optionally back up space "under" where the window will be drawn
	if (attr_save_mem != NULL && char_save_mem != NULL)
	{
		// copy to storage
		Text_CopyMemBoxLinearBuffer((uint8_t*)char_save_mem, x1, y1, x2, y2, PARAM_COPY_FROM_SCREEN, PARAM_FOR_TEXT_CHAR);
		Text_CopyMemBoxLinearBuffer((uint8_t*)attr_save_mem, x1, y1, x2, y2, PARAM_COPY_FROM_SCREEN, PARAM_FOR_TEXT_ATTR);
	}
	
	// optionally clear the background text and chars
	if (clear_first)
	{
		if (Text_FillBox(x1, y1, x2, y2, CH_SPACE, accent_color, back_color) == false)
		{
			return false;
		}
	}
	
	// draw the overall box
	Text_DrawBoxCoordsFancy(x1, y1, x2, y2, accent_color, back_color);
	
	// optionally enclose the header text by drawing a line under it, and making |- and -| chars line up
	if (enclose_header)
	{
		uint8_t	num_cols = (x2 - x1) + 1;

		Text_DrawHLine(x1 + 1, y1 + 2, num_cols - 2, SC_HLINE, accent_color, back_color, CHAR_AND_ATTR);
		Text_SetCharAtXY(x1, y1 + 2, SC_T_RIGHT);
		Text_SetCharAtXY(x2, y1 + 2, SC_T_LEFT);		
	}
	
	// optionally draw header text
	if (the_header_text != NULL)
	{
		Text_DrawStringAtXY(x1 + 1, y1 + 1, the_header_text, fore_color, back_color);
	}
	
	return true;
}


// Display a Text-based dialog box, with 1, 2, or 3 buttons, a title, and a message body
// Supports keyboard shortcuts for each button
// on A2560Ks, will use keyboard lighting to highlight the keyboard shortcuts
// returns -1 on error (such as can't fit buttons into the space allowed, or a button string is empty, etc.)
// returns 0, 1, 2 indicating which button was selected
// user hitting ESC will always cause -1 to be returned, regardless of keyboard shortcuts. This effectively gives you 2 shortcuts to say 'no'/'cancel'/'go back'.
//! @param	accent_color - Index to the desired accent color (0-15). Window frame, etc.
//! @param	fore_color - Index to the desired foreground text color (0-15).
//! @param	back_color - Index to the desired background color (0-15).
//! @param	cancel_color - Index to the desired foreground color (0-15) for the button that represents the cancel action
//! @param	affirm_color - Index to the desired background color (0-15) for the button that represents the confirm/affirm/ok action
int8_t Text_DisplayDialog(TextDialogTemplate* the_dialog_template, char* char_save_mem, char* attr_save_mem, uint8_t accent_color, uint8_t fore_color, uint8_t back_color, uint8_t cancel_color, uint8_t affirm_color)
{
	uint8_t			btn_width[3];
	uint8_t			avail_width;
	uint8_t			btn_x;
	uint8_t			btn_y;
	uint8_t			player_input;
	uint8_t			total_btn_width = 0;
	uint8_t			x1 = the_dialog_template->x_;
	uint8_t			y1 = the_dialog_template->y_;
	uint8_t			x2 = x1 + the_dialog_template->width_;
	uint8_t			y2 = y1 + the_dialog_template->height_;
	int8_t			the_result = DIALOG_ERROR;
	int8_t			i;
	
	// ** Validity checks
	
	// this function requires at least one button, or there will be no way to dismiss it
	if (the_dialog_template->num_buttons_ < 1)
	{
		return the_result;
	}
	
	// this function requires both header text and body text
	if (the_dialog_template->title_text_ == NULL || the_dialog_template->body_text_ == NULL)
	{
		return the_result;
	}
	
	avail_width = the_dialog_template->width_ - 2; // account for draw characters on edges
	
	// available (body) height is defined by by the space under the enclosed header (-3), and over the buttons (-4)
	//   bottom row takes 1, buttons take 1, and there is one row of padding above and below the buttons
// 	avail_height = the_dialog_template->height_ - 7;
	
	for (i = 0; i < the_dialog_template->num_buttons_; i++)
	{
		if (the_dialog_template->btn_label_[i] == NULL)
		{
			return the_result;
		}
		
		btn_width[i] = (uint8_t)General_Strnlen(the_dialog_template->btn_label_[i], TEXT_DIALOG_MAX_BTN_LABEL_LEN) + 2; // +2 because we force 2 spaces to right of all buttons
		total_btn_width += btn_width[i];
	}
	
	if (total_btn_width > avail_width)
	{
		return the_result;
	}
	
	
	// ** create the window itself, with its title
	
	// LOGIC:
	//   this is hard coded to use a specific color combination
	//   hardcoded also to always enclose the title in lines, and to clear the background first
	if (Text_DrawWindow( 
		x1, y1, 
		x2, y2,
		accent_color, fore_color, back_color, 
		the_dialog_template->title_text_, 
		char_save_mem, attr_save_mem, 
		PARAM_CLEAR_FIRST, 
		PARAM_ENCLOSE_HEADER
		) == false)
	{
		return the_result;
	}
	
	// ** draw the body text -- F256 f/manager version: not spending memory on text wrapping, so only 1 line of text supported!
	Text_DrawStringAtXY(
		the_dialog_template->x_ + 1, the_dialog_template->y_ + 3, 
		the_dialog_template->body_text_,
		fore_color, back_color
	);	
	
	// ** draw the buttons
	// go backwards from 3rd button (rightmost) to first button (leftmost)
	// build in 1 space to right of each button
	// if affirmative, draw in green. if non-affirmative, draw in red
	
	btn_y = y2 - 2; // -2: 1 for bottom line char, 1 for a spacer below button
	btn_x = x2 - 0; // -1: account for right line char, but space is built into button width -> set to 0 to get right results.
	i = the_dialog_template->num_buttons_ - 1;
	
	for (; i >= 0; i--)
	{
		uint8_t	btn_color;

		btn_x -= btn_width[i];
		
		if (the_dialog_template->default_button_id_ == i)
		{
			btn_color = affirm_color;
		}
		else
		{
			btn_color = cancel_color;
		}
		
		Text_DrawStringAtXY(btn_x, btn_y, the_dialog_template->btn_label_[i], btn_color, back_color);
	}

	// **get player input
	
	do
	{
		player_input = Keyboard_GetChar();
		
		if (player_input == CH_ESC || player_input == CH_RUNSTOP)
		{
			break;
		}
		
		for (i = 0; i < the_dialog_template->num_buttons_; i++)
		{
			if (the_dialog_template->btn_shortcut_[i] == player_input)
			{
				the_result = i;
				break;
			}
			else if (i == the_dialog_template->default_button_id_ && player_input == the_dialog_template->default_button_shortcut_)
			{
				the_result =i;
				break;
			}
		}
		
		// maybe it was the cancel shortcut instead?
		if (the_result == DIALOG_ERROR && player_input == the_dialog_template->cancel_button_shortcut_)
		{
			the_result = DIALOG_CANCEL;
		}
	}
	while (the_result == DIALOG_ERROR);

	// restore whatever had been under the text window
	// copy from storage
	Text_CopyMemBoxLinearBuffer((uint8_t*)char_save_mem, 
		x1, y1, 
		x2, y2, 
		PARAM_COPY_TO_SCREEN, PARAM_FOR_TEXT_CHAR
	);
	Text_CopyMemBoxLinearBuffer((uint8_t*)attr_save_mem, 
		x1, y1, 
		x2, y2, 
		PARAM_COPY_TO_SCREEN, PARAM_FOR_TEXT_ATTR
	);
	
	return the_result;
}


// Display a Text-based dialog box, with a title, a message body, and a place for users to input text
// returns false on error (eg, max string len is wider than dialog body), or if user refused to enter text
// returns true if user entered something
// populates the passed buffer with the text the user typed
// user hitting ESC will always cause false to be returned, regardless of keyboard shortcuts.
//! @param	accent_color - Index to the desired accent color (0-15).
//! @param	fore_color - Index to the desired foreground color (0-15).
//! @param	back_color - Index to the desired background color (0-15).
int8_t Text_DisplayTextEntryDialog(TextDialogTemplate* the_dialog_template, char* char_save_mem, char* attr_save_mem, char* the_buffer, int8_t the_max_length, uint8_t accent_color, uint8_t fore_color, uint8_t back_color)
{
	uint8_t			avail_width;
	uint8_t			input_x;
	uint8_t			input_y;
	uint8_t			x1 = the_dialog_template->x_;
	uint8_t			y1 = the_dialog_template->y_;
	uint8_t			x2 = x1 + the_dialog_template->width_;
	uint8_t			y2 = y1 + the_dialog_template->height_;
	int8_t			the_result = false;
	
	// ** Validity checks
	
	// this function requires both header text and body text
	if (the_dialog_template->title_text_ == NULL || the_dialog_template->body_text_ == NULL)
	{
		return the_result;
	}
	
	avail_width = the_dialog_template->width_ - 2; // account for draw characters on edges
	
	// available (body) height is defined by by the space under the enclosed header (-3), and over the buttons (-4)
	//   bottom row takes 1, buttons take 1, and there is one row of padding above and below the buttons
// 	avail_height = the_dialog_template->height_ - 7;
	
	if (the_max_length > avail_width)
	{
		return the_result;
	}
	
	
	// ** create the window itself, with its title
	
	// LOGIC:
	//   this is hard coded to use a specific color combination
	//   hardcoded also to always enclose the title in lines, and to clear the background first
	if (Text_DrawWindow( 
		x1, y1, 
		x2, y2,
		accent_color, fore_color, back_color, 
		the_dialog_template->title_text_, 
		char_save_mem, attr_save_mem, 
		PARAM_CLEAR_FIRST, 
		PARAM_ENCLOSE_HEADER
		) == false)
	{
		return the_result;
	}
	
	// ** draw the body text -- F256 f/manager version: not spending memory on text wrapping, so only 1 line of text supported!
	Text_DrawStringAtXY(
		the_dialog_template->x_ + 1, the_dialog_template->y_ + 3, 
		the_dialog_template->body_text_,
		fore_color, back_color
	);	
	
	input_y = y2 - 2; // -2: 1 for bottom line char, 1 for a spacer below button
	input_x = x1 + 1; // +1: get past box char

	// **get player input
	the_result = Text_GetStringFromUser(the_buffer, the_max_length, input_x, input_y, PARAM_USE_INSERT_MODE);

	// restore whatever had been under the text window
	// copy from storage
	Text_CopyMemBoxLinearBuffer((uint8_t*)char_save_mem, 
		x1, y1, 
		x2, y2, 
		PARAM_COPY_TO_SCREEN, PARAM_FOR_TEXT_CHAR
	);
	Text_CopyMemBoxLinearBuffer((uint8_t*)attr_save_mem, 
		x1, y1, 
		x2, y2, 
		PARAM_COPY_TO_SCREEN, PARAM_FOR_TEXT_ATTR
	);
	
	return the_result;
}






// **** User Input Functions ****

// get a string from the user and store in the passed buffer, drawing chars to screen as user types
// allows a maximum of the_max_length characters. Buffer must allow for max_length chars + a terminator!
// set overwrite_mode to true to start in overwrite, or false to start in auto-insert mode. Can be overridden by keyboard action. Use PARAM_USE_OVERWRITE_MODE/PARAM_USE_INSERT_MODE
// returns false if no string built.
bool Text_GetStringFromUser(char* the_buffer, int8_t the_max_length, uint8_t start_x, uint8_t start_y, bool overwrite_mode)
{
	char*		the_user_input = the_buffer;
	int8_t		x = start_x;
	int8_t		characters_remaining;
	int8_t		curr_pos;	// the cursor position within the string
	uint8_t		curr_len;	// the current length of the string
	int8_t		i;
	uint8_t		the_char;
	uint8_t		fore_text = COLOR_BRIGHT_WHITE;
	uint8_t		background = COLOR_BLACK;
	
	//DEBUG_OUT(("%s %d: entered; the_max_length=%i", __func__, __LINE__, the_max_length));

	Text_FillBox(
		start_x, start_y,
		start_x + the_max_length, start_y, 
		CH_SPACE, fore_text, background
	);

	// return false if the_max_length is so small we can't make a string
	if (the_max_length < 1)
	{
		return false;
	}

	if (the_max_length == 1)
	{
		the_user_input[0] = '\0';
		return false;
	}

	characters_remaining = the_max_length;
	curr_pos = 0;
	
	// if the passed buffer is not empty, write it out so users can edit. typical use case: rename a file. 
	curr_len = (uint8_t)General_Strnlen(the_buffer, the_max_length);
	
	if (curr_len > 0 && curr_len < the_max_length)
	{
		Text_DrawStringAtXY(x, start_y, the_buffer, fore_text, background);
		x += curr_len;
		characters_remaining -= curr_len;
		the_user_input += curr_len;
		curr_pos = curr_len; // ie, 1 past the end of the string
	}
	
	Text_SetXY(x, start_y);

	// have cursor blink while here
	Sys_EnableTextModeCursor(true);

	while ( (the_char = Keyboard_GetChar() ) != CH_ENTER)
	{
		//DEBUG_OUT(("%s %d: input=%x ('%c')", __func__, __LINE__, the_char, the_char));
		
		if (the_char == CH_ESC)
		{
			// ESC = same as typing nothing and hitting ENTER: cancel action
			return false;
		}
		else if (the_char == CH_BKSP)
		{
			//if (the_user_input != original_string) // original string was starting point of name string, so this prevents us from trying to delete past start
			if (curr_pos > 0) // prevents us from trying to delete past start
			{
				// if we are at end of string, turn cursor character to terminator.
				// if not at end, shift all chars to left 1 spot
				if (curr_pos < curr_len)
				{
					for (i = curr_pos - 1 ; i < curr_len; i++)
					{
						the_buffer[i] = the_buffer[i+1];
					}
					
					the_buffer[i] = '\0';
					Text_DrawStringAtXY(start_x, start_y, the_buffer, fore_text, background);
					Text_SetChar(CH_SPACE); // erase the last char in the string
				}
				else
				{
					*the_user_input = '\0';
					Text_SetCharAtXY(x-1, start_y, CH_SPACE);
				}
			
				// do visuals
				--x;
				Text_SetXY(x, start_y);

				--the_user_input;
				--curr_pos;
				
				// we just went back in the string, so from the new point, we have more chars available
				--curr_len;
				++characters_remaining;
			}
			else
			{
				// we backed up as far as the original string (in other words, nothing)
				if (x > start_x)
				{
					Text_SetXY(x, start_y);
				}

				x = start_x;
			}
		}
		else if (the_char == CH_DEL)
		{
			if (curr_pos < curr_len)
			{
				// user had cursored left at some point, and is ok to delete from the right
				// shift all chars from cursor rightwards, one slot to the left.
				
				for (i = curr_pos; i < curr_len; i++)
				{
					the_buffer[i] = the_buffer[i+1];
				}
				
				the_buffer[i] = '\0';
				Text_DrawStringAtXY(start_x, start_y, the_buffer, fore_text, background);
				Text_SetChar(CH_SPACE); // erase the last char in the string
				Text_SetXY(x, start_y); // reset cursor position
				
				// we just removed a char so we have more chars available
				--curr_len;
				++characters_remaining;
			}
			else
			{
				// player is at the end of the string, no way to further delete from right
				// do nothing
			}
		}
		else if (the_char == CH_CURS_UP)
		{
			// place cursor at start of string
			if (x != start_x)
			{
				if (curr_pos < curr_len)
				{
					Text_SetCharAtXY(x, start_y, *the_user_input);
				}
				else
				{
					Text_SetCharAtXY(x, start_y, CH_SPACE);
				}

				x = start_x;
				curr_pos = 0;
				the_user_input = the_buffer;
				Text_SetXY(x, start_y);
			}			
		}
		else if (the_char == CH_CURS_DOWN)
		{
			// place cursor at end of string
			if (curr_pos < curr_len)
			{
				Text_SetCharAtXY(x, start_y, *the_user_input);
				x = start_x + curr_len;
				curr_pos = curr_len;
				the_user_input = the_buffer + curr_len;
				Text_SetXY(x, start_y);
			}			
		}
		else if (the_char == CH_CURS_RIGHT)
		{
			if (curr_pos < curr_len)
			{
				// user had cursored left, and is now cursoring right
				Text_SetCharAtXY(x, start_y, *the_user_input);
				++the_user_input;
				++x;
				++curr_pos;
				Text_SetXY(x, start_y);
			}
			else
			{
				// player is at the end of the string, we don't want them to cursor further right
				// do nothing
			}
		}
		else if (the_char == CH_CURS_LEFT)
		{
			if (curr_pos > 0)
			{
				// user is cursoring towards beginning of string, but isn't there yet
				if (curr_pos < curr_len)
				{
					Text_SetCharAtXY(x, start_y, *the_user_input);
				}
				else
				{
					Text_SetCharAtXY(x, start_y, CH_SPACE);
				}

				--the_user_input;
				--x;
				--curr_pos;
				Text_SetXY(x, start_y);
			}
			else
			{
				// player is at the start of the string, we don't want them to cursor further left
				// do nothing
			}
		}
		else
		{
			// a typeable key has been hit: insert if enough chars (or do nothing if not), or typeover the current char if overwrite mode is on
			
			if (overwrite_mode == PARAM_USE_OVERWRITE_MODE)
			{
				*the_user_input = the_char;
				//DEBUG_OUT(("%s %d: the_user_input='%s', chrs remain=%u", __func__, __LINE__, the_user_input, characters_remaining));

				// if in middle of string, overwrite what was there, move cursor to right. 
				// if at end of string, and haven't hit max yet, show new char and move to right
				if (characters_remaining)
				{
					if (curr_pos == curr_len)
					{
						--characters_remaining;
						++curr_len;
					}
	
					Text_SetCharAtXY(x, start_y, the_char);
					++the_user_input;
					++x;
					++curr_pos;
					Text_SetXY(x, start_y);
				}
			}
			else
			{
				if (characters_remaining)
				{
					// move all chars to right of cursor right one slot
					for (i = curr_len-1; i >= curr_pos; i--)
					{
						the_buffer[i+1] = the_buffer[i];
					}

					*the_user_input = the_char;
					
					Text_DrawCharsAtXY(x, start_y, (uint8_t*)the_user_input, (curr_len - curr_pos) + 1);
					
					--characters_remaining;
					++curr_len;

					++the_user_input;
					++x;
					++curr_pos;
					Text_SetXY(x, start_y);
				}
			}
		}
	}

	// user hit enter - make sure we terminate the string at the end, not where the cursor may be
	the_user_input = the_buffer + curr_len;
	*the_user_input = '\0';

	// did user end up entering anything?
	if (curr_len == 0)
	{
		return false;
	}

	return true;
}




// DEBUG & TEST


// void Text_ShowColors(void)
// {
// 	uint8_t	i;
// 	
// 	for(i = 0; i<16; i++)
// 	{
// 		Text_SetCharAndColorAtXY(i,0,7,i,COLOR_BLACK);
// 	}
// }


// 	// TESTY stuff
// 	Text_SetXY(12,4);
// 	Text_DrawChars("Hello", 5);
// 	Text_SetChar('G');
// 	Text_SetAttr(129);
// 	Text_SetColor(COLOR_RED, COLOR_MEDIUM_GRAY);
// 	Text_SetChar('g');
// 	Text_SetCharAndColor('H', COLOR_MEDIUM_GRAY, COLOR_RED);
// 	Text_DrawHLine(12,5,5,'@', COLOR_BLUE, COLOR_RED, CHAR_AND_ATTR);
// 	Text_SetChar('J');
// 	Text_DrawHLine(18,6,5,'#', COLOR_BLUE, COLOR_RED, CHAR_AND_ATTR);
// 	Text_SetChar('K');
// 	Text_DrawVLine(18,7,5,'#', COLOR_RED, COLOR_BLUE, CHAR_AND_ATTR);
// 	Text_SetChar('L');
// 	Text_DrawString("Goodbye", COLOR_RED, COLOR_BLUE);
// 	Text_SetChar('M');
// 	// TESTY stuff end
	
