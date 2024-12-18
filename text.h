//! @file text.h

/*
 * text.h
 *
*  Created on: Feb 19, 2022
 *      Author: micahbly
 */

// This is a cut-down, semi-API-compatible version of the OS/f text.c file from Lich King (Foenix)
// adapted for Foenix F256 Jr starting November 29, 2022

#ifndef LIB_TEXT_H_
#define LIB_TEXT_H_


/* about this library: TextDisplay
 *
 * This handles writing and reading information to/from the VICKY's text mode memory
 *
 *** things this library needs to be able to do
 * work with either channel A or channel B
 * clear / fill an entire screen of text characters
 * clear / fill an entire screen of text attributes
 * invert the colors of a screen
 * clear / fill a smaller-than-screen rectangular area of text/attrs
 * Draw a char to a specified x, y coord
 * Get the currently displayed character at the specified coord
 * Set the foreground and background colors at the specified coord
 * Set the attribute value at the specified coord
 * Get the attribute value at the specified coord
 * Get the foreground or background color at the specified coord
 * draw a line using "graphic" characters
 * draw a box using "graphic" characters
 * copy a full screen of text or attr from an off-screen buffer
 * copy a full screen of text or attr TO an off-screen buffer
 * copy a full screen of text and attr between channel A and B
 * copy a rectangular area of text or attr TO/FROM an off-screen buffer
 * display a string at a specified x, y coord (no wrap)
 * display a pre-formatted string in a rectangular block on the screen, breaking on \n characters
 * display a string in a rectangular block on the screen, with wrap
 * display a string in a rectangular block on the screen, with wrap, taking a hook for a "display more" event, and scrolling text vertically up after hook func returns 'continue' (or exit, returning control to calling func, if hook returns 'stop')
 * replace current text font with another, loading from specified ram loc.
 */


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "general.h"

// C includes
#include <stdbool.h>
#include <stdint.h>


// F256 includes


// cc65 includes


/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/

#define SCREEN_NUM_COLS			80	// physical screens are all 80 columns wide in f256jr
#define SCREEN_NUM_ROWS			60
#define SCREEN_LAST_COL			(SCREEN_NUM_COLS - 1)
#define SCREEN_LAST_ROW			(SCREEN_NUM_ROWS - 1)
#define SCREEN_TOTAL_BYTES		(SCREEN_NUM_COLS * SCREEN_NUM_ROWS)
#define SCREEN_TEXT_MEMORY_LOC			0xC000	// start of text AND attribute memory for F256jr. text is is I/O page 2, attributes in I/O page 3. 

// named F256JR Default Colors (at least in SuperBASIC)
#define COLOR_BLACK				(uint8_t)0x00
#define COLOR_MEDIUM_GRAY		(uint8_t)0x01
#define COLOR_BLUE				(uint8_t)0x02
#define COLOR_GREEN				(uint8_t)0x03
#define COLOR_MAGENTA			(uint8_t)0x04
#define COLOR_BROWN				(uint8_t)0x05
#define COLOR_ORANGE			(uint8_t)0x06
#define COLOR_CYAN				(uint8_t)0x07
#define COLOR_DARK_GRAY			(uint8_t)0x08
#define COLOR_LIGHT_GRAY		(uint8_t)0x09
#define COLOR_BRIGHT_BLUE		(uint8_t)0x0A
#define COLOR_BRIGHT_GREEN		(uint8_t)0x0B
#define COLOR_BRIGHT_MAGENTA	(uint8_t)0x0C
#define COLOR_RED				(uint8_t)0x0D
#define COLOR_BRIGHT_YELLOW		(uint8_t)0x0E
#define COLOR_BRIGHT_WHITE		(uint8_t)0x0F

// update: the numbers shown in vicky2 file in morfe don't match up to what's shown on screen, at least with a2560 config. eg, 20/00/00 is not a super dark blue, it's some totally bright thing. need to spend some time mapping these out better. But since user configurable, will wait until real machine comes and I can make sure of what's in flash rom. 

// Named function parameters
#define PARAM_FOR_TEXT_ATTR				true	///< param for functions with for_attr
#define PARAM_FOR_TEXT_CHAR				false	// param for functions with for_attr

#define PARAM_COPY_TO_SCREEN			true	// param for functions doing block copy to/from screen / off-screen buffer
#define PARAM_COPY_FROM_SCREEN			false	// param for functions doing block copy to/from screen / off-screen buffer

#define PARAM_CLEAR_FIRST				true	///< param for functions that take a boolean determining if the space or object in question should be cleared first
#define PARAM_DO_NOT_CLEAR_FIRST		false	///< param for functions that take a boolean determining if the space or object in question should be cleared first

#define PARAM_ENCLOSE_HEADER			true	///< param for text window creation function. Using this will result in title having another text-line drawn immediately under it
#define PARAM_DO_NOT_ENCLOSE_HEADER		false	///< param for text window creation function. Using this will result in title being place in 'body' area of window

#define PARAM_USE_OVERWRITE_MODE		true	///< param for routines like Text_GetStringFromUser that handle getting string input from user. Using this will start routine in overwrite mode, so that each character typed overwrites any that was there before. 
#define PARAM_USE_INSERT_MODE			false	///< param for routines like Text_GetStringFromUser that handle getting string input from user. Using this will start routine in insert mode, so that each character typed causes any characters to the right of the cursor position to move over to make room for the new character.

#define PARAM_USE_PRIMARY_FONT_SLOT		true	///< param for font loading and switching functions. Using this will result in loading or switching to the primary text font in VICKY memory.
#define PARAM_USE_SECONDARY_FONT_SLOT	false	///< param for font loading and switching functions. Using this will result in loading or switching to the secondary text font in VICKY memory.


/*****************************************************************************/
/*            Char codes for drawing elements (f256jr_std)                   */
/*****************************************************************************/

// (see f256.h)

/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/

//typedef enum text_draw_choice
//{
#define CHAR_ONLY		0
#define ATTR_ONLY 		1
#define CHAR_AND_ATTR	2
//} text_draw_choice;


// **** Move these back into OS/f Text Library in the future!
//typedef enum dialog_result
#define DIALOG_ERROR	-2
#define DIALOG_CANCEL	-1
#define DIALOG_BTN_1	0
#define DIALOG_BTN_2	1
#define DIALOG_BTN_3	2


#define TEXT_DIALOG_MAX_BTN_LABEL_LEN	77	// F256jr supports max of 80 chars wide screen, and 2 chars required for drawing borders, and 1 char space to right required. 



/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/

// **** Move these back into OS/f Text Library in the future!

typedef struct TextDialogTemplate
{
	uint8_t		x_;
	uint8_t		y_;
	uint8_t		width_;
	uint8_t		height_;
	uint8_t		num_buttons_;
	char*		title_text_;
	char*		body_text_;
	char*		btn_label_[3];
	uint16_t	btn_keycolor_[3];
	uint8_t		btn_shortcut_[3];
	uint8_t		default_button_id_;
	uint8_t		default_button_shortcut_;
	uint8_t		cancel_button_shortcut_;
} TextDialogTemplate;


// **** Move these back into OS/f Text Library in the future!

/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/


// ** NOTE: there is no destructor or constructor for this library, as it does not track any allocated memory.


// **** Block copy functions ****

//! Copies characters and attributes from the left, to the right, for the passed length, backfilling with the char and attr passed
//!   Shift never extends beyond the current row of text. 
//! @param	working_buffer - valid pointer to a block of memory at least SCREEN_NUM_COLS in size, to act as a temporary line buffer for the operation. 
//! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	shift_count - the number of character positions text will be shifted. eg, '1' will shift everything to the right of x by 1 character.
//! @param	backfill_char - the character to place in the space freed up by copy. eg, if you shift 10 chars at positions 60-69 to 70-79, this char will be used to fill the slots from 60-69. 
//! @param	backfill_fore_color - foreground color that will be applied to the space opened up by the copy
//! @param	backfill_back_color - background color that will be applied to the space opened up by the copy
//! @return	Returns false on any error/invalid input.
bool Text_ShiftTextAndAttrRight(uint8_t* working_buffer, uint8_t x, uint8_t y, uint8_t shift_count, uint8_t backfill_char, uint8_t backfill_fore_color, uint8_t backfill_back_color);

//! Copies characters and attributes from the right, to the left, for the passed length, backfilling with the char and attr passed
//!   Shift never extends beyond the current row of text. 
//! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	shift_count - the number of character positions text will be shifted. eg, '1' will shift everything to the left of x by 1 character.
//! @param	backfill_char - the character to place in the space freed up by copy. eg, if you shift 10 chars at positions 70-79 to 60-69, this char will be used to fill the slots from 70-79. 
//! @param	backfill_fore_color - foreground color that will be applied to the space opened up by the copy
//! @param	backfill_back_color - background color that will be applied to the space opened up by the copy
//! @return	Returns false on any error/invalid input.
bool Text_ShiftTextAndAttrLeft(uint8_t x, uint8_t y, uint8_t shift_count, uint8_t backfill_char, uint8_t backfill_fore_color, uint8_t backfill_back_color);

//! scrolls all or part of text rows up ONE row.
//!   row 0 content is lost. row 1 becomes row 0, row 24 becomes row 23, row 24 is cleared.
//!   call Text_SetXY() first to establish the starting x,y
//!   set _zp_y_cnt to the number of rows from starting y to include in the scroll
//! @param	num_cols - the number of characters from starting x to include in the scroll
void __fastcall__ Text_ScrollTextUp(uint8_t num_cols);

//! scrolls all or part of text rows down ONE row.
//!   e.g, row 24 is lost. row 23 becomes row 24, row 1 becomes row 0
//!   call Text_SetXY() first to establish the starting x,y
//!   set _zp_y_cnt to the number of rows from starting y to include in the scroll
//! @param	num_cols - the number of characters from starting x to include in the scroll
void __fastcall__ Text_ScrollTextDown(uint8_t num_cols);

//! scrolls the text and attribute memory up ONE row.
//!   e.g, row 0 is lost. row 1 becomes row 0, row 49 becomes row 48, row 49 is cleared.
//! @param	y1 - the first row to scroll up
//! @param	y2 - the last row to scroll up
//! @return	Returns false on any error/invalid input.
bool Text_ScrollTextAndAttrRowsUp(uint8_t y1, uint8_t y2);

//! scrolls the text and attribute memory down ONE row.
//!   e.g, row 59 is lost. row 58 becomes row 59, row 0 becomes row 1, row 0 is cleared.
//! @param	y1 - the first row to scroll down
//! @param	y2 - the last row to scroll down. y2+1 is overwritten, y2+2 and beyond are left as is.
//! @return	Returns false on any error/invalid input.
bool Text_ScrollTextAndAttrRowsDown(uint8_t y1, uint8_t y2);

//! Copy a linear run of text or attr to or from a linear memory buffer.
//!   Use this if you do not have a full-sized (screen-size) off-screen buffer, and do not have a rectangular area 
//!   of the screen to copy to/from, but instead want to copy a single linear stream to/from a particular cursor position. 
//! @param	the_buffer - valid pointer to a block of memory to hold (or alternatively act as the source of) the character or attribute data for the specified screen memory. This will be read from first byte to last byte, without skipping. e.g., if you want to copy a 227 characters of text from the middle of the screen to this buffer, the buffer must be 227 bytes in length, and data will be written contiguously to it. 
//! @param	x - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	to_screen - true to copy to the screen from the buffer, false to copy from the screen to the buffer. Recommend using PARAM_COPY_TO_SCREEN/PARAM_COPY_FROM_SCREEN.
//! @param	for_attr - true to work with attribute data, false to work character data. Recommend using PARAM_FOR_TEXT_ATTR/PARAM_FOR_TEXT_CHAR.
//! @return	Returns false on any error/invalid input.
bool Text_CopyMemLinearBuffer(uint8_t* the_buffer, uint8_t x, uint8_t y, uint16_t the_len, bool to_screen, bool for_attr);

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
bool Text_CopyMemBoxLinearBuffer(uint8_t* the_buffer, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool to_screen, bool for_attr);

//! Copy a rectangular area of text or attr to or from an off-screen buffer of the same size as the physical screen buffer
//! @param	the_buffer - valid pointer to a block of memory to hold (or alternatively act as the source of) the character or attribute data for the specified rectangle of screen memory. This buffer must be the same size as the physical screen!
//! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	to_screen - true to copy to the screen from the buffer, false to copy from the screen to the buffer. Recommend using PARAM_COPY_TO_SCREEN/PARAM_COPY_FROM_SCREEN.
//! @param	for_attr - true to work with attribute data, false to work character data. Recommend using PARAM_FOR_TEXT_ATTR/PARAM_FOR_TEXT_CHAR.
//! @return	Returns false on any error/invalid input.
bool Text_CopyMemBox(uint8_t* the_buffer, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool to_screen, bool for_attr);


// **** Block fill functions ****


//! Clear the text screen and reset foreground and background colors
void Text_ClearScreen(uint8_t fore_color, uint8_t back_color);

//! Fill attribute or text char memory. 
//! @param	for_attr - true to work with attribute data, false to work character data. Recommend using PARAM_FOR_TEXT_ATTR/PARAM_FOR_TEXT_CHAR.
//! @param	the_fill - either a 1-byte character code, or a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_FillMemory(bool for_attr, uint8_t the_fill);

//! Fill character and attribute memory for a specific box area
//! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_char - the character to be used for the fill operation
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_FillBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t the_char, uint8_t fore_color, uint8_t back_color);

// //! Fill character memory for a specific box area
// //! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	the_char - the character to be used for the fill operation
// //! @return	Returns false on any error/invalid input.
// bool Text_FillBoxCharOnly(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t the_char);

//! Fill attribute memory for a specific box area
//! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_FillBoxAttrOnly(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fore_color, uint8_t back_color);

// //! Invert the colors of a rectangular block.
// //! As this requires sampling each character cell, it is no faster (per cell) to do for entire screen as opposed to a subset box
// //! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @return	Returns false on any error/invalid input.
// bool Text_InvertBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

//! Invert the colors of a linear run of text starting at the current x,y position
//!  call Text_SetXY() prior to calling
//! this function leaves x, y, and vram ptr at the initial position, not the final position
//! @param	the_length - the number of character positions that should be inverted, starting from the current x,y position.
void __fastcall__ Text_Invert(int the_length);




// **** FONT RELATED *****

//! replace the current font data with the data at the passed memory buffer
//! @param	new_font_data - Pointer to 2K (256 characters x 8 lines/bytes each) of font data. Each byte represents one line of an 8x8 font glyph.
//! @param	for_primary_font - true to update the primary font, false to update the secondary font. Recommend using PARAM_USE_PRIMARY_FONT_SLOT/PARAM_USE_SECONDARY_FONT_SLOT.
//! @return	Returns false on any error/invalid input.
bool Text_UpdateFontData(char* new_font_data, bool for_primary_font);



// **** Cursor positioning functions *****

//! Move cursor to the specified x, y coord
//! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
void Text_SetXY(uint8_t x, uint8_t y);

//! Return the current cursor location's X coordinate
//! @return	the horizontal position, between 0 and the screen's text_cols_vis_ - 1
uint8_t Text_GetX(void);

//! Return the current cursor location's Y coordinate
//! @return	the vertical position, between 0 and the screen's text_rows_vis_ - 1
uint8_t Text_GetY(void);



// **** Set char/attr functions *****


//! Set a char at a specified x, y coord, and advance cursor position by 1
//! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_char - the character to be used
//! @return	Returns false on any error/invalid input.
bool Text_SetCharAtXY(uint8_t x, uint8_t y, uint8_t the_char);

//! Set the attribute value at a specified x, y coord, and advance cursor position by 1
//! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_attribute_value - a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_SetAttrAtXY(uint8_t x, uint8_t y, uint8_t the_attribute_value);

//! Set the attribute value at a specified x, y coord based on the foreground and background colors passed, and advance cursor position by 1
//! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_SetColorAtXY(uint8_t x, uint8_t y, uint8_t fore_color, uint8_t back_color);

// //! Draw a char at a specified x, y coord, also setting the color attributes
// //! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
// //! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
// //! @param	the_char - the character to be used
// //! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
// //! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
// //! @return	Returns false on any error/invalid input.
// bool Text_SetCharAndAttrAtXY(uint8_t x, uint8_t y, uint8_t the_char, uint8_t the_attribute_value);

//! Draw a char at a specified x, y coord, also setting the color attributes, and advance cursor position by 1
//! @param	x - the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_char - the character to be used
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_SetCharAndColorAtXY(uint8_t x, uint8_t y, uint8_t the_char, uint8_t fore_color, uint8_t back_color);

//! Set a char at the current X/Y position, and advance cursor position by 1
//! @param	the_char - the character to be used
//! @return	Returns false on any error/invalid input.
bool Text_SetChar(uint8_t the_char);

//! Set the attribute value at the current X/Y position
//! @param	the_attribute_value - a 1-byte attribute code (foreground in high nibble, background in low nibble)
//! @return	Returns false on any error/invalid input.
bool Text_SetAttr(uint8_t the_attribute_value);

//! Set the attribute value at the current X/Y position based on the foreground and background colors passed
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_SetColor(uint8_t fore_color, uint8_t back_color);

// //! Draw a char at the current X/Y position, also setting the color attributes
// //! @param	the_char - the character to be used
// //! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
// //! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
// //! @return	Returns false on any error/invalid input.
// bool Text_SetCharAndAttr(uint8_t the_char, uint8_t the_attribute_value);

//! Draw a char at the current X/Y position, also setting the color attributes, and advance cursor position by 1
//! @param	the_char - the character to be used
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_SetCharAndColor(uint8_t the_char, uint8_t fore_color, uint8_t back_color);

// copy n-bytes into display memory, at the X/Y position specified
void Text_DrawCharsAtXY(uint8_t x, uint8_t y, uint8_t* the_buffer, uint16_t the_len);

// // copy n-bytes into display memory, at the current X/Y position
// bool Text_DrawCharsOLD(uint8_t* the_buffer, uint16_t the_len);

//! Copy n-bytes into display memory, at the current X/Y position
//! call Text_SetXY() prior to calling
//! set zp_ptr to the buffer address prior to calling
//! @param	the_length - the number of characters to be drawn
void __fastcall__ Text_DrawChars(int the_length);

//! draws the byte passed as 2 hex characters, at the current X,y pos
//!   call Text_SetXY() first to establish the starting x,y
//! @param	the_byte - the 8-bit number to be converted to hex and written to screen as 2 hex chars
void __fastcall__ Text_DrawByteAsHexChars (uint8_t the_byte);


// **** Get char/attr functions *****

//! Get the char at the current X/Y position
//! @return	Returns the character code at the current screen location
uint8_t Text_GetChar(void);

//! Get the char at the current X/Y position - 1. Does not change what the current X/Y pos is.
//! @return	Returns the character code at the char preceeding the current screen location
uint8_t Text_GetPrevChar(void);

//! Get the char at the current X/Y position + 1. Does not change what the current X/Y pos is.
//! @return	Returns the character code at the char following the current screen location
uint8_t Text_GetNextChar(void);

//! Get the char at the specified X/Y position, without changing the text engine's current position marker
//! @param	x: the horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y: the vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @return	Returns the character code at the specified screen location
uint8_t Text_GetCharAtXY(uint8_t x, uint8_t y);




// **** Drawing functions *****


//! Draws a horizontal line from specified coords, for n characters, using the specified char and/or attribute
//! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_line_len - The total length of the line, in characters, including the start and end character.
//! @param	the_char - the character to be used when drawing
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	the_draw_choice - controls the scope of the action, and is one of CHAR_ONLY, ATTR_ONLY, or CHAR_AND_ATTR. See the text_draw_choice enum.
void Text_DrawHLine(uint8_t x, uint8_t y, uint8_t the_line_len, uint8_t the_char, uint8_t fore_color, uint8_t back_color, uint8_t the_draw_choice);

//! Draws a vertical line from specified coords, for n characters, using the specified char and/or attribute
//! @param	x - the starting horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y - the starting vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	the_line_len - The total length of the line, in characters, including the start and end character.
//! @param	the_char - the character to be used when drawing
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	the_draw_choice - controls the scope of the action, and is one of CHAR_ONLY, ATTR_ONLY, or CHAR_AND_ATTR. See the text_draw_choice enum.
void Text_DrawVLine(uint8_t x, uint8_t y, uint8_t the_line_len, uint8_t the_char, uint8_t fore_color, uint8_t back_color, uint8_t the_draw_choice);

//! Draws a box based on 2 sets of coords, using the predetermined line and corner "graphics", and the passed colors
//! @param	x1 - the leftmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y1 - the uppermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	x2 - the rightmost horizontal position, between 0 and the screen's text_cols_vis_ - 1
//! @param	y2 - the lowermost vertical position, between 0 and the screen's text_rows_vis_ - 1
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
void Text_DrawBoxCoordsFancy(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fore_color, uint8_t back_color);



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
bool Text_DrawStringAtXY(uint8_t x, uint8_t y, char* the_string, uint8_t fore_color, uint8_t back_color);

//! Draw a string at the current X/Y position, also setting the color attributes.
//! If it is too long to display on the line it started, it will be truncated at the right edge of the screen.
//! No word wrap is performed. 
//! @param	the_string - the null-terminated string to be drawn
//! @param	fore_color - Index to the desired foreground color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @param	back_color - Index to the desired background color (0-15). The predefined macro constants may be used (COLOR_DK_RED, etc.), but be aware that the colors are not fixed, and may not correspond to the names if the LUT in RAM has been modified.
//! @return	Returns false on any error/invalid input.
bool Text_DrawString(char* the_string, uint8_t fore_color, uint8_t back_color);





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
bool Text_DrawWindow(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t accent_color, uint8_t fore_color, uint8_t back_color, char* the_header_text, char* char_save_mem, char* attr_save_mem, bool clear_first, bool enclose_header);

// Display a Text-based dialog box, with 1, 2, or 3 buttons, a title, and a message body
// Supports keyboard shortcuts for each button
// on A2560Ks, will use keyboard lighting to highlight the keyboard shortcuts
// returns -1 on error (such as can't fit buttons into the space allowed, or a button string is empty, etc.)
// returns 0, 1, 2 indicating which button was selected
// user hitting ESC will always cause -1 to be returned, regardless of keyboard shortcuts. This effectively gives you 2 shortcuts to say 'no'/'cancel'/'go back'.
//! @param	accent_color - Index to the desired accent color (0-15).
//! @param	fore_color - Index to the desired foreground color (0-15).
//! @param	back_color - Index to the desired background color (0-15).
//! @param	cancel_color - Index to the desired foreground color (0-15) for the button that represents the cancel action
//! @param	affirm_color - Index to the desired background color (0-15) for the button that represents the confirm/affirm/ok action
int8_t Text_DisplayDialog(TextDialogTemplate* the_dialog_template, char* char_save_mem, char* attr_save_mem, uint8_t accent_color, uint8_t fore_color, uint8_t back_color, uint8_t cancel_color, uint8_t affirm_color);

// Display a Text-based dialog box, with a title, a message body, and a place for users to input text
// returns false on error (eg, max string len is wider than dialog body), or if user refused to enter text
// returns true if user entered something
// populates the passed buffer with the text the user typed
// user hitting ESC will always cause false to be returned, regardless of keyboard shortcuts.
//! @param	accent_color - Index to the desired accent color (0-15).
//! @param	fore_color - Index to the desired foreground color (0-15).
//! @param	back_color - Index to the desired background color (0-15).
int8_t Text_DisplayTextEntryDialog(TextDialogTemplate* the_dialog_template, char* char_save_mem, char* attr_save_mem, char* the_buffer, int8_t the_max_length, uint8_t accent_color, uint8_t fore_color, uint8_t back_color);




// **** User Input Functions ****

// get a string from the user and store in the passed buffer, drawing chars to screen as user types
// allows a maximum of the_max_length characters. Buffer must allow for max_length chars + a terminator!
// set overwrite_mode to true to start in overwrite, or false to start in auto-insert mode. Can be overridden by keyboard action. Use PARAM_USE_OVERWRITE_MODE/PARAM_USE_INSERT_MODE
// returns false if no string built.
bool Text_GetStringFromUser(char* the_buffer, int8_t the_max_length, uint8_t start_x, uint8_t start_y, bool overwrite_mode);



#endif /* LIB_TEXT_H_ */
