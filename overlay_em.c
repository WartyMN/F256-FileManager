/*
 * overlay_em.c
 *
 *  Created on: Mar 7, 2024
 *      Author: micahbly
 *
 *  Routines for loading files into extended memory and manipulating/viewing EM
 *    all routines here are designed to be run as standalone, without reference to data or code in MAIN
 *    everything in this overlay should assume MAIN is OUT and unavailable
 *    (the primary feature of this overlay is to swap slices of EM into $2000-3FFF)
 *
 */



/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "overlay_em.h"
#include "app.h"
#include "comm_buffer.h"
#include "file.h"
#include "general.h"
#include "kernel.h"
#include "keyboard.h"
#include "memory.h"
#include "sys.h"
#include "text.h"
#include "strings.h"

// C includes
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// F256 includes
#include "f256.h"




/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/

#define	CH_LINE_BREAK	10


/*****************************************************************************/
/*                           File-scope Variables                            */
/*****************************************************************************/


#pragma data-name ("OVERLAY_3")

static uint8_t				em_temp_buffer_384b_storage[384];
static uint8_t*				em_temp_buffer_384b = em_temp_buffer_384b_storage;



/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

extern char*				global_string[NUM_STRINGS];
extern char*				global_string_buff1;
extern char*				global_string_buff2;

extern uint8_t				zp_bank_num;
extern uint8_t				io_bank_value_kernel;	// stores value for the physical bank pointing to C000-DFFF whenever we change it, so we can restore it.

#pragma zpsym ("zp_bank_num");


/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

// a partner to Inventory_DisplayStringWithLineBreaks
// displays the passed string starting at the x,y coord passed, line-wrapping where needed
// returns NULL if entire string was displayed, or returns pointer to next char needing display if available space was all used
char* EM_WrapAndDisplayString(char* the_message, uint8_t x, uint8_t y, uint8_t col_width, uint8_t max_allowed_rows);


/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/

// a partner to Inventory_DisplayStringWithLineBreaks
// displays the passed string starting at the x,y coord passed, line-wrapping where needed
// returns the number of lines it made it into. eg, wraps twice, it will return 3.
// returns NULL if entire string was displayed, or returns pointer to next char needing display if available space was all used
char* EM_WrapAndDisplayString(char* the_message, uint8_t x, uint8_t y, uint8_t col_width, uint8_t max_allowed_rows)
{
	uint16_t	the_len;
	uint8_t		lines_needed = 0;
	uint8_t		num_chars_checked = 0;
	char*		right_margin;
	char*		this_char;
	char*		start_of_string = the_message;
	bool		eol_found = false;
	bool		no_break_found = false;

	// check if string is even going to need wrapping
	the_len = strlen(the_message);

	while(*start_of_string)
	{
		// even if string IS shorter than width, we still have to process it, as it may contain several lines with line breaks
		
// 		// is remaining part of string small enough to already fit within the margins?
// 		if(the_len < col_width)
// 		{
// 			// pass the left-side, wrapped string to a helper method that will add any line breaks, and do actual display.
// 			Text_DrawStringAtXY(x, y, start_of_string, COLOR_BRIGHT_WHITE, COLOR_BLACK);
// 			//return ++lines_needed;
// 					sprintf(global_string_buff1, "string was shorter than screen width (len=%u)", the_len);
// 					Buffer_NewMessage(global_string_buff1);
// 			return NULL;
// 		}

		// before checking if word wrap is needed, see if there is a line break before margin
		this_char = start_of_string;
		right_margin = this_char + col_width;

		//debug
		if (this_char[0] == 0)
		{
			Buffer_NewMessage("first char of string was a terminator!");
		}
		
		while(*this_char && num_chars_checked < col_width && !eol_found)
		{
			if (*this_char == CH_LINE_BREAK)
			{
				right_margin = this_char;
				eol_found = true;
			}

			++this_char;
			++num_chars_checked;			
		}

		// is remaining part of string small enough to already fit within the margins?
		if(eol_found == false && the_len < col_width)
		{
			// pass the left-side, wrapped string to a helper method that will add any line breaks, and do actual display.
			Text_DrawStringAtXY(x, y, start_of_string, COLOR_BRIGHT_WHITE, COLOR_BLACK);
			//return ++lines_needed;
			return NULL;
		}

		while(*right_margin != ' ' && *right_margin != CH_LINE_BREAK && right_margin > start_of_string)
		{
			--right_margin;
		}

		// check for possibility of a run of 80 chars, with no space, and not terminated by line break.
		if (right_margin == start_of_string && eol_found == false)
		{
			right_margin = (start_of_string + col_width) - 0;
			no_break_found = true;
		}
		
		// cut off the left part of the string at this point
		*right_margin = '\0';

		// pass the left-side, wrapped string to a helper method that will add any line breaks, and do actual display.
		Text_DrawStringAtXY(x, y, start_of_string, COLOR_BRIGHT_WHITE, COLOR_BLACK);
		++y;
		++lines_needed;

		the_len -= (right_margin - start_of_string) + 1; // +1 is for the space or end-of-line
		
		if (no_break_found == true)
		{
			start_of_string = start_of_string + col_width;
		}
		else
		{
			start_of_string = right_margin + 1;
		}

		// are we ending up with more lines than can be shown on screen?
		if (lines_needed >= max_allowed_rows)
		{
			// just cut it off and return
			//return lines_needed;
			return start_of_string;
		}
	}

	//return lines_needed;
	return NULL;
}



/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/


// also causing line breaking, by calling it's partner function as it finds the end of string/para. <- character means line break.
// returns NULL if entire string was displayed, or returns pointer to next char needing display if available space was all used
char* EM_DisplayStringWithLineBreaks(char* the_message, uint8_t x, uint8_t y, uint8_t col_width, uint8_t max_allowed_rows)
{
	char*	this_char;

	bool	end_of_string = false;
	uint8_t	reformatted_lines = 0;
	uint8_t	lines_needed = 0;
	char*	start_of_string = the_message;
	char*	next_starting_pos;

	while(*start_of_string)
	{
		this_char = start_of_string;

// 		while(*this_char != CH_LINE_BREAK && *this_char)
// 		{
// 			++this_char;
// 		}
// 
// 		// hit a line break or the end of the string?
// 		if (!*this_char)
// 		{
// 			end_of_string = true;
// 		}
// 		else
// 		{
// 			*this_char = '\0';
// 		}

		// now put the first para of text through the line breaker, which will handle final display.
		next_starting_pos = EM_WrapAndDisplayString(start_of_string, x, y, col_width, 1);	// only ask it to wrap ONE line at a time
// 		y += reformatted_lines; // if wrapping requires more lines than our buffer can display, need to track that and pause
// 		lines_needed += reformatted_lines;
		++y;
		++lines_needed;
		
		// are we ending up with more lines than the calling method wants to permit? or did we hit end of string?
		if (next_starting_pos == NULL)
		{
			return NULL;
		}
// 		if (end_of_string)
// 		{
// 			return NULL;
// 		}
		else if (lines_needed >= max_allowed_rows)
		{
			// just cut it off and return
			return next_starting_pos;
		}

// 		if (next_starting_pos == NULL)
// 		{
// 			start_of_string = this_char + 1;
// 		}
// 		else
// 		{
// 			start_of_string = next_starting_pos;
// 		}
		start_of_string = next_starting_pos;
	}

	return NULL;
}


// displays the content found in EM as a text file, with wrapping, etc.
// num_chunks is the number of EM 256b chunks that need displaying
void EM_DisplayAsText(uint8_t num_chunks)
{
	// LOGIC
	//   Data must have already been loaded into EM_STORAGE_START_PHYS_ADDR
	//   If used to display random binary data, the results will be pretty unpleasant
	//   user can hit esc / runstop to stop at any time, or any other key to continue to the next screenful
	
	// LOGIC
	//   we only have 80x60 to work with, and we need a row for "hit space for more, esc to stop"
	//     so 59 rows * 80 bytes = 4720 max bytes can be shown
	//   we need 3 buffers:
	//     1 to capture the 256b coming in from EM each EM read
	//     1 to perform as a word/line-wrapped version of each 80 char row.
	//       with tab expansion, 80 bytes from EM might end up as 80*4=320. that's not a real scenario tho. 
	//       we just need an 81 byte buffer: get what's needed for 1 line, write as string with Text lib.
	//       would be faster, probably, to fill a 5k buffer fully, and memcpy straight to VRAM, but 5k is expensive. do in future if this is slow.
	//     1 buffer to hold what's left from the 256k read, after the last full line is written. by definition has to be less than 80 chars.
	//       after detecting we have <80 chars left, and more chunks to go, copy remainder to another 256b buffer. copy more in. process remainder buffer first.

	uint8_t		y;
	uint8_t		user_input;
//	uint16_t	num_bytes_to_read = MEM_TEXT_VIEW_BYTES_PER_ROW;
	bool		keep_going = true;
	bool		copy_again;
	uint8_t*	copy_buffer;
	uint8_t*	buffer_curr_loc;
	char*		remainder_buffer;
	char*		line_buffer;
	char*		line_buffer_remainder;
	uint8_t		i = 0;
	int16_t		unprocessed_bytes;
	//int16_t		copy_buffer_len;
	uint16_t	remain_len = 0;
	uint8_t		needed_bytes;
	uint8_t		available_bytes;
	uint8_t		bytes_displayed;
	int8_t		bytes_not_displayed;
	
	// primary local buffer will use 256b interbank buffers for file.
	//copy_buffer = (char*)STORAGE_FILE_BUFFER_1;
	copy_buffer = em_temp_buffer_384b;
	
	// catch-basin/remainder-of-text buffer will use the smaller 204b temp buffer. cannot use Get string buffer as we'll need that on screen changes.
	remainder_buffer = (char*)STORAGE_STRING_BUFFER_1;
	remainder_buffer[0] = 0;
	
	// set up the display line buffer, using the other 204b interbank buffers. (STORAGE_STRING_BUFFER_1_LEN)
	line_buffer = (char*)STORAGE_STRING_BUFFER_2;
	line_buffer[0] = 0;

	//Buffer_NewMessage("starting text display...");
	
	// loop until data is all displayed, or until user says stop
	
	// Page display loop
	y = 0;
	
	// EM chunk read loop
	while (keep_going == true && i < num_chunks)
	{
		App_EMDataCopy(copy_buffer, i++, PARAM_COPY_FROM_EM);

		buffer_curr_loc = copy_buffer;
// 		copy_buffer_len = General_Strnlen((char*)copy_buffer, STORAGE_FILE_BUFFER_1_LEN);	// can't assume 256b, last chunk may have less.
// 		if (i == num_chunks)
// 		{
// 			unprocessed_bytes = copy_buffer_len;	// can't assume 256b, last chunk may have less.
// 		}
// 		else
// 		{
// 			unprocessed_bytes = 256;
// 		}

		unprocessed_bytes = 256;
		copy_again = false;
		
		// per-row display loop
		do
		{		
			if (y == 0)
			{
				Text_ClearScreen(FILE_CONTENTS_FOREGROUND_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);
// 				sprintf(global_string_buff1, General_GetString(ID_STR_MSG_TEXT_VIEW_INSTRUCTIONS), "Memory View");
// 				Text_DrawStringAtXY(0, y++, global_string_buff1, FILE_CONTENTS_ACCENT_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);
			}

			// if there were any remaining bytes from the last EM copy, process those first
			needed_bytes =  MEM_TEXT_VIEW_BYTES_PER_ROW - remain_len;
	
			if (unprocessed_bytes > 0)
			{
				// determine how many bytes we have to work with, might not be enough for a full row
				if (unprocessed_bytes > needed_bytes)
				{
					available_bytes = needed_bytes;
				}
				else
				{
					available_bytes = unprocessed_bytes;
				}

// 				sprintf(global_string_buff1, "remain_len=%u, needed=%u, unprocessed=%u, avail=%u", remain_len, needed_bytes, unprocessed_bytes, available_bytes);
// 				Buffer_NewMessage(global_string_buff1);
			
				// either copy in a full (max row) worth of bytes from copy buffer, or top up the line buffer so it is as max (=80 chars=1 row on screen)
				if (remain_len > 0)
				{
					memcpy(line_buffer + remain_len, buffer_curr_loc, available_bytes);
					line_buffer[80] = 0;
// 						sprintf(global_string_buff1, "line_buffer='%s' after copying %u more", line_buffer, available_bytes);
// 						Buffer_NewMessage(global_string_buff1);
				}
				else
				{
					memcpy(line_buffer, buffer_curr_loc, available_bytes);
				}
				
				buffer_curr_loc += available_bytes;
				unprocessed_bytes -= available_bytes;
		
// 						sprintf(global_string_buff1, "buffer_curr_loc=%p, unprocessed_bytes=%u, i=%u", buffer_curr_loc, unprocessed_bytes, i);
// 						Buffer_NewMessage(global_string_buff1);
			}
			else
			{
				// stop after this display pass
				keep_going = false;
			}
			
	
			// now we have handled the max use case for 1 row: a string exactly 80 chars in length that doesn't need wrapping
			
			// process and display the chars in the line buffer
			line_buffer_remainder = EM_DisplayStringWithLineBreaks(line_buffer, 0, y, MEM_TEXT_VIEW_BYTES_PER_ROW, 1);
			
			// set up for the next line: copy remainder in line buffer to start of line buffer
			bytes_displayed = line_buffer_remainder - line_buffer;
			bytes_not_displayed = (MEM_TEXT_VIEW_BYTES_PER_ROW - bytes_displayed);
			
			if (bytes_not_displayed > 0)
			{
				memcpy(line_buffer, line_buffer_remainder, bytes_not_displayed);
				line_buffer[bytes_not_displayed] = 0;
			}
			else
			{
				line_buffer[0] = 0;
			}

			remain_len = General_Strnlen(line_buffer, STORAGE_STRING_BUFFER_1_LEN);
			
// 					sprintf(global_string_buff1, "displayed=%u, not_displayed=%i, line_buffer='%s'", bytes_displayed, bytes_not_displayed, line_buffer);
// 					Buffer_NewMessage(global_string_buff1);

			//user_input = Keyboard_GetChar();
			
			// check if we need to ask user to go on to a new screen
			++y;
			
			if (y == MAX_TEXT_VIEW_ROWS_PER_PAGE)
			{
				user_input = Keyboard_GetChar();
				
				if (user_input == CH_ESC || user_input == 'q' || user_input == CH_RUNSTOP)
				{
					keep_going = false;
				}
				else
				{
					y = 0;
				}
			}
			
			// check if we need to get more bytes from EM
			if (remain_len + unprocessed_bytes >= MEM_TEXT_VIEW_BYTES_PER_ROW)
			{
				// no, there are enough unprocessed bytes in the buffer to make up at least one full line
				//Buffer_NewMessage("no need to copy more yet");
				//Keyboard_GetChar();
			}
			else
			{
				// there isn't enough chars in line buffer and copy buffer to guarantee a full row.
				
				// have we already processed everything perhaps?
				if (unprocessed_bytes > 0)
				{
					// copy what's left into line buffer.
					if (remain_len == 0)
					{
						memcpy(line_buffer, buffer_curr_loc, unprocessed_bytes);
						line_buffer[unprocessed_bytes] = 0;
					}
					else
					{
						memcpy(line_buffer + remain_len, buffer_curr_loc, unprocessed_bytes);
						line_buffer[remain_len + unprocessed_bytes] = 0;
					}

					remain_len = General_Strnlen(line_buffer, STORAGE_STRING_BUFFER_1_LEN);
					// zero out anything left over from previous operations, as now we are not operating at full size for the line buffer.
					memset(line_buffer + remain_len, 0, MEM_TEXT_VIEW_BYTES_PER_ROW - remain_len);
					
// 						sprintf(global_string_buff1, "end of copy buffer: '%s', unprocessed=%u", buffer_curr_loc, unprocessed_bytes);
// 						Buffer_NewMessage(global_string_buff1);
// 						sprintf(global_string_buff1, "l buf: '%s', l buf remainder '%s'", line_buffer, line_buffer_remainder);
// 						Buffer_NewMessage(global_string_buff1);

					memset(copy_buffer, 0, STORAGE_FILE_BUFFER_1_LEN);

	 				//Keyboard_GetChar();
					
					unprocessed_bytes = 0;
					
					if (i < num_chunks)
					{
						copy_again = true;
					}
					else
					{
						// there isn't enough in copy buffer to fill a row, but also last chunk has already been read from EM
						
						
					}
				}
			}
			
		} while (copy_again == false && keep_going == true);		
	}
	
	// if user hasn't already said they are done, give them a chance to look at the last displayed page
	if (keep_going == true)
	{
		user_input = Keyboard_GetChar();
	}
	
	Keyboard_GetChar();
}

