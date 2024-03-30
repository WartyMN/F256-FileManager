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

#define NUM_MEMORY_BANKS		0x80
#define MAX_SEARCH_PHRASE_LEN	32	// arbitrary. 

#define PARAM_START_FROM_THIS_BANK		true	// parameter for EM_SearchMemory
#define PARAM_START_AFTER_LAST_HIT		false	// parameter for EM_SearchMemory


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

// searches memory starting at the passed bank num, for the sequence of characters found in global_search_phrase/len
// global_search_phrase can be NULL terminated or not (null terminator will not be searched for; use byte string to find instead if important.
// if new_search is false, it will start at the previous find position + 1. 
// prep before calling:
//    make sure global_search_phrase and global_search_phrase_len have been set
//    set ZP_TEMP_3 to the bank to start searching in (e.g, 9, to start searching in the 10th bank at $12000)
//    set ZP_TEMP_2 to the page within the bank to start search. (e.g., 1 to start searching at offset $100)
//    set ZP_TEMP_1 to the byte offset within the page to start search. if continueing a search, this should be (at least) 1 more than the start of the last result hit.
// search will continue until last bank num in system is hit, or a find is made.
// if no hit, will return false.
// if a match is found:
//    will return true
//    will set ZP_TEMP_1 to the byte offset within the current page being examined. (e.g, for hit at $A123: 35)
//    will set ZP_TEMP_2 to the page the hit was found on (e.g, for hit at $A123: 1)
//    will set ZP_TEMP_3 to the bank the hit was found on (e.g, for hit at $A123: 5)
bool EM_SearchMemory(bool new_search);

#endif /* OVERLAY_EM_H_ */
