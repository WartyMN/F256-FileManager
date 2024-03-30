/*
 * memsys.c
 *
 *  Created on: Mar 18, 2024
 *      Author: micahbly
 */


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "api.h"
#include "app.h"
#include "bank.h"
#include "comm_buffer.h"
#include "file.h"
#include "folder.h"
#include "general.h"
#include "kernel.h"
#include "list.h"
#include "list_panel.h"
#include "memsys.h"
#include "strings.h"
#include "text.h"

// C includes
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// F256 includes
#include "f256.h"



/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/

#define MEMSYS_KUPNAME_TEMP_BUFFER_LEN		17	// enough for 16-char name + terminator

/*****************************************************************************/
/*                          File-scoped Variables                            */
/*****************************************************************************/

#pragma data-name ("OVERLAY_MEMSYS")

static uint8_t				memsys_temp_kupname_buffer_storage[MEMSYS_KUPNAME_TEMP_BUFFER_LEN];
static uint8_t*				memsys_temp_kupname_buffer = memsys_temp_kupname_buffer_storage;


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

extern char*		global_temp_path_1;
extern char*		global_temp_path_2;

extern char*		global_string_buff1;
extern char*		global_string_buff2;


/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

// looks through all files in the file list, comparing the passed string to the filename of each file.
// Returns NULL if nothing matches, or returns pointer to first matching list item
WB2KList* MemSys_FindListItemByBankName(FMMemorySystem* the_memsys, char* the_bank_name);

// looks through all files in the file list, comparing the passed string to the filepath of each file.
// Returns NULL if nothing matches, or returns pointer to first matching list item
WB2KList* MemSys_FindListItemByBankPath(FMMemorySystem* the_memsys, char* the_bank_path, short the_compare_len);

// looks through all files in the file list, comparing the passed string to the filepath of each file.
// Returns NULL if nothing matches, or returns pointer to first matching BankObject
FMBankObject* MemSys_FindBankByBankPath(FMMemorySystem* the_memsys, char* the_bank_path, short the_compare_len);


/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/








/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/



// **** CONSTRUCTOR AND DESTRUCTOR *****

	
// constructor
// allocates space for the object and any string or other properties that need allocating
// if the passed memsys pointer is not NULL, it will pass it back without allocating a new one.
FMMemorySystem* MemSys_NewOrReset(FMMemorySystem* existing_memsys, bool is_flash)
{
	FMMemorySystem*		the_memsys;

	if (existing_memsys == NULL)
	{
		if ( (the_memsys = (FMMemorySystem*)calloc(1, sizeof(FMMemorySystem)) ) == NULL)
		{
			LOG_ERR(("%s %d: could not allocate memory to create new memory system object", __func__ , __LINE__));
			goto error;
		}
		LOG_ALLOC(("%s %d:	__ALLOC__	the_memsys	%p	size	%i", __func__ , __LINE__, the_memsys, sizeof(FMMemorySystem)));
	}
	else
	{
		the_memsys = existing_memsys;

		// zero out all child memory bank objects
		MemSys_ResetAllBanks(the_memsys);
	}

	// set some other props
	the_memsys->is_flash_ = is_flash;
	the_memsys->cur_row_ = 0; // leave at -1 until MemSys_SetBankSelectionByRow() or it won't detect a change

	return the_memsys;

error:
	if (the_memsys)		free(the_memsys);
	return NULL;
}


// destructor
// frees all allocated memory associated with the passed object, and the object itself
void MemSys_Destroy(FMMemorySystem** the_memsys)
{
	if (*the_memsys == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_FOLDER_TO_DESTROY_WAS_NULL);	// crash early, crash often
	}

	MemSys_ResetAllBanks(*the_memsys);
	
	// free the folder object itself
	LOG_ALLOC(("%s %d:	__FREE__	*the_memsys	%p	size	%i", __func__ , __LINE__, *the_memsys, sizeof(FMMemorySystem)));
	free(*the_memsys);
	*the_memsys = NULL;
}


// zero every child bank object and have it free any memory associated with it (name, description)
void MemSys_ResetAllBanks(FMMemorySystem* the_memsys)
{
	uint8_t		i;
	
	if (the_memsys == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_DESTROY_ALL_MEMSYS_WAS_NULL);	// crash early, crash often
	}
	
	for (i = 0; i < MEMORY_BANK_COUNT; i++)
	{
		FMBankObject*		this_bank = &the_memsys->bank_[i];
		
		Bank_Reset(this_bank);
	}

	return;
}


// **** SETTERS *****




// sets the row num (-1, or 0-n) of the currently selected bank
void MemSys_SetCurrentRow(FMMemorySystem* the_memsys, int16_t the_row_number)
{
	if (the_memsys == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_SET_CURR_ROW_FOLDER_WAS_NULL);	// crash early, crash often
	}
	
	the_memsys->cur_row_ = the_row_number;
}



// **** GETTERS *****


// returns the row num (-1, or 0-n) of the currently selected bank
int16_t MemSys_GetCurrentRow(FMMemorySystem* the_memsys)
{
	if (the_memsys == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_MEMSYS_GET_CURR_ROW_FOLDER_WAS_NULL);	// crash early, crash often
	}
	
	return the_memsys->cur_row_;
}


// returns the currently selected bank or NULL if no bank selected
FMBankObject* MemSys_GetCurrentBank(FMMemorySystem* the_memsys)
{
	if (the_memsys == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		//App_Exit(ERROR_MEMSYS_GET_CURR_ROW_FOLDER_WAS_NULL);	// crash early, crash often
		return NULL;
	}
	
	if (the_memsys->cur_row_ < 0)
	{
		return NULL;
	}
	
	return &the_memsys->bank_[the_memsys->cur_row_];
}


// returns the currently selected bank's bank_num or 255 if no bank selected
uint8_t MemSys_GetCurrentBankNum(FMMemorySystem* the_memsys)
{
	if (the_memsys == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		//App_Exit(ERROR_MEMSYS_GET_CURR_ROW_FOLDER_WAS_NULL);	// crash early, crash often
		return NULL;
	}
	
	if (the_memsys->cur_row_ < 0)
	{
		return NULL;
	}
	
	return the_memsys->bank_[the_memsys->cur_row_].bank_num_;
}


// gets the KUP-edness of the current bank: true if KUP, false if not a KUP bank or if no current bank selected
// returns false if the bank is not a KUP
bool MemSys_GetCurrentRowKUPState(FMMemorySystem* the_memsys)
{
	FMBankObject*		the_bank;
	
	if (the_memsys->cur_row_ < 0)
	{
		return false;
	}
	
	the_bank = &the_memsys->bank_[the_memsys->cur_row_];

	return (the_bank->is_kup_);
}


// // Returns NULL if nothing matches, or returns pointer to first BankObject with a KUP name that matches exactly
// FMBankObject* MemSys_FindBankByKUPName(FMMemorySystem* the_memsys, char* search_phrase, int compare_len)
// {
// 	// LOGIC:
// 	//   iterate through all banks in the memory system
// 	//   when comparing, the int compare_len is used to limit the number of chars of bank that are searched
// 
// 	uint8_t		i;
// 
// 	if (the_memsys == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return NULL;
// 	}
// 
// 	for (i=0; i < MEMORY_BANK_COUNT; i++)
// 	{
// 		if ( General_Strncasecmp(search_phrase, the_memsys->bank_[i].name_, compare_len) == 0)
// 		{
// 			return &the_memsys->bank_[i];
// 		}
// 	}
// 
// 	DEBUG_OUT(("%s %d: couldn't find bank name match for '%s'. compare_len=%i", __func__ , __LINE__, search_phrase, compare_len));
// 
// 	return NULL;
// }


// // Returns NULL if nothing matches, or returns pointer to first BankObject with a KUP description that contains the search phrase
// // DOES NOT REQUIRE a match to the full KUP description
// FMBankObject* MemSys_FindBankByKUPDescriptionContains(FMMemorySystem* the_memsys, char* search_phrase, int compare_len)
// {
// 	// LOGIC:
// 	//   iterate through all banks in the memory system
// 	//   when comparing, the int compare_len is used to limit the number of chars of bank that are searched
// 	
// 	// TODO: write a general routine to find matches to an arbitrary string (not NULL terminated?) of chars starting at a given memory loc
// 	//    what is here is just a starting match to the description string.
// 	//    hmm. check standard C libs, I think there is something already that will do this, at least with strings. won't help in memory.
// 
// 	uint8_t		i;
// 
// 	if (the_memsys == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return NULL;
// 	}
// 
// 	for (i=0; i < MEMORY_BANK_COUNT; i++)
// 	{
// 		if ( General_Strncasecmp(search_phrase, the_memsys->bank_[i].description_, compare_len) == 0)
// 		{
// 			return &the_memsys->bank_[i];
// 		}
// 	}
// 
// 	DEBUG_OUT(("%s %d: couldn't find bank description match for '%s'. compare_len=%i", __func__ , __LINE__, search_phrase, compare_len));
// 
// 	return NULL;
// }

// // Returns NULL if nothing matches, or returns pointer to first BankObject that contains the search phrase
// // starting_bank_num is the first bank num to start searching in
// // NOTE: does NOT wrap around to bank 0 after hitting bank 63
// FMBankObject* MemSys_FindBankContainingPhrase(FMMemorySystem* the_memsys, char* search_phrase, int compare_len, uint8_t starting_bank_num)
// {
// 	// TODO: write routine for bring EM in 256b chunks at a time, searching for first hit on given string of chars
// 	//       needs to return the exact memory loc
// 	//       needs to be able to search across 256b boundaries
// 	//       needs to be able to accept a starting addr that isn't 0 (in other words, "find next", not just find first).
// 	
// 	return NULL;
// }


// looks through all files in the bank list, comparing the passed row to that of each bank.
// Returns NULL if nothing matches, or returns pointer to first matching BankObject
FMBankObject* MemSys_FindBankByRow(FMMemorySystem* the_memsys, uint8_t the_row)
{
	uint8_t		i;

	if (the_memsys == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return NULL;
	}
	
	for (i=0; i < MEMORY_BANK_COUNT; i++)
	{
		if (the_memsys->bank_[i].row_ == the_row)
		{
			return &the_memsys->bank_[i];
		}
	}
	
	DEBUG_OUT(("%s %d: couldn't find row %i", __func__ , __LINE__, the_row));

	return NULL;
}






// **** OTHER FUNCTIONS *****


// populate the banks in a memory system by scanning EM
void MemSys_PopulateBanks(FMMemorySystem* the_memsys)
{	
	uint8_t		i;
	uint8_t*	copy_buffer;
	
	uint8_t		kup_version;
	char*		kup_name;
	char*		kup_args;	// we don't care, but need to get past them to get to description
	char*		kup_description;
	uint8_t		the_len;
	uint8_t		flash_offset;
	uint8_t		bank_num;
	int8_t		num_banks_in_kup = 0;		//Byte  2    the size of program in 8k blocks
	int8_t		remaining_kup_banks = 0;	//Byte  2    the size of program in 8k blocks

	if (the_memsys == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_POPULATE_FILES_FOLDER_WAS_NULL);	// crash early, crash often
	}
		
	if (the_memsys->is_flash_)
	{
		flash_offset = MEMORY_BANK_COUNT;
	}
	else
	{
		flash_offset = 0;
	}
	
	// use string buff 2 for interbank copying
	copy_buffer = (uint8_t*)global_string_buff2;
	
	// read the first 256 bytes of every bank in extended memory
	for (i = 0; i < MEMORY_BANK_COUNT; i++)
	{
		bank_num = i + flash_offset;
		
		App_EMDataCopy((uint8_t*)copy_buffer, bank_num, 0, PARAM_COPY_FROM_EM);
		
		// check for KUP continuation, or KUP signature $F2$56, or every other bank
		// LOGIC:
		//   The first bank in a KUP identifies itself with "F256" 2-byte signature. 
		//   That header also indicates how many banks the KUP uses. we will use that to group the KUP banks together
		
		if (remaining_kup_banks > 0)
		{
			// we are continuing a previously identified KUP bank
			sprintf(global_string_buff1, "%s.%i", memsys_temp_kupname_buffer, num_banks_in_kup - remaining_kup_banks); // will result in "myprog-1", "myprog-2", etc. 
			Bank_Init(&the_memsys->bank_[i], global_string_buff1, NULL, BANK_KUP_SECONDARY, bank_num, i);

			//DEBUG_OUT(("%s %d: EM bank %02x: '%s' banks in kup=%i, remain=%i", __func__ , __LINE__, i, global_string_buff1, num_banks_in_kup, remaining_kup_banks));
			
			--remaining_kup_banks;
		}
		else if (copy_buffer[0] == 0xF2 && copy_buffer[1] == 0x56)
		{
			remaining_kup_banks = num_banks_in_kup = copy_buffer[2];	// Byte  2    the size of program in 8k blocks
			kup_version = copy_buffer[6];
			
			// get name: all versions of KUP supported the name
			kup_name = (char*)&copy_buffer[10];
			the_len = General_Strnlen((char*)kup_name, 128);
			General_Strlcpy((char*)memsys_temp_kupname_buffer, kup_name, MEMSYS_KUPNAME_TEMP_BUFFER_LEN);	// get a local-to-this-overlay copy for use with any child banks of the KUP
			
			//DEBUG_OUT(("%s %d: EM bank %02x: '%s' banks in kup=%u, namebuf='%s'", __func__ , __LINE__, i, kup_name, num_banks_in_kup, memsys_temp_kupname_buffer));
			
			if (kup_version > 0)
			{
				kup_args = kup_name + the_len + 1;
				the_len = General_Strnlen((char*)kup_args, 128);
				kup_description = kup_args + the_len + 1;
			}
			else
			{
				kup_description = (char*)"";
			}
			
			//sprintf(global_string_buff1, "EM bank %02x: '%s': '%s'", i, kup_name, kup_description);
			//Buffer_NewMessage(global_string_buff1);
			//DEBUG_OUT(("%s %d: EM bank %02x: '%s': '%s'", __func__ , __LINE__, i, kup_name, kup_description));
			
			Bank_Init(&the_memsys->bank_[i], kup_name, kup_description, BANK_KUP_PRIMARY, bank_num, i);
			--remaining_kup_banks; // don't forget to remove this bank from the count of banks associated with this KUP
		}
		else
		{
			Bank_Init(&the_memsys->bank_[i], NULL, NULL, BANK_NON_KUP, bank_num, i);
			//DEBUG_OUT(("%s %d: EM bank %02x is not a KUP bank; name='%s', desc='%s'", __func__ , __LINE__, bank_num, the_memsys->bank_[i].name_, the_memsys->bank_[i].description_));
		}
	}	

	// do NOT set current row to first bank
	//the_memsys->cur_row_ = -1; // leave at -1 until MemSys_SetBankSelectionByRow() or it won't detect a change
}



// // copies the passed bank. 
// bool MemSys_CopyBank(FMMemorySystem* the_memsys, FMBankObject* the_bank, FMMemorySystem* the_target_folder)
// {
// 	int32_t				bytes_copied;
// 	uint8_t				name_uniqueifier;
// 	uint8_t				name_len;
// 	uint8_t				tries = 0;
// 	uint8_t				max_tries = 100;
// 	//char				filename_buffer[(FILE_MAX_FILENAME_SIZE*2)] = "";	// allow 2x size of buffer so we can snip off first by just advancing pointer
// 	//char*				new_filename = filename_buffer;
// 	char*				the_target_folder_path;
// 	bool				success = false;
// 	WB2KList*			the_target_file_item;
// 	
// 	if (the_memsys == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_COPY_FILE_SOURCE_FOLDER_WAS_NULL);	// crash early, crash often
// 	}
// 
// 	if (the_target_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: param the_target_folder was null", __func__ , __LINE__));
// 		App_Exit(ERROR_COPY_FILE_TARGET_FOLDER_WAS_NULL);	// crash early, crash often
// 	}
// 	
// 	// LOGIC:
// 	//   if a file, call routine to copy bytes of file. then call again for the info file.
// 	//   if a folder:
// 	//     We only have a folder object for the current source folder, we do not have one for the target
// 	//     So every time we encounter a source folder, we have to generate the equivalent path for the target and store in BankMover
// 	//     Then check if the folder path exists on the target volume. Call makedir to create the folder path. then copy info file bytes. 
// 	//       NOTE: there is an assumption that the target folder is a real, existing path, so no need to create "up the chain". This assumption is based on how copy files works.
// 	//     This routine will not attempt to delete folders and their contents if they already exist, it will happily copy files into those folders. (more windows than mac)
// 	//   in either case, add the file and its info file (if any) to any open windows showing the target folder
// 	//     NOTE: in case of folder, we have to copy the info file before updating the target folder chain, or info file will be placed IN the folder, rather than next to it
// 	
// 	if (the_bank->is_directory_)
// 	{
// 	}
// 	else
// 	{
// 		// handle a file...
// 
// 		// update target file path without adding the source file's filename to it
// // 		BankMover_UpdateCurrentTargetFolderPath(App_GetBankMover(global_app), the_memsys->folder_file_->file_path_);
// // 		the_target_folder_path = BankMover_GetCurrentTargetFolderPath(App_GetBankMover(global_app));
// 		
// 		// check if the new file path is the same as the old: would be the case in a 'duplicate this file' situation
// 		// if so, figure out a compliant name that is unique. in fact, don't compare to the file at all, compare to entire folder!
// 		strcpy(folder_temp_filename, the_bank->file_name_);
// 		name_uniqueifier = 48; // start artificially high so it resets to 48. 
// 		
// 		while ( (the_target_file_item = MemSys_FindListItemByBankName(the_target_folder, folder_temp_filename)) != NULL && tries < max_tries)
// 		{		
// 			// there is a file in this folder with the same name. 
// 			// make name unique, then proceed with copy
// 			// we have limited filesize to work with. if under limit, add '1'. if at limit, remove right-most character?
// 			
// 			name_len = strlen(folder_temp_filename);
// 			
// 			if (name_len < (FILE_MAX_FILENAME_SIZE-1) && name_uniqueifier > 57)
// 			{
// 				name_uniqueifier = 48; // ascii 48, a 0 char. 
// 				folder_temp_filename[name_len] = name_uniqueifier;
// 				folder_temp_filename[name_len+1] = '\0';
// 			}
// 			else if (name_uniqueifier > 57)
// 			{
// 				// name is already at max, and we have cycled through digits (or haven't started yet)
// 				// snip off leading char and try again
// 				++folder_temp_filename;
// 				name_uniqueifier = 48; // ascii 48, a 0 char. 
// 				folder_temp_filename[name_len] = name_uniqueifier;
// 				folder_temp_filename[name_len+1] = '\0';
// 			}
// 			else
// 			{
// 				// we are somewhere 1-9, replace last digit
// 				folder_temp_filename[name_len-1] = name_uniqueifier;
// 				++name_uniqueifier;
// 			}
// 			
// 			++tries;
// 		}
// 
// 		//sprintf(global_string_buff1, "new='%s', tries=%u", folder_temp_filename, tries);
// 		//Buffer_NewMessage(global_string_buff1);
// 		
// 		if (the_target_file_item != NULL)
// 		{
// 			// couldn't get a unique name
// 			//Buffer_NewMessage("couldn't make unique name");
// 			return false;
// 		}
// 		
// 		// build a file path for target file, based on BankMover's current target folder path and source file name
// 		the_target_folder_path = the_target_folder->file_path_;
// 		General_CreateBankPathFromFolderAndBank(global_temp_path_1, the_memsys->file_path_, the_bank->file_name_);
// 		General_CreateBankPathFromFolderAndBank(global_temp_path_2, the_target_folder_path, folder_temp_filename);
// 		
// 		//sprintf(global_string_buff1, "copy file src path='%s', tgt path='%s', size=%lu", global_temp_path_1, global_temp_path_2, the_bank->size_);
// 		//Buffer_NewMessage(global_string_buff1);
// 
// 		// call function to copy file bits
// 		//DEBUG_OUT(("%s %d: copying file '%s' to '%s'...", __func__ , __LINE__, the_bank->file_name_, global_temp_path_2));
// 		//Buffer_NewMessage(General_GetString(ID_STR_MSG_COPYING));
// 		
// 		bytes_copied = MemSys_CopyBankBytes(global_temp_path_1, global_temp_path_2, the_bank->size_);
// 		
// 		if (bytes_copied < 0)
// 		{
// 			return false;
// 		}
// 	}	
// 	
// 	// mark the file as not selected 
// 	//Bank_SetSelected(the_bank, false);
// 
// 	// add a copy of the file to this target folder
// 	success = MemSys_AddNewBankAsCopy(the_target_folder, the_bank);
// 	//Buffer_NewMessage("added copy of file object to target folder");
// 			
// 	return success;
// }

	
// select or unselect 1 file by row id, and change cur_row_ accordingly
bool MemSys_SetBankSelectionByRow(FMMemorySystem* the_memsys, uint16_t the_row, bool do_selection, uint8_t y_offset, bool as_active)
{
	FMBankObject*		the_bank;
	FMBankObject*		the_prev_selected_bank;

	the_bank = MemSys_FindBankByRow(the_memsys, the_row);
	
	if (the_bank == NULL)
	{
		return false;
	}

	if (do_selection)
	{
		// is this already the currently selected file? do we need to unselect a different one? (only 1 allowed at a time)	
		if (the_memsys->cur_row_ == the_row)
		{
			// we re-selected the current file. 
		}
		else
		{
			// something else was selected. find it, and mark it unselected. 
			the_prev_selected_bank = MemSys_FindBankByRow(the_memsys, the_memsys->cur_row_);
		
			if (the_prev_selected_bank == NULL)
			{
			}
			else
			{
				if (Bank_MarkUnSelected(the_prev_selected_bank, y_offset) == false)
				{
					// the passed file was null. do anything?
					LOG_ERR(("%s %d: couldn't mark bank '%s' as selected", __func__ , __LINE__, the_prev_selected_bank->name_));
					App_Exit(ERROR_BANK_MARK_UNSELECTED_BANK_WAS_NULL);
				}
			}
		}

		the_memsys->cur_row_ = the_row;

		if (Bank_MarkSelected(the_bank, y_offset, as_active) == false)
		{
			// the passed file was null. do anything?
			LOG_ERR(("%s %d: couldn't mark bank '%s' as selected", __func__ , __LINE__, the_bank->name_));
			App_Exit(ERROR_BANK_MARK_SELECTED_BANK_WAS_NULL);
		}
	}
	else
	{
		if (the_memsys->cur_row_ == the_row)
		{
			// we unselected the current file. set current selection to none.
			the_memsys->cur_row_ = -1;
		}

		if (Bank_MarkUnSelected(the_bank, y_offset) == false)
		{
			// the passed file was null. do anything?
			LOG_ERR(("%s %d: couldn't mark bank '%s' as selected", __func__ , __LINE__, the_bank->name_));
			App_Exit(ERROR_BANK_MARK_UNSELECTED_BANK_WAS_NULL);
		}
	}

	return true;
}	


// ask the user what to fill the current bank with, and fill it with that value
// returns false on any error, or if user escapes out of dialog box without entering a number
bool MemSys_FillCurrentBank(FMMemorySystem* the_memsys)
{
	int16_t				the_fill_value;
	FMBankObject*		the_bank;
	
	if (the_memsys->cur_row_ < 0)
	{
		return false;
	}
	
	the_bank = &the_memsys->bank_[the_memsys->cur_row_];

	the_fill_value = Bank_AskForFillValue();
	
	if (the_fill_value < 0)
	{
		return false;
	}
		
	Bank_Fill(the_bank, the_fill_value);
	
	return true;
}


// fills the current bank with 0s
// returns false on any error
bool MemSys_ClearCurrentBank(FMMemorySystem* the_memsys)
{
	FMBankObject*		the_bank;
	
	if (the_memsys->cur_row_ < 0)
	{
		return false;
	}
	
	the_bank = &the_memsys->bank_[the_memsys->cur_row_];

	Bank_Clear(the_bank);
	
	return true;
}


// runs (executes) the program in the current bank, if the bank is a KUP bank
// returns false if the bank is not a KUP
bool MemSys_ExecuteCurrentRow(FMMemorySystem* the_memsys)
{
	FMBankObject*		the_bank;
	
	if (the_memsys->cur_row_ < 0)
	{
		return false;
	}
	
	the_bank = &the_memsys->bank_[the_memsys->cur_row_];

	if (the_bank->is_kup_)
	{
		Kernal_RunNamed(the_bank->name_, strlen(the_bank->name_));	// this will only ever return in an error condition. 
	}
	
	// if still here, this is an error condition, or the bank wasn't KUP
	return false;
}



// TEMPORARY DEBUG FUNCTIONS

// // helper function called by List class's print function: prints folder total bytes, and calls print on each file
// void MemSys_Print(void* the_payload)
// {
// 	FMMemorySystem*		the_memsys = (FMMemorySystem*)(the_payload);
// 
// 	DEBUG_OUT(("+----------------------------------+-+------------+----------+--------+"));
// 	DEBUG_OUT(("|Bank                              |S|Size (bytes)|Date      |Time    |"));
// 	DEBUG_OUT(("+----------------------------------+-+------------+----------+--------+"));
// 	List_Print(the_memsys->list_, &Bank_Print);
// 	DEBUG_OUT(("+----------------------------------+-+------------+----------+--------+"));
// 	DEBUG_OUT(("Total bytes %lu", the_memsys->total_bytes_));
// }
// 
