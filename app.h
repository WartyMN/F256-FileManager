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

// hide __fastcall_ from everything but CC65 (to squash some warnings in LSP/BBEdit)
#ifndef __CC65__
	#define __fastcall__ 
#endif

#ifndef NULL
#  define NULL 0
#endif

#define MAJOR_VERSION	0
#define MINOR_VERSION	1
#define UPDATE_VERSION	6

#define VERSION_NUM_X	0
#define VERSION_NUM_Y	24


// there are 2 panels which can be accessed with the same code
#define NUM_PANELS					2
#define PANEL_ID_LEFT				0
#define PANEL_ID_RIGHT				1


#define APP_DIALOG_WIDTH			40
#define APP_DIALOG_HEIGHT			7
#define APP_DIALOG_BUFF_SIZE		((APP_DIALOG_WIDTH+2) * (APP_DIALOG_HEIGHT+2))	// for the temp char and color buffs when opening a window, this is how much mem we'll reserve for each

#define FILE_MAX_FILENAME_SIZE		(16+1)	// CBM DOS defined
#define FILE_MAX_PATHNAME_SIZE		(2+FILE_MAX_FILENAME_SIZE)	// not sure what defines this in our case. ?? current logic is 2 char drive (0:, 1:, 2:) + 16 char max file name for IEC. but fat32 supports much longer names... 
#define FILE_MAX_APPFILE_INFO_SIZE	255		// for info panel display about mime/app type, the max # of bytes to display
#define FILE_MAX_TEXT_PREVIEW_SIZE	255		// for info panel previews, the max # of bytes to read in and display
#define FILE_TYPE_MAX_SIZE_NAME		4	// mostly PRG/REL, but allowed for SUBD
#define FILE_SIZE_MAX_SIZE			16	// max size of human-readable file size. eg, "255 blocks", "1,200 MB"
#define FILE_BYTES_PER_BLOCK		254	// 1 block = 256b but really only 254
#define MAX_NUM_FILES_IEC			144 // The directory track should be contained totally on track 18. Sectors 1-18 contain the entries and sector 0 contains the BAM (Block Availability Map) and disk name/ID. Since the directory is only 18 sectors large (19 less one for the BAM), and each sector can contain only 8 entries (32 bytes per entry), the maximum number of directory entries is 18 * 8 = 144. http://justsolve.archiveteam.org/wiki/CBMFS
// BUT... 1581 supported 296 entries. hmm. 
#define FILE_COPY_BUFFER_SIZE		384	// arbitrarily set to match global temp buff 384b.


#define DEVICE_LOWEST_DEVICE_NUM	0
#define DEVICE_HIGHEST_DEVICE_NUM	2
#define DEVICE_MAX_DEVICE_COUNT		(DEVICE_HIGHEST_DEVICE_NUM - DEVICE_LOWEST_DEVICE_NUM + 1)


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
#define STORAGE_STRING_MERGE_BUFFER		global_temp_buff_192b_1		// match to PET tape #1 area above - 192 bytes
#define STORAGE_STRING_MERGE_BUFFER2	global_temp_buff_192b_2		// match to PET tape #2 buffer - 192 bytes

#define STORAGE_STRING_MERGE_BUFFER_SIZE	192 // will use for snprintf, strncpy, etc.

// secondary uses of storage areas



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

#define DIALOG_FOREGROUND_COLOR		COLOR_DARK_GRAY
#define DIALOG_BACKGROUND_COLOR		COLOR_LIGHT_GRAY
#define DIALOG_ACCENT_COLOR			COLOR_MEDIUM_GRAY
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
#define ACTION_CANCEL_ALT			CH_SPACE
#define ACTION_CONFIRM				CH_ENTER
#define ACTION_QUIT					'q'
#define ACTION_SORT_BY_NAME			'N'	// CH_F6
#define ACTION_SORT_BY_DATE			'D'	// CH_F7
#define ACTION_SORT_BY_SIZE			'S'	// CH_F8
#define ACTION_SORT_BY_TYPE			'T'	// CH_F9
#define ACTION_VIEW_AS_HEX			'h'
#define ACTION_VIEW_AS_TEXT			't'
#define ACTION_RENAME				'r'
#define ACTION_LAUNCH				'l'

#define ACTION_NEXT_DEVICE			'n'	// CH_F1
#define ACTION_REFRESH_PANEL		'f'	// CH_F2
#define ACTION_FORMAT_DISK			'`'	// CH_F3

#define ACTION_HELP				'?' // numpad key



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

/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/


// display error message, wait for user to confirm, and exit
void App_Exit(uint8_t the_error_number);




#endif /* FILE_MANAGER_H_ */
