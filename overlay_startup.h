/*
 * overlay_startup.h
 *
 *  Created on: Mar 11, 2024
 *      Author: micahbly
 */

#ifndef OVERLAY_STARTUP_H_
#define OVERLAY_STARTUP_H_

/* about this class
 *
 *  Routines for starting up f/manager, including show splash screen(s)
 *    Some code here originated in sys.c and other places before being moved here
 *
 */

/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

#include "app.h"
#include "text.h"
#include <stdint.h>


/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/


/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/

/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/


// load strings into memory and set up string pointers
void Startup_LoadString(void);

// clear screen and show app (foenix) logo, and machine logo if running from flash
void Startup_ShowLogo(void);

// enable the random number generator, and seed it
void Startup_InitializeRandomNumGen(void);

//! Initialize the system (primary entry point for all system initialization activity)
//! Starts up the memory manager, creates the global system object, runs autoconfigure to check the system hardware, loads system and application fonts, allocates a bitmap for the screen.
bool Sys_InitSystem(void);

//! Find out what kind of machine the software is running on, and configure the passed screen accordingly
//! Configures screen settings, RAM addresses, etc. based on known info about machine types
//! Configures screen width, height, total text rows and cols, and visible text rows and cols by checking hardware
//! @param	the_system: valid pointer to system object
//! @return	Returns false if the machine is known to be incompatible with this software. 
bool Sys_AutoConfigure(void);

//! Find out what kind of machine the software is running on, and determine # of screens available
//! @param	the_system: valid pointer to system object
//! @return	Returns false if the machine is known to be incompatible with this software. 
bool Sys_AutoDetectMachine(void);

//! Detect the current screen mode/resolution, and set # of columns, rows, H pixels, V pixels, accordingly
//! @param	the_screen: valid pointer to the target screen to operate on
//! @return	returns false on any error/invalid input.
bool Sys_DetectScreenSize(void);

//! Set the left/right and top/bottom borders
//! This will reset the visible text columns as a side effect
//! @param	border_width: width in pixels of the border on left and right side of the screen. Total border used with be the double of this.
//! @param	border_height: height in pixels of the border on top and bottom of the screen. Total border used with be the double of this.
//! @return	returns false on any error/invalid input.
void Sys_SetBorderSize(uint8_t border_width, uint8_t border_height);



#endif /* OVERLAY_STARTUP_H_ */
