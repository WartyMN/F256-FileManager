//! @file keyboard.h

/*
 * keyboard.h
 *
 *  Created on: Aug 23, 2022
 *      Author: micahbly
 */

// adapted for (Lich King) Foenix F256 Jr starting November 30, 2022
// adapted for f/manager Foenix F256 starting March 10, 2024


#ifndef KEYBOARD_H_
#define KEYBOARD_H_


/* about this class
 *
 *
 *
 *** things this class needs to be able to do
 * get a char code back from the keyboard when requested
 * interpret MCP scan codes, use modifiers to map scan codes to "char codes" defined here
 *
 *** things objects of this class have
 *
 *
 */


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/


// C includes
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/



/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/




/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/

typedef struct KeyRepeater
{
	uint8_t		key;		// Key-code to repeat
	uint8_t		cookie;
} KeyRepeater;

	
/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/

// **** USER INPUT UTILITIES *****

// Check to see if keystroke events pending - does not wait for a key
uint8_t Keyboard_GetKeyIfPressed(void);

// Wait for one character from the keyboard and return it
char Keyboard_GetChar(void);

// main event processor
void Keyboard_ProcessEvents(void);

// initiate the minute hand timer
void Keyboard_InitiateMinuteHand(void);


#endif /* KEYBOARD_H_ */
