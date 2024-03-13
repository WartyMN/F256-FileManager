/*
 * comm_buffer.c
 *
 *  Created on: Oct 21, 2022
 *      Author: micahbly
 */

// split off from code in game.c and screen.c, to have a dedicated comms buffer file


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "comm_buffer.h"
#include "app.h"
#include "general.h"
#include "keyboard.h"
#include "strings.h"
#include "text.h"

// C includes
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// F256 includes
#include "f256.h"




/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/

#define ROW_HIT_SPACE		COMM_BUFFER_LAST_ROW
#define COL_HIT_SPACE		(COMM_BUFFER_LAST_COL - 11) // long enough for "hit any key"

/*****************************************************************************/
/*                           File-scoped Variables                           */
/*****************************************************************************/

static uint8_t		comm_curr_buff_row = 0;
static char 		comm_buff[COMM_BUFF_SIZE];
static char			comm_temp_line_buff[COMM_BUFFER_MAX_STRING_LEN];	// temp storage for strings to be displayed in buffer; allows wrapping chars to be imebedded without modifying the source string
static char*		comm_row[COMM_BUFFER_NUM_ROWS];
static char**		comm_row_ptr[COMM_BUFFER_NUM_ROWS];	// ptp for the comms buffer rows, to allow scrolling


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/





/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

// pushes lines "up" by 1, so that line 3 becomes line 2, line 1 becomes line 3
void Buffer_ScrollUp(void);



/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/




// // shows a blinking thing at end of buffer and waits for user to hit the spacebar
// // use case: call every time 3 lines are displayed between user action inputs, to give user a chance to read everything
// void Buffer_GetUserToHitKey(void)
// {
// 	uint8_t	the_char;
// 
// 	// draw a 'hit space' message just above the buffer. this line is guaranteed not to be used by dungeon
// 	Text_DrawStringAtXY(COL_HIT_SPACE, ROW_HIT_SPACE, General_GetString(ID_STR_MSG_HIT_ANY_KEY), COLOR_BRIGHT_GREEN, COLOR_BLACK);	
// 	
// 	do
// 	{
// 		the_char = Keyboard_GetChar();
// 	}
// 	while (the_char != 32);
// 
// 	// undraw it
// 	Text_DrawStringAtXY(COL_HIT_SPACE, ROW_HIT_SPACE, General_GetString(ID_STR_MSG_HIT_ANY_KEY), COLOR_BLACK, COLOR_BLACK);	
// }


// pushes lines "up" by 1, so that line 3 becomes line 2, line 1 becomes line 3
void Buffer_ScrollUp(void)
{
	char*		temp;
	uint8_t		i;
	int8_t		last;
	int8_t		next_to_last;

	last = COMM_BUFFER_NUM_ROWS - 1;
	next_to_last = last - 1;
	temp = *comm_row_ptr[last];
	*comm_row_ptr[last] = *comm_row_ptr[0];
//printf("last=%i, next_to_last=%i \n", last, next_to_last);	
	for (i = 0; i < next_to_last; i++)
	{
		*comm_row_ptr[i] = *comm_row_ptr[i + 1];
	}
	
	*comm_row_ptr[next_to_last] = temp;

	Buffer_RefreshDisplay();
}

/*****************************************************************************/
/*                       Public Function Definitions                         */
/*****************************************************************************/




// Draw the status and message area framework
void Buffer_DrawCommunicationArea(void)
{
	// draw boxes around message area
	Text_DrawBoxCoordsFancy(
		COMM_BUFFER_BOX_FIRST_COL, COMM_BUFFER_BOX_FIRST_ROW,
		COMM_BUFFER_BOX_LAST_COL, COMM_BUFFER_BOX_LAST_ROW,
		BUFFER_ACCENT_COLOR, 
		BUFFER_BACKGROUND_COLOR
	);
}


// initialize the comms buffer and msg/status area (without drawing anything)
void Buffer_Initialize(void)
{
	uint8_t		i;
	
	// set up the comm buffer ptps
	for (i = 0; i < COMM_BUFFER_NUM_ROWS; i++)
	{
		comm_row[i] = &comm_buff[i * (COMM_BUFFER_NUM_COLS + 1)];
		comm_row_ptr[i] = &comm_row[i];
	}
	
	comm_curr_buff_row = 0;
}


// resets the buffer memory area to spaces, and pushes that change to the screen display
void Buffer_Clear(void)
{
	uint8_t i;

	for (i = 0; i < COMM_BUFFER_NUM_ROWS; i++)
	{
		*(comm_row[i]) = 0;
	}

	Text_FillBox(		
		COMM_BUFFER_FIRST_COL, COMM_BUFFER_FIRST_ROW, 
		COMM_BUFFER_LAST_COL, COMM_BUFFER_LAST_ROW, 
		CH_SPACE, 
		BUFFER_FOREGROUND_COLOR, 
		BUFFER_BACKGROUND_COLOR
	);
	
	// reset the buffer lines-since-you-did-something-yourself count.
	comm_curr_buff_row = 0;
}


// transfers all buffer lines to the screen display
void Buffer_RefreshDisplay(void)
{
	uint8_t		i;

	// first clear all lines. 
	// LOGIC: the inventory screen draws over player screen. 
	//   if each row of the comms buffer is not full of text/spaces, bits of the inventory screen will be left in place
	Text_FillBox(		
		COMM_BUFFER_FIRST_COL, COMM_BUFFER_FIRST_ROW, 
		COMM_BUFFER_LAST_COL, COMM_BUFFER_LAST_ROW, 
		CH_SPACE,
		BUFFER_FOREGROUND_COLOR, 
		BUFFER_BACKGROUND_COLOR
	);
	
	for (i = 0; i < COMM_BUFFER_NUM_ROWS; i++)
	{
		int8_t		this_len = strlen(*comm_row_ptr[i]);
//printf("'%s' \n", *comm_row_ptr[i]);	
		
		if ( this_len > COMM_BUFFER_NUM_COLS)
		{
			DEBUG_OUT(("%s %d: string was too long (%i vs %i): '%s'", __func__, __LINE__, COMM_BUFFER_NUM_COLS, this_len, *comm_row_ptr[i]));
			*(comm_row_ptr[i][COMM_BUFFER_NUM_COLS]) = 0;
		}
		Text_DrawStringAtXY(
			COMM_BUFFER_FIRST_COL, COMM_BUFFER_FIRST_ROW + i, 
			*comm_row_ptr[i],
			BUFFER_FOREGROUND_COLOR, 
			BUFFER_BACKGROUND_COLOR
		);
	}
}


// accepts a message as the bottom most row and displays it, scrolling other lines up
void Buffer_NewMessage(char* the_message)
{
	uint8_t		the_len;
	char*		right_margin;
	char*		start_of_string = comm_temp_line_buff;

	//DEBUG_OUT(("%s %d: msg='%s'", __func__, __LINE__, the_message));
	
	// check that we haven't already displayed 3 lines worth of buffer since user last hit a key
	// if we have, give user the blinky and wait for them to hit a key before displaying any more rows
	// skip this though if we are in stealth mode
	if (comm_curr_buff_row >= COMM_BUFFER_NUM_ROWS)
	{
		//Buffer_GetUserToHitKey();
		comm_curr_buff_row = 0;	// jan 18: without the "hit space to continue" thing, this whole thing doesn't make sense. Remove after testing.
	}

	// check if this is longer than we can display on one line
	the_len = strlen(the_message);

	// copy the string into the temp buffer so we can add line terminators without modifying source string
	strcpy(start_of_string, the_message);

	while(*start_of_string)
	{
		++comm_curr_buff_row;

		if (the_len < COMM_BUFFER_NUM_COLS)
		{
			sprintf(*comm_row_ptr[0], "%-*s", COMM_BUFFER_NUM_COLS, start_of_string);
			DEBUG_OUT(("%s %d: '%s'", __func__, __LINE__, *comm_row_ptr[0]));
			//printf("'%s'", *comm_row_ptr[0]);
			Buffer_ScrollUp();
			return;
		}

		right_margin = start_of_string + COMM_BUFFER_NUM_COLS;

		while (*right_margin != ' ')
		{
			--right_margin;
		}

		*right_margin = '\0';
		sprintf(*comm_row_ptr[0], "%-*s", COMM_BUFFER_NUM_COLS, start_of_string);
		Buffer_ScrollUp();

		// are we ending up with more lines than can be shown in buffer at once?
		// skip this if we are in stealth mode
		if (comm_curr_buff_row >= COMM_BUFFER_NUM_ROWS)
		{
			comm_curr_buff_row = 0;	// jan 18: without the "hit space to continue" thing, this whole thing doesn't make sense. Remove after testing.
		}

		the_len -= right_margin - start_of_string + 1; // +1 is for the space
		start_of_string = right_margin + 1;
	}
}


