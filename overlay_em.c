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
#include "bank.h"
#include "comm_buffer.h"
#include "debug.h"
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

extern uint8_t				global_search_phrase_len;
extern char*				global_search_phrase;
extern char*				global_search_phrase_human_readable;
extern char*				global_string[NUM_STRINGS];
extern char*				global_string_buff1;
extern char*				global_string_buff2;

extern bool					global_find_next_enabled;

extern uint8_t				zp_bank_num;
extern uint8_t				io_bank_value_kernel;	// stores value for the physical bank pointing to C000-DFFF whenever we change it, so we can restore it.

extern uint8_t	zp_search_loc_byte;
extern uint8_t	zp_search_loc_page;
extern uint8_t	zp_search_loc_bank;

#pragma zpsym ("zp_search_loc_byte");
#pragma zpsym ("zp_search_loc_page");
#pragma zpsym ("zp_search_loc_bank");

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


// searches memory starting at the passed bank num, for the sequence of characters found in global_search_phrase/len
// global_search_phrase can be NULL terminated or not (null terminator will not be searched for; use byte string to find instead if important.
// if new_search is false, it will start at the previous find position + 1. 
// prep before calling:
//    make sure global_search_phrase and global_search_phrase_len have been set
//    set ZP_SEARCH_LOC_BANK to the bank to start searching in (e.g, 9, to start searching in the 10th bank at $12000)
//    set ZP_SEARCH_LOC_PAGE to the page within the bank to start search. (e.g., 1 to start searching at offset $100)
//    set ZP_SEARCH_LOC_BYTE to the byte offset within the page to start search. if continueing a search, this should be (at least) 1 more than the start of the last result hit.
// search will continue until last bank num in system is hit, or a find is made.
// if no hit, will return false.
// if a match is found:
//    will return true
//    will set ZP_SEARCH_LOC_BYTE to the byte offset within the current page being examined. (e.g, for hit at $A123: 35)
//    will set ZP_SEARCH_LOC_PAGE to the page the hit was found on (e.g, for hit at $A123: 1)
//    will set ZP_SEARCH_LOC_BANK to the bank the hit was found on (e.g, for hit at $A123: 5)
bool EM_SearchMemory(bool new_search)
{
	// LOGIC
	//       after detecting we have <(search-string-length) chars left, and more chunks to go, copy remainder to another 256b buffer. copy more in. process remainder buffer first.

	uint32_t	find_location;
	uint16_t	remain_len = 0;
	uint16_t	this_remain_len;
	uint8_t		i;
	uint8_t		starting_offset;
	uint8_t		this_temp_bank_num;
	uint8_t		this_temp_page_num;
	char*		this_phrase;
	uint8_t*	buffer_curr_loc;
	uint8_t*	copy_buffer = (uint8_t*)STORAGE_FILE_BUFFER_1;
	uint8_t*	this_memory_loc;

	DEBUG_OUT(("%s %d: search_len=%u, new_search=%u, first 6 of search phrase = %x%x%x%x%x%x", __func__ , __LINE__, global_search_phrase_len, new_search, global_search_phrase[0], global_search_phrase[1], global_search_phrase[2], global_search_phrase[3], global_search_phrase[4], global_search_phrase[5]));

	// want to have this global flag start at false every time as there are multiple failure routes
	global_find_next_enabled = false;

	// if this is a new search, leave zp1-3 as is. 
	// if this is a find next operation, start at position immediately following the last good hit
	if (new_search == false)
	{
		if (zp_search_loc_byte < 255)
		{
			++zp_search_loc_byte;
		}
		else
		{
			zp_search_loc_byte = 0;
			
			if (zp_search_loc_page < PAGES_PER_BANK)
			{
				++zp_search_loc_page;
			}
			else
			{
				zp_search_loc_page = 0;
			
				if (zp_search_loc_bank < NUM_MEMORY_BANKS)
				{
					++zp_search_loc_bank;
				}
				else
				{
					// apparently last search ended on the last byte of system memory!
					return false;
				}
			}
		}
	}
	
	starting_offset = zp_search_loc_byte;
	remain_len = 256 - starting_offset;

	// bank loop
	while (*(uint8_t*)ZP_SEARCH_LOC_BANK < NUM_MEMORY_BANKS) //NUM_MEMORY_BANKS
	{
		
		// page loop
		while (*(uint8_t*)ZP_SEARCH_LOC_PAGE < PAGES_PER_BANK)
		{
			App_EMDataCopy(copy_buffer, zp_search_loc_bank, zp_search_loc_page, PARAM_COPY_FROM_EM);
	
			buffer_curr_loc = copy_buffer + starting_offset;
			starting_offset = 0; // only need this on the first page we copy in

			//DEBUG_OUT(("%s %d: remain_len=%u, buffer_curr_loc=%p", __func__ , __LINE__, remain_len, buffer_curr_loc ));
			//DEBUG_OUT(("%s %d: ZP_SEARCH_LOC_BYTE=%x, ZP_SEARCH_LOC_PAGE=%x, ZP_SEARCH_LOC_BANK=%x", __func__ , __LINE__, zp_search_loc_byte, zp_search_loc_page, zp_search_loc_bank));
			
			// start compare, looking for first byte in memory that matches first byte of search
			
			for (; remain_len > 0; buffer_curr_loc++, --remain_len, ++zp_search_loc_byte)
			{
				if (*buffer_curr_loc != *global_search_phrase)
				{
					continue;
				}
				
				//DEBUG_OUT(("%s %d: match to first char %x found at curr_loc=%p (%06lx), remain_len=%i", __func__ , __LINE__, *global_search_phrase, buffer_curr_loc , (uint32_t)((uint32_t)zp_search_loc_bank * (uint32_t)8192) + (uint32_t)zp_search_loc_page * (uint32_t)256 + zp_search_loc_byte, remain_len));
				// found match to first char
				this_phrase = global_search_phrase;
				this_memory_loc = buffer_curr_loc;
				this_remain_len = remain_len;
				
				for (i = 1; i < global_search_phrase_len; ++i, --this_remain_len)
				{
					++this_phrase;
					++this_memory_loc;
					
					if (this_remain_len == 1)
					{
						//DEBUG_OUT(("%s %d: hit end of page while checking a potential match %c,%p,%i", __func__ , __LINE__, *this_phrase, this_memory_loc, i));
						
						if (i == global_search_phrase_len)
						{
							//DEBUG_OUT(("%s %d: ... but the last char of phrase was the last byte of page, so it's ok", __func__ , __LINE__));
						}
						else
						{
							//DEBUG_OUT(("%s %d: ... still have %u more chars to find, so need to pull in a new bank temporarily", __func__ , __LINE__, global_search_phrase_len - i));
							// need to peek into the next bank, without throwing off zp #s. for all we know, this search is still going to fail (eg, we found "auto", phrase is "automobile", and first part of next bank is "matic", not "mobile".)
							// can't destroy the current buffer for this reason, so we'll use a string buffer as the temp one.
							
							// but can't search past end of memory, so check that first
							if (*(uint8_t*)ZP_SEARCH_LOC_BANK < NUM_MEMORY_BANKS || (*(uint8_t*)ZP_SEARCH_LOC_PAGE < PAGES_PER_BANK))
							{
								this_temp_bank_num = zp_search_loc_bank;
								
								// are we just going to the next page, or also to the next bank?
								if (zp_search_loc_page < PAGES_PER_BANK)
								{
									this_temp_page_num = zp_search_loc_page + 1;
								}
								else
								{
									this_temp_page_num = 0;
									++this_temp_bank_num;
								}
								App_EMDataCopy((uint8_t*)global_string_buff2, this_temp_bank_num, this_temp_page_num, PARAM_COPY_FROM_EM);
								// now we can continue matching check
								this_remain_len = 256;
								this_memory_loc = (uint8_t*)global_string_buff2;
							}
							else
							{
								DEBUG_OUT(("%s %d: can't do a temp bank read, because we were on the last page of the bank of memory! zp3=%u, zp2=%u", __func__ , __LINE__, zp_search_loc_bank, zp_search_loc_page));
								
								goto no_match;
							}
						}
					}
					
					if (*this_phrase != *this_memory_loc)
					{
						break;
					}
					//DEBUG_OUT(("%s %d: char match='%c' %x, i=%u", __func__ , __LINE__, *this_phrase, *this_phrase, i));
				}
				
				if (i == global_search_phrase_len)
				{
					// all the chars must have matched
					find_location = (uint32_t)((uint32_t)zp_search_loc_bank * (uint32_t)8192) + (uint32_t)zp_search_loc_page * 256 + zp_search_loc_byte;
					sprintf(global_string_buff1, General_GetString(ID_STR_MSG_SEARCH_BANK_SUCCESS), global_search_phrase_human_readable, find_location, *(uint8_t*)ZP_SEARCH_LOC_BANK);
					Buffer_NewMessage(global_string_buff1);
					
					return true;
				}					
			}
			
			// need to get another page of memory
			remain_len = 256;

			// give user a chance to stop search
			if (Keyboard_GetKeyIfPressed() == CH_RUNSTOP)
			{
				goto no_match;
			}

			++zp_search_loc_page;	// == page counter
			*(uint8_t*)ZP_SEARCH_LOC_BYTE = 0;	// start the byte-in-page counter over again
		}
		
		*(uint8_t*)ZP_SEARCH_LOC_PAGE = 0;	// start the page counter over again.
		
		++zp_search_loc_bank;	// == bank number
	}
	
no_match:
	zp_search_loc_byte = 255;	// to make it doubly clear we didn't find anything
	sprintf(global_string_buff1, General_GetString(ID_STR_MSG_SEARCH_BANK_FAILURE), global_search_phrase_human_readable);
	Buffer_NewMessage(global_string_buff1);
	return false;
}


