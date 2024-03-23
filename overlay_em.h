/*
 * overlay_em.h
 *
 *  Created on: Mar 7, 2024
 *      Author: micahbly
 */

#ifndef OVERLAY_EM_H_
#define OVERLAY_EM_H_

/* about this class
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

// also causing line breaking, by calling it's partner function as it finds the end of string/para. <- character means line break.
// returns NULL if entire string was displayed, or returns pointer to next char needing display if available space was all used
char* EM_DisplayStringWithLineBreaks(char* the_message, uint8_t x, uint8_t y, uint8_t col_width, uint8_t max_allowed_rows);

// displays the content found in EM as text, with wrapping, etc.
// em_bank_num is used to derive the base EM address
// num_pages is the number of EM 256b chunks that need displaying
// the_name is only used to provide feedback to the user about what they are viewing
void EM_DisplayAsText(uint8_t em_bank_num, uint8_t num_pages, char* the_name);

// displays the content found in EM as hex codes and text to right, similar to a ML monitor
// em_bank_num is used to derive the base EM address
// num_pages is the number of EM 256b chunks that need displaying
// the_name is only used to provide feedback to the user about what they are viewing
void EM_DisplayAsHex(uint8_t em_bank_num, uint8_t num_pages, char* the_name);

#endif /* OVERLAY_EM_H_ */
