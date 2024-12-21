/*
 * screen.c
 *
 *  Created on: Jan 11, 2023
 *      Author: micahbly
 *
 *  Routines for drawing and updating the UI elements
 *
 */



/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "screen.h"
#include "app.h"
#include "comm_buffer.h"
#include "debug.h"
#include "file.h"
#include "folder.h"
#include "general.h"
#include "kernel.h"
#include "keyboard.h"
#include "memory.h"
#include "sys.h"
#include "text.h"
#include "strings.h"

// C includes
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// F256 includes
#include "f256.h"




/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/

#define UI_BYTE_SIZE_OF_APP_TITLEBAR	240	// 3 x 80 rows for the title at top


/*****************************************************************************/
/*                           File-scope Variables                            */
/*****************************************************************************/

#pragma data-name ("OVERLAY_SCREEN")

static File_Panel		panel[NUM_PANELS] =
{
	{PANEL_ID_LEFT,		DEVICE_ID_UNSET,	UI_LEFT_PANEL_BODY_X1,	UI_VIEW_PANEL_BODY_Y1,	UI_LEFT_PANEL_BODY_X2,	UI_VIEW_PANEL_BODY_Y2,	UI_VIEW_PANEL_BODY_WIDTH},
	{PANEL_ID_RIGHT,	DEVICE_ID_UNSET,	UI_RIGHT_PANEL_BODY_X1,	UI_VIEW_PANEL_BODY_Y1,	UI_RIGHT_PANEL_BODY_X2,	UI_VIEW_PANEL_BODY_Y2,	UI_VIEW_PANEL_BODY_WIDTH},
};

static UI_Button		uibutton[NUM_BUTTONS] =
{
	// DEVICE actions
	{BUTTON_ID_DEV_SD_CARD,		UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_DEV_CMD_Y,		ID_STR_DEV_SD,				UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_SWITCH_TO_SD	}, 
	{BUTTON_ID_DEV_FLOPPY_1,	UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_DEV_CMD_Y + 1,	ID_STR_DEV_FLOPPY_1,		UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_SWITCH_TO_FLOPPY_1	}, 
	{BUTTON_ID_DEV_FLOPPY_2,	UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_DEV_CMD_Y + 2,	ID_STR_DEV_FLOPPY_2,		UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_SWITCH_TO_FLOPPY_2	}, 
	{BUTTON_ID_DEV_RAM,			UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_DEV_CMD_Y + 3,	ID_STR_DEV_RAM,				UI_BUTTON_STATE_ACTIVE,		UI_BUTTON_STATE_CHANGED,	ACTION_SWITCH_TO_RAM	}, 
	{BUTTON_ID_DEV_FLASH,		UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_DEV_CMD_Y + 4,	ID_STR_DEV_FLASH,			UI_BUTTON_STATE_ACTIVE,		UI_BUTTON_STATE_CHANGED,	ACTION_SWITCH_TO_FLASH	}, 
	{BUTTON_ID_REFRESH,			UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_DEV_CMD_Y + 5,	ID_STR_DEV_REFRESH_LISTING,	UI_BUTTON_STATE_ACTIVE,		UI_BUTTON_STATE_CHANGED,	ACTION_REFRESH_PANEL	}, 
	{BUTTON_ID_FORMAT,			UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_DEV_CMD_Y + 6,	ID_STR_DEV_FORMAT,			UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_FORMAT_DISK	}, 
	// DIRECTORY actions
	{BUTTON_ID_MAKE_DIR,		UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_DIR_CMD_Y,		ID_STR_DEV_MAKE_DIR,		UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_NEW_FOLDER	}, 
	{BUTTON_ID_SORT_BY_TYPE,	UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_DIR_CMD_Y + 1,	ID_STR_DEV_SORT_BY_TYPE,	UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_SORT_BY_TYPE	}, 
	{BUTTON_ID_SORT_BY_NAME,	UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_DIR_CMD_Y + 2,	ID_STR_DEV_SORT_BY_NAME,	UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_SORT_BY_NAME	}, 
	{BUTTON_ID_SORT_BY_SIZE,	UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_DIR_CMD_Y + 3,	ID_STR_DEV_SORT_BY_SIZE,	UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_SORT_BY_SIZE	}, 
	// FILE actions
	{BUTTON_ID_COPY,			UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_FILE_CMD_Y,		ID_STR_FILE_COPY_RIGHT,		UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_COPY	}, 
	{BUTTON_ID_DELETE,			UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_FILE_CMD_Y + 1,	ID_STR_FILE_DELETE,			UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_DELETE_ALT	}, 
	{BUTTON_ID_DUPLICATE,		UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_FILE_CMD_Y + 2,	ID_STR_FILE_DUP,			UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_DUPLICATE	}, 
	{BUTTON_ID_RENAME,			UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_FILE_CMD_Y + 3,	ID_STR_FILE_RENAME,			UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_RENAME	}, 
	// FILE & BANK actions
	{BUTTON_ID_TEXT_VIEW,		UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_FILE_CMD_Y + 4,	ID_STR_FILE_TEXT_PREVIEW,	UI_BUTTON_STATE_ACTIVE,		UI_BUTTON_STATE_CHANGED,	ACTION_VIEW_AS_TEXT	}, 
	{BUTTON_ID_HEX_VIEW,		UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_FILE_CMD_Y + 5,	ID_STR_FILE_HEX_PREVIEW,	UI_BUTTON_STATE_ACTIVE,		UI_BUTTON_STATE_CHANGED,	ACTION_VIEW_AS_HEX	}, 
	{BUTTON_ID_LOAD,			UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_FILE_CMD_Y + 6,	ID_STR_FILE_LOAD,			UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_LOAD	}, 
	// BANK actions
	{BUTTON_ID_BANK_FILL,		UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_FILE_CMD_Y + 7,	ID_STR_BANK_FILL,			UI_BUTTON_STATE_ACTIVE,		UI_BUTTON_STATE_CHANGED,	ACTION_FILL_MEMORY	}, 
	{BUTTON_ID_BANK_CLEAR,		UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_FILE_CMD_Y + 8,	ID_STR_BANK_CLEAR,			UI_BUTTON_STATE_ACTIVE,		UI_BUTTON_STATE_CHANGED,	ACTION_CLEAR_MEMORY	}, 
	{BUTTON_ID_BANK_FIND,		UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_FILE_CMD_Y + 9,	ID_STR_BANK_FIND,			UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_SEARCH_MEMORY	}, 
	{BUTTON_ID_BANK_FIND_NEXT,	UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_FILE_CMD_Y + 10,	ID_STR_BANK_FIND_NEXT,		UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_SEARCH_MEMORY	}, 
	
	
	// APP actions
	{BUTTON_ID_SET_CLOCK,		UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_APP_CMD_Y,		ID_STR_APP_SET_CLOCK,		UI_BUTTON_STATE_ACTIVE,		UI_BUTTON_STATE_CHANGED,	ACTION_SET_TIME	}, 
	{BUTTON_ID_ABOUT,			UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_APP_CMD_Y + 1,	ID_STR_APP_ABOUT,			UI_BUTTON_STATE_ACTIVE,		UI_BUTTON_STATE_CHANGED,	ACTION_ABOUT	}, 
	{BUTTON_ID_EXIT_TO_BASIC,	UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_APP_CMD_Y + 2,	ID_STR_APP_EXIT_TO_BASIC,	UI_BUTTON_STATE_ACTIVE,		UI_BUTTON_STATE_CHANGED,	ACTION_EXIT_TO_BASIC	}, 
	{BUTTON_ID_EXIT_TO_DOS,		UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_APP_CMD_Y + 3,	ID_STR_APP_EXIT_TO_DOS,		UI_BUTTON_STATE_ACTIVE,		UI_BUTTON_STATE_CHANGED,	ACTION_EXIT_TO_DOS	}, 
	{BUTTON_ID_QUIT,			UI_MIDDLE_AREA_START_X,		UI_MIDDLE_AREA_APP_CMD_Y + 4,	ID_STR_APP_QUIT,			UI_BUTTON_STATE_INACTIVE,	UI_BUTTON_STATE_CHANGED,	ACTION_QUIT	}, 
};
 
static uint8_t			screen_titlebar[UI_BYTE_SIZE_OF_APP_TITLEBAR] = 
{
	148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,141,142,143,144,145,146,147,32,102,47,109,97,110,97,103,101,114,32,0x46,0x32,0x35,0x36,32,140,139,138,137,136,135,134,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,
};


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

extern int8_t				global_connected_device[DEVICE_MAX_DEVICE_COUNT];	// will be 8, 9, etc, if connected, or -1 if not..
extern bool					global_started_from_flash;		// tracks whether app started from flash or from disk
extern bool					global_find_next_enabled;

extern bool					global_clock_is_visible;		// tracks whether or not the clock should be drawn. set to false when not showing main 2-panel screen.

extern char*				global_string[NUM_STRINGS];
extern char*				global_string_buff1;
extern char*				global_string_buff2;

extern TextDialogTemplate	global_dlg;	// dialog we'll configure and re-use for different purposes
extern char					global_dlg_title[36];	// arbitrary
extern char					global_dlg_body_msg[70];	// arbitrary
extern char					global_dlg_button[3][10];	// arbitrary
extern uint8_t				temp_screen_buffer_char[APP_DIALOG_BUFF_SIZE];	// WARNING HBD: don't make dialog box bigger than will fit!
extern uint8_t				temp_screen_buffer_attr[APP_DIALOG_BUFF_SIZE];	// WARNING HBD: don't make dialog box bigger than will fit!

extern uint8_t				zp_bank_num;
extern uint8_t				io_bank_value_kernel;	// stores value for the physical bank pointing to C000-DFFF whenever we change it, so we can restore it.

#pragma zpsym ("zp_bank_num");


/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

void Screen_DrawUI(void);

// attempts to convert the passed character to a byte value by treating it as hex. 
// if not hex, it will return -1
int16_t ScreenConvertHexCharToByteValue(uint8_t the_char);


/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/

// clear screen and draw main UI
void Screen_DrawUI(void)
{
	uint8_t		i;
	uint8_t		x1;
	uint8_t		y1;
	uint8_t		x2;
	uint8_t		y2;
	
	Text_ClearScreen(APP_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);
	
	// draw the title bar at top. 3x80
	Text_CopyMemBoxLinearBuffer((uint8_t*)&screen_titlebar, 0, 0, 79, 2, PARAM_COPY_TO_SCREEN, PARAM_FOR_TEXT_CHAR);
	Text_FillBoxAttrOnly(0, 0, 79, 0, APP_ACCENT_COLOR, APP_BACKGROUND_COLOR);
	Text_FillBoxAttrOnly(0, 2, 79, 2, APP_ACCENT_COLOR, APP_BACKGROUND_COLOR);
	Text_SetXY(48,1);
	Text_Invert(6);	// right-hand side vertical bars need to be inversed to grow from thin to fat


	// draw panels
	for (i = 0; i < NUM_PANELS; i++)
	{
		x1 = panel[i].x1_;
		y1 = panel[i].y1_;
		x2 = panel[i].x2_;
		y2 = panel[i].y2_;
		Text_DrawBoxCoordsFancy(x1, y1 - 2, x1 + (UI_PANEL_TAB_WIDTH - 1), y1, PANEL_FOREGROUND_COLOR, PANEL_BACKGROUND_COLOR);
		Text_DrawBoxCoordsFancy(x1, y1, x2, y2, PANEL_FOREGROUND_COLOR, PANEL_BACKGROUND_COLOR);
		Text_SetCharAtXY(x1, y1, SC_T_RIGHT);
		Text_SetCharAtXY(x1 + (UI_PANEL_TAB_WIDTH - 1), y1, SC_T_UP);
	}
	
	// draw device menu
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_DEV_MENU_Y, UI_MIDDLE_AREA_WIDTH, CH_UNDERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
	Text_DrawStringAtXY(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_DEV_MENU_Y + 1, General_GetString(ID_STR_MENU_DEVICE), MENU_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_DEV_MENU_Y + 2, UI_MIDDLE_AREA_WIDTH, CH_OVERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
		
	// draw directory menu
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_DIR_MENU_Y, UI_MIDDLE_AREA_WIDTH, CH_UNDERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
	Text_DrawStringAtXY(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_DIR_MENU_Y + 1, General_GetString(ID_STR_MENU_DIRECTORY), MENU_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_DIR_MENU_Y + 2, UI_MIDDLE_AREA_WIDTH, CH_OVERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
		
	// draw file menu
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_FILE_MENU_Y, UI_MIDDLE_AREA_WIDTH, CH_UNDERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
	Text_DrawStringAtXY(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_FILE_MENU_Y + 1, General_GetString(ID_STR_MENU_FILE), MENU_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_FILE_MENU_Y + 2, UI_MIDDLE_AREA_WIDTH, CH_OVERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
		
	// draw app menu
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_APP_MENU_Y, UI_MIDDLE_AREA_WIDTH, CH_UNDERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
	Text_DrawStringAtXY(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_APP_MENU_Y + 1, General_GetString(ID_STR_MENU_APP), MENU_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_APP_MENU_Y + 2, UI_MIDDLE_AREA_WIDTH, CH_OVERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);


	// also draw the comms area
	Buffer_DrawCommunicationArea();
	Buffer_RefreshDisplay();
}


// attempts to convert the passed character to a byte value by treating it as hex. 
// if not hex, it will return -1
int16_t ScreenConvertHexCharToByteValue(uint8_t the_char)
{
	if (the_char > 47 && the_char < 58)
	{
		// decimal digit char
		the_char -= 48;
	}
	else if (the_char > 64 && the_char < 71)
	{
		// uppercase hex digit char
		the_char -= 55; // A-F = 10-15 values
	}
	else if (the_char > 96 && the_char < 103)
	{
		// lowercase hex digit char
		the_char -= 87; // A-F = 10-15 values
	}
	else
	{
		return -1;
	}
	
	return the_char;
}



/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/

// draw just the 3 column headers in the specified panel
// if for_disk is true, will use name/type/size. if false, will use name/bank num/addr
void Screen_DrawPanelHeader(uint8_t x, bool for_disk)
{
	uint8_t		y;

	y = UI_VIEW_PANEL_BODY_Y1 + 1;

	// important to clear header row because when switching between file/bank views, you get "BANKPADDRESS"
	Text_FillBox(x, y, x + UI_VIEW_PANEL_BODY_WIDTH - 3, y, CH_SPACE, LIST_HEADER_COLOR, PANEL_BACKGROUND_COLOR);

	if (for_disk == true)
	{
		Text_DrawStringAtXY(x, y, General_GetString(ID_STR_LBL_FILENAME), LIST_HEADER_COLOR, PANEL_BACKGROUND_COLOR);
		x += UI_PANEL_FILETYPE_OFFSET;
		Text_DrawStringAtXY(x, y, General_GetString(ID_STR_LBL_FILETYPE), LIST_HEADER_COLOR, PANEL_BACKGROUND_COLOR);
		x += UI_PANEL_FILESIZE_OFFSET;
		Text_DrawStringAtXY(x, y, General_GetString(ID_STR_LBL_FILESIZE), LIST_HEADER_COLOR, PANEL_BACKGROUND_COLOR);
	}
	else
	{
		Text_DrawStringAtXY(x, y, General_GetString(ID_STR_LBL_FILENAME), LIST_HEADER_COLOR, PANEL_BACKGROUND_COLOR);
		x += UI_PANEL_BANK_NUM_OFFSET;
		Text_DrawStringAtXY(x, y, General_GetString(ID_STR_LBL_BANK_NUM), LIST_HEADER_COLOR, PANEL_BACKGROUND_COLOR);
		x += UI_PANEL_BANK_ADDR_OFFSET;
		Text_DrawStringAtXY(x, y, General_GetString(ID_STR_LBL_BANK_ADDRESS), LIST_HEADER_COLOR, PANEL_BACKGROUND_COLOR);
	}
}


// Sets active/inactive for menu items whose active state only needs to be set once, on app startup
// does not render
void Screen_SetInitialMenuStates(uint8_t num_disk_systems)
{
	uint8_t		i;
	
	for (i=0; i < num_disk_systems; i++)
	{
		// technically, could just do something like "uibutton[global_connected_device[i]].active_ = true, but if the IDs change, it would break. and button IDs are not explicitly DESIGNED to be same as disk IDs.
		//DEBUG_OUT(("%s %d: for num_disk_systems=%u, global_connected_device[i]=%u", __func__ , __LINE__, num_disk_systems, global_connected_device[i]));

		if (global_connected_device[i] == 0)
		{
			uibutton[BUTTON_ID_DEV_SD_CARD].active_ = true;
			uibutton[BUTTON_ID_DEV_SD_CARD].changed_ = true;
		}
		else if (global_connected_device[i] == 1)
		{
			uibutton[BUTTON_ID_DEV_FLOPPY_1].active_ = true;
			uibutton[BUTTON_ID_DEV_FLOPPY_1].changed_ = true;
		}
		else if (global_connected_device[i] == 2)
		{
			uibutton[BUTTON_ID_DEV_FLOPPY_2].active_ = true;
			uibutton[BUTTON_ID_DEV_FLOPPY_2].changed_ = true;
		}
	}

	if (global_started_from_flash == true)
	{
		uibutton[BUTTON_ID_QUIT].active_ = false;
		uibutton[BUTTON_ID_QUIT].changed_ = true;
	}
	else
	{
		uibutton[BUTTON_ID_QUIT].active_ = true;
		uibutton[BUTTON_ID_QUIT].changed_ = true;
	}

	//DEBUG_OUT(("%s %d: start from flash=%u, quit button active=%u SD card active=%u", __func__ , __LINE__, global_started_from_flash, uibutton[BUTTON_ID_QUIT].active_, uibutton[BUTTON_ID_DEV_SD_CARD].active_));
}


// Get user input and vet it against the menu items that are currently enabled
// returns ACTION_INVALID_INPUT if the key pressed was for a disabled menu item
// returns the key pressed if it matched an enabled menu item, or if wasn't a known (to Screen) input. This lets App still allow for cursor keys, etc, which aren't represented by menu items
uint8_t Screen_GetValidUserInput(void)
{
	uint8_t		user_input;
	uint8_t		i;
	
	user_input = Keyboard_GetChar();

	// check input against active menu items
	for (i = 0; i < NUM_BUTTONS; i++)
	{
		//DEBUG_OUT(("%s %d: btn %i change=%u, active=%u, %s", __func__ , __LINE__, i, uibutton[i].changed_, uibutton[i].active_, General_GetString(uibutton[i].string_id_)));
		
		// check if the key entered matches the key for any menu items
		if (uibutton[i].key_ == user_input)
		{
			// found a match, but is this menu enabled or disabled?
			if (uibutton[i].active_ == true)
			{
				// valid entry. 
				return user_input;
			}
			else
			{
				// invalid entry
				return ACTION_INVALID_INPUT;
			}
		}
	}

	// if still here, it wasn't tied to a menu item. May still be of interest to App, so return it.
	return user_input;
}


// determine which menu items should active, which inactive
// sets inactive/active, and flags any that changed since last evaluation
// does not render
void Screen_UpdateMenuStates(UI_Menu_Enabler_Info* the_enabling_info)
{
	bool	for_disk = the_enabling_info->for_disk_;
	bool	for_flash = the_enabling_info->for_flash_;
	bool	is_kup = the_enabling_info->is_kup_;
	uint8_t	the_file_type = the_enabling_info->file_type_;
	bool	other_panel_for_disk = the_enabling_info->other_panel_for_disk_;
	bool	other_panel_for_flash = the_enabling_info->other_panel_for_flash_;
	
// LOGIC:
//       - Pass it some info on the currently selected item and panel:
//         - Panel:
//             - # of files in panel
//             - Bank vs file
//             - Flash vs RAM
//         - File:
//             - File type (if file type=0, then assume no file selected/no file available)
//         - Also info on the selected item in the other panel:
//             - Bank vs file
//             - File type? 
//             - Flash vs RAM

	//DEBUG_OUT(("%s %d: for disk=%u, for flash=%u, is kup=%u, file_type=%u, other for disk=%u, other for flash=%u", __func__ , __LINE__, for_disk, for_flash, is_kup, the_file_type, other_panel_for_disk, other_panel_for_flash));
		
// #define _CBM_T_REG      0x10U   /* Bit set for regular files */
// #define _CBM_T_HEADER   0x05U   /* Disk header / title */
// #define _CBM_T_DIR      0x02U   /* IDE64 and CMD sub-directory */
// #define FNX_FILETYPE_FONT	200	// any 2k file ending in .fnt
// #define FNX_FILETYPE_EXE	201	// any .pgz, etc executable
// #define FNX_FILETYPE_BASIC	202	// a .bas file that f/manager will try to pass to SuperBASIC
// #define FNX_FILETYPE_MUSIC	203	// a .mod file that f/manager will try to pass to modojr
// #define FNX_FILETYPE_IMAGE	204 // a .256 or .lbm image file.

	// - Always active items that don’t need to be in screen overlay and don’t need activation check:
	//     - Superbasic, DOS, About. Set Time. Refresh panel.
	//     - View as text, view as Hex
	// will not actively set/unset. they start out activated and never change.

    // - Items that need a check on startup only, and can stay the same afterwords:
    //    - Quit
    //    - 0, 1, 2 (SD card, floppy1, floppy2)
    // will set these in a different function that is only called once, on startup.
	
	if (for_disk == false)
	{
		// handle memory system-specific menu items

		//     - Only active when a memory system is selected:
		//         - Search raw memory
		//     - Only active when RAM is selected:
		//         - Fill, Clear
		//     - Only active when flash/ram is selected, and other panel has a file system showing
		//         - Save 8192 byte bank to disk
		//     - Activated when memory system and is KUP bank:
		//         - load file (e.g, run a KUP if doing memory)

		if (uibutton[BUTTON_ID_BANK_FIND].active_ != true)
		{
			uibutton[BUTTON_ID_BANK_FIND].active_ = true;
			uibutton[BUTTON_ID_BANK_FIND].changed_ = true;
		}

		if (global_find_next_enabled == true)
		{
			if (uibutton[BUTTON_ID_BANK_FIND_NEXT].active_ != true)
			{
				uibutton[BUTTON_ID_BANK_FIND_NEXT].active_ = true;
				uibutton[BUTTON_ID_BANK_FIND_NEXT].changed_ = true;
			}
		}
		else
		{
			if (uibutton[BUTTON_ID_BANK_FIND_NEXT].active_ != false)
			{
				uibutton[BUTTON_ID_BANK_FIND_NEXT].active_ = false;
				uibutton[BUTTON_ID_BANK_FIND_NEXT].changed_ = true;
			}
		}

		if (for_flash == false)
		{
			if (uibutton[BUTTON_ID_BANK_FILL].active_ != true)
			{
				uibutton[BUTTON_ID_BANK_FILL].active_ = true;
				uibutton[BUTTON_ID_BANK_FILL].changed_ = true;
			}
	
			if (uibutton[BUTTON_ID_BANK_CLEAR].active_ != true)
			{
				uibutton[BUTTON_ID_BANK_CLEAR].active_ = true;
				uibutton[BUTTON_ID_BANK_CLEAR].changed_ = true;
			}
		}
		else
		{
			if (uibutton[BUTTON_ID_BANK_FILL].active_ != false)
			{
				uibutton[BUTTON_ID_BANK_FILL].active_ = false;
				uibutton[BUTTON_ID_BANK_FILL].changed_ = true;
			}
	
			if (uibutton[BUTTON_ID_BANK_CLEAR].active_ != false)
			{
				uibutton[BUTTON_ID_BANK_CLEAR].active_ = false;
				uibutton[BUTTON_ID_BANK_CLEAR].changed_ = true;
			}
		}

		if (is_kup == true)
		{
			if (uibutton[BUTTON_ID_LOAD].active_ != true)
			{
				uibutton[BUTTON_ID_LOAD].active_ = true;
				uibutton[BUTTON_ID_LOAD].changed_ = true;
			}
		}
		else
		{
			if (uibutton[BUTTON_ID_LOAD].active_ != false)
			{
				uibutton[BUTTON_ID_LOAD].active_ = false;
				uibutton[BUTTON_ID_LOAD].changed_ = true;
			}
		}
		
		// for copy, the other panel can't be flash, but in all other combinations, it should be active.
		if (other_panel_for_flash == true)
		{
			if (uibutton[BUTTON_ID_COPY].active_ != false)
			{
				uibutton[BUTTON_ID_COPY].active_ = false;
				uibutton[BUTTON_ID_COPY].changed_ = true;
			}
		}
		else
		{
			if (uibutton[BUTTON_ID_COPY].active_ != true)
			{
				uibutton[BUTTON_ID_COPY].active_ = true;
				uibutton[BUTTON_ID_COPY].changed_ = true;
			}
		}

		if (uibutton[BUTTON_ID_DELETE].active_ != false)
		{
			uibutton[BUTTON_ID_DELETE].active_ = false;
			uibutton[BUTTON_ID_DELETE].changed_ = true;
		}

		if (uibutton[BUTTON_ID_DUPLICATE].active_ != false)
		{
			uibutton[BUTTON_ID_DUPLICATE].active_ = false;
			uibutton[BUTTON_ID_DUPLICATE].changed_ = true;
		}

		if (uibutton[BUTTON_ID_RENAME].active_ != false)
		{
			uibutton[BUTTON_ID_RENAME].active_ = false;
			uibutton[BUTTON_ID_RENAME].changed_ = true;
		}

		if (uibutton[BUTTON_ID_FORMAT].active_ != false)
		{
			uibutton[BUTTON_ID_FORMAT].active_ = false;
			uibutton[BUTTON_ID_FORMAT].changed_ = true;
		}

		if (uibutton[BUTTON_ID_MAKE_DIR].active_ != false)
		{
			uibutton[BUTTON_ID_MAKE_DIR].active_ = false;
			uibutton[BUTTON_ID_MAKE_DIR].changed_ = true;
		}

		if (uibutton[BUTTON_ID_SORT_BY_TYPE].active_ != false)
		{
			uibutton[BUTTON_ID_SORT_BY_TYPE].active_ = false;
			uibutton[BUTTON_ID_SORT_BY_TYPE].changed_ = true;
		}

		if (uibutton[BUTTON_ID_SORT_BY_NAME].active_ != false)
		{
			uibutton[BUTTON_ID_SORT_BY_NAME].active_ = false;
			uibutton[BUTTON_ID_SORT_BY_NAME].changed_ = true;
		}

		if (uibutton[BUTTON_ID_SORT_BY_SIZE].active_ != false)
		{
			uibutton[BUTTON_ID_SORT_BY_SIZE].active_ = false;
			uibutton[BUTTON_ID_SORT_BY_SIZE].changed_ = true;
		}
	}
	else
	{
		// handle disk system-specific menu items

		//     - Only active when a file system is selected:
		//         - New Folder, Format Disk, copy file, duplicate file, rename file, delete file, sort by name/data/size/type
		//     - Only active when a file is selected, and other panel has RAM showing
		//         - Load 8192 byte bank from disk
		//     - Activated when file system and known file type:
		//         - Select file/load file

		if (uibutton[BUTTON_ID_DELETE].active_ != true)
		{
			uibutton[BUTTON_ID_DELETE].active_ = true;
			uibutton[BUTTON_ID_DELETE].changed_ = true;
		}

		if (uibutton[BUTTON_ID_DUPLICATE].active_ != true)
		{
			uibutton[BUTTON_ID_DUPLICATE].active_ = true;
			uibutton[BUTTON_ID_DUPLICATE].changed_ = true;
		}

		if (uibutton[BUTTON_ID_RENAME].active_ != true)
		{
			uibutton[BUTTON_ID_RENAME].active_ = true;
			uibutton[BUTTON_ID_RENAME].changed_ = true;
		}

		if (uibutton[BUTTON_ID_FORMAT].active_ != true)
		{
			uibutton[BUTTON_ID_FORMAT].active_ = true;
			uibutton[BUTTON_ID_FORMAT].changed_ = true;
		}

		if (uibutton[BUTTON_ID_MAKE_DIR].active_ != true)
		{
			uibutton[BUTTON_ID_MAKE_DIR].active_ = true;
			uibutton[BUTTON_ID_MAKE_DIR].changed_ = true;
		}

		if (uibutton[BUTTON_ID_SORT_BY_TYPE].active_ != true)
		{
			uibutton[BUTTON_ID_SORT_BY_TYPE].active_ = true;
			uibutton[BUTTON_ID_SORT_BY_TYPE].changed_ = true;
		}

		if (uibutton[BUTTON_ID_SORT_BY_NAME].active_ != true)
		{
			uibutton[BUTTON_ID_SORT_BY_NAME].active_ = true;
			uibutton[BUTTON_ID_SORT_BY_NAME].changed_ = true;
		}

		if (uibutton[BUTTON_ID_SORT_BY_SIZE].active_ != true)
		{
			uibutton[BUTTON_ID_SORT_BY_SIZE].active_ = true;
			uibutton[BUTTON_ID_SORT_BY_SIZE].changed_ = true;
		}

		if (the_file_type == _CBM_T_DIR || the_file_type == FNX_FILETYPE_FONT || the_file_type == FNX_FILETYPE_EXE || the_file_type == FNX_FILETYPE_IMAGE || the_file_type == FNX_FILETYPE_MUSIC || the_file_type == FNX_FILETYPE_BASIC)
		{
			if (uibutton[BUTTON_ID_LOAD].active_ != true)
			{
				uibutton[BUTTON_ID_LOAD].active_ = true;
				uibutton[BUTTON_ID_LOAD].changed_ = true;
			}
		}
		else
		{
			if (uibutton[BUTTON_ID_LOAD].active_ != false)
			{
				uibutton[BUTTON_ID_LOAD].active_ = false;
				uibutton[BUTTON_ID_LOAD].changed_ = true;
			}
		}

		// for copy, the other panel can't be flash, but in all other combinations, it should be active.
		if (other_panel_for_flash == true)
		{
			if (uibutton[BUTTON_ID_COPY].active_ != false)
			{
				uibutton[BUTTON_ID_COPY].active_ = false;
				uibutton[BUTTON_ID_COPY].changed_ = true;
			}
		}
		else
		{
			if (uibutton[BUTTON_ID_COPY].active_ != true)
			{
				uibutton[BUTTON_ID_COPY].active_ = true;
				uibutton[BUTTON_ID_COPY].changed_ = true;
			}
		}

		// disable all memory-system-only items

		if (uibutton[BUTTON_ID_BANK_FIND].active_ != false)
		{
			uibutton[BUTTON_ID_BANK_FIND].active_ = false;
			uibutton[BUTTON_ID_BANK_FIND].changed_ = true;
		}

		if (uibutton[BUTTON_ID_BANK_FIND_NEXT].active_ != false)
		{
			uibutton[BUTTON_ID_BANK_FIND_NEXT].active_ = false;
			uibutton[BUTTON_ID_BANK_FIND_NEXT].changed_ = true;
		}

		if (uibutton[BUTTON_ID_BANK_FILL].active_ != false)
		{
			uibutton[BUTTON_ID_BANK_FILL].active_ = false;
			uibutton[BUTTON_ID_BANK_FILL].changed_ = true;
		}

		if (uibutton[BUTTON_ID_BANK_CLEAR].active_ != false)
		{
			uibutton[BUTTON_ID_BANK_CLEAR].active_ = false;
			uibutton[BUTTON_ID_BANK_CLEAR].changed_ = true;
		}
	}
}


// renders the menu items, as either active or inactive, as appropriate. 
// active/inactive and changed/not changed must previously have been set
// if sparse_render is true, only those items that have a different enable decision since last render will be re-rendered. Set sparse_render to false if drawing menu for first time or after clearing screen, etc. 
void Screen_RenderMenu(bool sparse_render)
{
	uint8_t		i;
	uint8_t		x1;
	uint8_t		y1;
	uint8_t		text_color;

	// draw buttons
	for (i = 0; i < NUM_BUTTONS; i++)
	{
		//DEBUG_OUT(("%s %d: btn %i change=%u, active=%u, %s", __func__ , __LINE__, i, uibutton[i].changed_, uibutton[i].active_, General_GetString(uibutton[i].string_id_)));
		
		if (uibutton[i].changed_ == true || sparse_render == false)
		{
			text_color = (uibutton[i].active_ == true ? MENU_FOREGROUND_COLOR : MENU_INACTIVE_COLOR);
			x1 = uibutton[i].x1_;
			y1 = uibutton[i].y1_;
			Text_DrawHLine(UI_MIDDLE_AREA_START_X, y1, UI_MIDDLE_AREA_WIDTH, CH_SPACE, text_color, MENU_BACKGROUND_COLOR, CHAR_AND_ATTR);
			Text_DrawStringAtXY(x1, y1, General_GetString(uibutton[i].string_id_), text_color, MENU_BACKGROUND_COLOR);
			uibutton[i].changed_ = false;
		}
	}
}


// swap the copy >>> button for copy <<< and vice versa
void Screen_SwapCopyDirectionIndicator(void)
{
	if (uibutton[BUTTON_ID_COPY].string_id_ == ID_STR_FILE_COPY_RIGHT)
	{
		uibutton[BUTTON_ID_COPY].string_id_ = ID_STR_FILE_COPY_LEFT;
	}
	else
	{
		uibutton[BUTTON_ID_COPY].string_id_ = ID_STR_FILE_COPY_RIGHT;
	}

	uibutton[BUTTON_ID_COPY].changed_ = true;
}


// set up screen variables and draw screen for first time
void Screen_Render(void)
{
	global_clock_is_visible = true;
	//Text_ClearScreen(APP_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);
	Screen_DrawUI();
}


// have screen function draw the sort triangle in the right place
void Screen_UpdateSortIcons(uint8_t the_panel_x, void* the_sort_compare_function)
{
	// LOGIC:
	//    we want to draw CH_SORT_ICON immediately to the right of the column header which is now being sorted. 
	//    the positions for the sort icons are defined by UI_PANEL_FILENAME_SORT_OFFSET, etc. 
	//    we also need to undraw whatever had been set
	//    we are passed a pointer to the current compare function. we can use that to figure out what type of sort icon to draw. 
	
	// clear old icons
	Text_SetCharAtXY(the_panel_x + UI_PANEL_FILENAME_SORT_OFFSET, UI_VIEW_PANEL_HEADER_Y, CH_SPACE);
	Text_SetCharAtXY(the_panel_x + UI_PANEL_FILETYPE_SORT_OFFSET, UI_VIEW_PANEL_HEADER_Y, CH_SPACE);
	Text_SetCharAtXY(the_panel_x + UI_PANEL_FILESIZE_SORT_OFFSET, UI_VIEW_PANEL_HEADER_Y, CH_SPACE);
	
	// set new ones
	if (the_sort_compare_function == (void*)&File_CompareName)
	{
		Text_SetCharAndColorAtXY(the_panel_x + UI_PANEL_FILENAME_SORT_OFFSET, UI_VIEW_PANEL_HEADER_Y, CH_SORT_ICON, COLOR_BRIGHT_BLUE, COLOR_BLACK);
	}
	else if (the_sort_compare_function == (void*)&File_CompareFileTypeID)
	{
		Text_SetCharAndColorAtXY(the_panel_x + UI_PANEL_FILETYPE_SORT_OFFSET, UI_VIEW_PANEL_HEADER_Y, CH_SORT_ICON, COLOR_BRIGHT_BLUE, COLOR_BLACK);
	}
	else if (the_sort_compare_function == (void*)&File_CompareSize)
	{
		Text_SetCharAndColorAtXY(the_panel_x + UI_PANEL_FILESIZE_SORT_OFFSET, UI_VIEW_PANEL_HEADER_Y, CH_SORT_ICON, COLOR_BRIGHT_BLUE, COLOR_BLACK);
	}
}


// have screen function an icon for meatloaf mode, or clear it
void Screen_UpdateMeatloafIcon(uint8_t the_panel_x, bool meatloaf_mode)
{
	// LOGIC:
	//    we want to draw an icon representing "meatloaf" mode immediately to the right of the panel title tab
	
	Text_SetXY(the_panel_x + UI_LEFT_PANEL_TITLE_TAB_WIDTH, UI_VIEW_PANEL_TITLE_TAB_Y2);
	
	if (meatloaf_mode == true)
	{
		Text_SetChar(CH_UC_M);
	}
	else
	{
		Text_SetChar(CH_SPACE);
	}
}


// display information about f/manager
void Screen_ShowAppAboutInfo(void)
{
	// give credit for pexec flash loader, if we started from flash and not disk
	if (global_started_from_flash == true)
	{
		Buffer_NewMessage(General_GetString(ID_STR_ABOUT_FLASH_LOADER));
	}
	
	// show app name, version, and credit
	sprintf(global_string_buff1, General_GetString(ID_STR_ABOUT_FMANAGER), CH_MISC_COPY, MAJOR_VERSION, MINOR_VERSION, UPDATE_VERSION);
	Buffer_NewMessage(global_string_buff1);
	
	// also show current bytes free
	sprintf(global_string_buff1, General_GetString(ID_STR_N_BYTES_FREE), _heapmemavail());
	Buffer_NewMessage(global_string_buff1);
}


// show user a dialog and have them enter a string
// if a prefilled string is not needed, set starter_string to an empty string
// set max_len to the maximum number of bytes/characters that should be collected from user
// returns NULL if user cancels out of dialog, or returns a path to the string the user provided
char* Screen_GetStringFromUser(char* dialog_title, char* dialog_body, char* starter_string, uint8_t max_len)
{
	bool				success;
	uint8_t				orig_dialog_width;
	uint8_t				temp_dialog_width;
	
	// copy title and body text
	General_Strlcpy((char*)&global_dlg_title, dialog_title, 36);
	General_Strlcpy((char*)&global_dlg_body_msg, dialog_body, 70);

	// copy the starter string into the edit buffer so user can edit
	General_Strlcpy(global_string_buff2, starter_string, max_len + 1);
	
	// adjust dialog width temporarily, if necessary and possible
	orig_dialog_width = global_dlg.width_;
	temp_dialog_width = General_Strnlen(starter_string,  max_len);
	// account for situation where no starter string, but there is a length limit. 
	if (temp_dialog_width < max_len)
	{
		temp_dialog_width = max_len;
	}
	
	temp_dialog_width += 2; // +2 is for box draw chars
	
	DEBUG_OUT(("%s %d: orig_dialog_width=%u, temp width=%u, max_len=%u, starter='%s'", __func__ , __LINE__, orig_dialog_width, temp_dialog_width, max_len, starter_string));
	
	if (temp_dialog_width < orig_dialog_width)
	{
		temp_dialog_width = orig_dialog_width - 2;
	}
	else
	{
		global_dlg.width_ = temp_dialog_width;
		temp_dialog_width -= 2;
	}
	
	success = Text_DisplayTextEntryDialog(&global_dlg, (char*)&temp_screen_buffer_char, (char*)&temp_screen_buffer_attr, global_string_buff2, max_len, DIALOG_ACCENT_COLOR, DIALOG_FOREGROUND_COLOR, DIALOG_BACKGROUND_COLOR);

	// restore normal dialog width
	global_dlg.width_ = orig_dialog_width;

	// did user enter a name?
	if (success == false)
	{
		return NULL;
	}
	
	return global_string_buff2;
}


// show user a 2 button confirmation dialog and have them click a button
// returns true if user selected the "positive" button, or false if they selected the "negative" button
bool Screen_ShowUserTwoButtonDialog(char* dialog_title, uint8_t dialog_body_string_id, uint8_t positive_btn_label_string_id, uint8_t negative_btn_label_string_id)
{
	// copy title, body text, and buttons
	General_Strlcpy((char*)&global_dlg_title, dialog_title, 36);
	General_Strlcpy((char*)&global_dlg_body_msg, General_GetString(dialog_body_string_id), 70);
	General_Strlcpy((char*)&global_dlg_button[0], General_GetString(negative_btn_label_string_id), 10);
	General_Strlcpy((char*)&global_dlg_button[1], General_GetString(positive_btn_label_string_id), 10);
					
	global_dlg.num_buttons_ = 2;

	return Text_DisplayDialog(&global_dlg, (char*)&temp_screen_buffer_char, (char*)&temp_screen_buffer_attr, DIALOG_ACCENT_COLOR, DIALOG_FOREGROUND_COLOR, DIALOG_BACKGROUND_COLOR, COLOR_RED, COLOR_GREEN);
}


// utility function for checking user input for either normal string or series of numbers
// if preceded by "#" will check for list of 2-digit hex numbers. eg, (#FF,AA,01,00,EE).
// will convert to bytes and terminate with 0. In example above, it will return 5 as the len. 
// either way, will return the length of the set of characters that should be thought of as one unit. 
uint8_t ScreenEvaluateUserStringForHexSeries(char** the_string)
{
	uint8_t		this_byte;
	int16_t		byte[2];
	uint8_t		the_len = 0;
	char*		local_string = *the_string;
	char		converted_storage[16];	// 32 chars is max search len, but 1 for #, takes 2 for each byte, but + for terminator
	char*		converted = converted_storage;
	
	// LOGIC:
	//   if the string is just normal text, we don't change it, we just return the len
	//   if the string is a series of hex chars, we overwrite from the beginning of the string with the byte values

	//DEBUG_OUT(("%s %d: local_string='%s'", __func__ , __LINE__, local_string));
	
	this_byte = *local_string++;

	//DEBUG_OUT(("%s %d: first byte=%x ('%c')", __func__ , __LINE__, this_byte, this_byte));
	
	if (this_byte != '#')
	{
		// treat string as a normal string
		--local_string;
		//DEBUG_OUT(("%s %d: doesn't start with #, treating as string with len %u", __func__ , __LINE__, strlen(local_string)));
		return strlen(local_string);
	}

	// assume user was trying to provide series of hex numbers
	while (*local_string)
	{
		byte[0] = ScreenConvertHexCharToByteValue(*local_string++);
		byte[1] = ScreenConvertHexCharToByteValue(*local_string++);
		this_byte = *local_string++; // will be comma if there is another number encoded here
		//DEBUG_OUT(("%s %d: byte0=%x, byte1=%x, this_byte=%x (%c)", __func__ , __LINE__, byte[0], byte[1], this_byte, this_byte));
		//DEBUG_OUT(("%s %d: the_len=%u, val=%x", __func__ , __LINE__, the_len, (uint8_t)byte[0] * (uint8_t)16 + (uint8_t)byte[1]));
		
		if (byte[0] < 0 || byte[1] < 0)
		{
			// one or both chars were not hex chars. abandon effort.
			goto conversion_complete;
		}
		
		converted_storage[the_len++] = (uint8_t)byte[0] * (uint8_t)16 + (uint8_t)byte[1];
		//DEBUG_OUT(("%s %d: converted_storage[the_len-1]=%x", __func__ , __LINE__, converted_storage[the_len-1]));
		
		if (this_byte == 0)
		{
			// hit end of search string
			goto conversion_complete;
		}
		else if (this_byte != ',')
		{
			// not sure what's next, but let's give user benefit of doubt and assume they left out commas and just entered FFEEDD0102 etc.
			--local_string;
			//DEBUG_OUT(("%s %d: third char wasn't comma. the_len=%u, converted='%s'", __func__ , __LINE__, the_len, converted));
		}
	}
	
conversion_complete:
	converted_storage[the_len] = 0; // final terminator for good measure.
	memcpy(*the_string, converted_storage, the_len);
	//DEBUG_OUT(("%s %d: final conversion = %x%x%x%x%x%x, len=%u", __func__ , __LINE__, converted_storage[0], converted_storage[1], converted_storage[2], converted_storage[3], converted_storage[4], converted_storage[5], the_len));
	return the_len;
}

