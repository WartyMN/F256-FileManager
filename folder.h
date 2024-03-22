/*
 * folder.h
 *
 *  Created on: Nov 21, 2020
 *      Author: micahbly
 *
 *  This is a huge cut-down of the Amiga WorkBench2000 code, for F256 f/manager and B128 f/manager
 *    8-bit version started Jan 12, 2023
 */

#ifndef FOLDER_H_
#define FOLDER_H_


/* about this class: WB2KFolderObject
 *
 * This handles folders on disk, with FileObject children for any files in the folder
 *
 *** things this class needs to be able to do
 *
 * get a listing of the files in the folder
 * indicate if a passed file object or filepath is a match for something in its list
 * perform actions on all or only-selected files in the folder, using recursion and a function pointer
 * - get total of all file sizes
 * - delete
 * - move
 * - copy
 *
 *** things objects of this class have
 *
 * A list of WB2KFileObjects (potentially empty)
 * count of files in the folder, kept up to date
 * total size, in bytes, of all files in the folder, including files in sub-folders (on request; not kept up to date for performance reasons)
 *
 */


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

#include "app.h"
#include "file.h"
#include "list.h"

/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/

#define FOLDER_SYSTEM_ROOT_NAME		(char*)"[ROOT]"

#define PARAM_MAKE_COPY_OF_FOLDER_FILE			true	// for Folder_New()
#define PARAM_DO_NOT_MAKE_COPY_OF_FOLDER_FILE	false	// for Folder_New()

// #define DO_NOT_DESTROY_FILE_OBJECT	false	// for Folder_RemoveFileListItem()
// #define DESTROY_FILE_OBJECT			true	// for Folder_RemoveFileListItem()

#define PROCESS_FOLDER_FILE_BEFORE_CHILDREN	true	// for Folder_ProcessContents()
#define PROCESS_FOLDER_FILE_AFTER_CHILDREN	false	// for Folder_ProcessContents()

#define FOLDER_MAX_TRIES_AT_FOLDER_CREATION		128		// arbitrary, for use with Folder_CreateNewFolder; stop at "unnamed folder 128"

#define _CBM_T_REG      0x10U   /* Bit set for regular files */
#define _CBM_T_HEADER   0x05U   /* Disk header / title */
#define _CBM_T_DIR      0x02U   /* IDE64 and CMD sub-directory */
#define FNX_FILETYPE_FONT	200	// any 2k file ending in .fnt
#define FNX_FILETYPE_EXE	201	// any .pgz, etc executable
#define FNX_FILETYPE_BASIC	202	// a .bas file that f/manager will try to pass to SuperBASIC
#define FNX_FILETYPE_MUSIC	203	// a .mod file that f/manager will try to pass to modojr
#define FNX_FILETYPE_IMAGE	204 // a .256 or .lbm image file.



/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/

typedef enum folder_error_code
{
	FOLDER_ERROR_OUT_OF_MEMORY = -10,
	FOLDER_ERROR_NO_FILE_EXTENSION = -2,
	FOLDER_ERROR_NON_LETHAL_ERROR = -1,
	FOLDER_ERROR_NO_ERROR = 0,
	FOLDER_ERROR_SUCCESS = 1,
} folder_error_code;


/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/

// this is a duplicate of the FILE struct defined in cc65. defined to make it possible to access the f_fd and f_flags bytes easily.
// it is defined in cc65/asminc/_file.inc in ASM fully, but in include/stdio.h it is only typedef'ed, without the definition.
typedef struct FILEmimic
{
	uint8_t		f_fd;
	uint8_t		f_flags;
	uint8_t		f_pushback;
} FILEmimic;


typedef struct WB2KFolderObject
{
	WB2KFileObject*		folder_file_;
	WB2KList**			list_;
	uint16_t			file_count_;
	int16_t				cur_row_;							// 0-n: selected file num. 0=first file. -1 if no file. 
//	uint32_t			total_bytes_;
// 	uint32_t			selected_bytes_;
// 	uint16_t			total_blocks_;
// 	uint16_t			selected_blocks_;
	uint8_t				device_number_;						// For CBM, 8-9-10-11. for fnx, 0-1-2
	char*				file_path_;							// rather than having in file, where it gets stored a lot, will just have in folder. 
} WB2KFolderObject;


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/

void cmd_ls(void);
// return a human-readable(ish) string for the filetype of the filetype ID passed.
// see cbm_filetype.h
char* File_GetFileTypeString(uint8_t cbm_filetype_id);


// **** CONSTRUCTOR AND DESTRUCTOR *****

// constructor
// allocates space for the object and any string or other properties that need allocating
// if make_copy_of_folder_file is false, it will use the passed file object as is. Do not pass a file object that is owned by a folder already (without setting to true)!
WB2KFolderObject* Folder_New(WB2KFileObject* the_root_folder_file, bool make_copy_of_folder_file, uint8_t the_device_number);

// reset the folder, without destroying it, to a condition where it can be completely repopulated
// destroys all child objects except the folder file, which is emptied out
// recreates the folder file based on the device number and the new_path string (eg, "0:myfolder")
// returns false on any error
bool Folder_Reset(WB2KFolderObject* the_folder, uint8_t the_device_number, char* new_path);

// destructor
// frees all allocated memory associated with the passed object, and the object itself
void Folder_Destroy(WB2KFolderObject** the_folder);

// toss out the old folder, start over and renew
void Folder_RefreshListing(WB2KFolderObject* the_folder);


// **** SETTERS *****

// sets the row num (-1, or 0-n) of the currently selected file
void Folder_SetCurrentRow(WB2KFolderObject* the_folder, int16_t the_row_number);


// **** GETTERS *****

// // returns the list of files associated with the folder
// WB2KList** Folder_GetFileList(WB2KFolderObject* the_folder);

// // returns the file object for the root folder
// WB2KFileObject* Folder_GetFolderFile(WB2KFolderObject* the_folder);

// returns true if folder has any files/folders in it. based on curated file_count_ property, not on a live check of disk.
bool Folder_HasChildren(WB2KFolderObject* the_folder);

// returns total number of files in this folder
uint16_t Folder_GetCountFiles(WB2KFolderObject* the_folder);

// returns the row num (-1, or 0-n) of the currently selected file
int16_t Folder_GetCurrentRow(WB2KFolderObject* the_folder);

// // returns true if folder has any files/folders showing as selected
// bool Folder_HasSelections(WB2KFolderObject* the_folder);

// // returns number of currently selected files in this folder
// uint16_t Folder_GetCountSelectedFiles(WB2KFolderObject* the_folder);

// // returns the first selected file/folder in the folder.
// WB2KFileObject* Folder_GetFirstSelectedFile(WB2KFolderObject* the_folder);

// // returns the first file/folder in the folder.
// WB2KFileObject* Folder_GetFirstFile(WB2KFolderObject* the_folder);

// // Returns NULL if nothing matches, or returns pointer to first FileObject with a filename that starts with the same string as the one passed
// // DOES NOT REQUIRE a match to the full filename
// WB2KFileObject* Folder_FindFileByFileNameStartsWith(WB2KFolderObject* the_folder, char* string_to_match, int compare_len);

// looks through all files in the file list, comparing the passed row to that of each file.
// Returns NULL if nothing matches, or returns pointer to first matching FileObject
WB2KFileObject* Folder_FindFileByRow(WB2KFolderObject* the_folder, uint8_t the_row);

// **** OTHER FUNCTIONS *****

// Add a file object to the list of files without checking for duplicates.
// returns true in all cases. NOTE: this is part of series of functions designed to be called by Window_ModifyOpenFolders(), and all need to return bools.
bool Folder_AddNewFile(WB2KFolderObject* the_folder, WB2KFileObject* the_file);

// Add a file object to the list of files without checking for duplicates. This variant makes a copy of the file before assigning it. Use case: MoveFiles or CopyFiles.
// returns true in all cases. 
// NOTE: this is part of series of functions designed to be called by Window_ModifyOpenFolders(), and all need to return bools.
bool Folder_AddNewFileAsCopy(WB2KFolderObject* the_folder, WB2KFileObject* the_file);

// // removes the passed list item from the list of files in the folder. Does NOT delete file from disk. Does NOT delete the file object.
// // returns true if a matching file was found and successfully removed.
// // NOTE: this is part of series of functions designed to be called by Window_ModifyOpenFolders(), and all need to return bools.
// bool Folder_RemoveFile(WB2KFolderObject* the_folder, WB2KFileObject* the_file);

// // deletes the passed file/folder. If a folder, it must have been previously emptied of files.
// bool Folder_DeleteFile(WB2KFolderObject* the_folder, WB2KList* the_item, WB2KFolderObject* not_needed);

// // removes the passed list item from the list of files in the folder. Does NOT delete file from disk. Optionally frees the file object.
// void Folder_RemoveFileListItem(WB2KFolderObject* the_folder, WB2KList* the_item, bool destroy_the_file_object);

// // Create a new folder on disk, and a new file object for it, and assign it to this folder. 
// // if try_until_successful is set, will rename automatically with trailing number until it can make a new folder (by avoiding already-used names)
// bool Folder_CreateNewFolder(WB2KFolderObject* the_folder, char* the_file_name, bool try_until_successful);

// copies the passed file/folder. If a folder, it will create directory on the target volume if it doesn't already exist
bool Folder_CopyFile(WB2KFolderObject* the_folder, WB2KFileObject* the_file, WB2KFolderObject* the_target_folder);

// compare 2 folder objects. When done, the original_root_folder will have been updated with removals/additions as necessary to match the updated file list
// returns true if any changes were detected, or false if files appear to be identical
bool Folder_SyncFolderContentsByFilePath(WB2KFolderObject* original_root_folder, WB2KFolderObject* updated_root_folder);

// populate the files in a folder by doing a directory command
uint8_t Folder_PopulateFiles(WB2KFolderObject* the_folder);

// counts the bytes in the passed file/folder, and adds them to folder.selected_bytes_
bool Folder_CountBytes(WB2KFolderObject* the_folder, WB2KList* the_item, WB2KFolderObject* not_needed);

// processes, with recursion where necessary, the contents of a folder, using the passed function pointer to process individual files/empty folders.
// returns -1 in event of error, or count of files affected
int Folder_ProcessContents(WB2KFolderObject* the_folder, WB2KFolderObject* the_target_folder, uint8_t the_scope, bool do_folder_before_children, bool (* action_function)(WB2KFolderObject*, WB2KList*, WB2KFolderObject*));

// move every currently selected file into the specified folder. Use when you DO have a folder object to work with
// returns -1 in event of error, or count of files moved
int Folder_MoveSelectedFiles(WB2KFolderObject* the_folder, WB2KFolderObject* the_target_folder);

// move every currently selected file into the specified folder file. Use when you only have a target folder file, not a full folder object to work with.
// returns -1 in event of error, or count of files moved
int Folder_MoveSelectedFilesToFolderFile(WB2KFolderObject* the_folder, WB2KFileObject* the_target_folder_file);

// select or unselect 1 file by row id, and change cur_row_ accordingly
bool Folder_SetFileSelectionByRow(WB2KFolderObject* the_folder, uint16_t the_row, bool do_selection, uint8_t y_offset);

// populate the folder's List by iterating through the DOS List objects and treating each as a folder/file
bool Folder_PopulateVolumeList(WB2KFolderObject* the_folder);



// TEMPORARY DEBUG FUNCTIONS

// helper function called by List class's print function: prints folder total bytes, and calls print on each file
void Folder_Print(void* the_payload);


#endif /* FOLDER_H_ */
