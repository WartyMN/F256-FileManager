/*
 * memsys.h
 *
 *  Created on: Mar 18, 2024
 *      Author: micahbly
 */

#ifndef MEMORY_SYSTEM_H_
#define MEMORY_SYSTEM_H_


/* about this class: FMMemorySystem
 *
 *  This class holds functions and properties related to viewing and manipulating system memory, including extended memory, on the F256
 *
 *** things this class needs to be able to do
 *
 * represent either RAM or Flash memory (read-only in that event)
 * scan all memory banks and populate a list of banks
 * scan all memory banks and identify KUP programs and mark those banks as executable
 * search all memory banks for a given phrase
 * perform actions on the selected bank (via child FMBankObjects)
 * - clear
 * - fill
 * - search
 * - save to disk
 * - populate from disk
 * - execute KUP
 *
 *** things objects of this class have
 *
 * An array of FMBankObjects (potentially empty)
 * flag to indicate RAM vs Flash, R/W vs R
 * 
 *
 */


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

#include "app.h"
#include "bank.h"

/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/

#define MEMORY_BANK_COUNT		64	// 64 banks each for RAM and Flash in an F256


/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/



/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/

typedef struct FMMemorySystem
{
	bool				is_flash_;							// set to false when representing RAM, not flash.
	FMBankObject		bank_[MEMORY_BANK_COUNT];
	int16_t				cur_row_;							// 0-n: selected bank num. 0=first bank. -1 if no bank selected. 
} FMMemorySystem;


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/



// **** CONSTRUCTOR AND DESTRUCTOR *****

// constructor
// allocates space for the object and any string or other properties that need allocating
// if the passed memsys point is not NULL, it will not pass it back without allocating a new one.
FMMemorySystem* MemSys_New(FMMemorySystem* existing_memsys, bool is_flash);

// // reset the memory system, without destroying it, to a condition where it can be completely repopulated
// // zero out all child banks
// void MemSys_Reset(FMMemorySystem* the_memsys, bool is_flash);

// destructor
// frees all allocated memory associated with the passed object, and the object itself
void MemSys_Destroy(FMMemorySystem** the_memsys);


// **** SETTERS *****

// // sets the row num (-1, or 0-n) of the currently selected bank
// void MemSys_SetCurrentRow(FMMemorySystem* the_memsys, int16_t the_row_number);


// **** GETTERS *****

// returns the row num (-1, or 0-n) of the currently selected bank
int16_t MemSys_GetCurrentRow(FMMemorySystem* the_memsys);

// Returns NULL if nothing matches, or returns pointer to first BankObject with a KUP name that matches exactly
FMBankObject* MemSys_FindBankByKUPName(FMMemorySystem* the_memsys, char* search_phrase, int compare_len);

// Returns NULL if nothing matches, or returns pointer to first BankObject with a KUP description that contains the search phrase
// DOES NOT REQUIRE a match to the full KUP description
FMBankObject* MemSys_FindBankByKUPDescriptionContains(FMMemorySystem* the_memsys, char* search_phrase, int compare_len);

// Returns NULL if nothing matches, or returns pointer to first BankObject that contains the search phrase
// starting_bank_num is the first bank num to start searching in
// NOTE: does NOT wrap around to bank 0 after hitting bank 63
FMBankObject* MemSys_FindBankContainingPhrase(FMMemorySystem* the_memsys, char* search_phrase, int compare_len, uint8_t starting_bank_num);

// looks through all banks in the bank list, comparing the passed row to that of each bank.
// Returns NULL if nothing matches, or returns pointer to first matching bank object
FMBankObject* MemSys_FindBankByRow(FMMemorySystem* the_memsys, uint8_t the_row);

// **** OTHER FUNCTIONS *****

// copies the passed bank to the same bank in the target memory system
bool MemSys_CopyBank(FMMemorySystem* the_memsys, FMBankObject* the_bank, FMMemorySystem* the_target_memsys);

// populate the banks in a memory system by scanning EM
void MemSys_PopulateBanks(FMMemorySystem* the_memsys);

// select or unselect 1 bank by row id, and change cur_row_ accordingly
bool MemSys_SetBankSelectionByRow(FMMemorySystem* the_memsys, uint16_t the_row, bool do_selection, uint8_t y_offset);

// **** FILL AND CLEAR FUNCTIONS *****

// ask the user what to fill the current bank with, and fill it with that value
// returns false on any error, or if user escapes out of dialog box without entering a number
bool MemSys_FillCurrentBank(FMMemorySystem* the_memsys);

// fills the current bank with 0s
// returns false on any error
bool MemSys_ClearCurrentBank(FMMemorySystem* the_memsys);



// TEMPORARY DEBUG FUNCTIONS

// // helper function called by List class's print function: prints folder total bytes, and calls print on each file
// void MemSys_Print(void* the_payload);


#endif /* MEMORY_SYSTEM_H_ */
