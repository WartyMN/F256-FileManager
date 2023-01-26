/*
 * general.c
 *
 *  Created on: Feb 19, 2022
 *      Author: micahbly
 *
 *  - adapted from my Amiga WB2K project's general.c
 */

// This is a cut-down, semi-API-compatible version of the OS/f general.c file from Lich King (Foenix)
// adapted for Foenix F256 Jr starting November 29, 2022


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "general.h"
#include "app.h"
#include "strings.h"
#include "memory.h"
#include "sys.h"

// C includes
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
//#include <math.h>

// F256 includes
#include <f256.h>



/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/




/*****************************************************************************/
/*                          File-scoped Variables                            */
/*****************************************************************************/

static uint8_t			general_string_merge_buff_192b[192];


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

extern char*			global_string[NUM_STRINGS];
extern uint8_t*			global_temp_buff_192b_1;
extern uint8_t*			global_temp_buff_192b_2;

extern uint8_t			zp_bank_num;

#pragma zpsym ("zp_bank_num");

// logging related: only do if debugging is active
#if defined LOG_LEVEL_1 || defined LOG_LEVEL_2 || defined LOG_LEVEL_3 || defined LOG_LEVEL_4 || defined LOG_LEVEL_5
	static char				debug_buffer[256];	// create once, use for every debug and logging function
	static const char*		kDebugFlag[5] = {
								"[ERROR]",
								"[WARNING]",
								"[INFO]",
								"[DEBUG]",
								"[ALLOC]"
							};

	static FILE*			global_log_file;
#endif



/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/



/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/






/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/







// **** MATH UTILITIES *****

// //! Round a float to the nearest integer value
// //! THINK C's and SAS/C's math.h don't include round()
// //! from: https://stackoverflow.com/questions/4572556/concise-way-to-implement-round-in-c
// //! @param	the_float: a double value to round up/down
// //! @return	Returns an int with the rounded value
// int32_t General_Round(double the_float)
// {
//     if (the_float < 0.0)
//         return (int)(the_float - 0.5);
//     else
//         return (int)(the_float + 0.5);
// }
// note: disabled. see this cc65 error:
// Fatal: Floating point type is currently unsupported






// **** NUMBER<>STRING UTILITIES *****

// convert a file size in bytes to a human readable format using "10 bytes", "1.4 kb", "1 MB", etc. 
//   NOTE: formatted_file_size string must have been allocated before passing here
void General_MakeFileSizeReadable(unsigned long size_in_bytes, char* formatted_file_size)
{
	//double			final_size; // no float in cc65!
	uint32_t		final_size;
	
	// convert to float before doing any operations on it, to prevent integer weirdness
	//final_size = (double)size_in_bytes;
	final_size = size_in_bytes;
	
	if (size_in_bytes < 1024) // 1.0k
	{
		// show size in bytes, precisely
		sprintf((char*)formatted_file_size, "%lu b", size_in_bytes);
	}
	else if (size_in_bytes < 10240) // 10k
	{
		// show size in .1k chunks (eg, 9.4k)
		final_size /= 1024;
		sprintf((char*)formatted_file_size, "%.1f kb", final_size);
	}
	else if (size_in_bytes < 1048576) // 1 MB
	{
		// show size in 1k chunks
		final_size /= 1024;
		//size_in_bytes = General_Round(final_size);
		size_in_bytes = final_size /= 1024;
		sprintf((char*)formatted_file_size, "%lu kb", size_in_bytes);
	}
	else if (size_in_bytes < 10485760) // 10MB
	{
		// show size in .1M chunks (eg, 1.4MB)
		final_size /= 1048576;
		sprintf((char*)formatted_file_size, "%.1f Mb", final_size);
	}
	else
	{
		// show size in 1M chunks (eg, 1536 MB)
		final_size /= 1048576;
		//size_in_bytes = General_Round(final_size);
		size_in_bytes = final_size;
		sprintf((char*)formatted_file_size, "%lu Mb", size_in_bytes);
	}
	
	return;
}






// **** MISC STRING UTILITIES *****

// return the global string for the passed ID
// the string is copied into a temp buffer, and that is returned, to protect the original string
char* General_GetString(uint8_t the_string_id)
{
	strcpy((char*)general_string_merge_buff_192b, global_string[the_string_id]);

	return (char*)general_string_merge_buff_192b;
}


//! Convert a string, in place, to lower case
//! This overwrites the string with a lower case version of itself.
//! Warning: no length check is in place. Calling function must verify string is well-formed (terminated).
//! @param	the_string: the string to convert to lower case.
//! @return	Returns true if the string was modified by the process.
bool General_StrToLower(char* the_string)
{
    int16_t	i;
    int16_t	len = strlen(the_string);
    bool	change_made = false;
    
	for (i = 0; i < len; i++)
	{
	    char	this_char;
		
		this_char = the_string[i];
		the_string[i] = General_ToLower(the_string[i]);
		
		if (this_char != the_string[i])
		{
			change_made = true;
		}
	}

	return change_made;
}


//! Change the case of the passed character from upper to lower (if necessary)
//! Scope is limited to characters A-Z, ascii.
//! replacement for tolower() in c library, which doesn't seem to work [in Amiga WB2K] for some reason.
//! @return	a character containing the lowercase version of the passed character.
char General_ToLower(char the_char)
{
    char	lowered_value;
    
    lowered_value = (the_char >='A' && the_char<='Z') ? (the_char + 32) : (the_char);
    
    return lowered_value;
}


//! Allocates memory for a new string and copies up to max_len - 1 characters from the NUL-terminated string src to the new string, NUL-terminating the result
//! This is meant to be a one stop shop for getting a copy of a string
//! @param	src: The string to copy
//! @param	max_len: The maximum number of bytes to use in the destination string, including the terminator. If this is shorter than the length of the source string + 1, the resulting copy string will be capped at max_len - 1.
//! @return	a copy of the source string to max_len, or NULL on any error condition
char* General_StrlcpyWithAlloc(const char* src, signed long max_len)
{
	char*	dst;
	size_t	alloc_len;
	
	if (max_len < 1)
	{
		return NULL;
	}
	
	alloc_len = General_Strnlen(src, max_len) + 1;
	
	if ( (dst = (char*)calloc(alloc_len, sizeof(char)) ) == NULL)
	{
		return NULL;
	}
	else
	{
		General_Strlcpy(dst, src, max_len);
	}
	//LOG_ALLOC(("%s %d:	__ALLOC__	dst	%p	size	%i, string='%s'", __func__ , __LINE__, dst, General_Strnlen(src, max_len) + 1, dst));

	return dst;
}


//! Copies up to max_len - 1 characters from the NUL-terminated string src to dst, NUL-terminating the result
//! @param	src: The string to copy
//! @param	dst: The string to copy into. Calling function is responsible for ensuring this string is allocated, and has at least as much storage as max_len.
//! @param	max_len: The maximum number of bytes to use in the destination string, including the terminator. If this is shorter than the length of the source string + 1, the resulting copy string will be capped at max_len - 1.
//! @return	Returns the length of the source string, or -1 on any error condition
signed long General_Strlcpy(char* dst, const char* src, signed long max_len)
{
    const signed long	src_len = strlen(src);
 	
	if (max_len < 1)
	{
		return -1;
	}

    if (src_len + 1 < max_len)
    {
        memcpy(dst, src, src_len + 1);
    }
    else
    {
    	memcpy(dst, src, max_len - 1);
        dst[max_len - 1] = '\0';
    }
    
    return src_len;
}


//! Copies up to max_len - 1 characters from the NUL-terminated string src and appends to the end of dst, NUL-terminating the result
//! @param	src: The string to copy
//! @param	dst: The string to append to. Calling function is responsible for ensuring this string is allocated, and has at least as much storage as max_len.
//! @param	max_len: The maximum number of bytes to use in the destination string, including the terminator. If this is shorter than the length of src + length of dst + 1, the resulting copy string will be capped at max_len - 1.
//! @return	Returns the length of the attempted concatenated string: initial length of dst plus the length of src.
signed long General_Strlcat(char* dst, const char* src, signed long max_len)
{  	
	const signed long	src_len = strlen(src);
	const signed long 	dst_len = General_Strnlen(dst, max_len);
 	
	if (max_len > 0)
	{
		if (dst_len == max_len)
		{
			return max_len + src_len;
		}

		if (src_len < max_len - dst_len)
		{
			memcpy(dst + dst_len, src, src_len + 1);
		}
		else
		{
			memcpy(dst + dst_len, src, max_len - dst_len - 1);
			dst[max_len - 1] = '\0';
		}
	}

    return dst_len + src_len;
}


//! Makes a case sensitive comparison of the specified number of characters of the two passed strings
//! Stops processing once max_len has been reached, or when one of the two strings has run out of characters.
//! http://home.snafu.de/kdschem/c.dir/strings.dir/strncmp.c
//! TODO: compare this to other implementations, see which is faster. eg, https://opensource.apple.com/source/Libc/Libc-167/gen.subproj/i386.subproj/strncmp.c.auto.html
//! @param	string_1: the first string to compare.
//! @param	string_2: the second string to compare.
//! @param	max_len: the maximum number of characters to compare. Even if both strings are larger than this number, only this many characters will be compared.
//! @return	Returns 0 if the strings are equivalent (at least up to max_len). Returns a negative or positive if the strings are different.
int16_t General_Strncmp(const char* string_1, const char* string_2, size_t max_len)
{
	register uint8_t	u;
	
	do ; while( (u = (uint8_t)*string_1++) && (u == (uint8_t)*string_2++) && --max_len );

	if (u)
	{
		string_2--;
	}
	
	return (u - (uint8_t)*string_2);
}


//! Makes a case insensitive comparison of the specified number of characters of the two passed strings
//! Stops processing once max_len has been reached, or when one of the two strings has run out of characters.
//! Inspired by code from slashdot and apple open source
//! https://stackoverflow.com/questions/5820810/case-insensitive-string-comparison-in-c
//! https://opensource.apple.com/source/tcl/tcl-10/tcl/compat/strncasecmp.c.auto.html
//! @param	string_1: the first string to compare.
//! @param	string_2: the second string to compare.
//! @param	max_len: the maximum number of characters to compare. Even if both strings are larger than this number, only this many characters will be compared.
//! @return	Returns 0 if the strings are equivalent (at least up to max_len). Returns a negative or positive if the strings are different.
int16_t General_Strncasecmp(const char* string_1, const char* string_2, size_t max_len)
{
	//DEBUG_OUT(("%s %d: s1='%s'; s2='%s'; max_len=%i", __func__ , __LINE__, string_1, string_2, max_len));

	for (; max_len != 0; max_len--, string_1++, string_2++)
	{
		uint8_t	u1 = (uint8_t)*string_1;
		uint8_t	u2 = (uint8_t)*string_2;
		
		if (General_ToLower(u1) != General_ToLower(u2))
		{
			return General_ToLower(u1) - General_ToLower(u2);
		}
		
		if (u1 == '\0')
		{
			return 0;
		}
	}
	
	return 0;	
}


//! Measure the length of a fixed-size string
//! Safe(r) strlen function: will stop processing if no terminator found before max_len reached
// Inspired by apple/bsd strnlen.
//! @return	Returns strlen(the_string), if that is less than max_len, or max_len if there is no null terminating ('\0') among the first max_len characters pointed to by the_string.
signed long General_Strnlen(const char* the_string, size_t max_len)
{
	signed long	len;
 	
	for (len = 0; len < max_len; len++, the_string++)
	{
		if (!*the_string)
		{
			break;
		}
	}

	return (len);
}





// **** RECTANGLE UTILITIES *****













// **** FILENAME AND FILEPATH UTILITIES *****

// // allocate and return the portion of the path passed, minus the filename. In other words: return a path to the parent file.
// // calling method must free the string returned
// char* General_ExtractPathToParentFolderWithAlloc(const char* the_file_path)
// {
// 	// LOGIC: 
// 	//   PathPart includes the : if non-name part is for a volume. but doesn't not include trailing / if not a volume
// 	//   we want in include the trailing : and /, so calling routine can always just append a file name and get a legit path
// 	
// 	signed long	path_len;
// 	char*			the_directory_path;
// 
// 	// get a string for the directory portion of the filepath
// 	if ( (the_directory_path = (char*)calloc(FILE_MAX_PATHNAME_SIZE, sizeof(char)) ) == NULL)
// 	{
// 		LOG_ERR(("%s %d: could not allocate memory for the directory path", __func__ , __LINE__));
// 		return NULL;
// 	}
// 	
// 	path_len = (General_PathPart(the_file_path) - the_file_path) - 1;
// 	
// 	//DEBUG_OUT(("%s %d: pathlen=%lu; last char='%c'", __func__ , __LINE__, path_len, the_file_path[path_len]));
// 
// 	if (the_file_path[path_len] != ':')
// 	{
// 		// path wasn't to root of a volume, move 1 tick to the right to pick up the / that is already in the full path
// 		path_len++;
// 	}
// 
// 	path_len++;
// 
// 	General_Strlcpy(the_directory_path, the_file_path, path_len + 1);
// 	//DEBUG_OUT(("%s %d: pathlen=%lu; parent path='%s'", __func__ , __LINE__, path_len, the_directory_path));
// 	
// 	return the_directory_path;
// }


// allocate and return the filename portion of the path passed.
// calling method must free the string returned
char* General_ExtractFilenameFromPathWithAlloc(const char* the_file_path)
{
	char*	the_file_name;

	// get a string for the file name portion of the filepath
	if ( (the_file_name = (char*)calloc(FILE_MAX_PATHNAME_SIZE, sizeof(char)) ) == NULL)
	{
		LOG_ERR(("%s %d: could not allocate memory for the filename", __func__ , __LINE__));
		return NULL;
	}
	else
	{
		char*	the_file_name_part = General_NamePart(the_file_path);
		int16_t	filename_len = General_Strnlen(the_file_name_part, FILE_MAX_PATHNAME_SIZE);

		if (filename_len == 0)
		{
			// FilePart() might return a string with no text: that would indicate the file path is for the root of a file system or virtual device
			// in that case, we just use the file path minus : as the name

			// copy the part of the path minus the last char into the file name
			int16_t		path_len = General_Strnlen(the_file_path, FILE_MAX_PATHNAME_SIZE);
			General_Strlcpy(the_file_name, the_file_path, path_len);
		}
		else
		{
			General_Strlcpy(the_file_name, the_file_name_part, filename_len + 1);
		}
		LOG_ALLOC(("%s %d:	__ALLOC__	the_file_name	%p	size	%i", __func__ , __LINE__, the_file_name, FILE_MAX_PATHNAME_SIZE));
	}

	return the_file_name;
}


// populates the passed string by safely combining the passed file path and name, accounting for cases where path is a disk root
void General_CreateFilePathFromFolderAndFile(char* the_combined_path, char* the_folder_path, char* the_file_name)
{
	
	General_Strlcpy(the_combined_path, the_folder_path, FILE_MAX_PATHNAME_SIZE);

	// if the filename passed was empty, just return the original folder path. 
	//   otherwise you end up with "mypath" and file "" = "mypath/", which is bad. 
	if (the_file_name[0] == '\0')
	{
		return;
	}
	
	General_Strlcat(the_combined_path, the_file_name, FILE_MAX_PATHNAME_SIZE);

	//DEBUG_OUT(("%s %d: file '%s' and folder '%s' produces path of '%s'", __func__ , __LINE__, the_file_name, the_folder_path, the_combined_path));
}


// return the first char of the last part of a file path
// if no path part detected, returns the original string
// not guaranteed that this is a FILENAME, as if you passed a path to a dir, it would return the DIR name
// amigaDOS compatibility function (see FilePart)
char* General_NamePart(const char* the_file_path)
{
	char*	last_slash;
	
	last_slash = strchr(the_file_path, '/');
	
	if (last_slash && ++last_slash)
	{
		return last_slash;
	}
	
	return (char*)the_file_path;
}


// return everything to the left of the filename in a path. 
// amigaDOS compatibility function
char* General_PathPart(const char* the_file_path)
{
	char*	the_directory_path;
	char*	this_point;
	
	this_point = (char*)the_file_path;
	the_directory_path = this_point; // default to returning start of the string
	
	while (*this_point)
	{
		if (*this_point == '/')
		{
			the_directory_path = this_point;
		}
		
		this_point++;
	}
	
	return the_directory_path;
}


//! Extract file extension into the passed char pointer, as new lowercased string pointer, if any found.
//! @param	the_file_name: the file name to extract an extension from
//! @param	the_extension: a pre-allocated buffer that will contain the extension, if any is detected. Must be large enough to hold the extension! No bounds checking is done. 
//! @return	Returns false if no file extension found.
bool General_ExtractFileExtensionFromFilename(const char* the_file_name, char* the_extension)
{
	// LOGIC: 
	//   if the first char is the first dot from right, we'll count the whole thing as an extension
	//   if no dot char, then don't set extension, and return false
	
    char*	dot = strrchr((char*)the_file_name, '.');
    int16_t	i;

    // (re) set the file extension to "" in case we have to return. It may have a value from whatever previous use was
    the_extension[0] = '\0';

	if(!dot)
    {
    	return false;
    }

	for (i = 1; dot[i]; i++)
	{
		the_extension[i-1] = General_ToLower(dot[i]);
	}

	the_extension[i-1] = '\0';

	return true;
}


// return a human-readable(ish) string for the filetype of the filetype ID passed - no allocation
// see cbm_filetype.h
char* General_GetFileTypeString(uint8_t cbm_filetype_id)
{
	switch (cbm_filetype_id)
	{
// 		case _CBM_T_HEADER:
// 			return General_GetString(ID_STR_FILETYPE_DIR);
// 		
// 		case _CBM_T_SEQ:
// 			return General_GetString(ID_STR_FILETYPE_SEQ);
// 		
// 		case _CBM_T_PRG:
// 			return General_GetString(ID_STR_FILETYPE_PRG);
// 		
// 		case _CBM_T_REL:
// 			return General_GetString(ID_STR_FILETYPE_REL);
// 		
// 		case _CBM_T_DEL:
// 		case _CBM_T_DIR:
// 			return General_GetString(ID_STR_FILETYPE_SUBDIR);
		
		default:
			return General_GetString(ID_STR_FILETYPE_OTHER);
	}
// #define _CBM_T_REG      0x10U   /* Bit set for regular files */
// #define _CBM_T_SEQ      0x10U
// #define _CBM_T_PRG      0x11U
// #define _CBM_T_USR      0x12U
// #define _CBM_T_REL      0x13U
// #define _CBM_T_VRP      0x14U   /* Vorpal fast-loadable format */
// #define _CBM_T_DEL      0x00U
// #define _CBM_T_CBM      0x01U   /* 1581 sub-partition */
// #define _CBM_T_DIR      0x02U   /* IDE64 and CMD sub-directory */
// #define _CBM_T_LNK      0x03U   /* IDE64 soft-link */
// #define _CBM_T_OTHER    0x04U   /* File-type not recognized */
// #define _CBM_T_HEADER   0x05U   /* Disk header / title */
}




// **** TIME UTILITIES *****


//! Wait for the specified number of ticks before returning
//! In PET/B128 implementation, we don't bother with real ticks.
void General_DelayTicks(uint16_t ticks)
{
	uint16_t	i;
	uint16_t	j;
	
	for (i = 0; i < ticks; i++)
	{
		for (j = 0; j < 5; j++);
	}
}


// //! Wait for the specified number of seconds before returning
// //! In multi-tasking ever becomes a thing, this is not a multi-tasking-friendly operation. 
// void General_DelaySeconds(uint16_t seconds)
// {
// 	long	start_ticks = sys_time_jiffies();
// 	long	now_ticks = start_ticks;
// 	
// 	while ((now_ticks - start_ticks) / SYS_TICKS_PER_SEC < seconds)
// 	{
// 		now_ticks = sys_time_jiffies();
// 	}
// }








// **** USER INPUT UTILITIES *****

// // Wait for one character from the keyboard and return it
// char General_GetChar(void)
// {
// 	uint8_t		the_char;
// 	
// #ifdef _C256_FMX_
// 	char	(*Kernal_GetCharWithWait)(void);
// 	//GETCHW	$00:104C	Get a character from the input channel. Waits until data received. A=0 and Carry=1 if no data is waiting
// 	Kernal_GetCharWithWait = (void*)(0x00104c);
// 	
// 	the_char = Kernal_GetCharWithWait();
// #else
// 	the_char = getchar();
// #endif
// 
// 	return the_char;
// }





// **** MISC UTILITIES *****








// **** LOGGING AND DEBUG UTILITIES *****


#if defined LOG_LEVEL_1 || defined LOG_LEVEL_2 || defined LOG_LEVEL_3 || defined LOG_LEVEL_4 || defined LOG_LEVEL_5


// DEBUG functionality I want:
//   3 levels of logging (err/warn/info)
//   additional debug out function that leaves no footprint in compiled release version of code (calls to it also disappear)
//   able to pass format string and multiple variables when needed

void General_LogError(const char* format, ...)
{
	va_list		args;
	
	va_start(args, format);
	vsprintf(debug_buffer, format, args);
	va_end(args);

#ifdef _SIMULATOR_
	// f256jr emulator has a log to console feature:
	// *((long *)-4) = (long)&debug_buffer;
// 	__AX__ = &debug_buffer;
// 	asm("cb $300");
	Memory_DebugOut(); // calls a simple assembly routine that does "CB &debug_buffer". cc65's inline assembler only accepts valid 6502 opcodes.
#else
	// if not on simulator, write out to file
	fprintf(global_log_file, "%s %s\n", kDebugFlag[LogError], debug_buffer);
// 	printf("%s %s\r", kDebugFlag[LogError], debug_string);
#endif
}

void General_LogWarning(const char* format, ...)
{
	va_list		args;
	
	va_start(args, format);
	vsprintf(debug_buffer, format, args);
	va_end(args);

#ifdef _SIMULATOR_
	// f256jr emulator has a log to console feature:
	Memory_DebugOut(); // calls a simple assembly routine that does "CB &debug_buffer". cc65's inline assembler only accepts valid 6502 opcodes.
#else
	// if not on simulator, write out to file
	fprintf(global_log_file, "%s %s\n", kDebugFlag[LogWarning], debug_buffer);
// 	printf("%s %s\r", kDebugFlag[LogWarning], debug_string);
#endif
}

void General_LogInfo(const char* format, ...)
{
	va_list		args;
	
	va_start(args, format);
	vsprintf(debug_buffer, format, args);
	va_end(args);

#ifdef _SIMULATOR_
	// f256jr emulator has a log to console feature:
	Memory_DebugOut(); // calls a simple assembly routine that does "CB &debug_buffer". cc65's inline assembler only accepts valid 6502 opcodes.
#else
	// if not on simulator, write out to file
	fprintf(global_log_file, "%s %s\n", kDebugFlag[LogInfo], debug_buffer);
// 	printf("%s %s\r", kDebugFlag[LogInfo], debug_string);
#endif
}

void General_DebugOut(const char* format, ...)
{
	va_list		args;
	
	va_start(args, format);
	vsprintf(debug_buffer, format, args);
	va_end(args);
	
#ifdef _SIMULATOR_
	// f256jr emulator has a log to console feature:
	Memory_DebugOut(); // calls a simple assembly routine that does "CB &debug_buffer". cc65's inline assembler only accepts valid 6502 opcodes.
#else
	// if not on simulator, write out to file
	fprintf(global_log_file, "%s %s\n", kDebugFlag[LogDebug], debug_buffer);
// 	printf("%s %s\r", kDebugFlag[LogDebug], debug_string);
#endif
}

void General_LogAlloc(const char* format, ...)
{
	va_list		args;
	
	va_start(args, format);
	vsprintf(debug_buffer, format, args);
	va_end(args);
	
#ifdef _SIMULATOR_
	// f256jr emulator has a log to console feature:
	Memory_DebugOut(); // calls a simple assembly routine that does "CB &debug_buffer". cc65's inline assembler only accepts valid 6502 opcodes.
#else
	// if not on simulator, write out to file
	fprintf(global_log_file, "%s %s\n", kDebugFlag[LogAlloc], debug_buffer);
// 	printf("%s %s\r", kDebugFlag[LogAlloc], debug_string);
#endif
}

// initialize log file
// globals for the log file
bool General_LogInitialize(void)
{
	const char*		the_file_path = "fmanager_log.txt";

	global_log_file = fopen( the_file_path, "w");
	
	if (global_log_file == NULL)
	{
		printf("General_LogInitialize: log file could not be opened! \n");
		return false;
	}
	
	return true;
}

// close the log file
void General_LogCleanUp(void)
{
	if (global_log_file != NULL)
	{
		fclose(global_log_file);
	}
}

#endif
