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



#endif /* OVERLAY_STARTUP_H_ */
