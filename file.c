/*
 * file.c
 *
 *  Created on: Sep 5, 2020
 *      Author: micahbly
 *
 *  This is a huge cut-down of the Amiga WorkBench2000 code, for F256 f/manager and B128 f/manager
 *    8-bit version started Jan 12, 2023
 */




/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "app.h"
#include "comm_buffer.h"
#include "file.h"
#include "general.h"
#include "list_panel.h"
#include "screen.h"
#include "strings.h"
#include "text.h"

// C includes
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// F256 includes
#include "f256.h"


/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/


/*****************************************************************************/
/*                           File-scope Variables                            */
/*****************************************************************************/

static uint8_t		temp_file_extension_buffer[FILE_MAX_EXTENSION_SIZE];	// 8 probably larger than needed, but... 

/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

extern char*		global_string_buff1;
extern char*		global_string_buff2;
extern uint8_t*		global_temp_buff_192b_1;
extern uint8_t*		global_temp_buff_192b_2;
extern uint8_t*		global_temp_buff_384b;

extern char*		global_temp_path_1;
extern char*		global_temp_path_2;


/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/



/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/



/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/

// **** CONSTRUCTOR AND DESTRUCTOR *****


// constructor
// allocates space for the object, accepts the 2 string pointers (allocates and copies them)
WB2KFileObject* File_New(const char* the_file_name, const char* the_file_path, bool is_directory, uint32_t the_filesize, uint8_t the_filetype, uint8_t the_device_num, uint8_t the_unit_num, uint8_t the_row)
{
	WB2KFileObject*		the_file;
	bool				date_ok = false;

	if ( (the_file = (WB2KFileObject*)calloc(1, sizeof(WB2KFileObject)) ) == NULL)
	{
		sprintf(global_string_buff1, "could not allocate memory to create new file object '%s'", the_file_name);
		Buffer_NewMessage(global_string_buff1);
		LOG_ERR(("%s %d: could not allocate memory to create new file object", __func__ , __LINE__));
		goto error;
	}
	LOG_ALLOC(("%s %d:	__ALLOC__	the_file	%p	size	%i", __func__ , __LINE__, the_file, sizeof(WB2KFileObject)));

	// if a populated file name was passed, use it. otherwise, extract it from the filepath
	if ( the_file_name == NULL)
	{
		the_file->file_name_ = General_ExtractFilenameFromPathWithAlloc(the_file_path);
		LOG_WARN(("%s %d: null passed for file name; extracting from filepath '%s'", __func__ , __LINE__, the_file_path));
	}
	else
	{
		the_file->file_name_ = General_StrlcpyWithAlloc(the_file_name, FILE_MAX_FILENAME_SIZE);
		LOG_ALLOC(("%s %d:	__ALLOC__	the_file->file_name_	%p	size	%i", __func__ , __LINE__, the_file->file_name_, General_Strnlen(the_file->file_name_, FILE_MAX_FILENAME_SIZE) + 1));
	}

	if ( the_file->file_name_ == NULL)
	{
		//Buffer_NewMessage("could not allocate memory for the file name");
		LOG_ERR(("%s %d: could not allocate memory for the file name", __func__ , __LINE__));
		goto error;
	}

	if ( (the_file->file_path_ = General_StrlcpyWithAlloc(the_file_path, FILE_MAX_PATHNAME_SIZE)) == NULL)
	{
		//Buffer_NewMessage("could not allocate memory for the path name");
		LOG_ERR(("%s %d: could not allocate memory for the path name", __func__ , __LINE__));
		goto error;
	}
	LOG_ALLOC(("%s %d:	__ALLOC__	the_file->file_path_	%p	size	%i", __func__ , __LINE__, the_file->file_path_, General_Strnlen(the_file->file_path_, FILE_MAX_PATHNAME_SIZE) + 1));

// 	//DEBUG_OUT(("%s %d: device_name = '%s'", __func__ , __LINE__, the_device_name));
// 	if ( (the_file->device_name_ = General_StrlcpyWithAlloc(the_device_name, FILE_MAX_PATHNAME_SIZE)) == NULL)
// 	{
// 		LOG_ERR(("%s %d: could not allocate memory for the device name", __func__ , __LINE__));
// 		goto error;
// 	}
// 	LOG_ALLOC(("%s %d:	__ALLOC__	the_file->device_name_	%p	size	%i", __func__ , __LINE__, the_file->device_name_, General_Strnlen(the_file->device_name_, FILE_MAX_PATHNAME_SIZE) + 1));
	
	// remember fizesize, to use when moving/copying files, and giving status feedback to user
	the_file->size_ = the_filesize;
// 	sprintf(global_string_buff1, "%4lu blocks", the_filesize);
// 
// 	if ( (the_file->file_size_string_ = General_StrlcpyWithAlloc(global_string_buff1, FILE_SIZE_MAX_SIZE)) == NULL)
// 	{
// 		Buffer_NewMessage("could not allocate memory for human-readable file-size");
// 		LOG_ERR(("%s %d: could not allocate memory for human-readable file-size", __func__ , __LINE__));
// 		goto error;
// 	}
// 	LOG_ALLOC(("%s %d:	__ALLOC__	the_file->file_size_string_	%p	size	%i", __func__ , __LINE__, the_file->file_size_string_, General_Strnlen(the_file->file_size_string_, FILE_SIZE_MAX_SIZE) + 1));

	// accept filetype or determine subtype and use that instead
	if (the_filetype == _CBM_T_REG)
	{
		// get file extensions
		General_ExtractFileExtensionFromFilename(the_file->file_name_, (char*)&temp_file_extension_buffer);
		
		if (General_Strncasecmp((char*)&temp_file_extension_buffer, "fnt", FILE_MAX_EXTENSION_SIZE) == 0)
		{
			the_file->file_type_ = FNX_FILETYPE_FONT;
		}
		else if (General_Strncasecmp((char*)&temp_file_extension_buffer, "pgZ", FILE_MAX_EXTENSION_SIZE) == 0)
		{
			the_file->file_type_ = FNX_FILETYPE_EXE;
		}
		else if (General_Strncasecmp((char*)&temp_file_extension_buffer, "pgz", FILE_MAX_EXTENSION_SIZE) == 0)
		{
			the_file->file_type_ = FNX_FILETYPE_EXE;
		}
		else if (General_Strncasecmp((char*)&temp_file_extension_buffer, "pgx", FILE_MAX_EXTENSION_SIZE) == 0)
		{
			the_file->file_type_ = FNX_FILETYPE_EXE;
		}
		else
		{
			the_file->file_type_ = the_filetype;
		}
	}
	else
	{
		the_file->file_type_ = the_filetype;
	}
	
	the_file->is_directory_ = is_directory;
	the_file->device_number_ = the_device_num;
	the_file->unit_number_ = the_unit_num;

	// file is brand new: not selected yet.
	the_file->selected_ = false;

	the_file->row_ = the_row;
	
// 	// remember date stamp, for sorting, display to user, etc. Use OS functions to convert the DateStamp we got from ExAll to a datetime and strings
// 	// need 3 strings to hold date, each with max len LEN_DATSTRING
// 
// 	if ( (the_file->datetime_.dat_StrDate = (char*)calloc(LEN_DATSTRING + 1, sizeof(char)) ) != NULL)
// 	{
// 		if ( (the_file->datetime_.dat_StrDay = (char*)calloc(LEN_DATSTRING + 1, sizeof(char)) ) != NULL)
// 		{
// 			if ( (the_file->datetime_.dat_StrTime = (char*)calloc(LEN_DATSTRING + 1, sizeof(char)) ) != NULL)
// 			{
// 				the_file->datetime_.dat_Format = FORMAT_INT;
// 				the_file->datetime_.dat_Flags = DTF_FUTURE;
// 				the_file->datetime_.dat_Stamp.ds_Days = the_datetime.ds_Days;
// 				the_file->datetime_.dat_Stamp.ds_Minute = the_datetime.ds_Minute;
// 				the_file->datetime_.dat_Stamp.ds_Tick = the_datetime.ds_Tick;
// 
// 				if (DateToStr(&the_file->datetime_))
// 				{
// 					date_ok = true;
// 				}
// 			}
// 			LOG_ALLOC(("%s %d:	__ALLOC__	the_file->datetime_.dat_StrDate	%p	size	%i", __func__ , __LINE__, the_file->datetime_.dat_StrDate, LEN_DATSTRING + 1));
// 			LOG_ALLOC(("%s %d:	__ALLOC__	the_file->datetime_.dat_StrDay	%p	size	%i", __func__ , __LINE__, the_file->datetime_.dat_StrDay, LEN_DATSTRING + 1));
// 			LOG_ALLOC(("%s %d:	__ALLOC__	the_file->datetime_.dat_StrTime	%p	size	%i", __func__ , __LINE__, the_file->datetime_.dat_StrTime, LEN_DATSTRING + 1));
// 		}
// 	}
// 
// 	if (date_ok == false)
// 	{
// 		LOG_ERR(("%s %d: could not process the file date", __func__ , __LINE__));
// 		goto error;
// 	}

	return the_file;

error:
	if (the_file) File_Destroy(&the_file);
	return NULL;
}


// duplicator
// makes a copy of the passed file object
WB2KFileObject* File_Duplicate(WB2KFileObject* the_original_file)
{
	WB2KFileObject*		the_duplicate_file;
// 	bool				date_ok;
	
	if (the_original_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return NULL;
	}
	
	if ( (the_duplicate_file = (WB2KFileObject*)calloc(1, sizeof(WB2KFileObject)) ) == NULL)
	{
		LOG_ERR(("%s %d: could not allocate memory to create new file object", __func__ , __LINE__));
		goto error;
	}
	LOG_ALLOC(("%s %d:	__ALLOC__	the_duplicate_file	%p	size	%i", __func__ , __LINE__, the_duplicate_file, sizeof(WB2KFileObject)));

	if ( (the_duplicate_file->file_name_ = General_StrlcpyWithAlloc(the_original_file->file_name_, FILE_MAX_FILENAME_SIZE)) == NULL)
	{
		LOG_ERR(("%s %d: could not allocate memory for the file name", __func__ , __LINE__));
		goto error;
	}
	LOG_ALLOC(("%s %d:	__ALLOC__	the_duplicate_file->file_name_	%p	size	%i", __func__ , __LINE__, the_duplicate_file->file_name_, General_Strnlen(the_duplicate_file->file_name_, FILE_MAX_FILENAME_SIZE) + 1));

	if ( (the_duplicate_file->file_path_ = General_StrlcpyWithAlloc(the_original_file->file_path_, FILE_MAX_PATHNAME_SIZE)) == NULL)
	{
		LOG_ERR(("%s %d: could not allocate memory for the path name", __func__ , __LINE__));
		goto error;
	}
	LOG_ALLOC(("%s %d:	__ALLOC__	the_duplicate_file->file_path_	%p	size	%i", __func__ , __LINE__, the_duplicate_file->file_path_, General_Strnlen(the_duplicate_file->file_path_, FILE_MAX_PATHNAME_SIZE) + 1));

	// remember fizesize, to use when moving/copying files, and giving status feedback to user
	the_duplicate_file->size_ = the_original_file->size_;
	the_duplicate_file->device_number_ = the_original_file->device_number_;
	the_duplicate_file->unit_number_ = the_original_file->unit_number_;

// 	if ( (the_duplicate_file->file_size_string_ = General_StrlcpyWithAlloc(the_original_file->file_size_string_, FILE_MAX_PATHNAME_SIZE)) == NULL)
// 	{
// 		LOG_ERR(("%s %d: could not allocate memory for the file size string", __func__ , __LINE__));
// 		goto error;
// 	}
// 	LOG_ALLOC(("%s %d:	__ALLOC__	the_duplicate_file->file_size_string_	%p	size	%i", __func__ , __LINE__, the_duplicate_file->file_size_string_, General_Strnlen(the_duplicate_file->file_size_string_, FILE_SIZE_MAX_SIZE) + 1));


// 	// remember date stamp, for sorting, display to user, etc. Use OS functions to convert the DateStamp we got from ExAll to a datetime and strings
// 	// need 3 strings to hold date, each with max len LEN_DATSTRING
// 	date_ok = false;
// 
// 	if ( (the_duplicate_file->datetime_.dat_StrDate = (char*)calloc(LEN_DATSTRING + 1, sizeof(char)) ) != NULL)
// 	{
// 		if ( (the_duplicate_file->datetime_.dat_StrDay = (char*)calloc(LEN_DATSTRING + 1, sizeof(char)) ) != NULL)
// 		{
// 			if ( (the_duplicate_file->datetime_.dat_StrTime = (char*)calloc(LEN_DATSTRING + 1, sizeof(char)) ) != NULL)
// 			{
// 				the_duplicate_file->datetime_.dat_Format = FORMAT_INT;
// 				the_duplicate_file->datetime_.dat_Flags = DTF_FUTURE;
// 				the_duplicate_file->datetime_.dat_Stamp.ds_Days = the_original_file->datetime_.dat_Stamp.ds_Days;
// 				the_duplicate_file->datetime_.dat_Stamp.ds_Minute = the_original_file->datetime_.dat_Stamp.ds_Minute;
// 				the_duplicate_file->datetime_.dat_Stamp.ds_Tick = the_original_file->datetime_.dat_Stamp.ds_Tick;
// 
// 				if (DateToStr(&the_duplicate_file->datetime_))
// 				{
// 					date_ok = true;
// 				}
// 				
// 				LOG_ALLOC(("%s %d:	__ALLOC__	the_duplicate_file->datetime_.dat_StrDate	%p	size	%i", __func__ , __LINE__, the_duplicate_file->datetime_.dat_StrDate, LEN_DATSTRING + 1));
// 				LOG_ALLOC(("%s %d:	__ALLOC__	the_duplicate_file->datetime_.dat_StrDay	%p	size	%i", __func__ , __LINE__, the_duplicate_file->datetime_.dat_StrDay, LEN_DATSTRING + 1));
// 				LOG_ALLOC(("%s %d:	__ALLOC__	the_duplicate_file->datetime_.dat_StrTime	%p	size	%i", __func__ , __LINE__, the_duplicate_file->datetime_.dat_StrTime, LEN_DATSTRING + 1));
// 			}
// 		}
// 	}
// 
// 	if (date_ok == false)
// 	{
// 		LOG_ERR(("%s %d: could not process the file date", __func__ , __LINE__));
// 		goto error;
// 	}


	// get filetype
	the_duplicate_file->is_directory_ = the_original_file->is_directory_;
	the_duplicate_file->file_type_ = the_original_file->file_type_; // ok to use same one, as both are just pointing to the same file type object anyway.

	// file is brand new: not selected yet.
	the_duplicate_file->selected_ = false;

	// absolute and relative position info
	the_duplicate_file->x_ = the_original_file->x_;
	the_duplicate_file->display_row_ = the_original_file->display_row_;
	the_duplicate_file->row_ = the_original_file->row_;
	
	return the_duplicate_file;

error:
	if (the_duplicate_file) File_Destroy(&the_duplicate_file);
	return NULL;
}


// destructor
// frees all allocated memory associated with the passed file object, and the object itself
void File_Destroy(WB2KFileObject** the_file)
{

	if (*the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_FILE_TO_DESTROY_WAS_NULL);	// crash early, crash often
	}

// 	// free the date time structure's child strings
// 	if ((*the_file)->datetime_.dat_StrDate)
// 	{
// 		LOG_ALLOC(("%s %d:	__FREE__	(*the_file)->datetime_.dat_StrDate	%p	size	%i", __func__ , __LINE__, (*the_file)->datetime_.dat_StrDate, LEN_DATSTRING + 1));
// 		free((*the_file)->datetime_.dat_StrDate);
// 		(*the_file)->datetime_.dat_StrDate = NULL;
// 	}
// 	
// 	if ((*the_file)->datetime_.dat_StrDay)	
// 	{
// 		LOG_ALLOC(("%s %d:	__FREE__	(*the_file)->datetime_.dat_StrDay	%p	size	%i", __func__ , __LINE__, (*the_file)->datetime_.dat_StrDay, LEN_DATSTRING + 1));
// 		free((*the_file)->datetime_.dat_StrDay);
// 		(*the_file)->datetime_.dat_StrDay = NULL;
// 	}
// 	
// 	if ((*the_file)->datetime_.dat_StrTime)
// 	{
// 		LOG_ALLOC(("%s %d:	__FREE__	(*the_file)->datetime_.dat_StrTime	%p	size	%i", __func__ , __LINE__, (*the_file)->datetime_.dat_StrTime, LEN_DATSTRING + 1));
// 		free((*the_file)->datetime_.dat_StrTime);
// 		(*the_file)->datetime_.dat_StrTime = NULL;
// 	}
// 	

	if ((*the_file)->file_name_ != NULL)
	{
		LOG_ALLOC(("%s %d:	__FREE__	(*the_file)->file_name_	%p	size	%i", __func__ , __LINE__, (*the_file)->file_name_, General_Strnlen((*the_file)->file_name_, FILE_MAX_FILENAME_SIZE) + 1));
		free((*the_file)->file_name_);
		(*the_file)->file_name_ = NULL;
	}
	
	if ((*the_file)->file_path_ != NULL)
	{
		LOG_ALLOC(("%s %d:	__FREE__	(*the_file)->file_path_	%p	size	%i", __func__ , __LINE__, (*the_file)->file_path_, General_Strnlen((*the_file)->file_path_, FILE_MAX_PATHNAME_SIZE) + 1));
		free((*the_file)->file_path_);
		(*the_file)->file_path_ = NULL;
	}
	
// 	if ((*the_file)->file_size_string_ != NULL)
// 	{
// 		LOG_ALLOC(("%s %d:	__FREE__	(*the_file)->file_size_string_	%p	size	%i", __func__ , __LINE__, (*the_file)->file_size_string_, General_Strnlen((*the_file)->file_size_string_, FILE_SIZE_MAX_SIZE) + 1));
// 		free((*the_file)->file_size_string_);
// 		(*the_file)->file_size_string_ = NULL;
// 	}
	
//	if (the_file->file_type_ != NULL)
//	{
//		//FileType_Destroy(the_file->file_type_); // do not destroy the filetype until the app is exiting. other files can easily be using this filetype
//		//FreeVec(the_file->file_type_);
//		the_file->file_type_ = NULL;
//	}

	LOG_ALLOC(("%s %d:	__FREE__	*the_file	%p	size	%i", __func__ , __LINE__, *the_file, sizeof(WB2KFileObject)));
	free(*the_file);
	*the_file = NULL;
}



// **** SETTERS *****

// set files selected/unselected status (no visual change)
void File_SetSelected(WB2KFileObject* the_file, bool selected)
{
	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return;
	}
	
	the_file->selected_ = selected;

	return;
}


/// updates the icon's size/position information
void File_UpdatePos(WB2KFileObject* the_file, uint8_t x, int8_t display_row, uint16_t row)
{
	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return;
	}
	
	the_file->x_ = x;
	the_file->display_row_ = display_row;
	the_file->row_ = row;
}


// update the existing file name to the passed one, freeing any previous one and allocating anew.
bool File_UpdateFileName(WB2KFileObject* the_file, const char* new_file_name)
{
	bool		success;
	
	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}
	
	// LOGIC: 
	//   couple things to know: 
	//   1. this is likely called by File_Rename(). File_Rename() can be called with a user rename, in which case it will get passed a temp string with new name
	//      File_Rename() can also be called by FileMover.movefiles(), in which case it will be passed the file->file_name_ (which doesn't change)
	//      Need to test for getting passed the file->file_name_ and skip out in that event or we'll free ourselves without having any new to use.
	//   2. even if not from move, it's possible we could get same actual name, just in different string. Might as well check for it.
	//   Label_SetText() is probably time expensive

	if (the_file->file_name_ == new_file_name)
	{
		return true;
	}
	
	if (General_Strncasecmp(the_file->file_name_, new_file_name, FILE_MAX_FILENAME_SIZE) == 0)
	{
		return true;
	}
		
	if (the_file->file_name_ != NULL)
	{
		LOG_ALLOC(("%s %d:	__FREE__	the_file->file_name_	%p	size	%i", __func__ , __LINE__, the_file->file_name_, General_Strnlen(the_file->file_name_, FILE_MAX_FILENAME_SIZE) + 1));
		free(the_file->file_name_);
		the_file->file_name_ = NULL;
	}

	the_file->file_name_ = General_StrlcpyWithAlloc(new_file_name, FILE_MAX_FILENAME_SIZE);
	LOG_ALLOC(("%s %d:	__ALLOC__	the_file->file_name_	%p	size	%i", __func__ , __LINE__, the_file->file_name_, General_Strnlen(the_file->file_name_, FILE_MAX_FILENAME_SIZE) + 1));
	
	success = (the_file->file_name_ != NULL);
	
	return success;
}


// update the existing file path to the passed one, freeing any previous one and allocating anew.
bool File_UpdateFilePath(WB2KFileObject* the_file, const char* new_file_path)
{	
	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}

	if (the_file->file_path_ != NULL)
	{
		LOG_ALLOC(("%s %d:	__FREE__	the_file->file_path_	%p	size	%i", __func__ , __LINE__, the_file->file_path_, General_Strnlen(the_file->file_path_, FILE_MAX_PATHNAME_SIZE) + 1));
		free(the_file->file_path_);
		the_file->file_path_ = NULL;
	}
	
	the_file->file_path_ = General_StrlcpyWithAlloc(new_file_path, FILE_MAX_PATHNAME_SIZE);
	LOG_ALLOC(("%s %d:	__ALLOC__	the_file->file_path_	%p	size	%i", __func__ , __LINE__, the_file->file_path_, General_Strnlen(the_file->file_path_, FILE_MAX_PATHNAME_SIZE) + 1));
	
	return ( (the_file->file_path_ != NULL) );
}




// **** GETTERS *****


// get the selected/not selected state of the file
bool File_IsSelected(WB2KFileObject* the_file)
{
	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}

	return the_file->selected_;
}


// returns true if file object represents a folder
bool File_IsFolder(WB2KFileObject* the_file)
{
	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}

	return the_file->is_directory_;
}


// returns the filename - no allocation
char* File_GetFileNameString(WB2KFileObject* the_file)
{
	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return NULL;
	}

	return the_file->file_name_;
}


// // allocates and returns a copy of the modified date and time as string (for use with list mode headers or with info panel, etc.)
// char* File_GetFileDateStringCopy(WB2KFileObject* the_file)
// {
// 	char*	the_timedate;
// 
// 	if (the_file == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return NULL;
// 	}
// 
// 	// combine date and time into a single string
// 	if ( (the_timedate = (char *)calloc(MAX_SIZE_TIMEDATE_STRING + 1, sizeof(char)) ) == NULL)
// 	{
// 		goto error;
// 	}
// 	LOG_ALLOC(("%s %d:	__ALLOC__	the_timedate	%p	size	%i", __func__ , __LINE__, the_timedate, MAX_SIZE_TIMEDATE_STRING + 1));
// 
// 	sprintf((char*)the_timedate, "%s  %s", the_file->datetime_.dat_StrDate, the_file->datetime_.dat_StrTime);
// 
// 	return the_timedate;
// 	
// error:
// 	return NULL;
// }


// // allocates and returns a copy of the file size as human readable string (for use with list mode headers or with info panel, etc.)
// char* File_GetFileSizeStringCopy(WB2KFileObject* the_file)
// {
// 	char*	the_filesize;
// 
// 	if (the_file == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return NULL;
// 	}
// 
// 	// readable filesize
// 	if ( (the_filesize = (char *)calloc(FILE_TYPE_MAX_SIZE_NAME + 1, sizeof(char)) ) == NULL)
// 	{
// 		goto error;
// 	}
// 	LOG_ALLOC(("%s %d:	__ALLOC__	the_filesize	%p	size	%i", __func__ , __LINE__, the_filesize, FILE_TYPE_MAX_SIZE_NAME + 1));
// 	
// 	General_MakeFileSizeReadable(the_file->size_, the_filesize);
// 
// 	return the_filesize;
// 	
// error:
// 	return NULL;
// }


// returns the filetype id
uint8_t File_GetFileTypeID(WB2KFileObject* the_file)
{
	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return 255;
	}
	
	return the_file->file_type_;
}


// populate a buffer with bytes from the file, reading the specified number of bytes into the buffer. Display the buffer chars. Returns false on any error
bool File_GetTextContents(WB2KFileObject* the_file, char* the_buffer)
{
	// LOGIC
	//   does not care about file type: any time of file will allowed
	//   open file for reading > get first chunk of bytes to read > terminate with \0 > display to screen > wait for user > repeat or return
	//   return false on any error
	
	// LOGIC
	//   we only have 80x25 to work with, and we need a row for "hit space for more, esc to stop"
	//     so 24 rows * 80 bytes = 1920 max bytes can be shown
	//   we only need one buffer as we read and print to screen line by line (80 bytes)
	//   we need to keep the file stream open until it is used up, or user exits loop

	int16_t		s_bytes_read_from_disk;
	uint16_t	num_bytes_to_read = MEM_TEXT_VIEW_BYTES_PER_ROW;
	bool		keep_going = true;
	uint8_t		y;
	uint8_t		user_input;
	char*		buffer_start = the_buffer;
	FILE*		the_file_handler;

	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}

// 	Buffer_NewMessage("starting text read...");
	
	the_file_handler = fopen((char*)the_file->file_path_, "r");
	//the_file_handler = fopen((char*)the_file->file_name_, "r");
	
	if (the_file_handler == NULL)
	{
		//sprintf(global_string_buff1, "file '%s' could not be opened for text display", the_file->file_path_);
		//Buffer_NewMessage(global_string_buff1);
		LOG_ERR(("%s %d: file '%s' could not be opened for reading", __func__ , __LINE__, the_file->file_path_));
		goto error;
	}
	

// 	Buffer_NewMessage("file opened ok...");

	// loop until file is all read/displayed, or until user says stop
	while (keep_going == true)
	{
		y = 0;
		Text_ClearScreen(FILE_CONTENTS_FOREGROUND_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);
		sprintf(global_string_buff1, General_GetString(ID_STR_MSG_TEXT_VIEW_INSTRUCTIONS), the_file->file_name_);
		Text_DrawStringAtXY(0, y++, global_string_buff1, FILE_CONTENTS_ACCENT_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);
				
		// per-row loop
		do
		{
			the_buffer = buffer_start;

			s_bytes_read_from_disk = fread(the_buffer, sizeof(char), num_bytes_to_read, the_file_handler);
	
			if ( s_bytes_read_from_disk < 0)
			{
				//Buffer_NewMessage("s_bytes_read_from_disk < 0");
				LOG_ERR(("%s %d: reading file '%s' resulted in error %i", __func__ , __LINE__, the_file->file_name_, s_bytes_read_from_disk));
				goto error;
			}
	
			if ( s_bytes_read_from_disk == 0)
			{
				//Buffer_NewMessage("s_bytes_read_from_disk == 0 (end of file)");
				LOG_ERR(("%s %d: reading file '%s' produced 0 bytes", __func__ , __LINE__, the_file->file_name_));
				keep_going = false;
			}
		
			if ( s_bytes_read_from_disk < num_bytes_to_read )
			{
				// we hit end of file
				//Buffer_NewMessage("s_bytes_read_from_disk was less than full row");
				//LOG_ERR(("%s %d: reading file '%s' expected %u bytes, got %i bytes", __func__ , __LINE__, the_file->file_name_, num_bytes_to_read, s_bytes_read_from_disk));
				keep_going = false;
			}

			Text_DrawCharsAtXY(0, y, (uint8_t*)the_buffer, s_bytes_read_from_disk);
		
			++y;
		} while (y < MAX_TEXT_VIEW_ROWS_PER_PAGE && keep_going == true);
		
		user_input = getchar();
		
		if (user_input == CH_ESC || user_input == 'q' || user_input == CH_RUNSTOP)
		{
			keep_going = false;
		}		
	}

	fclose(the_file_handler);
	
	
	return true;
	
error:
	if (the_file_handler) fclose(the_file_handler);
	return false;
}


// populate a buffer with bytes from the file, reading the specified number of bytes into the buffer. Display the buffer chars. Returns false on any error
bool File_GetHexContents(WB2KFileObject* the_file, char* the_buffer)
{
	// LOGIC
	//   does not care about file type: any time of file will allowed
	//   open file for reading > get first chunk of bytes to read > terminate with \0 > display to screen > wait for user > repeat or return
	//   return false on any error
	
	// LOGIC
	//   we only have 80x25 to work with, and we need a row for "hit space for more, esc to stop"
	//     so 24 rows * 80 bytes = 1920 max bytes can be shown
	//   we only need one buffer as we read and print to screen line by line (80 bytes)
	//   we need to keep the file stream open until it is used up, or user exits loop

	int16_t		s_bytes_read_from_disk;
	uint16_t	num_bytes_to_read = MEM_DUMP_BYTES_PER_ROW;
	bool		keep_going = true;
	uint8_t		y;
	uint8_t		cut_off_pos;
	uint8_t		user_input;
	FILE*		the_file_handler;
	uint8_t*	loc_in_file = 0x000;	// will track the location within the file, so we can show to users on left side. 

	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}

// 	Buffer_NewMessage("starting text read...");
	
	the_file_handler = fopen((char*)the_file->file_path_, "r");
	//the_file_handler = fopen((char*)the_file->file_name_, "r");
	
	if (the_file_handler == NULL)
	{
		sprintf(global_string_buff1, "file '%s' could not be opened for hex display", the_file->file_path_);
		Buffer_NewMessage(global_string_buff1);
		LOG_ERR(("%s %d: file '%s' could not be opened for reading", __func__ , __LINE__, the_file->file_path_));
		goto error;
	}
	

// 	Buffer_NewMessage("file opened ok...");

	// loop until file is all read/displayed, or until user says stop
	while (keep_going == true)
	{
		y = 0;
		Text_ClearScreen(FILE_CONTENTS_FOREGROUND_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);
		sprintf(global_string_buff1, General_GetString(ID_STR_MSG_HEX_VIEW_INSTRUCTIONS), the_file->file_name_);
		Text_DrawStringAtXY(0, y++, global_string_buff1, FILE_CONTENTS_ACCENT_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);
				
		// per-row loop
		do
		{
			memset(the_buffer, 0, MEM_DUMP_BYTES_PER_ROW);
			
			s_bytes_read_from_disk = fread(the_buffer, sizeof(char), num_bytes_to_read, the_file_handler);
	
			if ( s_bytes_read_from_disk < 0)
			{
				//Buffer_NewMessage("s_bytes_read_from_disk < 0");
				LOG_ERR(("%s %d: reading file '%s' resulted in error %i", __func__ , __LINE__, the_file->file_name_, s_bytes_read_from_disk));
				goto error;
			}
	
			if ( s_bytes_read_from_disk == 0)
			{
				//Buffer_NewMessage("s_bytes_read_from_disk == 0 (end of file)");
				LOG_ERR(("%s %d: reading file '%s' produced 0 bytes", __func__ , __LINE__, the_file->file_name_));
				keep_going = false;
			}
		
			if ( s_bytes_read_from_disk < num_bytes_to_read )
			{
				// we hit end of file
				//Buffer_NewMessage("s_bytes_read_from_disk was less than full row");
				//LOG_ERR(("%s %d: reading file '%s' expected %u bytes, got %i bytes", __func__ , __LINE__, the_file->file_name_, num_bytes_to_read, s_bytes_read_from_disk));
				keep_going = false;
			}

			sprintf(global_string_buff2, "%p: ", loc_in_file);
			Text_DrawStringAtXY(0, y, global_string_buff2, FILE_CONTENTS_ACCENT_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);
		
			sprintf(global_string_buff2, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x  ", 
				the_buffer[0], the_buffer[1], the_buffer[2], the_buffer[3], the_buffer[4], the_buffer[5], the_buffer[6], the_buffer[7], 
				the_buffer[8], the_buffer[9], the_buffer[10], the_buffer[11], the_buffer[12], the_buffer[13], the_buffer[14], the_buffer[15]);
			
			// now cut off the string if we didn't read a full line
			if ( s_bytes_read_from_disk < num_bytes_to_read )
			{
				cut_off_pos = (uint8_t)s_bytes_read_from_disk;
				cut_off_pos *= 3;	// each char represented by 2 hex digits and a space
				global_string_buff2[cut_off_pos] = '\0';
			}
			
			Text_DrawStringAtXY(MEM_DUMP_START_X_FOR_HEX, y, global_string_buff2, FILE_CONTENTS_FOREGROUND_COLOR, FILE_CONTENTS_BACKGROUND_COLOR);

			// render chars with char draw function to avoid problem of 0s getting treated as nulls in sprintf
			Text_DrawCharsAtXY(MEM_DUMP_START_X_FOR_CHAR, y, (uint8_t*)the_buffer, s_bytes_read_from_disk);
		
			loc_in_file += MEM_DUMP_BYTES_PER_ROW;
			++y;

		
		} while (y < MAX_TEXT_VIEW_ROWS_PER_PAGE && keep_going == true);
		
		user_input = getchar();
		
		if (user_input == CH_ESC || user_input == 'q' || user_input == CH_RUNSTOP)
		{
			keep_going = false;
		}		
	}

	fclose(the_file_handler);
	
	
	return true;
	
error:
	if (the_file_handler) fclose(the_file_handler);
	return false;
}



// // get the free disk space on the parent disk of the file
// // returns -1 in event of error
// int16_t File_GetFreeBytesOnDisk(WB2KFileObject* the_file)
// {
// 	BPTR				the_file_lock;
// 	int16_t			bytes_free = 0;
// 	struct InfoData*	the_info_data;
// 	bool				success;
// 	
// 	// LOGIC: 
// 	//   AmigaDOS needs a lock on any file in a disk to return an Info object with the disk's free and used space.
// 	//   Info() requires the struct InfoData to be long-word aligned, so we'll use AllocVec
// 	
// 	if (the_file == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return -1;
// 	}
// 
// 	if ( (the_info_data = (struct InfoData*)AllocVec(sizeof(struct InfoData), MEMF_ANY) ) == NULL)
// 	{
// 		LOG_ERR(("%s %d: could not allocate memory for the struct InfoData", __func__ , __LINE__));
// 		goto error;
// 	}
// 	LOG_ALLOC(("%s %d:	__ALLOC__	the_info_data	%p	size	%i", __func__ , __LINE__, the_info_data, sizeof(struct InfoData)));
// 
// 	// try to get lock on the dictionary file
// 	if (!(the_file_lock = Lock((CONST_STRPTR)the_file->file_path_, ACCESS_READ)))
// 	{
// 		LOG_ERR(("%s %d: Couldn't get lock on file '%s'", __func__ , __LINE__, the_file->file_path_));
// 		goto error;
// 	}
// 	
// 	success = Info(the_file_lock, the_info_data);
// 
// 	if ( success == false)
// 	{
// 		LOG_ERR(("%s %d: Couldn't get an InfoData object for disk containing file '%s'", __func__ , __LINE__, the_file->file_path_));
// 		UnLock(the_file_lock);
// 		goto error;
// 	}
// 	
// 	bytes_free = (the_info_data->id_NumBlocks - the_info_data->id_NumBlocksUsed) * the_info_data->id_BytesPerBlock;
// 
// 	//DEBUG_OUT(("%s %d: Disk containing file '%s' has %li bytes free", __func__ , __LINE__, the_file->file_path_, bytes_free));
// 	//DEBUG_OUT(("%s %d: Disk containing file '%s' has %li bytes used", __func__ , __LINE__, the_file->file_path_, the_info_data->id_NumBlocksUsed * the_info_data->id_BytesPerBlock));
// 	
// 	UnLock(the_file_lock);
// 	
// 	LOG_ALLOC(("%s %d:	__FREE__	the_info_data	%p	size	%i", __func__ , __LINE__, the_info_data, sizeof(struct InfoData)));
// 	FreeVec(the_info_data);
// 	the_info_data = NULL;
// 
// 	return bytes_free;
// 
// error:
// 	if (the_info_data)	FreeVec(the_info_data);
// 	return -1;
// }





// **** OTHER FUNCTIONS *****


// delete the passed file/folder. If a folder, it must have been previously emptied of files.
bool File_Delete(WB2KFileObject* the_file, void* not_needed)
{
	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}

	if (remove(the_file->file_path_) < 0)
	{
		LOG_ERR(("%s %d: not able to delete file '%s' @ '%s'", __func__ , __LINE__, the_file->file_name_, the_file->file_path_));
		goto error;
	}
	else
	{
		LOG_INFO(("%s %d: deleted '%s'", __func__ , __LINE__, the_file->file_path_));
	}

	return true;

error:
	return false;
}


// // open the passed file with the chosen application (or just the application, if an app)
// bool File_Open(WB2KFileObject* the_file)
// {
// 	bool					is_workbench_app;
// 	WB2KFileType*			the_app_type;
// 	
// 	// LOGIC:
// 	//   if file is an executable, we don't need any args.
// 	//   if file is not, we need to open the exec for it, and pass an arg for the file to be opened.
// 	//   we classify apps into 2 categories: DOS and Workbench
// 	//     DOS apps we call via CONSOLE (FILE_TYPE_CATEGORY_APP_DOS)
// 	//     Workbench apps we start via CreateLaunch (task process) (FILE_TYPE_CATEGORY_APP_WB)
// 
// 	if (the_file == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return false;
// 	}
// 
// 	if (the_file->file_type_->is_exec_ == true)
// 	{
// 		the_app_type = the_file->file_type_;
// 	}
// 	else
// 	{
// 		// need to get the open-with app before we know if it's console or WB style startup
// 
// 		if ( (the_app_type = File_GetOpenWithFileType(the_file)) == NULL)
// 		{
// 			LOG_WARN(("%s %d: could not get an open-with app type for file '%s'", __func__ , __LINE__, the_file->file_name_));
// 			return false;
// 		}
// 	}
// 	
// 	is_workbench_app = General_Strncasecmp(FileType_GetCategory(the_app_type), FILE_TYPE_CATEGORY_APP_DOS, FILE_TYPE_MAX_SIZE_CATEGORY);
// 	
// 	//DEBUG_OUT(("%s %d: the open-with app category: workbench startup=%i", __func__ , __LINE__, is_workbench_app));
// 
// 	if (is_workbench_app)
// 	{
// 		return File_OpenViaWorkbench(the_file);
// 	}
// 	else
// 	{
// 		return File_OpenViaConsole(the_file);
// 	}
// }


// renames a file and its info file, if present
bool File_Rename(WB2KFileObject* the_file, const char* new_file_name, const char* new_file_path)
{
	//char	temp_buff[80];
	int8_t	result_code;
	
	// LOGIC:
	//   remake file path using new name and old file path, then call Rename()

	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}

	//sprintf(global_string_buff1, "old path: '%s', new path: '%s'", the_file->file_path_, new_file_path);
	//Buffer_NewMessage(global_string_buff1);
	
	if ( (result_code = rename( the_file->file_path_, new_file_path )) < 0)
	{
		//sprintf(temp_buff, "rename returned err code %i", result_code);
		//Buffer_NewMessage((char*)&temp_buff);
		LOG_ERR(("%s %d: Rename action failed with file '%s'", __func__ , __LINE__, the_file->file_name_));
		goto error;
	}
	else
	{
		//DEBUG_OUT(("%s %d: Rename action succeeded; new_file_name='%s', pre-rename file name='%s'", __func__ , __LINE__, new_file_name, the_file->file_name_));
		//DEBUG_OUT(("%s %d: Rename action succeeded; new_file_path='%s', pre-rename file path='%s'", __func__ , __LINE__, new_file_path, the_file->file_path_));
		
		if (File_UpdateFileName(the_file, new_file_name) == false)
		{
			LOG_ERR(("%s %d: Rename action failed with file '%s': could not update file name", __func__ , __LINE__, new_file_name));
			goto error;
		}
		
		if (File_UpdateFilePath(the_file, new_file_path) == false)
		{
			LOG_ERR(("%s %d: Rename action failed with file '%s': could not update file path", __func__ , __LINE__, new_file_path));
			goto error;
		}
	}

	return true;
	
error:
	return false;
}


// mark file as selected, and refresh display accordingly
bool File_MarkSelected(WB2KFileObject* the_file, int8_t y_offset)
{
	// LOGIC: if file was already selected, don't do anything. don't change state, don't change visual appearance

	//DEBUG_OUT(("%s %d: filename: '%s', selected=%i", __func__ , __LINE__, the_file->file_name_, the_file->selected_));

	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}

	if (the_file->selected_ == false)
	{
		the_file->selected_ = true;

		// re-render with selected state
		File_Render(the_file, the_file->selected_, y_offset, true);	// if we're here, panel must be active?
	}

	return true;
}

// mark file as un-selected, and refresh display accordingly
bool File_MarkUnSelected(WB2KFileObject* the_file, int8_t y_offset)
{
	// LOGIC: if file was already un-selected, don't do anything. don't change state, don't change visual appearance

	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}

	if (the_file->selected_ == true)
	{
		the_file->selected_ = false;

		// re-render with un-selected state
		File_Render(the_file, the_file->selected_, y_offset, true);	// if we're here, panel must be active?
	}

	return true;
}


/// render filename and any other relevant labels at the previously established coordinates
// if as_selected is true, will render with inversed text. Otherwise, will render normally.
// if as_active is true, will render in LIST_ACTIVE_COLOR, otherwise in LIST_INACTIVE_COLOR
void File_Render(WB2KFileObject* the_file, bool as_selected, int8_t y_offset, bool as_active)
{
	uint8_t	x1;
	uint8_t	x2;
	uint8_t	sizex;
	uint8_t	typex;
	uint8_t	the_color;
	int8_t	y;
	
	// LOGIC:
	//   Panel is responsible for having flowed the content in a way that each file either has a displayable display_row_ value, or -1.
	//   y_offset is the first displayable row of the parent panel
	
	if (the_file == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return;
	}

	if (as_active)
	{
		the_color = LIST_ACTIVE_COLOR;
	}
	else
	{
		the_color = LIST_INACTIVE_COLOR;
	}
	
	x1 = the_file->x_;
	x2 = the_file->x_ + (UI_PANEL_INNER_WIDTH - 1);
	typex = x1 + UI_PANEL_FILETYPE_OFFSET;
	sizex = typex + UI_PANEL_FILESIZE_OFFSET - 1; // "bytes" is 5 in len, but we are using 6 digit size, so start one before bytes.
	
	if (the_file->display_row_ != -1)
	{
		sprintf(global_string_buff1, "%6lu", the_file->size_);
		y = the_file->display_row_ + y_offset;
		Text_FillBox(x1, y, x2, y, CH_SPACE, the_color, APP_BACKGROUND_COLOR);
		Text_DrawStringAtXY( x1, y, the_file->file_name_, the_color, APP_BACKGROUND_COLOR);
		Text_DrawStringAtXY( sizex, y, global_string_buff1, the_color, APP_BACKGROUND_COLOR);
		Text_DrawStringAtXY( typex, y, General_GetFileTypeString(the_file->file_type_), the_color, APP_BACKGROUND_COLOR);
		
		if (as_selected == true)
		{
			Text_InvertBox(x1, y, x2, y);
		}
	}
	else
	{
		//DEBUG_OUT(("%s %d: can't render; x1=%i, x2=%i, x_bound=%i, x_offset=%i (%s)", __func__ , __LINE__, x1, x2, x_bound, x_offset, the_label->text_));
	}
}


// // helper function called by List class's print function: prints one file entry
// void File_Print(void* the_payload)
// {
// 	WB2KFileObject*		this_file = (WB2KFileObject*)(the_payload);
// 
// 	DEBUG_OUT(("|%-34s|%-1i|%-12lu|%-10s|%-8s|", this_file->file_name_, this_file->selected_, this_file->size_, this_file->datetime_.dat_StrDate, this_file->datetime_.dat_StrTime));
// }


// ***** comparison functions used to compare to list items with Wb2KFileObject payloads

bool File_CompareSize(void* first_payload, void* second_payload)
{
	WB2KFileObject*		file_1 = (WB2KFileObject*)first_payload;
	WB2KFileObject*		file_2 = (WB2KFileObject*)second_payload;

	if (file_1->size_ > file_2->size_)
	{
		return true;
	}
	else
	{
		return false;
	}
}


bool File_CompareFileTypeID(void* first_payload, void* second_payload)
{
	WB2KFileObject*		file_1 = (WB2KFileObject*)first_payload;
	WB2KFileObject*		file_2 = (WB2KFileObject*)second_payload;

	if (file_1->file_type_ > file_2->file_type_)
	{
		return true;
	}
	else
	{
		return false;
	}
}


bool File_CompareName(void* first_payload, void* second_payload)
{
	WB2KFileObject*		file_1 = (WB2KFileObject*)first_payload;
	WB2KFileObject*		file_2 = (WB2KFileObject*)second_payload;

	if (General_Strncasecmp(file_1->file_name_, file_2->file_name_, FILE_MAX_FILENAME_SIZE) > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}


// bool File_CompareDate(void* first_payload, void* second_payload)
// {
// 	WB2KFileObject*		file_1 = (WB2KFileObject*)first_payload;
// 	WB2KFileObject*		file_2 = (WB2KFileObject*)second_payload;
// 
// 	if (file_1->datetime_.dat_Stamp.ds_Days > file_2->datetime_.dat_Stamp.ds_Days)
// 	{
// 		return true;
// 	}
// 	else if (file_2->datetime_.dat_Stamp.ds_Days > file_1->datetime_.dat_Stamp.ds_Days)
// 	{
// 		return false;
// 	}
// 	else
// 	{
// 		if (file_1->datetime_.dat_Stamp.ds_Minute > file_2->datetime_.dat_Stamp.ds_Minute)
// 		{
// 			return true;
// 		}
// 		else if (file_2->datetime_.dat_Stamp.ds_Minute > file_1->datetime_.dat_Stamp.ds_Minute)
// 		{
// 			return false;
// 		}
// 		else
// 		{
// 			if (file_1->datetime_.dat_Stamp.ds_Tick > file_2->datetime_.dat_Stamp.ds_Tick)
// 			{
// 				return true;
// 			}
// 			else
// 			{
// 				return false;
// 			}
// 		}
// 	}
// }
// 
// 