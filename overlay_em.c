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
#define	CH_LINE_RETURN	13


/*****************************************************************************/
/*                           File-scope Variables                            */
/*****************************************************************************/


#pragma data-name ("OVERLAY_EM")

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
// 		// is remaining part of string small enough to already fit within the margins?
		// BUT: even if string IS shorter than width, we still have to process it, as it may contain several lines with line breaks
		
		// before checking if word wrap is needed, see if there is a line break before margin
		this_char = start_of_string;
		right_margin = this_char + col_width;

		//debug
		if (this_char[0] == 0)
		{
			//DEBUG_OUT(("%s %d: first char of string was a terminator!", __func__ , __LINE__));
		}
		
		while(*this_char && num_chars_checked < col_width && !eol_found)
		{
			if (*this_char == CH_LINE_BREAK || *this_char == CH_LINE_RETURN)
			{
				right_margin = this_char;
				eol_found = true;
				//DEBUG_OUT(("%s %d: line break detected @ %u: '%s'", __func__ , __LINE__, num_chars_checked, start_of_string));
			}

			++this_char;
			++num_chars_checked;			
		}

		// is remaining part of string small enough to already fit within the margins?
		if (eol_found == false && the_len < col_width)
		{
			// pass the left-side, wrapped string to a helper method that will add any line breaks, and do actual display.
			Text_DrawStringAtXY(x, y, start_of_string, COLOR_BRIGHT_WHITE, COLOR_BLACK);
			//DEBUG_OUT(("%s %d: this string had no breaks and fits without wrapping. len=%u '%s'", __func__ , __LINE__, the_len, start_of_string));
			return NULL;
		}

		while(*right_margin != ' ' && (*right_margin != CH_LINE_BREAK && *right_margin != CH_LINE_RETURN) && right_margin > start_of_string)
		{
			--right_margin;
		}

		// check for possibility of a run of 80 chars, with no space, and not terminated by line break.
		if (right_margin == start_of_string && eol_found == false)
		{
			right_margin = (start_of_string + col_width) - 0;
			no_break_found = true;
			//DEBUG_OUT(("%s %d: line with no spaces and no end-of-line marker: '%s'", __func__ , __LINE__, start_of_string));
		}
		
		// cut off the left part of the string at this point
		*right_margin = '\0';

		// pass the left-side, wrapped string to a helper method that will add any line breaks, and do actual display.
		//DEBUG_OUT(("%s %d: %s", __func__ , __LINE__, start_of_string));
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
			return start_of_string;
		}
	}

	return NULL;
}



/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/


// also causing line breaking, by calling it's partner function as it finds the end of string/para. <- character means line break.
// returns NULL if entire string was displayed, or returns pointer to next char needing display if available space was all used
char* EM_DisplayStringWithLineBreaks(char* the_message, uint8_t x, uint8_t y, uint8_t col_width, uint8_t max_allowed_rows)
{
	uint8_t	lines_needed = 0;
	char*	start_of_string = the_message;
	char*	next_starting_pos;

	while(*start_of_string)
	{
		// put the first para of text through the line breaker, which will handle final display.
		next_starting_pos = EM_WrapAndDisplayString(start_of_string, x, y, col_width, 1);	// only ask it to wrap ONE line at a time
		++y;
		++lines_needed;
		
		// are we ending up with more lines than the calling method wants to permit? or did we hit end of string?
		if (next_starting_pos == NULL)
		{
			return NULL;
		}
		else if (lines_needed >= max_allowed_rows)
		{
			// just cut it off and return
			return next_starting_pos;
		}

		start_of_string = next_starting_pos;
	}

	return NULL;
}


// displays the content found in EM as text, with wrapping, etc.
// em_bank_num is used to derive the base EM address
// num_pages is the number of EM 256b chunks that need displaying
// the_name is only used to provide feedback to the user about what they are viewing
void EM_DisplayAsText(uint8_t em_bank_num, uint8_t num_pages, char* the_name)
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

	uint8_t		i = 0;
	uint8_t		y = 0;
	uint8_t		user_input;
	uint8_t		needed_bytes;
	uint8_t		available_bytes;
	int8_t		bytes_not_displayed;
	bool		keep_going = true;
	bool		user_exit = false;
	bool		copy_again;
	uint8_t*	copy_buffer;
	uint8_t*	buffer_curr_loc;
	char*		line_buffer;
	char*		line_buffer_remainder;
	int16_t		unprocessed_bytes;
	uint16_t	remain_len = 0;
	
	// primary local buffer will use 384b dedicated storage in the EM overlay (only needs 256 technically, but this gives us some flex)
	copy_buffer = em_temp_buffer_384b;
	
	// set up the display line buffer, using the other 204b interbank buffers. (STORAGE_STRING_BUFFER_1_LEN)
	line_buffer = (char*)STORAGE_STRING_BUFFER_2;
	line_buffer[0] = 0;

	// EM chunk read loop
	while (keep_going == true && i < num_pages)
	{
		App_EMDataCopy(copy_buffer, em_bank_num, i++, PARAM_COPY_FROM_EM);

		buffer_curr_loc = copy_buffer;
		unprocessed_bytes = 256;
		copy_again = false;
		
		// per-row display loop
		do
		{		
			if (y == 0)
			{
				Text_ClearScreen(FILE_CONTENTS_FOREGROUND_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);
				sprintf(global_string_buff1, General_GetString(ID_STR_MSG_TEXT_VIEW_INSTRUCTIONS), the_name);
				Text_DrawStringAtXY(0, y++, global_string_buff1, FILE_CONTENTS_ACCENT_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);
				++y;
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

				// either copy in a full (max row) worth of bytes from copy buffer, or top up the line buffer so it is as max (=80 chars=1 row on screen)
				if (remain_len > 0)
				{
					memcpy(line_buffer + remain_len, buffer_curr_loc, available_bytes);
					line_buffer[80] = 0;
				}
				else
				{
					memcpy(line_buffer, buffer_curr_loc, available_bytes);
				}
				
				buffer_curr_loc += available_bytes;
				unprocessed_bytes -= available_bytes;
			}
			else
			{
				// stop after this display pass
				keep_going = false;
			}
			
	
			// now we have handled the max use case for 1 row: a string exactly 80 chars in length that doesn't need wrapping
			
			// process and display the chars in the line buffer
			line_buffer_remainder = EM_DisplayStringWithLineBreaks(line_buffer, 0, y, MEM_TEXT_VIEW_BYTES_PER_ROW, 1);
			//DEBUG_OUT(("%s %d: EM_DisplayStringWithLineBreaks return=%p", __func__ , __LINE__, line_buffer_remainder));
			
			if (line_buffer_remainder == NULL)
			{
				// whole string was displayed, whether it stretched across screen or not
				bytes_not_displayed = 0;
				line_buffer_remainder = line_buffer;
			}
			else
			{
				// set up for the next line: copy remainder in line buffer to start of line buffer
				bytes_not_displayed = (MEM_TEXT_VIEW_BYTES_PER_ROW - (line_buffer_remainder - line_buffer));
			}
			//DEBUG_OUT(("%s %d: not displayed=%u", __func__ , __LINE__, bytes_not_displayed));
			//DEBUG_OUT(("%s %d: buff remainder='%s'", __func__ , __LINE__, line_buffer_remainder));
			
			if (bytes_not_displayed > 0)
			{
				memcpy(line_buffer, line_buffer_remainder, bytes_not_displayed);
				line_buffer[bytes_not_displayed] = 0;
			}
			else
			{
				line_buffer[0] = 0;
				//memset(line_buffer, 0, STORAGE_STRING_BUFFER_1_LEN);
			}

			remain_len = General_Strnlen(line_buffer, STORAGE_STRING_BUFFER_1_LEN);
			
			//DEBUG_OUT(("%s %d: remain_len=%u", __func__ , __LINE__, remain_len));
			//DEBUG_OUT(("%s %d: buff='%s'", __func__ , __LINE__, line_buffer));

			//user_input = Keyboard_GetChar();
			
			// check if we need to ask user to go on to a new screen
			++y;
			
			if (y == MAX_TEXT_VIEW_ROWS_PER_PAGE)
			{
				user_input = Keyboard_GetChar();
				
				if (user_input == CH_ESC || user_input == 'q' || user_input == CH_RUNSTOP)
				{
					keep_going = false;
					user_exit = true;
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
				//DEBUG_OUT(("%s %d: unprocessed=%u, remain_len=%u", __func__ , __LINE__, unprocessed_bytes, remain_len));
				//DEBUG_OUT(("%s %d: buff='%s'", __func__ , __LINE__, line_buffer));
				
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

					//DEBUG_OUT(("%s %d: remain_len=%u", __func__ , __LINE__, remain_len));
					//DEBUG_OUT(("%s %d: buff='%s'", __func__ , __LINE__, line_buffer));
					
					memset(copy_buffer, 0, STORAGE_FILE_BUFFER_1_LEN);

					unprocessed_bytes = 0;					
				}
				
				if (i < num_pages)
				{
					copy_again = true;
				}
				else
				{
					// there isn't enough in copy buffer to fill a row, but also last chunk has already been read from EM
											
				}
			}
			
		} while (copy_again == false && keep_going == true);		
	}
	
	// if user hasn't already said they are done, give them a chance to look at the last displayed page
	if (user_exit != true)
	{
		Keyboard_GetChar();
	}
}


// displays the content found in EM as hex codes and text to right, similar to a ML monitor
// em_bank_num is used to derive the base EM address
// num_pages is the number of EM 256b pages that need displaying
// the_name is only used to provide feedback to the user about what they are viewing
void EM_DisplayAsHex(uint8_t em_bank_num, uint8_t num_pages, char* the_name)
{
	// LOGIC
	//   Data must have already been loaded into EM at the em_bank_num specified
	//   user can hit esc / runstop to stop at any time, or any other key to continue to the next screenful
	//   for the 'address', we start at 0 assuming this is a file, and we are counting from start of file
	//     but if the em_bank_num <> EM_STORAGE_START_PHYS_BANK_NUM, we're almost certainly viewing memory, in which case show calculated EM address
	
	// LOGIC
	//   we only have 80x60 to work with, and we need a row for "hit space for more, esc to stop"
	//     only 16 bytes of hex can be shown on one row of 80 chars (2 per byte + 1 space; plus 16 chars at right for view, plus addr at left)
	//     so 59 rows * 16 bytes = 944 max bytes can be shown
	//     we read 256 b chunks from EM, so we need about 3.7 chunks (59/16) per screenful.
	//   we need 1 buffer:
	//     1 to capture the 256b coming in from EM each EM read. we can peel off 16 byte slices of this for each row.

	uint8_t		user_input;
	uint8_t		rows_displayed_this_chunk;
	uint8_t		i = 0;
	uint8_t		y = 0;
	uint32_t	loc_in_file = 0x0000;	// will track the location within the file, so we can show to users on left side. 
	bool		keep_going = true;
	bool		user_exit = false;
	bool		copy_again;
	uint8_t*	copy_buffer;
	uint8_t*	buffer_curr_loc;
	
	// primary local buffer will use 384b dedicated storage in the EM overlay (only needs 256 technically, but this gives us some flex)
	copy_buffer = em_temp_buffer_384b;
	
	// are we showing a file on disk, or actually showing memory
	if (em_bank_num == EM_STORAGE_START_PHYS_BANK_NUM)
	{
		loc_in_file = 0x0000;
	}
	else
	{
		loc_in_file = (uint32_t)((uint32_t)em_bank_num * (uint32_t)8192);
	}
	
	// EM chunk read loop
	while (keep_going == true && i < num_pages)
	{
		App_EMDataCopy(copy_buffer, em_bank_num, i++, PARAM_COPY_FROM_EM);

		buffer_curr_loc = copy_buffer;
		copy_again = false;
		rows_displayed_this_chunk = 0;
		
		// process each chunk and any remainder from previous read cycle
		do
		{		
			if (y == 0)
			{
				Text_ClearScreen(FILE_CONTENTS_FOREGROUND_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);
				sprintf(global_string_buff1, General_GetString(ID_STR_MSG_HEX_VIEW_INSTRUCTIONS), the_name);
				Text_DrawStringAtXY(0, y++, global_string_buff1, FILE_CONTENTS_ACCENT_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);
				++y;
			}
			
			// address display at left
			sprintf(global_string_buff1, "%06lX: ", loc_in_file);
			Text_DrawStringAtXY(0, y, global_string_buff1, FILE_CONTENTS_ACCENT_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);
		
			// main hex display in middle
			sprintf(global_string_buff1, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x  ", 
				buffer_curr_loc[0], buffer_curr_loc[1], buffer_curr_loc[2], buffer_curr_loc[3], buffer_curr_loc[4], buffer_curr_loc[5], buffer_curr_loc[6], buffer_curr_loc[7], 
				buffer_curr_loc[8], buffer_curr_loc[9], buffer_curr_loc[10], buffer_curr_loc[11], buffer_curr_loc[12], buffer_curr_loc[13], buffer_curr_loc[14], buffer_curr_loc[15]);
			Text_DrawStringAtXY(MEM_DUMP_START_X_FOR_HEX, y, global_string_buff1, FILE_CONTENTS_FOREGROUND_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);

			// 'text' display at right
			// render chars with char draw function to avoid problem of 0s getting treated as nulls in sprintf
			Text_DrawCharsAtXY(MEM_DUMP_START_X_FOR_CHAR, y, (uint8_t*)buffer_curr_loc, MEM_DUMP_BYTES_PER_ROW);
		
			loc_in_file += MEM_DUMP_BYTES_PER_ROW;
			buffer_curr_loc += MEM_DUMP_BYTES_PER_ROW;
			++rows_displayed_this_chunk;
			
			// check if we need to ask user to go on to a new screen
			++y;
			
			if (y == MAX_TEXT_VIEW_ROWS_PER_PAGE)
			{
				user_input = Keyboard_GetChar();
				
				if (user_input == CH_ESC || user_input == 'q' || user_input == CH_RUNSTOP)
				{
					keep_going = false;
					user_exit = true;
				}
				else
				{
					y = 0;
				}
			}
			
			// check if we need to get more bytes from EM
			if (rows_displayed_this_chunk > 15)
			{
				// whole chunk has now been displayed = we need to get another chunk
				//sprintf(global_string_buff1, "need more data, i=%u", i);
				//Buffer_NewMessage(global_string_buff1);
				
				if (i < num_pages)
				{
					copy_again = true;
				}
				else
				{
					// there isn't enough in copy buffer to fill a row, but also last chunk has already been read from EM
					keep_going = false;					
				}
			}
			
		} while (copy_again == false && keep_going == true);		
	}
	
	// if user hasn't already said they are done, give them a chance to look at the last displayed page
	if (user_exit != true)
	{
		Keyboard_GetChar();
	}
}



// // scan through all EM, checking each 8k bank for a KUP program signature
// void EM_ScanForKUP(void)
// {
// 	uint8_t		i = 0;
// 	uint8_t*	copy_buffer;
// 	
// 	uint8_t		kup_version;
// 	uint8_t*	kup_name;
// 	uint8_t*	kup_args;	// we don't care, but need to get past them to get to description
// 	uint8_t*	kup_description;
// 	uint8_t		the_len;
// 	
// 	// primary local buffer will use 384b dedicated storage in the EM overlay (only needs 256 technically, but this gives us some flex)
// 	copy_buffer = em_temp_buffer_384b;
// 	
// 	// read the first 256 bytes of every bank in extended memory
// 	for (i = 8; i < 80; i++)
// 	{
// 		App_EMDataCopy(copy_buffer, i, 0, PARAM_COPY_FROM_EM);
// 		
// 		// check for KUP signature $F2$56
// 		if (copy_buffer[0] == 0xF2 && copy_buffer[1] == 0x56)
// 		{
// 			kup_version = copy_buffer[6];
// 			
// 			// get name: all versions of KUP supported the name
// 			kup_name = &copy_buffer[10];
// 			the_len = General_Strnlen((char*)kup_name, 128);
// 			
// 			if (kup_version > 0)
// 			{
// 				kup_args = kup_name + the_len + 1;
// 				the_len = General_Strnlen((char*)kup_args, 128);
// 				kup_description = kup_args + the_len + 1;
// 			}
// 			else
// 			{
// 				kup_description = (uint8_t*)"";
// 			}
// 			
// 			sprintf(global_string_buff1, "EM bank %02x: '%s': '%s'", i, kup_name, kup_description);
// 			Buffer_NewMessage(global_string_buff1);
// 		}
// 	}	
// }
