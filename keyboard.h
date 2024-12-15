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

// ASCII code points for certain control characters
#define CH_LF			0x0a
#define CH_FF			0x0c
#define CH_CR			0x0d

// response returned by keyboard input (not necessarily the same as the character that should be displayed if key is processed)

#define CH_ALT_OFFSET		128	// this is amount that will be added to key press if ALT is held down.

#define CH_ENTER        13
#define CH_ESC          27
#define CH_TAB          9
#define CH_BKSP   		0x08	
#define CH_DEL   		0x7F	// 127	
// #define CH_DEL          0x08
#define CH_RUNSTOP		3

#define CH_F1      		0x81	
#define CH_F2      		0x82	
#define CH_F3      		0x83	
#define CH_F4      		0x84	
#define CH_F5      		0x85	
#define CH_F6      		0x86	
#define CH_F7      		0x87	
#define CH_F8     		0x88	
#define CH_F9      		0x89	
#define CH_F10     		0x8a	
#define CH_F11     		0x8b	
#define CH_F12    		0x8c	
#define CH_CURS_UP      0x10
#define CH_CURS_DOWN    0x0e
#define CH_CURS_LEFT    0x02
#define CH_CURS_RIGHT   0x06
#define CH_ZERO			48		// this is just so we can do match to convert a char '5' into an integer 5, etc.
#define CH_NINE			57		// for number ranges when converting user input to numbers

// keypad
#define CH_K0      		'0'	
#define CH_K1      		'1'	
#define CH_K2      		'2'		
#define CH_K3      		'3'		
#define CH_K4      		'4'		
#define CH_K5      		'5'		
#define CH_K6     		'6'		
#define CH_K7  			'7'		
#define CH_K8  			'8'		
#define CH_K9    		'9'		
#define CH_KENTER   	(CH_ENTER)	
#define CH_KPLUS    	'+'	
#define CH_KMINUS    	'-'	
#define CH_KTIMES   	'*'	
#define CH_KDIV   		'/'	
#define CH_KPOINT  		'.'	


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
