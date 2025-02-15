/*
 * folder.c
 *
 *  Created on: Nov 21, 2020
 *      Author: micahbly
 */


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "api.h"
#include "app.h"
#include "comm_buffer.h"
#include "debug.h"
#include "file.h"
#include "folder.h"
#include "general.h"
#include "list.h"
#include "list_panel.h"
#include "strings.h"
#include "text.h"

// C includes
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <device.h>
//#include <dirent.h>
#include "dirent.h"

// F256 includes
#include "f256.h"



/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/


/*****************************************************************************/
/*                          File-scoped Variables                            */
/*****************************************************************************/

static char			folder_temp_filename_buffer[FILE_MAX_FILENAME_SIZE];
static char*		folder_temp_filename = folder_temp_filename_buffer;


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

extern char*		global_temp_path_1;
extern char*		global_temp_path_2;

extern char*		global_retrieved_em_filename;

extern char*		global_string_buff1;


/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

// // looks through all files in the file list, comparing the passed file object, and turning true if found in the list
// // use case: checking if a given file in a selection pool is also the potential target for a drag action
// bool Folder_InFileList(WB2KFolderObject* the_folder, WB2KFileObject* the_file, uint8_t the_scope);

// looks through all files in the file list, comparing the passed string to the filename of each file.
// Returns NULL if nothing matches, or returns pointer to first matching list item
WB2KList* Folder_FindListItemByFileName(WB2KFolderObject* the_folder, char* the_file_name);

// looks through all files in the file list, comparing the passed string to the filepath of each file.
// Returns NULL if nothing matches, or returns pointer to first matching list item
WB2KList* Folder_FindListItemByFilePath(WB2KFolderObject* the_folder, char* the_file_path, short the_compare_len);

// looks through all files in the file list, comparing the passed string to the filepath of each file.
// Returns NULL if nothing matches, or returns pointer to first matching FileObject
WB2KFileObject* Folder_FindFileByFilePath(WB2KFolderObject* the_folder, char* the_file_path, short the_compare_len);

// copy file bytes. Returns number of bytes copied, or -1 in event of any error
int32_t Folder_CopyFileBytes(const char* the_source_file_path, const char* the_target_file_path, int32_t expected_bytes);


/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/


// copy file bytes. Returns number of bytes copied, or -1 in event of any error
int32_t Folder_CopyFileBytes(const char* the_source_file_path, const char* the_target_file_path, int32_t expected_bytes)
{
	FILE*		the_source_handle;
	FILE*		the_target_handle;
	uint8_t*	the_buffer = (uint8_t*)STORAGE_FILE_BUFFER_1;
	int16_t		bytes_read = 0;
	int32_t		total_bytes_read = 0;
	uint32_t	percent_read = 0;
	bool		keep_going = true;
	

	// prepare to use progress bar
	App_ShowProgressBar();
	
	// Open source file for Reading
	the_source_handle = fopen(the_source_file_path, "r");
	
	if (the_source_handle == NULL)
	{
		//sprintf(global_string_buff1, "source file '%s' could not be opened", the_source_file_path);
		//Buffer_NewMessage(global_string_buff1);
		//LOG_ERR(("%s %d: file '%s' could not be opened for reading", __func__ , __LINE__, the_source_file_path));
		goto error;
	}
	else
	{
		// Get a target file handle for Writing
		the_target_handle = Folder_GetTargetHandleForWriting(the_target_file_path);

		if (the_target_handle == NULL)
		{
			LOG_ERR(("%s %d: file '%s' could not be opened for writing", __func__ , __LINE__, the_target_file_path));
			goto error;
		}

		// loop until source file EOF, writing STORAGE_FILE_BUFFER_1_LEN bytes per loop (sized to available buffer)
		do
		{
			bytes_read = fread(the_buffer, 1, STORAGE_FILE_BUFFER_1_LEN, the_source_handle);
	
			if ( bytes_read < 0)
			{
				//Buffer_NewMessage("bytes_read < 0");
				//LOG_ERR(("%s %d: reading file '%s' resulted in error %i", __func__ , __LINE__, the_source_file_path, bytes_read));
				goto error;
			}
	
			if ( bytes_read == 0)
			{
				//Buffer_NewMessage("bytes_read == 0 (end of file)");
				//LOG_ERR(("%s %d: reading file '%s' produced 0 bytes", __func__ , __LINE__, the_source_file_path));
				keep_going = false;
			}
		
			if ( bytes_read < STORAGE_FILE_BUFFER_1_LEN )
			{
				// we hit end of file
				//Buffer_NewMessage("s_bytes_read_from_disk was less than full row");
				//LOG_ERR(("%s %d: reading file '%s' expected %u bytes, got %i bytes", __func__ , __LINE__, App_GetFilenameFromEM(the_file), num_bytes_to_read, s_bytes_read_from_disk));
				keep_going = false;
			}

			fwrite(the_buffer, 1, bytes_read, the_target_handle);
			total_bytes_read += (uint32_t)bytes_read;
			
			percent_read = (uint32_t)total_bytes_read;	// REALLY don't want to do math with signed ints
			percent_read = (percent_read * 100) / (uint32_t)expected_bytes;
			
			//sprintf(global_string_buff1, "bytes read=%d, percent_read=%lu, total_bytes_read=%i", bytes_read, percent_read, total_bytes_read);
			//Buffer_NewMessage(global_string_buff1);

			App_UpdateProgressBar((uint8_t)percent_read);
			
		} while (keep_going == true);
		
		fclose(the_source_handle);
		fclose(the_target_handle);
	}

	// clear the progress bar
	App_HideProgressBar();
		
	return total_bytes_read;
	
error:
	if (the_source_handle)	fclose(the_source_handle);
	if (the_target_handle)	fclose(the_target_handle);
	App_HideProgressBar();
	
	return -1;
}


// // looks through all files in the file list, comparing the passed file object, and turning true if found in the list
// // use case: checking if a given file in a selection pool is also the potential target for a drag action
// bool Folder_InFileList(WB2KFolderObject* the_folder, WB2KFileObject* the_file, uint8_t the_scope)
// {
// 	WB2KList*	the_item;
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 	
// 	the_item = *(the_folder->list_);
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject* this_file = (WB2KFileObject*)(the_item->payload_);
// 
// 		// is this the item we are looking for?
// 		if (the_scope == LIST_SCOPE_ALL || (the_scope == LIST_SCOPE_SELECTED && this_file->selected_) || (the_scope == LIST_SCOPE_NOT_SELECTED && this_file->selected_ == false))
// 		{
// 			if (this_file == the_file)
// 			{
// 				return true;
// 			}
// 		}
// 
// 		the_item = the_item->next_item_;
// 	}
// 
// 	return false;
// }


// looks through all files in the file list, comparing the passed string to the filename of each file.
// Returns NULL if nothing matches, or returns pointer to first matching list item
WB2KList* Folder_FindListItemByFileName(WB2KFolderObject* the_folder, char* the_file_name)
{
	// LOGIC:
	//   iterate through all files in the panel's list
	//   when comparing, the int compare_len is used. This allows an incoming string with .info to be matched easily against a parent filename without the .info.
	WB2KList*	the_item;
	short		the_compare_len = General_Strnlen(the_file_name, FILE_MAX_FILENAME_SIZE);

	if (the_folder == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return NULL;
	}
	
	the_item = *(the_folder->list_);

	while (the_item != NULL)
	{
		WB2KFileObject* this_file = (WB2KFileObject *)(the_item->payload_);

		// is this the item we are looking for?
		//DEBUG_OUT(("%s %d: examining file '%s' (len %i) against '%s' (len %i)", __func__ , __LINE__, App_GetFilenameFromEM(this_file->id_), General_Strnlen(App_GetFilenameFromEM(this_file->id_), FILE_MAX_PATHNAME_SIZE), the_file_name, the_compare_len));
		
		App_GetFilenameFromEM(this_file);
		
		if ( General_Strnlen(global_retrieved_em_filename, FILE_MAX_FILENAME_SIZE) == the_compare_len )
		{			
			//DEBUG_OUT(("%s %d: lengths reported as match", __func__ , __LINE__));
			
			if ( (General_Strncasecmp(the_file_name, global_retrieved_em_filename, the_compare_len)) == 0)
			{
				return the_item;
			}
		}

		the_item = the_item->next_item_;
	}

	//DEBUG_OUT(("%s %d: no match to filename '%s'", __func__ , __LINE__, the_file_name));
	
	return NULL;
}


// // looks through all files in the file list, comparing the passed string to the filepath of each file.
// // if check_device_name is set, it will not return a match unless the drive codes also match up
// // Returns NULL if nothing matches, or returns pointer to first matching list item
// WB2KList* Folder_FindListItemByFilePath(WB2KFolderObject* the_folder, char* the_file_path, short the_compare_len)
// {
// 	// LOGIC:
// 	//   iterate through all files in the panel's list
// 	//   when comparing, the int compare_len is used. This allows an incoming string with .info to be matched easily against a parent filename without the .info.
// 	WB2KList*	the_item;
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return NULL;
// 	}
// 	
// 	the_item = *(the_folder->list_);
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject* this_file = (WB2KFileObject *)(the_item->payload_);
// 
// 		// is this the item we are looking for?
// 		//DEBUG_OUT(("%s %d: examining file '%s' (len %i) against '%s' (len %i)", __func__ , __LINE__, this_file->file_path_, General_Strnlen(this_file->file_path_, FILE_MAX_PATHNAME_SIZE), the_file_path, the_compare_len));
// 		
// 		if ( General_Strnlen(this_file->file_path_, FILE_MAX_PATHNAME_SIZE) == the_compare_len )
// 		{			
// 			//DEBUG_OUT(("%s %d: lengths reported as match", __func__ , __LINE__));
// 			
// 			if ( (General_Strncasecmp(the_file_path, this_file->file_path_, the_compare_len)) == 0)
// 			{
// 				return the_item;
// 			}
// 		}
// 
// 		the_item = the_item->next_item_;
// 	}
// 	
// 	return NULL;
// }


// // looks through all files in the file list, comparing the passed string to the filepath of each file.
// // Returns NULL if nothing matches, or returns pointer to first matching FileObject
// WB2KFileObject* Folder_FindFileByFilePath(WB2KFolderObject* the_folder, char* the_file_path, short the_compare_len)
// {
// 	// LOGIC:
// 	//   iterate through all files in the panel's list
// 	//   when comparing, the int compare_len is used. This allows an incoming string with .info to be matched easily against a parent filename without the .info.
// 
// 	WB2KList*	the_item;
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return NULL;
// 	}
// 
// 	the_item = Folder_FindListItemByFilePath(the_folder, the_file_path, the_compare_len);
// 
// 	if (the_item == NULL)
// 	{
// 		//DEBUG_OUT(("%s %d: couldn't find path '%s'", __func__ , __LINE__, the_file_path));
// 		return NULL;
// 	}
// 	
// 	return (WB2KFileObject *)(the_item->payload_);
// }





/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/



// **** CONSTRUCTOR AND DESTRUCTOR *****


// constructor
// allocates space for the object and any string or other properties that need allocating
// if the passed folder pointer is not NULL, it will pass it back without allocating a new one.
// if the passed folder pointer is NULL, it will reset the folder, without destroying it, to a condition where it can be completely repopulated
// destroys all child objects except the folder file, which is emptied out
// recreates the folder file based on the device number and the new_path string (eg, "0:myfolder")
// returns NULL on any non-fatal error
WB2KFolderObject* Folder_NewOrReset(WB2KFolderObject* the_existing_folder,uint8_t the_device_number, char* new_path)
{
	WB2KFolderObject*	the_folder;
	char**				this_string_p;
	
	if (the_existing_folder == NULL)
	{
		if ( (the_folder = (WB2KFolderObject*)calloc(1, sizeof(WB2KFolderObject)) ) == NULL)
		{
			LOG_ERR(("%s %d: could not allocate memory to create new folder object", __func__ , __LINE__));
			goto error;
		}
		LOG_ALLOC(("%s %d:	__ALLOC__	the_folder	%p	size	%i", __func__ , __LINE__, the_folder, sizeof(WB2KFolderObject)));
	}
	else
	{
		the_folder = the_existing_folder;

		// free all files in the folder's file list
		Folder_DestroyAllFiles(the_folder);
		
		// free the list
		LOG_ALLOC(("%s %d:	__FREE__	(the_folder)->list_	%p	size	%i", __func__ , __LINE__, (the_folder)->list_, sizeof(WB2KList*)));
		free((the_folder)->list_);
		(the_folder)->list_ = NULL;
		
		// free strings
LOG_ALLOC(("%s %d:	this_string_p=%p, &the_folder->file_name_=%p", __func__ , __LINE__, this_string_p, &the_folder->file_name_));
		this_string_p = &the_folder->file_name_;
LOG_ALLOC(("%s %d:	this_string_p=%p, &the_folder->file_name_=%p", __func__ , __LINE__, this_string_p, &the_folder->file_name_));
		the_folder->file_name_ = NULL;
LOG_ALLOC(("%s %d:	this_string_p=%p, &the_folder->file_name_=%p", __func__ , __LINE__, this_string_p, &the_folder->file_name_));
		
		if (*this_string_p)
		{
			free(*this_string_p);
		}
		
		this_string_p = &the_folder->file_path_;
		the_folder->file_path_ = NULL;
		
		if (*this_string_p)
		{
			free(*this_string_p);
		}		
	}
	
	// initiate the list, but don't add the first node yet (we don't have any items yet)
	if ( (the_folder->list_ = (WB2KList**)calloc(1, sizeof(WB2KList*)) ) == NULL)
	{
		LOG_ERR(("%s %d: could not allocate memory to create new list", __func__ , __LINE__));
		goto error;
	}
	LOG_ALLOC(("%s %d:	__ALLOC__	the_folder->list_	%p	size	%i", __func__ , __LINE__, the_folder->list_, sizeof(WB2KList*)));	
	
	// set/reset some other props
	the_folder->cur_row_ = 0;
	the_folder->file_count_ = 0;
	the_folder->device_number_ = the_device_number;
	the_folder->is_meatloaf_ = false;
	
	
	// set folderpath and filename to match the value passed for path
	if ( (the_folder->file_path_ = General_StrlcpyWithAlloc(new_path, FILE_MAX_PATHNAME_SIZE)) == NULL)
	{
		//Buffer_NewMessage("could not allocate memory for the path name");
		LOG_ERR(("%s %d: could not allocate memory for the path name", __func__ , __LINE__));
		goto error;
	}
	LOG_ALLOC(("%s %d:	__ALLOC__	the_folder->file_path_	%p	size	%i	%s", __func__ , __LINE__, the_folder->file_path_, General_Strnlen(the_folder->file_path_, FILE_MAX_PATHNAME_SIZE) + 1, the_folder->file_path_));

	// do not set filename yet. Will be set by populate folder logic.

	return the_folder;

error:
	if (the_folder)		free(the_folder);
	return NULL;
}


// destructor
// frees all allocated memory associated with the passed object, and the object itself
void Folder_Destroy(WB2KFolderObject** the_folder)
{
	if (*the_folder == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_FOLDER_TO_DESTROY_WAS_NULL);	// crash early, crash often
	}

	if ((*the_folder)->file_path_ != NULL)
	{
		LOG_ALLOC(("%s %d:	__FREE__	(*the_folder)->file_path_	%p	size	%i	%s", __func__ , __LINE__, (*the_folder)->file_path_, General_Strnlen((*the_folder)->file_path_, FILE_MAX_PATHNAME_SIZE) + 1, (*the_folder)->file_path_));
		free((*the_folder)->file_path_);
		(*the_folder)->file_path_ = NULL;
	}

	if ((*the_folder)->file_name_ != NULL)
	{
		LOG_ALLOC(("%s %d:	__FREE__	(*the_folder)->file_name_	%p	size	%i	%s", __func__ , __LINE__, (*the_folder)->file_name_, General_Strnlen((*the_folder)->file_name_, FILE_MAX_FILENAME_SIZE) + 1, (*the_folder)->file_name_));
		free((*the_folder)->file_name_);
		(*the_folder)->file_name_ = NULL;
	}


	// free all files in the folder's file list
	Folder_DestroyAllFiles(*the_folder);
	LOG_ALLOC(("%s %d:	__FREE__	(*the_folder)->list_	%p	size	%i", __func__ , __LINE__, (*the_folder)->list_, sizeof(WB2KList*)));
	free((*the_folder)->list_);
	(*the_folder)->list_ = NULL;
	
	// free the folder object itself
	LOG_ALLOC(("%s %d:	__FREE__	*the_folder	%p	size	%i", __func__ , __LINE__, *the_folder, sizeof(WB2KFolderObject)));
	free(*the_folder);
	*the_folder = NULL;
}


// free every fileobject in the panel's list, and remove the nodes from the list
void Folder_DestroyAllFiles(WB2KFolderObject* the_folder)
{
	int			num_nodes = 0;
	WB2KList*	the_item;

	if (the_folder == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_DESTROY_ALL_FOLDER_WAS_NULL);	// crash early, crash often
	}
	
	the_item = *(the_folder->list_);

	while (the_item != NULL)
	{
		WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);
		// sprintf(global_string_buff1, "Folder_DestroyAllFiles: destroying '%s'...", App_GetFilenameFromEM(this_file->id_));
		// Buffer_NewMessage(global_string_buff1);	
		
		File_Destroy(&this_file);
		++num_nodes;
		--the_folder->file_count_;

		the_item = the_item->next_item_;
	}

	// now free up the list items themselves
	List_Destroy(the_folder->list_);

	//DEBUG_OUT(("%s %d: %i files freed", __func__ , __LINE__, num_nodes));
	//Buffer_NewMessage("Done destroying all files in folder");
	
	return;
}




// **** SETTERS *****




// sets the row num (-1, or 0-n) of the currently selected file
void Folder_SetCurrentRow(WB2KFolderObject* the_folder, int16_t the_row_number)
{
	if (the_folder == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_SET_CURR_ROW_FOLDER_WAS_NULL);	// crash early, crash often
	}
	
	if (the_folder->file_count_ < (the_row_number+1))
	{
		the_folder->cur_row_ = -1;
	}
	else
	{
		the_folder->cur_row_ = the_row_number;
	}
}



// **** GETTERS *****

// // returns the list of files associated with the folder
// WB2KList** Folder_GetFileList(WB2KFolderObject* the_folder)
// {
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return NULL;
// 	}
// 	
// 	return the_folder->list_;
// }


// // returns the file object for the root folder
// WB2KFileObject* Folder_GetFolderFile(WB2KFolderObject* the_folder)
// {
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return NULL;
// 	}
// 	
// 	return the_folder->folder_file_;
// }


// // returns true if folder has any files/folders in it. based on curated file_count_ property, not on a live check of disk.
// bool Folder_HasChildren(WB2KFolderObject* the_folder)
// {
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 	
// 	if (the_folder == NULL)
// 	{
// 		return false;
// 	}
// 
// 	if (the_folder->file_count_ == 0)
// 	{
// 		return false;
// 	}
// 
// 	return true;
// }


// returns total number of files in this folder
uint16_t Folder_GetCountFiles(WB2KFolderObject* the_folder)
{
	if (the_folder == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
	}
	
	return the_folder->file_count_;
}


// returns the row num (-1, or 0-n) of the currently selected file
int16_t Folder_GetCurrentRow(WB2KFolderObject* the_folder)
{
	if (the_folder == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_GET_CURR_ROW_FOLDER_WAS_NULL);	// crash early, crash often
	}
	
	return the_folder->cur_row_;
}


// returns the currently selected file, or NULL if no file is marked as selected
WB2KFileObject* Folder_GetCurrentFile(WB2KFolderObject* the_folder)
{
	if (the_folder->cur_row_ < 0)
	{
		return NULL;
	}
	
	return Folder_FindFileByRow(the_folder, the_folder->cur_row_);
}


// returns the file type of the currently selected file, or 0 if no file is marked as selected
uint8_t Folder_GetCurrentFileType(WB2KFolderObject* the_folder)
{
	WB2KFileObject*		the_file;
	
	the_file =  Folder_GetCurrentFile(the_folder);
	
	if (the_file == NULL)
	{
		return 0;
	}
	
	return the_file->file_type_;
}



// // returns true if folder has any files/folders showing as selected
// bool Folder_HasSelections(WB2KFolderObject* the_folder)
// {
// 	// TODO: OPTIMIZATION - think about having dedicated loop for this check that stops on first hit. will be faster with bigger folders.
// 	// TODO: OPTIMIZATION - think about tracking this as a class property instead, and update when files are selected/unselected? Not sure which would be faster. 
// 	
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 
// 	if (Folder_GetCountSelectedFiles(the_folder) == 0)
// 	{
// 		return false;
// 	}
// 
// 	return true;
// }


// // returns number of currently selected files in this folder
// uint16_t Folder_GetCountSelectedFiles(WB2KFolderObject* the_folder)
// {
// 	// LOGIC:
// 	//   iterate through all files in the folder's list and count any that are marked as selected
// 
// 	uint16_t	the_count = 0;
// 	WB2KList*		the_item;
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 
// 	the_item = *(the_folder->list_);
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);
// 
// 		if (this_file->selected_)
// 		{
// 			++the_count;
// 		}
// 
// 		the_item = the_item->next_item_;
// 	}
// 
// 	return the_count;
// }


// // returns the first selected file/folder in the folder.
// // use Folder_GetCountSelectedFiles() first if you need to make sure you will be getting the only selected file.
// WB2KFileObject* Folder_GetFirstSelectedFile(WB2KFolderObject* the_folder)
// {
// 	// LOGIC:
// 	//   iterate through all files in the folder's list and return the first file/folder marked as selected
// 
// 	WB2KList*		the_item;
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return NULL;
// 	}
// 
// 	the_item = *(the_folder->list_);
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);
// 
// 		if (this_file->selected_)
// 		{
// 			return this_file;
// 		}
// 
// 		the_item = the_item->next_item_;
// 	}
// 
// 	return NULL;
// }


// // returns the first file/folder in the folder.
// WB2KFileObject* Folder_GetFirstFile(WB2KFolderObject* the_folder)
// {
// 	WB2KList*		the_item;
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return NULL;
// 	}
// 
// 	the_item = *(the_folder->list_);
// 
// 	if (the_item != NULL)
// 	{
// 		WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);
// 		return this_file;
// 	}
// 
// 	return NULL;
// }


// // returns the lowest or highest row number used by all the selected files in the folder
// // WARNING: will always return a number, even if no files selected, so calling function must have made it's own checks on selection where necessary
// uint16_t Folder_GetMinOrMaxSelectedRow(WB2KFolderObject* the_folder, bool find_max)
// {
// 	// LOGIC:
// 	//   iterate through all files in the folder's list and keep track of the lowest row # for those that are selected
// 
// 	uint16_t	boundary = 0xFFFF;
// 	WB2KList*		the_item;
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 
// 	the_item = *(the_folder->list_);
// 
// 	if (find_max)
// 	{
// 		boundary = 0;
// 	}
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);
// 
// 		if (this_file->selected_)
// 		{
// 			uint16_t		this;
// 
// 			this = this_file->row_;
// 			
// 			if (find_max)
// 			{
// 				if (this > boundary) boundary = this;
// 			}
// 			else
// 			{
// 				if (this < boundary) boundary = this;
// 			}
// 		}
// 
// 		the_item = the_item->next_item_;
// 	}
// 
// 	return boundary;
// }


// // looks through all files in the file list, comparing the passed string to the filename_ of each file.
// // Returns NULL if nothing matches, or returns pointer to first FileObject with a filename that starts with the same string as the one passed
// // DOES NOT REQUIRE a match to the full filename. case insensitive search is used.
// WB2KFileObject* Folder_FindFileByFileNameStartsWith(WB2KFolderObject* the_folder, char* string_to_match, int compare_len)
// {
// 	// LOGIC:
// 	//   iterate through all files in the panel's list
// 	//   when comparing, the int compare_len is used to limit the number of chars of filename that are searched
// 
// 	WB2KList*		the_item;
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return NULL;
// 	}
// 
// 	the_item = *(the_folder->list_);
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);
// 
// 		// is this the item we are looking for?
// 		if ( General_Strncasecmp(string_to_match, App_GetFilenameFromEM(this_file->id_), compare_len) == 0)
// 		{
// 			return this_file;
// 		}
// 
// 		the_item = the_item->next_item_;
// 	}
// 
// 	DEBUG_OUT(("%s %d: couldn't find filename match for '%s'. compare_len=%i", __func__ , __LINE__, string_to_match, compare_len));
// 
// 	return NULL;
// }


// looks through all files in the file list, comparing the passed row to that of each file.
// Returns NULL if nothing matches, or returns pointer to first matching FileObject
WB2KFileObject* Folder_FindFileByRow(WB2KFolderObject* the_folder, uint8_t the_row)
{
	WB2KList*	the_item;

	if (the_folder == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return NULL;
	}
	
	the_item = *(the_folder->list_);

	while (the_item != NULL)
	{
		WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);

		// is this the item we are looking for?
		if ( this_file->row_ == the_row)
		{
			return this_file;
		}

		the_item = the_item->next_item_;
	}

	DEBUG_OUT(("%s %d: couldn't find row %i", __func__ , __LINE__, the_row));

	return NULL;
}






// **** OTHER FUNCTIONS *****


// populate the files in a folder by doing a directory command
uint8_t Folder_PopulateFiles(uint8_t the_panel_id, WB2KFolderObject* the_folder)
{	
	bool				skip_this_file;
	bool				file_added;
	uint8_t				meatloaf_info_file_cnt = 0;	// if in meatloaf mode, treat first 4 files as info-only files. convert last one to '..'
	uint8_t				meatloaf_slash_cnt = 0;			// used to parse the INFO file row and tell if we're on root or not.
	uint8_t				i;
	uint8_t				filename_len;
	uint32_t			calc_file_size;
	char*				this_file_name;
	struct DIR*			dir;
	struct dirent*		dirent;
	uint8_t				the_error_code = ERROR_NO_ERROR;
	uint16_t			file_cnt = 0;
	WB2KFileObject*		this_file;
	DateTime			this_datetime;
	uint16_t			the_block_size;

	//	uint8_t* 	temp_ptr;// = (uint8_t*)&dirent->d_blocks;
	
	if (the_folder == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_POPULATE_FILES_FOLDER_WAS_NULL);	// crash early, crash often
	}

	if (the_folder->file_path_ == NULL)
	{
		//sprintf(global_string_buff1, "filepath for folder '%s' was null", the_folder->file_path_);
		//Buffer_NewMessage(global_string_buff1);
		LOG_ERR(("%s %d: passed folder's filepath was null", __func__ , __LINE__));
		App_Exit(ERROR_FOLDER_WAS_NULL);	// crash early, crash often
	}

	// set up base path for the folder + /. we will use this to build the filepaths for the files individually
	
	General_Strlcpy(global_temp_path_1, the_folder->file_path_, FILE_MAX_PATHNAME_SIZE);

	// reset panel's file count, as we will be starting over from zero
	the_folder->file_count_ = 0;

	// account for FAT32 sectors vs IEC blocks when estimating file szie
	if (the_folder->device_number_ == 0)
	{
		// SD-card = FAT32
		the_block_size = FILE_BYTES_PER_BLOCK;
	}
	else
	{
		// a floppy = CBMDOS
		the_block_size = FILE_BYTES_PER_BLOCK_IEC;
	}
	
    /* print directory listing */

	dir = Kernel_OpenDir(the_folder->file_path_);

	if (! dir) {
		//sprintf(global_string_buff1, "Kernel_OpenDir failed. filepath='%s'. errno=%u", the_folder->file_path_, errno);
		//sprintf(global_string_buff1, "Kernel_OpenDir failed. filepath='%s'", the_folder->file_path_);
		//Buffer_NewMessage(global_string_buff1);
		return ERROR_COULD_NOT_OPEN_DIR;
	}
	
    while ( (dirent = Kernel_ReadDir(dir)) != NULL )
    {
        // is this is the disk name, or a file?
		//temp_ptr = (uint8_t*)&dirent->d_bytes;
		//temp_ptr = (uint8_t*)&dirent->d_blocks;
		//sprintf(global_string_buff1, "dirent->d_name='%s', dirent->d_type=%u,", dirent->d_name, dirent->d_type);
		//sprintf(global_string_buff1, "dirent->d_name='%s', dirent->d_type=%u, dirent->d_name[0]=%u", dirent->d_name, dirent->d_type, dirent->d_name[0]);
		//sprintf(global_string_buff1, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", temp_ptr[0], temp_ptr[1], temp_ptr[2], temp_ptr[3], temp_ptr[4], temp_ptr[5], temp_ptr[6], temp_ptr[7], temp_ptr[8], temp_ptr[9]);
		//sprintf(global_string_buff1, "%s %02X %02X %02X %02X %02X %02X %02X %02X", dirent->d_name, temp_ptr[0], temp_ptr[1], temp_ptr[2], temp_ptr[3], temp_ptr[4], temp_ptr[5], temp_ptr[6], temp_ptr[7]);
		
		//Buffer_NewMessage(global_string_buff1);

		skip_this_file = false;
		meatloaf_slash_cnt = 0;
		
		if (the_folder->is_meatloaf_ == true && meatloaf_info_file_cnt < 5)
		{
			// LOGIC:
			//   meatloaf mode won't be detected until the first file, the label, is read in, so it won't get caught by this.
			//   next, meatloaf will present 5 "files" of 0 bytes. 
			//     these consist of:
			//     [URL]
			//     c64.meatloaf.cc (or whatever)
			//     [PATH]
			//     /DEMO/
			//     ---------------------------
			//   Believe all will be presented in all-caps, as far as Foenix is concerned. 
			
			if (_DE_ISREG(dirent->d_type) == true || _DE_ISDIR(dirent->d_type)  == true)	// remember, microkernel doesn't check, it just assumes dir if block size is 0.
        	{
				//DEBUG_OUT(("%s %d: file '%s' believed to be meatloaf info line #%u", __func__ , __LINE__, dirent->d_name, meatloaf_info_file_cnt));
				//sprintf(global_string_buff1, "file '%s' believed to be meatloaf info line #%u", dirent->d_name, meatloaf_info_file_cnt);
				//Buffer_NewMessage(global_string_buff1);
				
				if (meatloaf_info_file_cnt == 0)
				{
					if (General_Strncasecmp((char*)dirent->d_name, "[URL]", 5) == 0)
					{
						meatloaf_info_file_cnt = 1;
						skip_this_file = true;
					}
					else
					{
						// when meatloaf displays contents on first load (what it knows internally vs some URL), it doesn't have the 5 info lines
						// we need to let this file get processed
					}					
				}
				else if (meatloaf_info_file_cnt == 1)
				{
					// if file cnt = 1, we're past the [URL] line. we don't know what this content will be, but need to skip it.
					{
						meatloaf_info_file_cnt = 2; 
						skip_this_file = true;
					}
				}
				else if (meatloaf_info_file_cnt == 2)
				{
					if (General_Strncasecmp((char*)dirent->d_name, "[PATH]", 6) == 0)
					{
						meatloaf_info_file_cnt = 3;
						skip_this_file = true;
					}
					else if (General_Strncasecmp((char*)dirent->d_name, "------", 6) == 0)
					{
						// if no path, the path and line under it are skipped, and it goes directly to the ----- line.
						meatloaf_info_file_cnt = 5;
						skip_this_file = true;
					}
				}
				else if (meatloaf_info_file_cnt == 3)
				{
					// if file cnt = 3, we're past the [PATH] line. we want to skip this line, but first test it, because it can tell us if we are at root or not
					// if we're at root, we canNOT inject ".." because there will be no where to go. 
					// in MEATLOAF, a root will look like "/MEATLOAF/" and a non-root might look like "/DEMO/APPS/". so, if 3 or more /s, we're saying it's not root.

					//sprintf(global_string_buff1, "file '%s' believed to be meatloaf info line #%u", dirent->d_name, meatloaf_info_file_cnt);
					//Buffer_NewMessage(global_string_buff1);
					
					this_file_name = dirent->d_name;
					filename_len = strlen(this_file_name);
					
					for (i = 0; i < filename_len; i++)
					{
						if (this_file_name[i] == '/')
						{
							meatloaf_slash_cnt++;
						}
					}
					
					//sprintf(global_string_buff1, "file '%s', info line #%u, slash cnt=%u", dirent->d_name, meatloaf_info_file_cnt, meatloaf_slash_cnt);
					//Buffer_NewMessage(global_string_buff1);
				
					// if this is NOT the top level / root dir, insert a fake file to represent the '..' parent directory folder.
					if (meatloaf_slash_cnt > 1)
					{
						// insert a fake file to represent the '..' parent directory folder	
						this_file = File_New(the_panel_id, "..", PARAM_FILE_IS_FOLDER, 0, _CBM_T_DIR, file_cnt, &this_datetime);
				
						if (this_file == NULL)
						{
							goto error;
						}
				
						// Add this file to the list of files
						file_added = Folder_AddNewFile(the_folder, this_file);
						++file_cnt;						
					}
					
					meatloaf_info_file_cnt = 4;
					skip_this_file = true;
				}
				else if (meatloaf_info_file_cnt == 4)
				{
					// if file cnt = 4, we're past the line after [PATH], and looking at the '--------' line: need to skip it.
					{
						meatloaf_info_file_cnt = 5;
						skip_this_file = true;
					}
				}
			}
// 			else
// 			{
// 				if (General_Strncasecmp((char*)dirent->d_name, "SD", 2) == 0)
// 				{
// 					// this isn't one of the info files we were checking, but we DO want to skip it. it's a 0 byte file telling you you have an SD card attached apparently. (to meatloaf)
// 					skip_this_file = true;
// 				}
// // 				else
// // 				{
// // 					sprintf(global_string_buff1, "meatloaf mode info cnt=%u but file '%s' not regular??", meatloaf_info_file_cnt, dirent->d_name);
// // 					Buffer_NewMessage(global_string_buff1);
// // 				}
// 			}
		}
	
		if (skip_this_file == false)
		{
			if (_DE_ISDIR(dirent->d_type))
			{
				//sprintf(global_string_buff1, "file '%s' identified as folder by _DE_ISDIR", dirent->d_name);
				//Buffer_NewMessage(global_string_buff1);
				
				this_file_name = dirent->d_name;
				
				if (this_file_name[0] == '.' && this_file_name[1] != '.')
				{
					// this is a dir starting with '.' probably macOS junk, OR
					// this is a the current dir ".", and we still don't need to see it.
	
				}
				else
				{				
					//sprintf(global_string_buff1, "file '%s' detected as dir, setting path to '%s'", this_file_name, global_temp_path_2);
					//Buffer_NewMessage(global_string_buff1);
				
					this_file = File_New(the_panel_id, this_file_name, PARAM_FILE_IS_FOLDER, 0, _CBM_T_DIR, file_cnt, &this_datetime);
		
					if (this_file == NULL)
					{
						goto error;
					}
		
					// Add this file to the list of files
					file_added = Folder_AddNewFile(the_folder, this_file);
		
					// if this is first file in scan, preselect it
					if (file_cnt == 0)
					{
						this_file->selected_ = true;
					}
			
					++file_cnt;
					
					//sprintf(global_string_buff1, "file '%s' identified as folder by _DE_ISDIR, added=%u", dirent->d_name, file_added);
					//Buffer_NewMessage(global_string_buff1);
				}
			}
			else if (_DE_ISLBL(dirent->d_type))
			{
				//sprintf(global_string_buff1, "%s %d: file '%s' identified by _DE_ISLBL", __func__ , __LINE__, dirent->d_name);
				//Buffer_NewMessage(global_string_buff1);

				this_file_name = dirent->d_name;
	
				if (this_file_name[0] == '0' && this_file_name[1] == ':')
				{
					// this is the internal SD card. give a more user-friendly name
					the_folder->file_name_ = General_StrlcpyWithAlloc(General_GetString(ID_STR_DEV_SD_CARD), FILE_MAX_FILENAME_SIZE);
				}
				else if (this_file_name[0] == NO_DISK_PRESENT_FILE_NAME || this_file_name[0] == NO_DISK_PRESENT_ANYMORE_FILE_NAME)
				{
					sprintf(global_string_buff1, General_GetString(ID_STR_ERROR_NO_DISK), the_folder->device_number_);
					Buffer_NewMessage(global_string_buff1);
					the_error_code = ERROR_COULD_NOT_OPEN_DIR;
					break;
				}
				else
				{
					// check for presence of "MEATLOAF" in the file name, and if found, set this folder to meatloaf mode.
					
					if ( this_file_name[0] == ' ' && this_file_name[1] == ' ')
					{
						this_file_name += 2;	// skip past the 2 spaces in "  MEATLOAF" sub dirs. annoying!
						
						if (General_Strncasecmp(this_file_name, General_GetString(ID_STR_LBL_MEATLOAF_DIR_NAME), 8) == 0)
						{
							the_folder->is_meatloaf_ = true;
							General_Strlcpy(this_file_name + 8, General_GetString(ID_STR_LBL_MEATLOAF_LOCAL_MODIFIER), 9);
						}
					}
					else if (General_Strncasecmp(this_file_name, General_GetString(ID_STR_LBL_MEATLOAF_DIR_NAME), 8) == 0)
					{
						the_folder->is_meatloaf_ = true;
					}
	
					the_folder->file_name_ = General_StrlcpyWithAlloc(this_file_name, FILE_MAX_FILENAME_SIZE);
				}
				
				//DEBUG_OUT(("%s %d: file '%s' identified by _DE_ISLBL", __func__ , __LINE__, dirent->d_name));
			}
			else if (_DE_ISREG(dirent->d_type))
			{
				//DEBUG_OUT(("%s %d: file '%s' identified by _DE_ISREG", __func__ , __LINE__, dirent->d_name));
				
				this_file_name = dirent->d_name;
			
				if (this_file_name[0] == '.')
				{
					// this is a file starting with '.'. probably macOS junk. don't need to see it.
	
				}
				else
				{
		
	// 				this_datetime.year = dirent->year;
	// 				this_datetime.month = dirent->month;
	// 				this_datetime.day = dirent->day;
	// 				this_datetime.hour = dirent->hour;
	// 				this_datetime.min = dirent->min;
	// 				this_datetime.sec = dirent->sec;
					
					// LOGIC:
					//   normally, anything with ISREG is a regular file, ie, not a directory
					//   however, with MEATLOAF, the "files" can be "folder" (links). 
					//   we are using assumption that any "file" with 1 or 0 blocks is actually a directory
					
					if (the_folder->is_meatloaf_ == true && the_block_size < 2)
					{
						// treat as directory. meatloaf will do the right thing when it is "loaded"
						
						//sprintf(global_string_buff1, "file '%s' detected as file but treating as meatloaf dir with path to '%s'", this_file_name, global_temp_path_2);
						//Buffer_NewMessage(global_string_buff1);
					
						this_file = File_New(the_panel_id, this_file_name, PARAM_FILE_IS_FOLDER, 0, _CBM_T_DIR, file_cnt, &this_datetime);
			
						if (this_file == NULL)
						{
							goto error;
						}		
					}
					else
					{
						calc_file_size = (uint32_t)the_block_size * (uint32_t)dirent->d_blocks;
						this_file = File_New(the_panel_id, this_file_name, PARAM_FILE_IS_NOT_FOLDER, calc_file_size, _CBM_T_REG, file_cnt, &this_datetime);
			
						if (this_file == NULL)
						{
							goto error;
						}
					}
					
		
					// Add this file to the list of files
					file_added = Folder_AddNewFile(the_folder, this_file);
		
					// if this is first file in scan, preselect it
					if (file_cnt == 0)
					{
						this_file->selected_ = true;
					}
			
					++file_cnt;
					
					//DEBUG_OUT(("%s %d: file '%s' identified by _DE_ISREG", __func__ , __LINE__, dirent->d_name));
					//sprintf(global_string_buff1, "file '%s' datetime=%u-%u-%u %u:%u:%u", dirent->d_name, this_datetime.year, this_datetime.month, this_datetime.day, this_datetime.hour, this_datetime.min, this_datetime.sec);
					//Buffer_NewMessage(global_string_buff1);
					//sprintf(global_string_buff1, "file '%s' (%s) identified by _DE_ISREG", dirent->d_name, global_temp_path_2);
					//Buffer_NewMessage(global_string_buff1);
					//sprintf(global_string_buff1, "cnt=%u, new file='%s' ('%s')", file_cnt, App_GetFilenameFromEM(this_file->id_), this_file->file_path_);
					//Buffer_NewMessage(global_string_buff1);
				}
			}
		}
	}

	Kernel_CloseDir(dir);

	// insert a fake file to represent the "take me home" choice in MEATLOAF
	if (the_folder->is_meatloaf_ == true)
	{
		// insert a fake file to represent the '^' home directory folder	
		this_file = File_New(the_panel_id, "^", PARAM_FILE_IS_FOLDER, 0, _CBM_T_DIR, file_cnt, &this_datetime);

		if (this_file == NULL)
		{
			goto error;
		}

		// Add this file to the list of files
		file_added = Folder_AddNewFile(the_folder, this_file);
		++file_cnt;						
	}


	// set current row to first file, or -1
	the_folder->cur_row_ = (file_cnt > 0 ? 0 : -1);
	
	// debug
// 	List_Print(the_folder->list_, &File_Print);
// 	DEBUG_OUT(("%s %d: Total bytes %lu", __func__ , __LINE__, the_folder->total_bytes_));
// 	Folder_Print(the_folder);

	sprintf(global_string_buff1, General_GetString(ID_STR_N_FILES_FOUND), file_cnt);
	Buffer_NewMessage(global_string_buff1);
	
	return (the_error_code);

error:
	LOG_ERR(("%s %d: Could not allocate memory for file object", __func__ , __LINE__));
	the_error_code = ERROR_COULD_NOT_CREATE_NEW_FILE_OBJECT;

	if (dir)	Kernel_CloseDir(dir);
	
	return (the_error_code);
}


// copies the currently selected file
bool Folder_CopyCurrentFile(WB2KFolderObject* the_folder, WB2KFolderObject* the_target_folder)
{
	WB2KFileObject*		the_file;

	the_file = Folder_GetCurrentFile(the_folder);

	if (the_file == NULL)
	{
		return false;
	}
	
	return Folder_CopyFile(the_folder, the_file, the_target_folder);
}


// copies the passed file/folder. If a folder, it will create directory on the target volume if it doesn't already exist
bool Folder_CopyFile(WB2KFolderObject* the_folder, WB2KFileObject* the_file, WB2KFolderObject* the_target_folder)
{
	int32_t				bytes_copied;
	uint8_t				name_uniqueifier;
	uint8_t				name_len;
	uint8_t				tries = 0;
	uint8_t				max_tries = 100;
	//char				filename_buffer[(FILE_MAX_FILENAME_SIZE*2)] = "";	// allow 2x size of buffer so we can snip off first by just advancing pointer
	//char*				new_filename = filename_buffer;
	char*				the_target_folder_path;
	bool				success = false;
	WB2KList*			the_target_file_item;
	
	if (the_folder == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_COPY_FILE_SOURCE_FOLDER_WAS_NULL);	// crash early, crash often
	}

	if (the_target_folder == NULL)
	{
		LOG_ERR(("%s %d: param the_target_folder was null", __func__ , __LINE__));
		App_Exit(ERROR_COPY_FILE_TARGET_FOLDER_WAS_NULL);	// crash early, crash often
	}
	
	// LOGIC:
	//   if a file, call routine to copy bytes of file. then call again for the info file.
	//   if a folder:
	//     We only have a folder object for the current source folder, we do not have one for the target
	//     So every time we encounter a source folder, we have to generate the equivalent path for the target and store in FileMover
	//     Then check if the folder path exists on the target volume. Call makedir to create the folder path. then copy info file bytes. 
	//       NOTE: there is an assumption that the target folder is a real, existing path, so no need to create "up the chain". This assumption is based on how copy files works.
	//     This routine will not attempt to delete folders and their contents if they already exist, it will happily copy files into those folders. (more windows than mac)
	//   in either case, add the file and its info file (if any) to any open windows showing the target folder
	//     NOTE: in case of folder, we have to copy the info file before updating the target folder chain, or info file will be placed IN the folder, rather than next to it
	
	if (the_file->is_directory_)
	{
// 		// handle a folder...
// 
// 		BPTR 			the_dir_lock;
// 		BPTR 			the_new_dir_lock;
// //		FileMover_UpdateCurrentTargetFolderPath(App_GetFileMover(global_app), the_file->file_path_);
// 		the_target_folder_path = FileMover_GetCurrentTargetFolderPath(App_GetFileMover(global_app));
// 		
// 		// have FileMover build a new target folder path
// 		FileMover_UpdateCurrentTargetFolderPath(App_GetFileMover(global_app), the_file->file_path_);
// 		the_target_folder_path = FileMover_GetCurrentTargetFolderPath(App_GetFileMover(global_app));
// 				
// 		// try to get lock on the  directory, and if we can't, make a new directory
// 		if ( (the_dir_lock = Lock((STRPTR)the_target_folder_path, SHARED_LOCK)) == 0)
// 		{
// 			//DEBUG_OUT(("%s %d: not able to lock target folder '%s'; suggests it doesn't exist yet; will create", __func__ , __LINE__, the_target_folder_path));
// 			
// 			if ( (the_new_dir_lock = CreateDir((STRPTR)the_target_folder_path)) == 0)
// 			{
// 				LOG_ERR(("%s %d: not able to create target folder '%s'! This will cause subsequent copy actions to fail!", __func__ , __LINE__, the_target_folder_path));
// 				UnLock(the_dir_lock);
// 				return false;
// 			}
// 			
// 			DEBUG_OUT(("%s %d: created folder '%s'", __func__ , __LINE__, the_target_folder_path));
// 			UnLock(the_new_dir_lock);
// 	
// 			FileMover_IncrementProcessedFileCount(App_GetFileMover(global_app));
// 		}
// 		else
// 		{
// 			DEBUG_OUT(("%s %d: got a lock on folder '%s'; suggests it already exists", __func__ , __LINE__, the_target_folder_path));
// 		}
	}
	else
	{
		// handle a file...

		// update target file path without adding the source file's filename to it
// 		FileMover_UpdateCurrentTargetFolderPath(App_GetFileMover(global_app), the_folder->folder_file_->file_path_);
// 		the_target_folder_path = FileMover_GetCurrentTargetFolderPath(App_GetFileMover(global_app));
		
		// check if the new file path is the same as the old: would be the case in a 'duplicate this file' situation
		// if so, figure out a compliant name that is unique. in fact, don't compare to the file at all, compare to entire folder!
		strcpy(folder_temp_filename, App_GetFilenameFromEM(the_file));
		name_uniqueifier = 48; // start artificially high so it resets to 48. 
		
		while ( (the_target_file_item = Folder_FindListItemByFileName(the_target_folder, folder_temp_filename)) != NULL && tries < max_tries)
		{		
			// there is a file in this folder with the same name. 
			// make name unique, then proceed with copy
			// we have limited filesize to work with. if under limit, add '1'. if at limit, remove right-most character?
			
			name_len = strlen(folder_temp_filename);
			
			if (name_len < (FILE_MAX_FILENAME_SIZE-1) && name_uniqueifier > 57)
			{
				name_uniqueifier = 48; // ascii 48, a 0 char. 
				folder_temp_filename[name_len] = name_uniqueifier;
				folder_temp_filename[name_len+1] = '\0';
			}
			else if (name_uniqueifier > 57)
			{
				// name is already at max, and we have cycled through digits (or haven't started yet)
				// snip off leading char and try again
				++folder_temp_filename;
				name_uniqueifier = 48; // ascii 48, a 0 char. 
				folder_temp_filename[name_len] = name_uniqueifier;
				folder_temp_filename[name_len+1] = '\0';
			}
			else
			{
				// we are somewhere 1-9, replace last digit
				folder_temp_filename[name_len-1] = name_uniqueifier;
				++name_uniqueifier;
			}
			
			++tries;
		}

		//sprintf(global_string_buff1, "new='%s', tries=%u", folder_temp_filename, tries);
		//Buffer_NewMessage(global_string_buff1);
		
		if (the_target_file_item != NULL)
		{
			// couldn't get a unique name
			//Buffer_NewMessage("couldn't make unique name");
			return false;
		}
		
		// build a file path for target file, based on FileMover's current target folder path and source file name
		the_target_folder_path = the_target_folder->file_path_;
		General_CreateFilePathFromFolderAndFile(global_temp_path_1, the_folder->file_path_, App_GetFilenameFromEM(the_file));
		General_CreateFilePathFromFolderAndFile(global_temp_path_2, the_target_folder_path, folder_temp_filename);
		
		//sprintf(global_string_buff1, "copy file src path='%s', tgt path='%s', size=%lu", global_temp_path_1, global_temp_path_2, the_file->size_);
		//Buffer_NewMessage(global_string_buff1);

		// call function to copy file bits
		//DEBUG_OUT(("%s %d: copying file '%s' to '%s'...", __func__ , __LINE__, App_GetFilenameFromEM(the_file), global_temp_path_2));
		//Buffer_NewMessage(General_GetString(ID_STR_MSG_COPYING));
		
		bytes_copied = Folder_CopyFileBytes(global_temp_path_1, global_temp_path_2, the_file->size_);
		
		if (bytes_copied < 0)
		{
			return false;
		}
	}	
	
	// mark the file as not selected 
	//File_SetSelected(the_file, false);

	// add a copy of the file to this target folder
	success = Folder_AddNewFileAsCopy(the_target_folder, the_file);
	//Buffer_NewMessage("added copy of file object to target folder");
			
	return success;
}


// // deletes the passed file/folder. If a folder, it must have been previously emptied of files.
// bool Folder_DeleteFile(WB2KFolderObject* the_folder, WB2KList* the_item, WB2KFolderObject* not_needed)
// {
// 	WB2KFileObject*		the_file;
// 	//bool				result_doesnt_matter;
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 
// 	the_file = (WB2KFileObject*)the_item->payload_;
// 
// 	General_CreateFilePathFromFolderAndFile(global_temp_path_1, the_folder->file_path_, App_GetFilenameFromEM(the_file));
// 	
// // 	FileMover_SetCurrentFileName(App_GetFileMover(global_app), App_GetFilenameFromEM(the_file));
// 	
// 	// delete the files
// 	if (File_Delete(global_temp_path_1, the_file->is_directory_) == false)
// 	{
// 		return false;
// 	}
// 
// // 	FileMover_IncrementProcessedFileCount(App_GetFileMover(global_app));
// 
// 	LOG_INFO(("%s %d: deleted file '%s' from disk", __func__ , __LINE__, App_GetFilenameFromEM(the_file)));
// 
// // 	// if this was a folder file, check if any open windows were representing its contents, and close them. 
// // 	if (the_file->is_directory_)
// // 	{
// // 		WB2KList*			the_window_item;
// // 	
// // 		while ((the_window_item = App_FindSurfaceListItemByFilePath(global_app, the_file->file_path_)) != NULL)
// // 		{
// // 			// a window was open with this volume / file as its root folder. close the window
// // 			App_CloseOneWindow(global_app, the_window_item);
// // 		}
// // 	}
// // 
// 	// update the count of files and remove this item from the folder's list of files
// 	Folder_RemoveFileListItem(the_folder, the_item, DESTROY_FILE_OBJECT);
// 	
// 	return true;
// }


// // removes the passed list item from the list of files in the folder. Does NOT delete file from disk. Optionally frees the file object.
// void Folder_RemoveFileListItem(WB2KFolderObject* the_folder, WB2KList* the_item, bool destroy_the_file_object)
// {
// 	WB2KFileObject*		the_file;
// 	uint32_t			bytes_removed = 0;
// 	uint16_t			blocks_removed = 0;
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 
// 	the_file = (WB2KFileObject*)(the_item->payload_);
// 	
// 	// before removing, count up the bytes for the file and it's info file, if any. 
// // 	bytes_removed = the_file->size_;
// // 	blocks_removed = the_file->num_blocks_;
// 	
// 	//DEBUG_OUT(("%s %d: file '%s' is being removed from folder '%s' (current bytes=%lu, bytes being removed=%lu)", __func__ , __LINE__, App_GetFilenameFromEM(the_file), the_folder->folder_file_->file_name_, the_folder->total_bytes_, bytes_removed));
// 	
// 	if (destroy_the_file_object)
// 	{
// 		File_Destroy(&the_file);
// 	}
// 	
// 	--the_folder->file_count_;
// // 	the_folder->total_bytes_ -= bytes_removed;
// // 	the_folder->total_blocks_ -= blocks_removed;
// 	List_RemoveItem(the_folder->list_, the_item);
// 	LOG_ALLOC(("%s %d:	__FREE__	the_item	%p	size	%i", __func__ , __LINE__, the_item, sizeof(WB2KList)));
// 	free(the_item);
// 	the_item = NULL;
// 	
// 	return;
// }


// // removes the passed list item from the list of files in the folder. Does NOT delete file from disk. Does NOT delete the file object.
// // returns true if a matching file was found and successfully removed.
// // NOTE: this is part of series of functions designed to be called by Window_ModifyOpenFolders(), and all need to return bools.
// bool Folder_RemoveFile(WB2KFolderObject* the_folder, WB2KFileObject* the_file)
// {
// 	WB2KList*		the_item;
// 	
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 
// 	the_item = Folder_FindListItemByFileName(the_folder, App_GetFilenameFromEM(the_file));
// 	
// 	if (the_item == NULL)
// 	{
// 		// just means this folder never contained a version of this file
// 		return false;
// 	}
// 	
// 	Folder_RemoveFileListItem(the_folder, the_item, DO_NOT_DESTROY_FILE_OBJECT);
// 	
// 	//DEBUG_OUT(("%s %d: file '%s' was removed from folder '%s'", __func__ , __LINE__, App_GetFilenameFromEM(the_file), the_folder->folder_file_->file_name_));
// 	
// 	return true;
// }


// // Create a new folder on disk, and a new file object for it, and assign it to this folder. 
// // if try_until_successful is set, will rename automatically with trailing number until it can make a new folder (by avoiding already-used names)
// bool Folder_CreateNewFolder(WB2KFolderObject* the_folder, char* the_file_name, bool try_until_successful)
// {
// 	WB2KFileObject*		the_file;
// 	bool				created_file_ok = false;
// 	BPTR 				the_dir_lock;
// 	BPTR 				the_new_dir_lock;
// 	char		the_path_buffer[FILE_MAX_PATHNAME_SIZE] = "";
// 	char*		the_target_folder_path = the_path_buffer;
// 	char		the_filename_buffer[FILE_MAX_FILENAME_SIZE] = "";
// 	char*		the_target_file_name = the_filename_buffer;
// 	uint16_t		next_filename_count = 2;	// "unnamed folder 2", "unnamed folder 3", etc. 
// 	struct DiskObject*	the_disk_object;
// 	struct DateStamp*	the_datetime;
// 	
// 	// LOGIC:
// 	//   create a new directory on disk with the filename specified
// 	//     if try_until_successful is set, rename automatically with trailing number until success
// 	//     first try for a lock on the path; if lock succeeds, you know there is an existing file
// 	//     NO OVERCREATION!
// 	//   call Folder_AddNewFile() to add the WB2K file object to the folder
// 	//   mark the folder (file) as selected (expected behavior)
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 
// 		
// 	//DEBUG_OUT(("%s %d: before adding new folder, folder '%s' has %lu total bytes, %lu selected files", __func__ , __LINE__, the_folder->folder_file_->file_path_, the_folder->total_bytes_, Folder_GetCountSelectedFiles(the_folder)));
// 
// 	// copy the preferred filename into local storage
// 	General_Strlcpy(the_target_file_name, the_file_name, FILE_MAX_FILENAME_SIZE);
// 	
// 	// loop as many times as necessary until we confirm no file/folder exists at the specified path
// 	//   unless try_until_successful == false
// 	
// 	while (created_file_ok == false)
// 	{
// 		// build a file path for folder file, based on current (parent) folder file path and passed file name
// 		General_CreateFilePathFromFolderAndFile(the_target_folder_path, the_folder->folder_file_->file_path_, the_target_file_name);
// 	
// 		// try to get lock on the  directory to see if it's already in use
// 		if ( (the_dir_lock = Lock((STRPTR)the_target_folder_path, SHARED_LOCK)) == 0)
// 		{
// 			//DEBUG_OUT(("%s %d: not able to lock target folder '%s'; suggests it doesn't exist yet; will create", __func__ , __LINE__, the_target_folder_path));
// 			
// 			if ( (the_new_dir_lock = CreateDir((STRPTR)the_target_folder_path)) == 0)
// 			{
// 				LOG_ERR(("%s %d: not able to create target folder '%s'!", __func__ , __LINE__, the_target_folder_path));
// 				UnLock(the_dir_lock);
// 				return false;
// 			}
// 			
// 			//DEBUG_OUT(("%s %d: created folder '%s'", __func__ , __LINE__, the_target_folder_path));
// 			UnLock(the_new_dir_lock);
// 			
// 			created_file_ok = true;
// 		}
// 		else
// 		{
// 			//DEBUG_OUT(("%s %d: got a lock on folder '%s'; suggests it already exists", __func__ , __LINE__, the_target_folder_path));
// 			
// 			if (try_until_successful == false)
// 			{
// 				// give up at first fail; do not attempt to make unnamed folder 2, 3, etc.
// 				LOG_WARN(("%s %d: requested folder name was already taken while trying to create a new folder at '%s'", __func__ , __LINE__, the_target_folder_path));
// 				return false;
// 			}
// 			
// 			// add/change the number at end of folder name and try again
// 			sprintf((char*)the_target_file_name, "%s %u", the_file_name, next_filename_count);
// 		
// 			// check if we should abandon the effort
// 			if (next_filename_count > FOLDER_MAX_TRIES_AT_FOLDER_CREATION)
// 			{
// 				LOG_ERR(("%s %d: Reached maximum allowed folder names while trying to create a new folder at '%s'", __func__ , __LINE__, the_target_folder_path));
// 				return false;
// 			}
// 		}
// 		
// 		next_filename_count++;
// 	}
// 	
// 	// get timestamp we can use for the folder and the folder.info file
// 	// won't be exactly accurate necessarily, but is for display purposes. If we didn't make it, we'd have to get file lock and example both folder and .info file
// 	the_datetime = General_GetCurrentDateStampWithAlloc();
// 	
// 	// make WB2K file object for the folder that now exists on disk
// 	the_file = File_New(the_target_file_name, PARAM_FILE_IS_FOLDER, the_folder->icon_rport_, 0, *the_datetime, NULL);
// 
// 	LOG_ALLOC(("%s %d:	__ALLOC__	the_datetime	%p	size	%i", __func__ , __LINE__, the_datetime, sizeof(struct DateStamp)));
// 	free(the_datetime);
// 	the_datetime = NULL;
// 	
// 	if (the_file == NULL)
// 	{
// 		LOG_ERR(("%s %d: Could not allocate memory for file object", __func__ , __LINE__));
// 		return false;
// 	}
// 	
// 	// we want the file to be selected for the user
// 	File_SetSelected(the_file, true);
// 
// 	// Add this file to the list of files
// 	if ( Folder_AddNewFile(the_folder, the_file) == true && create_info_file == true)
// 	{
// 		// create info file and associate with the file
// 		
// 		// Create info file on disk too (one will not exist, but this function will have AmigaOS create one)
// 		// NOTE: do this before changing target folder path to point to the .info file
// 		the_disk_object = General_GetInfoStructFromPath(the_target_folder_path, WBDRAWER);
// 		PutDiskObject(the_target_folder_path, the_disk_object);
// 
// 		General_Strlcat(the_target_file_name, FILE_INFO_EXTENSION, FILE_MAX_PATHNAME_SIZE);
// 		General_Strlcat(the_target_folder_path, FILE_INFO_EXTENSION, FILE_MAX_PATHNAME_SIZE);
// 
// 		// TODO: do more robust/elegant solution for showing size of a default folder info file. user could have set their system up with a huge info file
// 
// 		the_info_file = InfoFile_New(the_target_file_name, NULL, FOLDER_UGLY_HACK_DEFAULT_FOLDER_INFO_FILE_SIZE);
// 
// 		if ( the_info_file == NULL)
// 		{
// 			LOG_ERR(("%s %d: Could not create an info file object for '%s'", __func__ , __LINE__, the_target_folder_path));
// 			File_Destroy(&the_file);
// 			return false;
// 		}
// 
// 		// assign the disk structure to the info file (whether we just created it, or it had been there all the time)
// 		the_file->info_file_->info_struct_ = the_disk_object;
// 
// 		// assign the info file to the file
// 		the_file->info_file_ = the_info_file;
// 
// 		// add the info file's size to the ancestor folder
// 		the_folder->total_bytes_ += the_file->info_file_->size_;
// 		
// 		//DEBUG_OUT(("%s %d: after adding new folder, folder '%s' has %lu total bytes, %lu selected files", __func__ , __LINE__, the_folder->folder_file_->file_path_, the_folder->total_bytes_, Folder_GetCountSelectedFiles(the_folder)));
// 	}
// 		
// 	return true;
// }

	
// Add a file object to the list of files without checking for duplicates.
// returns true in all cases. 
// NOTE: this is part of series of functions designed to be called by Window_ModifyOpenFolders(), and all need to return bools.
bool Folder_AddNewFile(WB2KFolderObject* the_folder, WB2KFileObject* the_file)
{
	WB2KList*	the_new_item;
// 	uint32_t	bytes_added = 0;
// 	uint16_t	blocks_added = 0;
	
	if (the_folder == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
	}

	// account for bytes of file and info file, if any
// 	bytes_added = the_file->size_;
// 	blocks_added = the_file->num_blocks_;
	
	the_new_item = List_NewItem((void *)the_file);
	List_AddItem(the_folder->list_, the_new_item);
	the_folder->file_count_++;
// 	the_folder->total_bytes_ += bytes_added;
// 	the_folder->total_blocks_ += blocks_added;
	
	//DEBUG_OUT(("%s %d: file '%s' was added to folder '%s'", __func__ , __LINE__, App_GetFilenameFromEM(the_file), the_folder->folder_file_->file_name_));
	//DEBUG_OUT(("%s %d: file '%s' was added to folder folder '%s' (current bytes=%lu, bytes being added=%lu)", __func__ , __LINE__, App_GetFilenameFromEM(the_file), the_folder->folder_file_->file_name_, the_folder->total_bytes_, bytes_added));
	
	return true;
}
	
	
// Add a file object to the list of files without checking for duplicates. This variant makes a copy of the file before assigning it. Use case: MoveFiles or CopyFiles.
// returns true in all cases. 
// NOTE: this is part of series of functions designed to be called by Window_ModifyOpenFolders(), and all need to return bools.
bool Folder_AddNewFileAsCopy(WB2KFolderObject* the_folder, WB2KFileObject* the_file)
{
	WB2KFileObject*		the_copy_of_file;
	
	if (the_folder == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
	}

	if ( (the_copy_of_file = File_Duplicate(the_file)) == NULL)
	{
		LOG_ERR(("%s %d: Couldn't get a duplicate of the file object", __func__ , __LINE__));
		App_Exit(ERROR_DEFINE_ME); // crash early, crash often
		return false;
	}

	return Folder_AddNewFile(the_folder, the_copy_of_file);
}


// // compare 2 folder objects. When done, the original_root_folder will have been updated with removals/additions as necessary to match the updated file list
// // if the folder passed is a system root object, and if a folder (disk) has been removed from it, then any windows open from that disk will be closed
// // returns true if any changes were detected, or false if files appear to be identical
// bool Folder_SyncFolderContentsByFilePath(WB2KFolderObject* original_root_folder, WB2KFolderObject* updated_root_folder)
// {
// 	bool				changes_made = false;
// 	WB2KList**			original_files_list;
// 	WB2KList**			updated_files_list;
// 	WB2KList*			the_original_list_item;
// 	WB2KList*			the_updated_list_item;
// 	WB2KList*			the_item_to_be_deleted;
// 	
// 	// LOGIC for folder compare:
// 	//   we have a list of files from an original version of a folder
// 	//   we have a list of files from a (potentially) updated version of a folder
// 
// 	//   iterate through original file list, comparing each to new file list
// 	//   for any filename match, remove the new item from the new list
// 	//   for a with no filename match, it means it was ejected. 
// 	//   for a file with no filename match, add to the "removed files" list
// 	
// 	// LOGIC for per-file compare:
// 	//   compare name and date of local file to remote descriptor
// 	//   for anything NEW, add a file-request to the queue
// 	//   for anything newer, add a file-request to the queue
// 	//   don't do anything for not-found-locally, or same-as-local
// 	
// 	if (original_root_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: param original_root_folder was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 
// 	if (updated_root_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: param updated_root_folder was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 
// 	original_files_list = Folder_GetFileList(original_root_folder);
// 	updated_files_list = Folder_GetFileList(updated_root_folder);
// 	
// 	// DEBUG
// 	//DEBUG_OUT(("%s %d: orig files before starting:", __func__ , __LINE__));
// 	//List_Print(original_files_list, &File_Print);
// 	//DEBUG_OUT(("%s %d: updated files before starting:", __func__ , __LINE__));
// 	//List_Print(updated_files_list, &File_Print);
// 	
// 	// iterate through original file list, comparing to everything in the new files list
// 	the_original_list_item = *(original_files_list);
// 
// 	while (the_original_list_item != NULL)
// 	{		
// 		WB2KFileObject* 	the_original_list_file = (WB2KFileObject*)(the_original_list_item->payload_);
// 		short				the_compare_len = strlen((char*)the_original_list_file->file_path_);
// 		
// 		the_updated_list_item = Folder_FindListItemByFilePath(updated_root_folder, the_original_list_file->file_path_, the_compare_len);
// 	
// 		if (the_updated_list_item == NULL)
// 		{
// 			// this file only exists in the original files list. it has been removed/is unavailable
// 			//DEBUG_OUT(("%s %d: orig file '%s' not found in updated list", __func__ , __LINE__, the_original_list_file->file_path_));
// 			the_item_to_be_deleted = the_original_list_item;
// 			changes_made = true;
// 		}
// 		else
// 		{
// 			// this file was in original listing, and in new listing. remove from updated list.
// 			//DEBUG_OUT(("%s %d: orig file '%s' found in both lists", __func__ , __LINE__, the_original_list_file->file_path_));
// 			the_item_to_be_deleted = NULL;
// 			Folder_RemoveFileListItem(updated_root_folder, the_updated_list_item, DESTROY_FILE_OBJECT);			
// 		}		
// 		
// 		the_original_list_item = the_original_list_item->next_item_;
// 		
// 		if (the_item_to_be_deleted)
// 		{
// 			Folder_RemoveFileListItem(original_root_folder, the_item_to_be_deleted, DESTROY_FILE_OBJECT); // do after getting next item in list
// 		}
// 	}
// 
// 	// LOGIC
// 	//   we have checked all the original files against the updated ones.
// 	//   anything still left in the updated list can be understood to have been recently added (eg, an inserted disk)
// 	//   we want to transfer these items to the original folder
// 	//   we do NOT want to remove/destroy them from the updated folder, because the payloads are shared. 
// 
// 	// DEBUG
// 	//DEBUG_OUT(("%s %d: orig files after first pass:", __func__ , __LINE__));
// 	//List_Print(original_files_list, &File_Print);
// 	//DEBUG_OUT(("%s %d: updated files after first pass:", __func__ , __LINE__));
// 	//List_Print(updated_files_list, &File_Print);
// 	
// 	the_updated_list_item = *(updated_files_list);
// 
// 	while (the_updated_list_item != NULL)
// 	{
// 		WB2KFileObject* 	the_updated_list_file = (WB2KFileObject*)(the_updated_list_item->payload_);
// 		bool				file_added;
// 	
// 		//DEBUG_OUT(("%s %d: updated file '%s' had no equivalent; adding to original folder", __func__ , __LINE__, the_updated_list_file->file_path_));
// 
// 		// Add this file to the list of files
// 		file_added = Folder_AddNewFile(original_root_folder, the_updated_list_file);
// 		
// 		// remove this file from the updated folder object so we can destroy the folder safely later
// 		the_item_to_be_deleted = the_updated_list_item;
// 		the_updated_list_item = the_updated_list_item->next_item_;
// 		List_RemoveItem(updated_files_list, the_item_to_be_deleted); // do after getting next item in list
// 		LOG_ALLOC(("%s %d:	__FREE__	the_item_to_be_deleted	%p	size	%i", __func__ , __LINE__, the_item_to_be_deleted, sizeof(WB2KList)));
// 		free(the_item_to_be_deleted);
// 		the_item_to_be_deleted = NULL;
// 		
// 		changes_made = true;
// 	}
// 
// 	// DEBUG
// 	//DEBUG_OUT(("%s %d: orig files after 2nd pass:", __func__ , __LINE__));
// 	//List_Print(original_files_list, &File_Print);
// 	//DEBUG_OUT(("%s %d: updated files after 2nd pass:", __func__ , __LINE__));
// 	//List_Print(updated_files_list, &File_Print);
// 
// 	return changes_made;
// }


// // counts the bytes in the passed file/folder, and adds them to folder.selected_bytes_
// bool Folder_CountBytes(WB2KFolderObject* the_folder, WB2KList* the_item, WB2KFolderObject* not_needed)
// {
// 	WB2KFileObject*		the_file;
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 
// 	the_file = (WB2KFileObject*)the_item->payload_;
// 	
// // NOTE Jan 14, 2023: need to look into this. probably needs total redesign. unsure it's even needed though. maybe for FAT32 on f256.
// // 	FileMover_AddToSelectedCount(App_GetFileMover(global_app), the_file->size_);
// 
// 	//DEBUG_OUT(("%s %d: counted file '%s' in '%s'; selected bytes now = %lu", __func__ , __LINE__, App_GetFilenameFromEM(the_file),  the_folder->folder_file_->file_name_, FileMover_GetSelectedByteCount()));
// 	
// 	return true;
// }


// // processes, with recursion where necessary, the contents of a folder, using the passed function pointer to process individual files/empty folders.
// // returns -1 in event of error, or count of files affected
// int Folder_ProcessContents(WB2KFolderObject* the_folder, WB2KFolderObject* the_target_folder, uint8_t the_scope, bool do_folder_before_children, bool (* action_function)(WB2KFolderObject*, WB2KList*, WB2KFolderObject*))
// {
// 	// LOGIC:
// 	//   iterate through all files in the folder's list
// 	//   for each file that matches the passed scope:
// 	//     if the "file" is a folder, then recurse by calling this function again in order to process the children of that folder
// 	//     if the file is a file, then call the action_function passed
// 	//   note: scope is only allowed to be set at the first call, not in recursions. all recursion calls from this function will be passed LIST_SCOPE_ALL
// 
// 	int			num_files = 0;
// 	int			result;
// 	WB2KList*	the_item;
// 	WB2KList*	next_item;
// 	uint8_t		the_error_code;
// 	
// 	// sanity checks
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed source folder was NULL", __func__ , __LINE__));
// 		return -1;
// 	}
// 
// 	the_item = *(the_folder->list_);
// 
// 	if (the_item == NULL)
// 	{
// 		return false;
// 	}
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject*		this_file;
// 		
// 		next_item = the_item->next_item_; // capture this early, because if the action function is a delete, we may be removing this list item very shortly
// 
// 		this_file = (WB2KFileObject*)(the_item->payload_);
// 		//DEBUG_OUT(("%s %d: looking at file '%s'", __func__ , __LINE__, App_GetFilenameFromEM(this_file->id_)));
// 
// 		if (the_scope == LIST_SCOPE_ALL || (the_scope == LIST_SCOPE_SELECTED && File_IsSelected(this_file)) || (the_scope == LIST_SCOPE_NOT_SELECTED && !File_IsSelected(this_file)))
// 		{
// 			// if this is a folder, get a folder object for it, and recurse if not empty
// 			if (this_file->is_directory_)
// 			{
// 				WB2KFolderObject*	the_sub_folder;
// 
// 				// LOGIC:
// 				//   For a folder, there are 2 options in this function
// 				//   1. Process the folder file with the action before processing it's children OR
// 				//   2. Process the folder file with the action AFTER processing the children
// 				//   Typically, a copy type operation will need the folder to get actioned first (so that target folder structure gets created)
// 				//   A delete action would need the inverse: delete the children, then the folder. 
// 				//   For some other actions, it won't matter (eg, count bytes)
// 				
// 				if (do_folder_before_children)
// 				{
// 					//DEBUG_OUT(("%s %d: Executing helper function on folder file '%s' before processing children", __func__ , __LINE__, App_GetFilenameFromEM(this_file->id_)));
// 					
// 					if ((*action_function)(the_folder, the_item, the_target_folder) == false)
// 					{
// 						DEBUG_OUT(("%s %d: Error executing helper function on folder file '%s'", __func__ , __LINE__, App_GetFilenameFromEM(this_file->id_)));
// 						goto error;
// 					}
// 				}
// 				
// 				if ( (the_sub_folder = Folder_New(this_file, PARAM_MAKE_COPY_OF_FOLDER_FILE) ) == NULL)
// 				{
// 					// couldn't get a folder object. probably should be returning some kind of error condition. TODO
// 					LOG_ERR(("%s %d:  couldn't get a folder object for '%s'", __func__ , __LINE__, App_GetFilenameFromEM(this_file->id_)));
// 					goto error;
// 				}
// 				else
// 				{
// 					// LOGIC: if folder is empty, we can process it as a file. otherwise, recurse
// 					// 2021/06/03: this will never be true, because Folder_New doesn't call populate! all HasChildren does is look at file_count_ as 0 or not 0. 
// 					
// 					// have root folder populate its list of files
// 					if ( (the_error_code = Folder_PopulateFiles(the_sub_folder)) > ERROR_NO_ERROR)
// 					{
// 						LOG_ERR(("%s %d: folder '%s' reported that file population failed with error %u", __func__ , __LINE__, the_sub_folder->folder_file_->file_name_, the_error_code));
// 						goto error;
// 					}
// 
// 					
// 					if (Folder_HasChildren(the_sub_folder))
// 					{
// 						DEBUG_OUT(("%s %d: folder '%s' has 1 or more children", __func__ , __LINE__, the_sub_folder->folder_file_->file_name_));
// 						result = Folder_ProcessContents(the_sub_folder, the_target_folder, LIST_SCOPE_ALL, do_folder_before_children, action_function);
// 
// 						if (result == -1)
// 						{
// 							// error condition
// 							LOG_ERR(("%s %d: Folder_ProcessContents failed on folder '%s'", __func__ , __LINE__, the_sub_folder->folder_file_->file_name_));
// 							goto error;
// 						}
// 
// 						num_files += result;
// 					}
// 					else
// 					{
// 						DEBUG_OUT(("%s %d: folder '%s' has no children", __func__ , __LINE__, App_GetFilenameFromEM(this_file->id_)));
// 					}
// 
// 					if (do_folder_before_children == false)
// 					{
// 						//DEBUG_OUT(("%s %d: Executing helper function on folder file '%s' after processing children", __func__ , __LINE__, App_GetFilenameFromEM(this_file->id_)));
// 					
// 						if ((*action_function)(the_folder, the_item, the_target_folder) == false)
// 						{
// 							DEBUG_OUT(("%s %d: Error executing helper function on folder file '%s'", __func__ , __LINE__, App_GetFilenameFromEM(this_file->id_)));
// 							goto error;
// 						}
// 					}
// 
// 					++num_files;
// 				}
// 
// 				Folder_Destroy(&the_sub_folder);
// 			}
// 			else
// 			{
// 				// call the action function
// 				if ((*action_function)(the_folder, the_item, the_target_folder) == false)
// 				{
// 					goto error;
// 				}
// 
// 				++num_files;
// 			}
// 		}
// 
// 		the_item = next_item;
// 	}
// 
// 	return num_files;
// 
// error:
// 	return -1;
// }


// move every currently selected file into the specified folder. Use when you DO have a folder object to work with
// returns -1 in event of error, or count of files moved
// //   NOTE: calling function must have already checked that folders are on the same device!
// int Folder_MoveSelectedFiles(WB2KFolderObject* the_folder, WB2KFolderObject* the_target_folder)
// {
// 	// LOGIC:
// 	//   iterate through all files in the source folder's list
// 	//   for any file that is listed as selected, move it (via rename) to the specified target folder
// 
// 	int				num_files = 0;
// 	WB2KList*		the_item;
// 	WB2KList*		temp_item;
// 
// 	if (the_folder == NULL || the_target_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: the source and/or target folder was NULL", __func__ , __LINE__));
// 		goto error;
// 	}
// 
// 	the_item = *(the_folder->list_);
// 
// 	if (the_item == NULL)
// 	{
// 		return -1;
// 	}
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);
// 
// 		if (File_IsSelected(this_file) == true)
// 		{
// 			WB2KFileObject*		same_named_file_in_target;
// 			char				target_file_path_buffer[FILE_MAX_PATHNAME_SIZE];
// 			char*				target_file_path = target_file_path_buffer;
// 
// 
// 			// check that files are not being moved/copied into a sub-folder
// 			
// 			// LOGIC:
// 			//   If source folder path is found, in its entirety, in the target path, we must block the copy/move
// 			//   We only do this check with the target file is a directory
// 			//   We have to ensure the source path ends with : or /, or it can find a partial name. (eg, RAM:folder would show as bad match for RAM:folder1/some folder)
// 			//   We have to compare with whatever the shortest of the 2 paths is
// 			//     case A: "RAM:AcmeTest/" > "RAM:AcmeTest/lvl1/" (into a subfolder)
// 			//     case B: "RAM:AcmeTest/" > "RAM:AcmeTest/" (into itself)
// 			
// 			if (this_file->is_directory_)
// 			{
// 				int32_t			src_file_path_len;
// 				int32_t			tgt_folder_path_len;
// 				int32_t			shortest_path;
// 				char			temp_source_path[FILE_MAX_PATHNAME_SIZE];
// 				char*			source_path_for_compare = temp_source_path;
// 
// 				// prep source file path for comparison
// 			
// 				src_file_path_len = strlen((char*)this_file->file_path_);
// 			
// 				General_Strlcpy(source_path_for_compare, this_file->file_path_, FILE_MAX_PATHNAME_SIZE);
// 
// 				if ( source_path_for_compare[src_file_path_len - 1] != ':')
// 				{
// 					General_Strlcat(source_path_for_compare, (char*)"/", FILE_MAX_PATHNAME_SIZE); // TODO: replace hard-coded
// 					src_file_path_len++;
// 				}
// 			
// 				// compare prepped source folder path to target folder path
// 
// 				tgt_folder_path_len = strlen((char*)the_target_folder->folder_file_->file_path_);
// 				shortest_path = src_file_path_len < tgt_folder_path_len ? src_file_path_len : tgt_folder_path_len;
// 			
// 				if (General_Strncasecmp(source_path_for_compare, the_target_folder->folder_file_->file_path_, shortest_path) == 0)
// 				{
// 					// TODO: implement this once I add back the text-based-dialog window functions
// // 					General_ShowAlert(App_GetBackdropSurface(global_app)->window_, MSG_StatusMovingFiles, ALERT_DIALOG_SHOW_AS_ERROR, ALERT_DIALOG_NO_CANCEL_BTN, (char*)MSG_StatusMovingFilesIntoChildError);
// 					DEBUG_OUT(("%s %d: **NOT safe to move folder** (%s > %s)", __func__ , __LINE__, this_file->file_path_, the_target_folder->folder_file_->file_path_));
// 					return -1;	
// 				}
// 				else
// 				{
// 					//DEBUG_OUT(("%s %d: (safe to move folder) (%s > %s) (%lu, %lu)", __func__ , __LINE__, this_file->file_path_, the_target_folder->folder_file_->file_path_, tgt_folder_path_len, src_file_path_len));
// 				}
// 			}
// 
// 
// 			// compare new path to existing paths in target dir to see if (same-named) file already exists there
// 
// 			General_CreateFilePathFromFolderAndFile(target_file_path, the_target_folder->folder_file_->file_path_, App_GetFilenameFromEM(this_file->id_));
// 			same_named_file_in_target = Folder_FindFileByFilePath(the_target_folder, target_file_path, strlen((char*)target_file_path));
// 
// 			if (same_named_file_in_target != NULL)
// 			{
// 				// do what, exactly? warn user one file at a time? With a dialogue box? fail/stop? ignore this file, and continue with the rest? 
// 				DEBUG_OUT(("%s %d: File '%s' already exists in the destination folder. This file will not be moved.", __func__ , __LINE__, App_GetFilenameFromEM(this_file->id_)));
// 			
// 				the_item = the_item->next_item_;
// 			}
// 			else
// 			{
// 				bool		result_doesnt_matter;
// 				
// 				if ( File_Rename(this_file, App_GetFilenameFromEM(this_file->id_), target_file_path) == false)
// 				{
// 					LOG_ERR(("%s %d: Move action failed with file '%s' -> '%s'", __func__ , __LINE__, this_file->file_path_, target_file_path));
// 					goto error;
// 				}
// 
// 				// mark the file as not selected in its new location
// 				File_SetSelected(this_file, false);
// 
// 				// add a copy of the file to any open panels match the target folder (but aren't it), then add the original to this target folder
// 				//DEBUG_OUT(("%s %d: Adding file '%s' to any matching open windows/panels...", __func__ , __LINE__, App_GetFilenameFromEM(this_file->id_)));
// 				// NOTE Jan 14, 2023: think about having something to handle when same disk is open in both panels. TODO
// // 				result_doesnt_matter = App_ModifyOpenFolders(global_app, the_target_folder, this_file, &Folder_AddNewFileAsCopy);
// 				result_doesnt_matter = Folder_AddNewFile(the_target_folder, this_file);
// 				
// 				++num_files;
// 				
// 				// remove file from any open panels match the source folder (but aren't it), then remove from this source folder
// 				//DEBUG_OUT(("%s %d: Removing file '%s' from any matching open windows/panels...", __func__ , __LINE__, App_GetFilenameFromEM(this_file->id_)));
// 				// NOTE Jan 14, 2023: think about having something to handle when same disk is open in both panels. TODO
// // 				result_doesnt_matter = App_ModifyOpenFolders(global_app, the_folder, this_file, &Folder_RemoveFile);
// 				temp_item = the_item->next_item_;
// 				Folder_RemoveFileListItem(the_folder, the_item, DO_NOT_DESTROY_FILE_OBJECT);
// 				the_item = temp_item;
// 			}
// 		}
// 		else
// 		{
// 			the_item = the_item->next_item_;
// 		}
// 	}
// 
// 	return num_files;
// 	
// error:
// 	return -1;
// }


// // move every currently selected file into the specified folder file. Use when you only have a target folder file, not a full folder object to work with.
// // returns -1 in event of error, or count of files moved
// int Folder_MoveSelectedFilesToFolderFile(WB2KFolderObject* the_folder, WB2KFileObject* the_target_folder_file)
// {
// 	// LOGIC:
// 	//   iterate through all files in the folder's list
// 	//   for any file that is listed as selected, move it (via rename) to the specified target folder
// 
// 	int				num_files = 0;
// 	char	target_file_path_buffer[FILE_MAX_PATHNAME_SIZE];
// 	char*	target_file_path = target_file_path_buffer;
// 	WB2KList*		the_item;
// 	WB2KList*		temp_item;
// 
// 	if (the_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
// 	}
// 
// 	if (the_target_folder_file == NULL)
// 	{
// 		LOG_ERR(("%s %d: the target folder file was NULL", __func__ , __LINE__));
// 		goto error;
// 	}
// 
// 	the_item = *(the_folder->list_);
// 
// 	if (the_item == NULL)
// 	{
// 		return false;
// 	}
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);
// 
// 		if (File_IsSelected(this_file) == true)
// 		{
// 			// move file
// 			General_CreateFilePathFromFolderAndFile(target_file_path, the_target_folder_file->file_path_, App_GetFilenameFromEM(this_file->id_));
// 
// 			if ( rename( this_file->file_path_, target_file_path ) == 0)
// 			{
// 				LOG_ERR(("%s %d: Move action failed with file '%s'", __func__ , __LINE__, App_GetFilenameFromEM(this_file->id_)));
// 				goto error;
// 			}
// 
// 			// mark the file as not selected (in its new location, it wouldn't be selected()
// 			// no point in doing this, as we are going to destroy this file object shortly anyway
// 			//File_SetSelected(this_file, false);
// 
// 			++num_files;
// 
// 			// remove file from the parent panel's list of files
// 			--the_folder->file_count_;
// 			temp_item = the_item->next_item_;
// 			File_Destroy(&this_file);
// 			List_RemoveItem(the_folder->list_, the_item);
// 			LOG_ALLOC(("%s %d:	__FREE__	the_item	%p	size	%i", __func__ , __LINE__, the_item, sizeof(WB2KList)));
// 			free(the_item);
// 			the_item = temp_item;
// 		}
// 		else
// 		{
// 			the_item = the_item->next_item_;
// 		}
// 	}
// 
// 	return num_files;
// 	
// error:
// 	return -1;
// }


// select or unselect 1 file by row id, and change cur_row_ accordingly
WB2KFileObject* Folder_SetFileSelectionByRow(WB2KFolderObject* the_folder, uint16_t the_row, bool do_selection, uint8_t y_offset)
{
	WB2KFileObject*		the_file;
	WB2KFileObject*		the_prev_selected_file;

	the_file = Folder_FindFileByRow(the_folder, the_row);
	
	if (the_file == NULL)
	{
		return NULL;
	}


	if (do_selection)
	{
		// is this already the currently selected file? do we need to unselect a different one? (only 1 allowed at a time)	
		if (the_folder->cur_row_ != the_row)
		{
			// something else was selected. find it, and mark it unselected. 
			the_prev_selected_file = Folder_FindFileByRow(the_folder, the_folder->cur_row_);

			if (the_prev_selected_file == NULL)
			{
			}
			else
			{
				if (File_MarkUnSelected(the_prev_selected_file, y_offset) == false)
				{
					// the passed file was null. do anything?
					LOG_ERR(("%s %d: couldn't mark file '%s' as selected", __func__ , __LINE__, the_prev_selected_file->file_name_));
					App_Exit(ERROR_FILE_MARK_UNSELECTED_FILE_WAS_NULL);
				}
			}
		}

		the_folder->cur_row_ = the_row;

		if (File_MarkSelected(the_file, y_offset) == false)
		{
			// the passed file was null. do anything?
			LOG_ERR(("%s %d: couldn't mark file '%s' as selected", __func__ , __LINE__, App_GetFilenameFromEM(the_file)));
			App_Exit(ERROR_FILE_MARK_SELECTED_FILE_WAS_NULL);
		}
	}
	else
	{
		if (the_folder->cur_row_ == the_row)
		{
			// we unselected the current file. set current selection to none.
			the_folder->cur_row_ = -1;
		}

		if (File_MarkUnSelected(the_file, y_offset) == false)
		{
			// the passed file was null. do anything?
			LOG_ERR(("%s %d: couldn't mark file '%s' as selected", __func__ , __LINE__, App_GetFilenameFromEM(the_file)));
			App_Exit(ERROR_FILE_MARK_UNSELECTED_FILE_WAS_NULL);
		}
	}

	return the_file;
}	


// get a file handle for the target path, in "write" mode
// returns NULL on any error, including not being able to get a good handle
FILE* Folder_GetTargetHandleForWriting(const char* the_target_file_path)
{
	FILEmimic*  target_mimic;
	uint8_t		tries_made = 0;
	bool		good_tgt_handle = false;
	FILE*		the_target_handle;
		
	// Open target file for Writing
	// NOTE: this is in a loop because I was getting cases where the 2nd copy operation would get a bad handle. then 3rd would work, etc. 
	
	while (good_tgt_handle == false && tries_made < 6)
	{
		the_target_handle = fopen(the_target_file_path, "w");
	
		if (the_target_handle == NULL)
		{
			LOG_ERR(("%s %d: file '%s' could not be opened for writing", __func__ , __LINE__, the_target_file_path));
			return NULL;
		}

		target_mimic = (FILEmimic*)the_target_handle;
		
		//sprintf(global_string_buff1, "tries=%u, tgt file f_fd=%x, f_flags=%x, f_pushback=%x, tgt FILE=%p", tries_made, target_mimic->f_fd, target_mimic->f_flags, target_mimic->f_pushback, the_target_handle);
		//Buffer_NewMessage(global_string_buff1);
		
		if (target_mimic->f_fd != 1)
		{
			good_tgt_handle = true;
		}
		else
		{
			//fclose(the_target_handle);
			// NOTE: if I close the "bad" target handle, the system will keep reselecting it. and since it's bad, it will never work
			//   leaving it open seems to basically have the effect of "forgetting" this file handle, which lets it move on to the
			//   next, which seems to work consistently. I don't know what cc65 is doing here, or why this happens. 
		}
		
		++tries_made;
	}
	
	if (good_tgt_handle == false)
	{
		//LOG_ERR(("%s %d: file '%s' could not be opened for writing", __func__ , __LINE__, the_target_file_path));
		return NULL;
	}
	
	return the_target_handle;
}


// TEMPORARY DEBUG FUNCTIONS

// // helper function called by List class's print function: prints folder total bytes, and calls print on each file
// void Folder_Print(void* the_payload)
// {
// 	WB2KFolderObject*		the_folder = (WB2KFolderObject*)(the_payload);
// 
// 	DEBUG_OUT(("+----------------------------------+-+------------+----------+--------+"));
// 	DEBUG_OUT(("|File                              |S|Size (bytes)|Date      |Time    |"));
// 	DEBUG_OUT(("+----------------------------------+-+------------+----------+--------+"));
// 	List_Print(the_folder->list_, &File_Print);
// 	DEBUG_OUT(("+----------------------------------+-+------------+----------+--------+"));
// 	DEBUG_OUT(("Total bytes %lu", the_folder->total_bytes_));
// }
// 
