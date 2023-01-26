/*
 * comm_buffer.h
 *
 *  Created on: Oct 21, 2022
 *      Author: micahbly
 */

// originally for Lich King, adapted for FileManager on Jan 18, 2023

#ifndef COMM_BUFFER_H_
#define COMM_BUFFER_H_



/* about this class: Buffer
 *
 * This incorporates the overall game world
 *
 *** things this class needs to be able to do
 *
 * manage a N row comms buffer
 * clear buffer
 * refresh buffer (after full screen was used, etc.)
 *
 *
 *** things objects of this class have
 *
 * char buffers
 *
 */


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "app.h"

// C includes
#include <stdbool.h>

// cc65 includes



/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/

#define COMM_BUFFER_MAX_STRING_LEN	192	// we only allow for one line at a time, but keep a 192b buffer available for overruns?

#define COMM_BUFFER_NUM_COLS	(80 - 2)	// account for box draw chars
#define COMM_BUFFER_NUM_ROWS	7
#define COMM_BUFFER_FIRST_COL	1	// allow for 1 col of box draw chars
#define COMM_BUFFER_FIRST_ROW	52
#define COMM_BUFFER_LAST_COL	(COMM_BUFFER_NUM_COLS - 0)
#define COMM_BUFFER_LAST_ROW	58
#define COMM_BUFF_SIZE			(COMM_BUFFER_NUM_ROWS * (COMM_BUFFER_NUM_COLS + 1))
// comms buffer box
#define COMM_BUFFER_BOX_NUM_COLS		(COMM_BUFFER_NUM_COLS + 2)	// with box draw chars
#define COMM_BUFFER_BOX_NUM_ROWS		(COMM_BUFFER_NUM_ROWS + 2)	// with box draw chars
#define COMM_BUFFER_BOX_FIRST_COL		(COMM_BUFFER_FIRST_COL - 1)
#define COMM_BUFFER_BOX_LAST_COL		(COMM_BUFFER_LAST_COL + 1)
#define COMM_BUFFER_BOX_FIRST_ROW		(COMM_BUFFER_FIRST_ROW - 1)
#define COMM_BUFFER_BOX_LAST_ROW		(COMM_BUFFER_LAST_ROW + 1)


/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/


/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/

// Draw the status and message area framework
void Buffer_DrawCommunicationArea(void);

// fake allocs the buffer memory area. does not change screen display
void Buffer_Initialize(void);

// resets the buffer memory area to spaces, and pushes that change to the screen display
void Buffer_Clear(void);

// transfers all buffer lines to the screen display
void Buffer_RefreshDisplay(void);

// accepts a message as the bottom most row and displays it, scrolling other lines up
void Buffer_NewMessage(char* the_message);



#endif /* COMM_BUFFER_H_ */
