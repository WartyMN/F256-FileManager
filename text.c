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
#include "app.h"
#include "text.h"
#include "general.h"
#include "sys.h"

// C includes
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// F256 includes
#include "f256.h"




/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/



/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/



/*****************************************************************************/
/*                          File-scoped Variables                            */
/*****************************************************************************/

uint8_t		last_x;
uint8_t		last_y;

/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

extern System*			global_system;


/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

//! Validate the coordinates are within the bounds of the specified screen. 
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the horizontal position to validate. Must be between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the vertical position to validate. Must be between 0 and the screen's text_rows_vis_ - 1
bool Text_ValidateXY(int8_t x, int8_t y);

// Fill attribute or text char memory. Writes to char memory if for_attr is false.
// calling function must validate the screen ID before passing!
//! @return	Returns false on any error/invalid input.
bool Text_FillMemory(bool for_attr, uint8_t the_fill);

//! Fill character and attribute memory for a specific box area
//! calling function must validate screen id, coords, attribute value before passing!
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	width: width, in character cells, of the rectangle to be filled
//! @param	height: height, in character cells, of the rectangle to be filled
//! @param	the_attribute_value: a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_FillMemoryBoxBoth(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t the_char, uint8_t the_attribute_value);

//! Fill character OR attribute memory for a specific box area
//! calling function must validate screen id, coords, attribute value before passing!
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	width: width, in character cells, of the rectangle to be filled
//! @param	height: height, in character cells, of the rectangle to be filled
//! @param	for_attr: true to work with attribute data, false to work character data. Recommend using SCREEN_FOR_TEXT_ATTR/SCREEN_FOR_TEXT_CHAR.
//! @param	the_fill: either a 1-byte character code, or a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_FillMemoryBox(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool for_attr, uint8_t the_fill);

/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/

// **** NOTE: all functions in private section REQUIRE pre-validated parameters. 
// **** NEVER call these from your own functions. Always use the public interface. You have been warned!


//! Fill attribute or text char memory. 
//! calling function must validate the screen ID before passing!
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	for_attr: true to work with attribute data, false to work character data. Recommend using SCREEN_FOR_TEXT_ATTR/SCREEN_FOR_TEXT_CHAR.
//! @param	the_fill: either a 1-byte character code, or a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_FillMemory(bool for_attr, uint8_t the_fill)
{
	uint16_t	the_write_len;
	uint8_t*	the_write_loc;

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

	the_write_len = SCREEN_TOTAL_BYTES;
	the_write_loc = (uint8_t*)SCREEN_TEXT_MEMORY_LOC;
	memset(the_write_loc, the_fill, the_write_len);
		
	Sys_RestoreIOPage();

	//printf("Text_FillMemory: done \n");
	//DEBUG_OUT(("%s %d: done (for_attr=%u, the_fill=%u)", __func__, __LINE__, for_attr, the_fill));

	return true;
}


//! Fill character and attribute memory for a specific box area
//! calling function must validate screen id, coords, attribute value before passing!
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	width: width, in character cells, of the rectangle to be filled
//! @param	height: height, in character cells, of the rectangle to be filled
//! @param	the_attribute_value: a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_FillMemoryBoxBoth(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t the_char, uint8_t the_attribute_value)
{
	uint8_t*	the_write_loc;
	uint8_t		max_row;

	// LOGIC: 
	//   On F256jr, the write len and write locs are same for char and attr memory, difference is IO page 2 or 3

	// set up initial loc
	the_write_loc = Text_GetMemLocForXY(x, y);
	
	max_row = y + height;
	
	for (; y <= max_row; y++)
	{
		Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
		memset(the_write_loc, the_attribute_value, width);
		Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
		memset(the_write_loc, the_char, width);

		the_write_loc += SCREEN_NUM_COLS;
	}
		
	Sys_RestoreIOPage();
			
	return true;
}


//! Fill character OR attribute memory for a specific box area
//! calling function must validate screen id, coords, attribute value before passing!
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	width: width, in character cells, of the rectangle to be filled
//! @param	height: height, in character cells, of the rectangle to be filled
//! @param	for_attr: true to work with attribute data, false to work character data. Recommend using SCREEN_FOR_TEXT_ATTR/SCREEN_FOR_TEXT_CHAR.
//! @param	the_fill: either a 1-byte character code, or a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_FillMemoryBox(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool for_attr, uint8_t the_fill)
{
	uint8_t*	the_write_loc;
	uint8_t		max_row;

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

	// set up initial loc
	the_write_loc = Text_GetMemLocForXY(x, y);
	
	max_row = y + height;
	
	for (; y <= max_row; y++)
	{
		memset(the_write_loc, the_fill, width);
		the_write_loc += SCREEN_NUM_COLS;
	}
		
	Sys_RestoreIOPage();
			
	return true;
}




/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/

// ** NOTE: there is no destructor or constructor for this library, as it does not track any allocated memory. It works on the basis of a screen ID, which corresponds to the text memory for Vicky's Channel A and Channel B video memory.


// **** Block copy functions ****


//! Copy a rectangular area of text or attr to or from a linear memory buffer.
//!   Use this if you do not have a full-sized (screen-size) off-screen buffer, but instead have a block perhaps just big enough to hold the rect.
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	the_buffer: valid pointer to a block of memory to hold (or alternatively act as the source of) the character or attribute data for the specified rectangle of screen memory. This will be read from first byte to last byte, without skipping. e.g., if you want to copy a 40x5 rectangle of text from the middle of the screen to this buffer, the buffer must be 40*5=200 bytes in length, and data will be written contiguously to it. 
//! @param	x1: the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1: the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2: the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2: the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	to_screen: true to copy to the screen from the buffer, false to copy from the screen to the buffer. Recommend using SCREEN_COPY_TO_SCREEN/SCREEN_COPY_FROM_SCREEN.
//! @param	for_attr: true to work with attribute data, false to work character data. Recommend using SCREEN_FOR_TEXT_ATTR/SCREEN_FOR_TEXT_CHAR.
//! @return	Returns false on any error/invalid input.
bool Text_CopyMemBoxLinearBuffer(uint8_t* the_buffer, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool to_screen, bool for_attr)
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
	the_buffer_loc = the_buffer;
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
		
		the_buffer_loc += the_write_len;
		the_vram_loc += SCREEN_NUM_COLS;
	}
		
	Sys_RestoreIOPage();

	return true;
}


// //! Copy a rectangular area of text or attr to or from an off-screen buffer of the same size as the physical screen buffer
// //! @param	the_screen: valid pointer to the target screen to operate on
// //! @param	the_buffer: valid pointer to a block of memory to hold (or alternatively act as the source of) the character or attribute data for the specified rectangle of screen memory. This buffer must be the same size as the physical screen!
// //! @param	x1: the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y1: the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	x2: the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y2: the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	to_screen: true to copy to the screen from the buffer, false to copy from the screen to the buffer. Recommend using SCREEN_COPY_TO_SCREEN/SCREEN_COPY_FROM_SCREEN.
// //! @param	for_attr: true to work with attribute data, false to work character data. Recommend using SCREEN_FOR_TEXT_ATTR/SCREEN_FOR_TEXT_CHAR.
// //! @return	Returns false on any error/invalid input.
// bool Text_CopyMemBox(uint8_t* the_buffer, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool to_screen, bool for_attr)
// {
// 	uint8_t*		the_vram_loc;
// 	uint8_t*		the_buffer_loc;
// 	uint8_t			the_write_len;
// 	int16_t			initial_offset;
// 
// 	// LOGIC: 
// 	//   On F256jr, the write len and write locs are same for char and attr memory, difference is IO page 2 or 3
// 
// 	if (for_attr)
// 	{
// 		Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
// 	}
// 	else
// 	{
// 		Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
// 	}
// 		
// 	// get initial read/write locs
// 	initial_offset = (SCREEN_NUM_COLS * y1) + x1;
// 	the_buffer_loc = the_buffer + initial_offset;
// 	the_write_len = x2 - x1 + 1;
// 
// 	the_vram_loc = (uint8_t*)SCREEN_TEXT_MEMORY_LOC + initial_offset;
// 	
// 	// do copy one line at a time	
// 
// //DEBUG_OUT(("%s %d: vramloc=%p, buffer=%p, bufferloc=%p, to_screen=%i, the_write_len=%i", the_vram_loc, the_buffer, the_buffer_loc, to_screen, the_write_len));
// 
// 	for (; y1 <= y2; y1++)
// 	{
// 		if (to_screen)
// 		{
// 			memcpy(the_vram_loc, the_buffer_loc, the_write_len);
// 		}
// 		else
// 		{
// 			memcpy(the_buffer_loc, the_vram_loc, the_write_len);
// 		}
// 		
// 		the_buffer_loc += SCREEN_NUM_COLS;
// 		the_vram_loc += SCREEN_NUM_COLS;
// 	}
// 		
// 	Sys_RestoreIOPage();
// 
// 	return true;
// }



// **** Block fill functions ****



//! Clear the text screen and reset foreground and background colors
void Text_ClearScreen(uint8_t fore_color, uint8_t back_color)
{
	uint8_t			the_attribute_value;

	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);
	//DEBUG_OUT(("%s %d: the_attribute_value=%u", __func__, __LINE__, the_attribute_value));

	Text_FillMemory(SCREEN_FOR_TEXT_CHAR, ' ');
	Text_FillMemory(SCREEN_FOR_TEXT_ATTR, the_attribute_value);
}


//! Fill character and attribute memory for a specific box area
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x1: the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1: the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2: the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2: the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_char: the character to be used for the fill operation
//! @param	fore_color: Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color: Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_FillBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t the_char, uint8_t fore_color, uint8_t back_color)
{
	uint8_t		dy;
	uint8_t		dx;
	uint8_t		the_attribute_value;

 	// add 1 to H line len, because dx becomes width, and if width = 0, then memset gets 0, and nothing happens.
	// dy can be 0 and you still get at least one row done.
	dx = x2 - x1 + 1;
	dy = y2 - y1 + 0;

	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);

	return Text_FillMemoryBoxBoth(x1, y1, dx, dy, the_char, the_attribute_value);
}


// //! Fill character memory for a specific box area
// //! @param	the_screen: valid pointer to the target screen to operate on
// //! @param	x1: the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y1: the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	x2: the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y2: the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	the_char: the character to be used for the fill operation
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
// 	// dy can be 0 and you still get at least one row done.
// 	dx = x2 - x1 + 1;
// 	dy = y2 - y1 + 0;
// 
// 	return Text_FillMemoryBox(x1, y1, dx, dy, SCREEN_FOR_TEXT_CHAR, the_char);
// }


//! Fill attribute memory for a specific box area
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x1: the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1: the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2: the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2: the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	fore_color: Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color: Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
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
	// dy can be 0 and you still get at least one row done.
	dx = x2 - x1 + 1;
	dy = y2 - y1 + 0;

	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);

	return Text_FillMemoryBox(x1, y1, dx, dy, SCREEN_FOR_TEXT_ATTR, the_attribute_value);
}


//! Invert the colors of a rectangular block.
//! As this requires sampling each character cell, it is no faster (per cell) to do for entire screen as opposed to a subset box
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x1: the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1: the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2: the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2: the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @return	Returns false on any error/invalid input.
bool Text_InvertBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
	uint8_t			the_attribute_value;
	uint8_t			the_inversed_value;
	uint8_t*		the_write_loc;
	uint8_t			the_col;
	uint8_t			skip_len;
	uint8_t			back_nibble;
	uint8_t			fore_nibble;
	
	// get initial read/write loc
	the_write_loc = Text_GetMemLocForXY(x1, y1);	
	
	// amount of cells to skip past once we have written the specified line len
	skip_len = SCREEN_NUM_COLS - (x2 - x1) - 1;

	Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
	
	for (; y1 <= y2; y1++)
	{
		for (the_col = x1; the_col <= x2; the_col++)
		{
			the_attribute_value = (unsigned char)*the_write_loc;
			
			// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
			back_nibble = ((the_attribute_value & 0xF0) >> 4);
			fore_nibble = ((the_attribute_value & 0x0F) << 4);
			the_inversed_value = (fore_nibble | back_nibble);
			
			*the_write_loc++ = the_inversed_value;
		}

		the_write_loc += skip_len;
	}
		
	Sys_RestoreIOPage();

	return true;
}




// **** Set char/attr functions *****

// NOTE: all functions from here lower that pass an x/y will update the last_x/last_y parameters.

//! Set a char at a specified x, y coord
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_char: the character to be used
//! @return	Returns false on any error/invalid input.
bool Text_SetCharAtXY(uint8_t x, uint8_t y, uint8_t the_char)
{
	uint8_t*	the_write_loc;
	
	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	
	the_write_loc = Text_GetMemLocForXY(x, y);	
	*the_write_loc = the_char;
		
	Sys_RestoreIOPage();

	last_x = x;
	last_y = y;
	
	return true;
}


//! Set the attribute value at a specified x, y coord
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_attribute_value: a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_SetAttrAtXY(uint8_t x, uint8_t y, uint8_t the_attribute_value)
{
	uint8_t*	the_write_loc;
	
	Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
	
	the_write_loc = Text_GetMemLocForXY(x, y);	
	*the_write_loc = the_attribute_value;
		
	Sys_RestoreIOPage();

	last_x = x;
	last_y = y;
	
	return true;
}


//! Set the attribute value at a specified x, y coord based on the foreground and background colors passed
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	fore_color: Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color: Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_SetColorAtXY(uint8_t x, uint8_t y, uint8_t fore_color, uint8_t back_color)
{
	uint8_t			the_attribute_value;

	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);

	last_x = x;
	last_y = y;
	
	return Text_SetAttrAtXY(x, y, the_attribute_value);
}


// //! Draw a char at a specified x, y coord, also setting the color attributes
// //! @param	the_screen: valid pointer to the target screen to operate on
// //! @param	x: the horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y: the vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	the_char: the character to be used
// //! @param	fore_color: Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
// //! @param	back_color: Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
// //! @return	Returns false on any error/invalid input.
// bool Text_SetCharAndAttrAtXY(uint8_t x, uint8_t y, uint8_t the_char, uint8_t the_attribute_value)
// {
// 	uint8_t*		the_write_loc;
// 
// 	the_write_loc = Text_GetMemLocForXY(x, y);	
// 	
// 	Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
// 	*the_write_loc = the_attribute_value;
// 	Sys_RestoreIOPage();
// 
// 	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
// 	*the_write_loc = the_char;
// 	Sys_RestoreIOPage();
// 
// 	last_x = x;
// 	last_y = y;
// 	
// 	return true;
// }


//! Draw a char at a specified x, y coord, also setting the color attributes
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_char: the character to be used
//! @param	fore_color: Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color: Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_SetCharAndColorAtXY(uint8_t x, uint8_t y, uint8_t the_char, uint8_t fore_color, uint8_t back_color)
{
	uint8_t*		the_write_loc;
	uint8_t			the_attribute_value;
			
	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);

	the_write_loc = Text_GetMemLocForXY(x, y);	
	
	Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);
	*the_write_loc = the_attribute_value;
	Sys_RestoreIOPage();

	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);
	*the_write_loc = the_char;
	Sys_RestoreIOPage();

	last_x = x;
	last_y = y;
	
	return true;
}


// //! Set a char at a y*80+x screen index point. 
// bool Text_SetCharAtMemLoc(uint16_t the_write_loc, uint8_t the_char)
// {
// 	the_write_loc += SCREEN_TEXT_MEMORY_LOC;
// 	
// 	*(uint8_t*)the_write_loc = the_char;
// 	
// 	return true;
// }


// copy n-bytes into display memory, at the X/Y position specified
bool Text_DrawCharsAtXY(uint8_t x, uint8_t y, uint8_t* the_buffer, uint16_t the_len)
{
	uint8_t*		the_char_loc;
	uint16_t		i;
		
	// set up char and attribute memory initial loc
	the_char_loc = Text_GetMemLocForXY(x, y);

	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);

	// draw the string
	for (i = 0; i < the_len; i++)
	{
		*the_char_loc++ = the_buffer[i];
	}

	Sys_RestoreIOPage();

	last_x = x + i;
	last_y = y;
	
	return true;
}




// **** FONT RELATED *****

//! replace the current font data with the data at the passed memory buffer
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	new_font_data: Pointer to 2K (256 characters x 8 lines/bytes each) of font data. Each byte represents one line of an 8x8 font glyph.
//! @return	Returns false on any error/invalid input.
bool Text_UpdateFontData(char* new_font_data, bool for_primary_font)
{
	uint8_t*	target_font_addr;
	
	if (new_font_data == NULL)
	{
		LOG_ERR(("%s %d: passed font data was NULL", __func__, __LINE__));
		return false;
	}

	if (for_primary_font)
	{
		target_font_addr = (uint8_t*)FONT_MEMORY_BANK0;
	}
	else
	{
		target_font_addr = (uint8_t*)FONT_MEMORY_BANK1;
	}
	
	Sys_SwapIOPage(VICKY_IO_PAGE_FONT_AND_LUTS);

	memcpy(target_font_addr, new_font_data, (8*256));
		
	Sys_RestoreIOPage();

	return true;
}



// **** Get char/attr functions *****





// **** Drawing functions *****


//! Draws a horizontal line from specified coords, for n characters, using the specified char and/or attribute
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_line_len: The total length of the line, in characters, including the start and end character.
//! @param	the_char: the character to be used when drawing
//! @param	fore_color: Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color: Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	the_draw_choice: controls the scope of the action, and is one of CHAR_ONLY, ATTR_ONLY, or CHAR_AND_ATTR. See the text_draw_choice enum.
//! @return	Returns false on any error/invalid input.
void Text_DrawHLine(uint8_t x, uint8_t y, uint8_t the_line_len, uint8_t the_char, uint8_t fore_color, uint8_t back_color, uint8_t the_draw_choice)
{
	uint8_t			the_attribute_value;
	
	// LOGIC: 
	//   an H line is just a box with 1 row, so we can re-use Text_FillMemoryBox(Both)(). These routines use memset, so are quicker than for loops. 
	
	if (the_draw_choice == CHAR_ONLY)
	{
		Text_FillMemoryBox(x, y, the_line_len, 0, SCREEN_FOR_TEXT_CHAR, the_char);
	}
	else
	{
		// calculate attribute value from passed fore and back colors
		// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground

		the_attribute_value = ((fore_color << 4) | back_color);
	
		if (the_draw_choice == ATTR_ONLY)
		{
			Text_FillMemoryBox(x, y, the_line_len, 0, SCREEN_FOR_TEXT_ATTR, the_attribute_value);
		}
		else
		{
			Text_FillMemoryBoxBoth(x, y, the_line_len, 0, the_char, the_attribute_value);
		}
	}

	last_x = x + the_line_len;
	last_y = y;
}


//! Draws a vertical line from specified coords, for n characters, using the specified char and/or attribute
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_line_len: The total length of the line, in characters, including the start and end character.
//! @param	the_char: the character to be used when drawing
//! @param	fore_color: Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color: Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	the_draw_choice: controls the scope of the action, and is one of CHAR_ONLY, ATTR_ONLY, or CHAR_AND_ATTR. See the text_draw_choice enum.
//! @return	Returns false on any error/invalid input.
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

	last_x = x;
	last_y = y + dy;
}


//! Draws a box based on 2 sets of coords, using the predetermined line and corner "graphics", and the passed colors
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x1: the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1: the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2: the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2: the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	fore_color: Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color: Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
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

	last_x = x2;
	last_y = y2;
}



// **** Draw string functions *****


//! Draw a string at a specified x, y coord, also setting the color attributes.
//! If it is too long to display on the line it started, it will be truncated at the right edge of the screen.
//! No word wrap is performed. 
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_string: the null-terminated string to be drawn
//! @param	fore_color: Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color: Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_DrawStringAtXY(uint8_t x, uint8_t y, char* the_string, uint8_t fore_color, uint8_t back_color)
{
	uint8_t*		the_char_loc;
	uint8_t*		the_attr_loc;
	uint8_t			the_attribute_value;
	uint8_t			i;
	uint8_t			max_col;
	uint8_t			draw_len;
	
	draw_len = strlen(the_string); // can't be wider than the screen anyway
	max_col = SCREEN_NUM_COLS - 1;
	
	if (x + draw_len > max_col)
	{
		draw_len = (max_col - x) + 1;
	}
	
	//DEBUG_OUT(("%s %d: draw_len=%i, max_col=%i, x=%i", __func__, __LINE__, draw_len, max_col, x));
	//printf("%s %d: draw_len=%i, max_col=%i, x=%i \n", __func__, __LINE__, draw_len, max_col, x);
	
	// calculate attribute value from passed fore and back colors
	// LOGIC: text mode only supports 16 colors. lower 4 bits are back, upper 4 bits are foreground
	the_attribute_value = ((fore_color << 4) | back_color);

	// set up char and attribute memory initial loc
	the_attr_loc = the_char_loc = Text_GetMemLocForXY(x, y);

	//printf("%s %d: the_char_loc=%p, *charloc=%u \n", __func__, __LINE__, the_char_loc, *the_char_loc);
	//printf("%s %d: string=%s \n", __func__, __LINE__, the_string);
	
	// draw the string
	Sys_SwapIOPage(VICKY_IO_PAGE_CHAR_MEM);

	for (i = 0; i < draw_len; i++)
	{
		*the_char_loc++ = the_string[i];
	}
		
	Sys_RestoreIOPage();

	// draw the attributes

	Sys_SwapIOPage(VICKY_IO_PAGE_ATTR_MEM);

	memset(the_attr_loc, the_attribute_value, draw_len);
		
	Sys_RestoreIOPage();

	last_x = x + i;
	last_y = y;
	
	return true;
}



// **** Plotting functions ****


//! Calculate the VRAM location of the specified coordinate
//! @param	the_screen: valid pointer to the target screen to operate on
//! @param	x: the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	for_attr: true to work with attribute data, false to work character data. Recommend using SCREEN_FOR_TEXT_ATTR/SCREEN_FOR_TEXT_CHAR.
uint8_t* Text_GetMemLocForXY(uint8_t x, uint8_t y)
{
	uint8_t*	the_write_loc;
	uint16_t	initial_offset;
	
	// LOGIC:
	//   For plotting the VRAM, A2560 uses the full width, regardless of borders. 
	//   So even if only 72 are showing, the screen is arranged from 0-71 for row 1, then 80-151 for row 2, etc. 
	
	initial_offset = (SCREEN_NUM_COLS * y) + x;
	the_write_loc = (uint8_t*)SCREEN_TEXT_MEMORY_LOC + initial_offset;
	
	//DEBUG_OUT(("%s %d: screen=%i, x=%i, y=%i, for-attr=%i, calc=%i, loc=%p", __func__, __LINE__, (int16_t)the_screen_id, x, y, for_attr, (the_screen->text_mem_cols_ * y) + x, the_write_loc));

	last_x = x;
	last_y = y;
	
	return the_write_loc;
}




// **** "Text Window" Functions ****
// **** Move these back into OS/f Text Library in the future!


// general function for drawing a "window"-like text object using draw chars
// can supply a title, and specify if it should optionally draw another row under the title
// can supply background color, line color, and text color. 
// can say if you want background cleared
// can pass a pointer to a buffer where the text/color under the window will be saved before drawing (for easy restore later)
bool Text_DrawWindow(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fore_color, uint8_t back_color, uint8_t text_color, char* the_header_text, char* char_save_mem, char* attr_save_mem, bool clear_first, bool enclose_header)
{
	//DEBUG_OUT(("%s %d: char save=%p, attr save=%p", __func__, __LINE__, char_save_mem, attr_save_mem));

	// optionally back up space "under" where the window will be drawn
	if (attr_save_mem != NULL && char_save_mem != NULL)
	{
		// copy to storage
		Text_CopyMemBoxLinearBuffer((uint8_t*)char_save_mem, x1, y1, x2, y2, SCREEN_COPY_FROM_SCREEN, SCREEN_FOR_TEXT_CHAR);
		Text_CopyMemBoxLinearBuffer((uint8_t*)attr_save_mem, x1, y1, x2, y2, SCREEN_COPY_FROM_SCREEN, SCREEN_FOR_TEXT_ATTR);
	}
	
	// optionally clear the background text and chars
	if (clear_first)
	{
		if (Text_FillBox(x1, y1, x2, y2, CH_SPACE, fore_color, back_color) == false)
		{
			return false;
		}
	}
	
	// draw the overall box
	Text_DrawBoxCoordsFancy(x1, y1, x2, y2, fore_color, back_color);
	
	// optionally enclose the header text by drawing a line under it, and making |- and -| chars line up
	if (enclose_header)
	{
		uint16_t	num_cols = (x2 - x1) + 1;

		Text_DrawHLine(x1 + 1, y1 + 2, num_cols - 2, SC_HLINE, fore_color, back_color, CHAR_AND_ATTR);
		Text_SetCharAtXY(x1, y1 + 2, SC_T_RIGHT);
		Text_SetCharAtXY(x2, y1 + 2, SC_T_LEFT);		
	}
	
	// optionally draw header text
	if (the_header_text != NULL)
	{
		Text_DrawStringAtXY(x1 + 1, y1 + 1, the_header_text, text_color, back_color);
	}
	
	return true;
}


// Display a Text-based dialog box, with 1, 2, or 3 buttons, a title, and a message body
// Supports keyboard shortcuts for each button
// on A2560Ks, will use keyboard lighting to highlight the keyboard shortcuts
// returns -1 on error (such as can't fit buttons into the space allowed, or a button string is empty, etc.)
// returns 0, 1, 2 indicating which button was selected
// user hitting ESC will always cause -1 to be returned, regardless of keyboard shortcuts. This effectively gives you 2 shortcuts to say 'no'/'cancel'/'go back'.
int8_t Text_DisplayDialog(TextDialogTemplate* the_dialog_template, char* char_save_mem, char* attr_save_mem)
{
	uint8_t			btn_width[3];
	uint8_t			avail_width;
// 	uint8_t			avail_height;
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
		
		btn_width[i] = General_Strnlen(the_dialog_template->btn_label_[i], TEXT_DIALOG_MAX_BTN_LABEL_LEN) + 1; // +1 because we force a space to right of all buttons
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
		DIALOG_ACCENT_COLOR, DIALOG_BACKGROUND_COLOR, DIALOG_FOREGROUND_COLOR, 
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
		DIALOG_FOREGROUND_COLOR, DIALOG_BACKGROUND_COLOR
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
		
		if (the_dialog_template->btn_is_affirmative_[i])
		{
			btn_color = DIALOG_AFFIRM_COLOR;
		}
		else
		{
			btn_color = DIALOG_CANCEL_COLOR;
		}
		
		btn_x -= btn_width[i];
		Text_DrawStringAtXY(btn_x, btn_y, the_dialog_template->btn_label_[i], APP_BACKGROUND_COLOR, btn_color);
	}

	// **get player input
	
	do
	{
		player_input = getchar();
		
		if (player_input == CH_ESC || player_input == CH_RUNSTOP)
		{
			break;
		}
		
		for (i = 0; i < the_dialog_template->num_buttons_; i++)
		{
			if (the_dialog_template->btn_shortcut_[i] == player_input)
			{
				the_result = i;
			}
		}
	}
	while (the_result == DIALOG_ERROR);

	// restore whatever had been under the text window
	// copy from storage
	Text_CopyMemBoxLinearBuffer((uint8_t*)char_save_mem, 
		x1, y1, 
		x2, y2, 
		SCREEN_COPY_TO_SCREEN, SCREEN_FOR_TEXT_CHAR
	);
	Text_CopyMemBoxLinearBuffer((uint8_t*)attr_save_mem, 
		x1, y1, 
		x2, y2, 
		SCREEN_COPY_TO_SCREEN, SCREEN_FOR_TEXT_ATTR
	);
	
	return the_result;
}


// Display a Text-based dialog box, with a title, a message body, and a place for users to input text
// returns false on error (eg, max string len is wider than dialog body), or if user refused to enter text
// returns true if user entered something
// populates the passed buffer with the text the user typed
// user hitting ESC will always cause false to be returned, regardless of keyboard shortcuts.
int8_t Text_DisplayTextEntryDialog(TextDialogTemplate* the_dialog_template, char* char_save_mem, char* attr_save_mem, char* the_buffer, uint8_t the_max_length)
{
	uint8_t			avail_width;
// 	uint8_t			avail_height;
	uint8_t			input_x;
	uint8_t			input_y;
	uint8_t			x1 = the_dialog_template->x_;
	uint8_t			y1 = the_dialog_template->y_;
	uint8_t			x2 = x1 + the_dialog_template->width_;
	uint8_t			y2 = y1 + the_dialog_template->height_;
	bool			the_result = false;
	
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
		DIALOG_ACCENT_COLOR, DIALOG_BACKGROUND_COLOR, DIALOG_FOREGROUND_COLOR, 
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
		DIALOG_FOREGROUND_COLOR, DIALOG_BACKGROUND_COLOR
	);	
	
	input_y = y2 - 2; // -2: 1 for bottom line char, 1 for a spacer below button
	input_x = x1 + 1; // +1: get past box char

	// **get player input
	the_result = Text_GetStringFromUser(the_buffer, the_max_length, input_x, input_y);

	// restore whatever had been under the text window
	// copy from storage
	Text_CopyMemBoxLinearBuffer((uint8_t*)char_save_mem, 
		x1, y1, 
		x2, y2, 
		SCREEN_COPY_TO_SCREEN, SCREEN_FOR_TEXT_CHAR
	);
	Text_CopyMemBoxLinearBuffer((uint8_t*)attr_save_mem, 
		x1, y1, 
		x2, y2, 
		SCREEN_COPY_TO_SCREEN, SCREEN_FOR_TEXT_ATTR
	);
	
	return the_result;
}






// **** User Input Functions ****

// get a string from the user and store in the passed buffer, drawing chars to screen as user types
// allows a maximum of the_max_length characters. Buffer must allow for max_length chars + a terminator!
// returns false if no string built.
bool Text_GetStringFromUser(char* the_buffer, int8_t the_max_length, uint8_t start_x, uint8_t start_y)
{
	char*		the_user_input = the_buffer;
	int8_t		x = start_x;
	int8_t		characters_remaining;
	int8_t		curr_pos;	// the cursor position within the string
	uint8_t		curr_len;	// the current length of the string
	uint8_t		i;
	uint8_t		the_char;
	uint8_t		the_cursor_char_code = SC_CHECKERED;
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
	curr_len = General_Strnlen(the_buffer, the_max_length);
	
	if (curr_len > 0 && curr_len < the_max_length)
	//if (the_buffer[0] != '\0')
	{
		Text_DrawStringAtXY(x, start_y, the_buffer, fore_text, background);
		x += curr_len;
		characters_remaining -= curr_len;
		the_user_input += curr_len;
		curr_pos = curr_len; // ie, 1 past the end of the string
	}
	

	// have cursor blink while here
	//Sys_EnableTextModeCursor(true);	// NOTE: on f256jr, this would work. would also need to set dc14-dc17 as cursor moves. skipping because already have working cursor.

	Text_SetCharAtXY(x, start_y, the_cursor_char_code);
	//gotoxy(x, SPLASH_GET_NAME_INPUT_Y);
	
	while ( (the_char = getchar() ) != CH_ENTER)
	{
		//DEBUG_OUT(("%s %d: input=%x ('%c')", __func__, __LINE__, the_char, the_char));
		
		if (the_char == CH_DEL)
		{
			//if (the_user_input != original_string) // original string was starting point of name string, so this prevents us from trying to delete past start
			if (curr_pos > 0) // prevents us from trying to delete past start
			{
				// if we are at end of string, turn cur character to terminator.
				// if not at end, shift all chars to left 1 spot
				if (curr_pos < curr_len)
				{
					for (i = curr_pos - 1 ; i < curr_len; i++)
					{
						the_buffer[i] = the_buffer[i+1];
					}
					
					the_buffer[i] = '\0';
					Text_DrawStringAtXY(start_x, start_y, the_buffer, fore_text, background);
					Text_SetCharAtXY(start_x + (curr_len - 1), start_y, CH_SPACE); // erase the last char in the string
				}
				else
				{
					*the_user_input = '\0';
					Text_SetCharAtXY(x, start_y, CH_SPACE);
				}
			
				// do visuals
				--x;
				Text_SetCharAtXY(x, start_y, the_cursor_char_code);
				//gotoxy(x, SPLASH_GET_NAME_INPUT_Y);

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
					Text_SetCharAtXY(x, start_y, the_cursor_char_code);
					//gotoxy(x, SPLASH_GET_NAME_INPUT_Y);
				}

				x = start_x;
			}
		}
		else if (the_char == CH_CURS_UP)
		{
			// place cursor at start of string
			if (x != start_x)
			{
				Text_SetCharAtXY(x, start_y, *the_user_input);
				x = start_x;
				curr_pos = 0;
				the_user_input = the_buffer;
				Text_SetCharAtXY(x, start_y, the_cursor_char_code);
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
				Text_SetCharAtXY(x, start_y, the_cursor_char_code);
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
				Text_SetCharAtXY(x, start_y, the_cursor_char_code);				
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
				Text_SetCharAtXY(x, start_y, the_cursor_char_code);				
			}
			else
			{
				// player is at the start of the string, we don't want them to cursor further left
				// do nothing
			}
		}
		else
		{
			// NOTE: c64 / CBM DOS apparently allows all chars to be used, so no filter is used here
			
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
				Text_SetCharAtXY(x, start_y, the_cursor_char_code);
			}
// 			else
// 			{
// 				// no space to display more, so don't show the character the user typed.
// 				Text_SetCharAtXY(x, start_y, the_cursor_char_code);
// 				//gotoxy(x, SPLASH_GET_NAME_INPUT_Y);
// 			}
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