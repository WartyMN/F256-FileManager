/*
 * bank.h
 *
 *  Created on: Mar 18, 2024
 *      Author: micahbly
 */

#ifndef BANK_H_
#define BANK_H_



/* about this class: FMBankObject
 *
 *  This class holds functions and properties related to viewing and manipulating individual banks of system memory, including extended memory, on the F256
 *
 *** things this class needs to be able to do
 *
 * represent either RAM or Flash memory (read-only in that event)
 * scan the memory it represents and identify if it contains a KUP program and extract info if it does
 * search the memory it represents for a given phrase
 * perform actions on the the memory it represents:
 * - clear
 * - fill
 * - search
 *
 *** things objects of this class have
 *
 * Name (system-generated if not a KUP bank)
 * Description (system-generated if not a KUP bank)
 * Address (20-bit)
 * BankNum (0-127)
 * flag to indicate whether or not it can be executed
 * 
 *
 */


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

#include "app.h"

#include "f256.h"

/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/


/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/

typedef enum bank_type
{
	BANK_KUP_PRIMARY			= 0,	// this is the signature bank (first bank) of a KUP program
	BANK_KUP_SECONDARY 			= 1,	// this is the 2nd+ bank of a KUP program
	BANK_NON_KUP				= 2,	// this is any memory bank with no association with a KUP
	BANK_MAX_TYPE				= 3,
} bank_type;


/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/

typedef struct FMBankObject
{
	bool				is_kup_;
	uint8_t				bank_num_;			// 0-127, it's location within F256 MMU. 64+ are flash, 0-63 are RAM
	bool				selected_;
	uint8_t				x_;
	int8_t				display_row_;		// offset from the first displayed row of parent panel. -1 if not to be visible.
	uint8_t				row_;				// row_ is relative to the first bank in the memory system. 
	uint32_t			addr_;				// 20-bit address in physical system memory 00 0000 - FF FFFF
	char*				name_;				// system-generated if not a KUP bank
	char*				description_;		// system-generated if not a KUP bank
} FMBankObject;


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/



// **** CONSTRUCTOR AND DESTRUCTOR *****

// constructor
// Banks are not allocated dynamically, they are fixed array elements of the parent memory system object
// the name and description are, however, allocated so that they can be used even when bank itself is not mapped into CPU space
void Bank_Init(FMBankObject* the_bank, const char* the_name, const char* the_description, bank_type the_bank_type, uint8_t the_bank_num, uint8_t the_row);

// destructor
// frees all allocated memory associated with the passed object but does not free the bank (no allocation for banks)
void Bank_Reset(FMBankObject* the_bank);


// **** SETTERS *****

// // set files selected/unselected status (no visual change)
// void Bank_SetSelected(FMBankObject* the_bank, bool selected);

// updates the icon's size/position information
void Bank_UpdatePos(FMBankObject* the_bank, uint8_t x, int8_t display_row, uint16_t row);



// **** GETTERS *****

// get the selected/not selected state of the bank
bool Bank_IsSelected(FMBankObject* the_bank);



// **** FILL AND CLEAR FUNCTIONS *****

// fills the bank with 0s
void Bank_Clear(FMBankObject* the_bank);

// fills the bank with the passed value
void Bank_Fill(FMBankObject* the_bank, uint8_t the_fill_value);

// asks the user to enter a fill value and returns it
// returns a negative number if user aborts, or a 0-255 number.
int16_t Bank_AskForFillValue(void);



// **** OTHER FUNCTIONS *****

// mark file as selected, and refresh display accordingly
bool Bank_MarkSelected(FMBankObject* the_bank, int8_t y_offset);

// mark file as un-selected, and refresh display accordingly
bool Bank_MarkUnSelected(FMBankObject* the_bank, int8_t y_offset);

// render name, description, address and any other relevant labels at the previously established coordinates
// if as_selected is true, will render with inversed text. Otherwise, will render normally.
// if as_active is true, will render in LIST_ACTIVE_COLOR, otherwise in LIST_INACTIVE_COLOR
void Bank_Render(FMBankObject* the_bank, bool as_selected, int8_t y_offset, bool as_active);

// helper function called by List class's print function: prints one bank entry
void Bank_Print(void* the_payload);

#endif /* BANK_H_ */
