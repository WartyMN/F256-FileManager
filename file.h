/*
 * file.h
 *
 *  Created on: Sep 5, 2020
 *      Author: micahbly
 *
 *  This is a huge cut-down of the Amiga WorkBench2000 code, for F256 f/manager and B128 f/manager
 *    8-bit version started Jan 12, 2023
 */

#ifndef FILE_H_
#define FILE_H_



/* about this class: WB2KFile
 *
 * This handles
 *
 *** things this class needs to be able to do
 *
 *** things objects of this class have
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

#define PARAM_FILE_IS_FOLDER			true	// File_New() parameter
#define PARAM_FILE_IS_NOT_FOLDER		false	// File_New() parameter

#define FILE_MAX_EXTENSION_SIZE			8		// probably larger than needed, but... 


/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/


/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/

typedef struct WB2KFileObject
{
	bool				is_directory_;
	uint8_t				file_type_;			// F256jr... do what with this??
	uint32_t			size_;				// FAT16: not sure max file size
	struct DateTime		datetime_;			// IEC: not available. save for when SD card / FAT16 is available
	bool				selected_;
	uint8_t				x_;
	int8_t				display_row_;		// offset from the first displayed row of parent panel. -1 if not to be visible.
	uint8_t				row_;				// row_ is relative to the first file in the folder. 
	char*				file_name_;
	//char*				file_size_string_;	// human-readable version of file size
} WB2KFileObject;


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/



// **** CONSTRUCTOR AND DESTRUCTOR *****

// constructor
// allocates space for the object, accepts the 2 string pointers (allocates and copies them)
WB2KFileObject* File_New(const char* the_file_name, bool is_directory, uint32_t the_filesize, uint8_t the_filetype, uint8_t the_row, DateTime* the_datetime);


// duplicator
// makes a copy of the passed file object
WB2KFileObject* File_Duplicate(WB2KFileObject* the_original_file);

// destructor
// frees all allocated memory associated with the passed object, and the object itself
void File_Destroy(WB2KFileObject** the_file);


// **** SETTERS *****

// // set files selected/unselected status (no visual change)
// void File_SetSelected(WB2KFileObject* the_file, bool selected);

/// updates the icon's size/position information
void File_UpdatePos(WB2KFileObject* the_file, uint8_t x, int8_t display_row, uint16_t row);

// update the existing file name to the passed one, freeing any previous one and allocating anew.
bool File_UpdateFileName(WB2KFileObject* the_file, const char* new_file_name);

// // update the existing file path to the passed one, freeing any previous one and allocating anew.
// bool File_UpdateFilePath(WB2KFileObject* the_file, const char* new_file_path);


// **** GETTERS *****

// get the selected/not selected state of the file
bool File_IsSelected(WB2KFileObject* the_file);

// returns true if file object represents a folder
bool File_IsFolder(WB2KFileObject* the_file);

// // returns the filename - no allocation
// char* File_GetFileNameString(WB2KFileObject* the_file);

// // allocates and returns a copy of the modified date and time as string (for use with list mode headers or with info panel, etc.)
// char* File_GetFileDateStringCopy(WB2KFileObject* the_file);

// allocates and returns a copy of the file size as human readable string (for use with list mode headers or with info panel, etc.)
char* File_GetFileSizeStringCopy(WB2KFileObject* the_file);

// Populates the primary font data area in VICKY with bytes read from disk
// Returns false on any error
bool File_ReadFontData(char* the_file_path);

// Load the selected file into EM, starting at the address associated with the specified em_bank_num
// Returns false on any error
bool File_LoadFileToEM(char* the_file_path, uint8_t em_bank_num);

// get the free disk space on the parent disk of the file
// returns -1 in event of error
int16_t File_GetFreeBytesOnDisk(WB2KFileObject* the_file);


// **** OTHER FUNCTIONS *****

// delete the passed file/folder. If a folder, it must have been previously emptied of files.
bool File_Delete(char* the_file_path, bool is_directory);

// renames a file and its info file, if present
bool File_Rename(WB2KFileObject* the_file, const char* new_file_name, const char* old_file_path, const char* new_file_path);

// mark file as selected, and refresh display accordingly
bool File_MarkSelected(WB2KFileObject* the_file, int8_t y_offset);

// mark file as un-selected, and refresh display accordingly
bool File_MarkUnSelected(WB2KFileObject* the_file, int8_t y_offset);

// render filename and any other relevant labels at the previously established coordinates
// if as_selected is true, will render with inversed text. Otherwise, will render normally.
// if as_active is true, will render in LIST_ACTIVE_COLOR, otherwise in LIST_INACTIVE_COLOR
void File_Render(WB2KFileObject* the_file, bool as_selected, int8_t y_offset, bool as_active);

// helper function called by List class's print function: prints one file entry
void File_Print(void* the_payload);

// ***** comparison functions used to compare to list items with Wb2KFileObject payloads
bool File_CompareSize(void* first_payload, void* second_payload);
bool File_CompareName(void* first_payload, void* second_payload);
bool File_CompareFileTypeID(void* first_payload, void* second_payload);
// bool File_CompareDate(void* first_payload, void* second_payload);


#endif /* FILE_H_ */
