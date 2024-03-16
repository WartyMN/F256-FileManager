/*
 * app.h
 *
 *  Created on: Jan 10, 2023
 *      Author: micahbly
 *
 *  A pseudo commander-style 2-column file manager
 *
 */
 
#ifndef FILE_MANAGER_H_
#define FILE_MANAGER_H_

/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/


// project includes

// C includes
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
//#include <string.h>



/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/

#define PARAM_COPY_TO_EM			true	// App_EMDataCopyDMA() parameter
#define PARAM_COPY_FROM_EM			false	// App_EMDataCopyDMA() parameter

// hide __fastcall_ from everything but CC65 (to squash some warnings in LSP/BBEdit)
#ifndef __CC65__
	#define __fastcall__ 
#endif

#ifndef NULL
#  define NULL 0
#endif

#define MAJOR_VERSION	0
#define MINOR_VERSION	1
#define UPDATE_VERSION	22

#define VERSION_NUM_X	0
#define VERSION_NUM_Y	24


// there are 2 panels which can be accessed with the same code
#define NUM_PANELS					2
#define PANEL_ID_LEFT				0
#define PANEL_ID_RIGHT				1


#define APP_DIALOG_WIDTH			42
#define APP_DIALOG_HEIGHT			7
#define APP_DIALOG_BUFF_SIZE		((APP_DIALOG_WIDTH+2) * (APP_DIALOG_HEIGHT+2))	// for the temp char and color buffs when opening a window, this is how much mem we'll reserve for each

#define FILE_MAX_FILENAME_SIZE_CBM	(16+1)	// CBM DOS defined
#define FILE_MAX_FILENAME_SIZE		(40+1)	// in F256 kernel, total path can't be longer than 255 chars.
#define FILE_MAX_PATHNAME_SIZE		(255)	// in F256 kernel, total path can't be longer than 255 chars.
#define FILE_MAX_APPFILE_INFO_SIZE	255		// for info panel display about mime/app type, the max # of bytes to display
#define FILE_MAX_TEXT_PREVIEW_SIZE	255		// for info panel previews, the max # of bytes to read in and display
#define FILE_TYPE_MAX_SIZE_NAME		4	// mostly PRG/REL, but allowed for SUBD
#define FILE_SIZE_MAX_SIZE			16	// max size of human-readable file size. eg, "255 blocks", "1,200 MB"
#define FILE_BYTES_PER_BLOCK_IEC	254	// for CBM DOS, 1 block = 256b but really only 254
#define FILE_BYTES_PER_BLOCK		256	// for FAT32, 1 block = 256b
#define MAX_NUM_FILES_IEC			144 // The directory track should be contained totally on track 18. Sectors 1-18 contain the entries and sector 0 contains the BAM (Block Availability Map) and disk name/ID. Since the directory is only 18 sectors large (19 less one for the BAM), and each sector can contain only 8 entries (32 bytes per entry), the maximum number of directory entries is 18 * 8 = 144. http://justsolve.archiveteam.org/wiki/CBMFS
// BUT... 1581 supported 296 entries. hmm. 


#define DEVICE_LOWEST_DEVICE_NUM	0
#define DEVICE_HIGHEST_DEVICE_NUM	2
#define DEVICE_MAX_DEVICE_COUNT		(DEVICE_HIGHEST_DEVICE_NUM - DEVICE_LOWEST_DEVICE_NUM + 1)

#define NO_DISK_PRESENT_FILE_NAME	31	// this is a char I see reported as a "islbl" by tool when scanning a floppy disk drive with no disk in it. 
#define NO_DISK_PRESENT_ANYMORE_FILE_NAME	28	// this is a char I see reported as a "islbl" by tool when scanning a floppy disk drive with no disk in it, when previously it was scanned with a disk in it.  

#define MEM_DUMP_BYTES_PER_ROW		16
#define MAX_MEM_DUMP_LEN			(24 * MEM_DUMP_BYTES_PER_ROW)	// 24*16 = 384
#define MEM_DUMP_START_X_FOR_HEX	7
#define MEM_DUMP_START_X_FOR_CHAR	(MEM_DUMP_START_X_FOR_HEX + MEM_DUMP_BYTES_PER_ROW * 3)

#define MEM_TEXT_VIEW_BYTES_PER_ROW	80
#define MAX_TEXT_VIEW_ROWS_PER_PAGE	(60-1)	// allow 1 line for instructions at top
#define MAX_TEXT_VIEW_LEN			(MAX_TEXT_VIEW_ROWS_PER_PAGE * MEM_TEXT_VIEW_BYTES_PER_ROW)	// 51*80 = 4080



/*****************************************************************************/
/*                             MEMORY LOCATIONS                              */
/*****************************************************************************/

// temp storage for data outside of normal cc65 visibility - extra memory!
#define STORAGE_GETSTRING_BUFFER			0x0400	// interbank buffer to store individual strings retrieved from EM
#define STORAGE_GETSTRING_BUFFER_LEN		256	// 1-page buffer. see cc65 memory config file. this is outside cc65 space.
#define STORAGE_FILE_BUFFER_1				0x0500	// interbank buffer for file reading operations
#define STORAGE_FILE_BUFFER_1_LEN			256	// 1-page buffer. see cc65 memory config file. this is outside cc65 space.
#define STORAGE_STRING_BUFFER_1				(STORAGE_FILE_BUFFER_1 + STORAGE_FILE_BUFFER_1_LEN)	// temp string merge/etc buff
#define STORAGE_STRING_BUFFER_1_LEN			204	// 204b buffer. see cc65 memory config file. this is outside cc65 space.
#define STORAGE_STRING_BUFFER_2				(STORAGE_STRING_BUFFER_1 + STORAGE_STRING_BUFFER_1_LEN)	// temp string merge/etc buff
#define STORAGE_STRING_BUFFER_1_LEN			204	// 204b buffer. 
#define STORAGE_TEMP_UNUSED_1B				(STORAGE_STRING_BUFFER_2 + STORAGE_STRING_BUFFER_2_LEN)	// 799 is hard coded, so this is just noting that we have 1 unused byte here.


#define STRING_STORAGE_SLOT                0x06
#define STRING_STORAGE_VALUE               0x12
#define STRING_STORAGE_PHYS_ADDR           0x24000



/*****************************************************************************/
/*                           App-wide color choices                          */
/*****************************************************************************/

#define APP_FOREGROUND_COLOR		COLOR_BRIGHT_BLUE
#define APP_BACKGROUND_COLOR		COLOR_BLACK
#define APP_ACCENT_COLOR			COLOR_BLUE

#define BUFFER_FOREGROUND_COLOR		COLOR_CYAN
#define BUFFER_BACKGROUND_COLOR		COLOR_BLACK
#define BUFFER_ACCENT_COLOR			COLOR_BRIGHT_BLUE

#define PANEL_FOREGROUND_COLOR		COLOR_GREEN
#define PANEL_BACKGROUND_COLOR		COLOR_BLACK
#define PANEL_ACCENT_COLOR			COLOR_BRIGHT_BLUE

#define LIST_ACTIVE_COLOR			COLOR_BRIGHT_GREEN
#define LIST_INACTIVE_COLOR			COLOR_GREEN

#define LIST_HEADER_COLOR			COLOR_BRIGHT_WHITE

#define MENU_FOREGROUND_COLOR		COLOR_CYAN
#define MENU_INACTIVE_COLOR			COLOR_BRIGHT_BLUE
#define MENU_BACKGROUND_COLOR		COLOR_BLACK
#define MENU_ACCENT_COLOR			COLOR_BRIGHT_BLUE

#define DIALOG_FOREGROUND_COLOR		COLOR_BRIGHT_YELLOW
#define DIALOG_BACKGROUND_COLOR		COLOR_BLUE
#define DIALOG_ACCENT_COLOR			COLOR_BRIGHT_BLUE
#define DIALOG_AFFIRM_COLOR			COLOR_GREEN
#define DIALOG_CANCEL_COLOR			COLOR_RED

#define FILE_CONTENTS_FOREGROUND_COLOR	COLOR_BRIGHT_GREEN
#define FILE_CONTENTS_BACKGROUND_COLOR	COLOR_BLACK
#define FILE_CONTENTS_ACCENT_COLOR		COLOR_GREEN


/*****************************************************************************/
/*                                   Command Keys                            */
/*****************************************************************************/


// key codes for user input
#define ACTION_INVALID_INPUT	255	// this will represent illegal keyboard command by user

// primary keys: designed for use on B128/C128 numpad
#define MOVE_UP						'8'
#define MOVE_RIGHT					'6'
#define MOVE_DOWN					'2'
#define MOVE_LEFT					'4'
// alternative keys: designed for use on standard keyboard (e.g, on laptop running an emulator)
#define MOVE_UP_ALT					CH_CURS_UP
#define MOVE_RIGHT_ALT				CH_CURS_RIGHT
#define MOVE_DOWN_ALT				CH_CURS_DOWN
#define MOVE_LEFT_ALT				CH_CURS_LEFT

#define ACTION_SELECT				CH_ENTER // numpad key
#define ACTION_SWAP_ACTIVE_PANEL	CH_TAB
#define ACTION_DELETE				CH_DEL
#define ACTION_DELETE_ALT			'x'
#define ACTION_COPY					'c'
#define ACTION_DUPLICATE			'd'
#define ACTION_CANCEL				CH_ESC
#define ACTION_CANCEL_ALT			CH_RUNSTOP
#define ACTION_CONFIRM				CH_ENTER
#define ACTION_SORT_BY_NAME			'N'	// CH_F6
#define ACTION_SORT_BY_DATE			'D'	// CH_F7
#define ACTION_SORT_BY_SIZE			'S'	// CH_F8
#define ACTION_SORT_BY_TYPE			'T'	// CH_F9
#define ACTION_VIEW_AS_HEX			'h'
#define ACTION_VIEW_AS_TEXT			't'
#define ACTION_RENAME				'r'
#define ACTION_LOAD					'l'
#define ACTION_NEW_FOLDER			'm' // for "mkdir"

#define ACTION_NEXT_DEVICE			'n'	// CH_F1
#define ACTION_REFRESH_PANEL		'f'	// CH_F2
#define ACTION_FORMAT_DISK			'`'	// CH_F3

#define ACTION_SET_TIME				'C' // c for set CLOCK
#define ACTION_ABOUT				'a' 
#define ACTION_EXIT_TO_BASIC		'b'
#define ACTION_EXIT_TO_DOS			's'
#define ACTION_QUIT					'q'

#define ACTION_HELP					'?' // numpad key



/*****************************************************************************/
/*                                 Error Codes                               */
/*****************************************************************************/

#define ERROR_NO_ERROR													0
#define ERROR_NO_FILES_IN_FILE_LIST										101
#define ERROR_PANEL_WAS_NULL											102
#define ERROR_PANEL_ROOT_FOLDER_WAS_NULL								103
#define ERROR_PANEL_TARGET_FOLDER_WAS_NULL								104
#define ERROR_FOLDER_WAS_NULL											105
#define ERROR_FILE_WAS_NULL												106
#define ERROR_COULD_NOT_OPEN_DIR										107
#define ERROR_COULD_NOT_CREATE_NEW_FILE_OBJECT							108
#define ERROR_FOLDER_FILE_WAS_NULL										109
#define ERROR_NO_CONNECTED_DRIVES_FOUND									110
#define ERROR_FILE_TO_DESTROY_WAS_NULL									111
#define ERROR_DESTROY_ALL_FOLDER_WAS_NULL								112
#define ERROR_FILE_DUPLICATE_FAILED										113
#define ERROR_FOLDER_TO_DESTROY_WAS_NULL								114
#define ERROR_SET_CURR_ROW_FOLDER_WAS_NULL								115
#define ERROR_GET_CURR_ROW_FOLDER_WAS_NULL								116
#define ERROR_SET_FILE_SEL_BY_ROW_PANEL_WAS_NULL						117
#define ERROR_FILE_MARK_SELECTED_FILE_WAS_NULL							118
#define ERROR_FILE_MARK_UNSELECTED_FILE_WAS_NULL						119
#define ERROR_PANEL_INIT_FOLDER_WAS_NULL								120
#define ERROR_COPY_FILE_SOURCE_FOLDER_WAS_NULL							121
#define ERROR_COPY_FILE_TARGET_FOLDER_WAS_NULL							122
#define ERROR_POPULATE_FILES_FOLDER_WAS_NULL							123
#define ERROR_COULD_NOT_CREATE_ROOT_FOLDER_FILE_LEFT					124
#define ERROR_COULD_NOT_CREATE_ROOT_FOLDER_OBJ_LEFT						125
#define ERROR_COULD_NOT_CREATE_ROOT_FOLDER_FILE_RIGHT					126
#define ERROR_COULD_NOT_CREATE_ROOT_FOLDER_OBJ_RIGHT					127

#define ERROR_DEFINE_ME													255



/*****************************************************************************/
/*                                  Overlays                                 */
/*****************************************************************************/

#define NUM_OVERLAYS			9

#define OVERLAY_SCREEN			0
#define OVERLAY_FOLDER			1
#define OVERLAY_EM				2
#define OVERLAY_STARTUP			3
#define OVERLAY_COMBAT			4
#define OVERLAY_INVENTORY		5
#define OVERLAY_GAMEOVER		6
#define OVERLAY_NOTICE_BOARD	7
#define OVERLAY_CREATE_CAVERN	8

#define OVERLAY_1_SLOT                     0x05
#define OVERLAY_1_VALUE                    0x08
#define OVERLAY_2_SLOT                     0x05
#define OVERLAY_2_VALUE                    0x09
#define OVERLAY_3_SLOT                     0x05
#define OVERLAY_3_VALUE                    0x0A
#define OVERLAY_4_SLOT                     0x05
#define OVERLAY_4_VALUE                    0x0B
#define OVERLAY_5_SLOT                     0x05
#define OVERLAY_5_VALUE                    0x0C
#define OVERLAY_6_SLOT                     0x05
#define OVERLAY_6_VALUE                    0x0D
#define OVERLAY_7_SLOT                     0x05
#define OVERLAY_7_VALUE                    0x0E
#define OVERLAY_8_SLOT                     0x05
#define OVERLAY_8_VALUE                    0x0F
#define OVERLAY_9_SLOT                     0x05
#define OVERLAY_9_VALUE                    0x10
#define OVERLAY_10_RESERVED_SLOT           0x05
#define OVERLAY_10_RESERVED_VALUE          0x11

#define OVERLAY_START_ADDR					0xA000	// in CPU memory space, the start of overlay memory
#define OVERLAY_SLOT						0x05
#define IO_SLOT								0x06

#define CUSTOM_FONT_PHYS_ADDR              0x3A000	// temporary buffer for loading in a font?
#define CUSTOM_FONT_SLOT                   0x05
#define CUSTOM_FONT_VALUE                  0x1D

/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/

// typedef enum file_list_scope
// {
// 	LIST_SCOPE_ALL = 0,
// 	LIST_SCOPE_SELECTED = 1,
// 	LIST_SCOPE_NOT_SELECTED = 2,
// } file_list_scope;

#define LIST_SCOPE_ALL				0
#define LIST_SCOPE_SELECTED			1
#define LIST_SCOPE_NOT_SELECTED		2

/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/

// also defined in f256.h

// typedef struct DateStamp {
//    int32_t	ds_Days;	      /* Number of days since Jan. 1, 1978 */
//    int32_t	ds_Minute;	      /* Number of minutes past midnight */
//    int32_t	ds_Tick;	      /* Number of ticks past minute */
// } DateStamp; /* DateStamp */

typedef struct Overlay
{
	uint8_t		lut_slot_;	// 08-0f, refers to ZP address
	uint8_t		lut_value_; // 00-80, refers to physical memory, each 8K bank goes up by 1
} Overlay;


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/

// Draws the progress bar frame on the screen
void App_ShowProgressBar(void);

// Hides the progress bar frame on the screen
void App_HideProgressBar(void);

// draws the 'bar' part of the progress bar, according to a % complete passed (0-100 integer)
void App_UpdateProgressBar(uint8_t progress_bar_total);

// copy 256b chunks of data between specified 6502 addr and the fixed address range in EM, without bank switching
// chunk_num is used to calculate distance from the base EM address
// set to_em to true to copy from CPU space to EM, or false to copy from EM to specified CPU addr.
void App_EMDataCopyDMA(uint8_t* cpu_addr, uint8_t chunk_num, bool to_em);

// read the real time clock and display it
void App_DisplayTime(void);

// display error message, wait for user to confirm, and exit
void App_Exit(uint8_t the_error_number);

// Brings the requested overlay into memory
void App_LoadOverlay(uint8_t the_overlay_id);


#endif /* FILE_MANAGER_H_ */
