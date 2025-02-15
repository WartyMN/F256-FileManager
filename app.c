/*
 * app.c
 *
 *  Created on: Jan 10, 2023
 *      Author: micahbly
 *
 *  A pseudo commander-style 2-column file manager
 *
 */
 


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/


// project includes
#include "app.h"
#include "comm_buffer.h"
#include "debug.h"
#include "file.h"
#include "list_panel.h"
#include "folder.h"
#include "general.h"
#include "keyboard.h"
#include "memory.h"
#include "memsys.h"
#include "overlay_em.h"
#include "overlay_startup.h"
#include "text.h"
#include "screen.h"
#include "strings.h"
#include "sys.h"

// C includes
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// cc65 includes
#include <device.h>
//#include <unistd.h>
#include <cc65.h>
#include "dirent.h"
#include "f256.h"
#include "api.h"
#include "kernel.h"



/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/

#define CH_PROGRESS_BAR_FULL	CH_CHECKERBOARD
#define PROGRESS_BAR_Y			(COMM_BUFFER_FIRST_ROW - 4)
#define PROGRESS_BAR_START_X	UI_MIDDLE_AREA_START_X
#define PROGRESS_BAR_WIDTH		10	// number of characters in progress bar
#define PROGRESS_BAR_DIVISOR	8	// number of slices in one char: divide % by this to get # of blocks to draw.
#define COLOR_PROGRESS_BAR		COLOR_CYAN


/*****************************************************************************/
/*                          File-scoped Variables                            */
/*****************************************************************************/

static UI_Menu_Enabler_Info	app_menu_enabler;		// info to pass to Screen to help it decide which menu items should be enabled

static uint8_t				app_active_panel_id;	// PANEL_ID_LEFT or PANEL_ID_RIGHT
static uint8_t				app_connected_drive_count;

static uint8_t				app_progress_bar_char[8] = 
{
	CH_SPACE,
	CH_PROGRESS_BAR_CHECKER_CH1,
	CH_PROGRESS_BAR_CHECKER_CH1+1,
	CH_PROGRESS_BAR_CHECKER_CH1+2,
	CH_PROGRESS_BAR_CHECKER_CH1+3,
	CH_PROGRESS_BAR_CHECKER_CH1+4,
	CH_PROGRESS_BAR_CHECKER_CH1+5,
	CH_PROGRESS_BAR_CHECKER_CH1+6,
};

/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

char					app_search_phrase_human_readable_storage[MAX_SEARCH_PHRASE_LEN + 1];
char					app_search_phrase_storage[MAX_SEARCH_PHRASE_LEN + 1];
char*					global_search_phrase_human_readable = app_search_phrase_human_readable_storage;
char*					global_search_phrase = app_search_phrase_storage;
uint8_t					global_search_phrase_len;


char*					global_named_app_dos = "dos";
char*					global_named_app_basic = "basic";

int8_t					global_connected_device[DEVICE_MAX_DEVICE_COUNT];	// will be 8, 9, etc, if connected, or -1 if not. 

bool					global_started_from_flash;		// tracks whether app started from flash or from disk
bool					global_clock_is_visible;		// tracks whether or not the clock should be drawn. set to false when not showing main 2-panel screen.
bool					global_find_next_enabled = false;	// tracks whether find next is callable. turned on by finding at a match, turned off by hitting end of memory without a match.

WB2KViewPanel			app_file_panel[2];


TextDialogTemplate		global_dlg;


char					global_dlg_title[36];	// arbitrary
char					global_dlg_body_msg[70];	// arbitrary
char					global_dlg_button[3][10];	// arbitrary

char*					global_string_buff1 = (char*)STORAGE_STRING_BUFFER_1;
char*					global_string_buff2 = (char*)STORAGE_STRING_BUFFER_2;	// HBD: this is used for EM copy with 256 width, but only holds 204 bytes. it overwrites the ONCE code and startup code slightly. 
// char					global_string_buff2_buffer[256];
// char*					global_string_buff2 = global_string_buff2_buffer;
// char					global_string_buff1_buffer[256];
// char*					global_string_buff1 = global_string_buff1_buffer;

char					global_temp_path_1_buffer[FILE_MAX_PATHNAME_SIZE];
char					global_temp_path_2_buffer[FILE_MAX_PATHNAME_SIZE] = "";
char*					global_temp_path_1 = global_temp_path_1_buffer;
char*					global_temp_path_2 = global_temp_path_2_buffer;

char					global_retrieved_em_filename_buffer[FILE_MAX_FILENAME_SIZE];	// for retrieving from EM
char*					global_retrieved_em_filename = global_retrieved_em_filename_buffer;

uint8_t					temp_screen_buffer_char[APP_DIALOG_BUFF_SIZE];	// WARNING HBD: don't make dialog box bigger than will fit!
uint8_t					temp_screen_buffer_attr[APP_DIALOG_BUFF_SIZE];	// WARNING HBD: don't make dialog box bigger than will fit!

extern uint8_t				zp_bank_num;


#pragma zpsym ("zp_bank_num");

/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

// swap the active panels. 
void App_SwapActivePanel(void);

// scan for connected devices, and return count. Returns -1 if error.
int8_t	App_ScanDevices(void);

// initialize various objects - once per run
void App_Initialize(void);

// initialize the specified panel for use with a disk system
void App_InitializePanelForDisk(uint8_t panel_id);

// initialize the specified panel for use with RAM or Flash
void App_InitializePanelForMemory(uint8_t panel_id, bool for_flash);

// handles user input
uint8_t App_MainLoop(void);


/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/


// swap the active panels. 
void App_SwapActivePanel(void)
{
	// mark old panel inactive, mark new panel active, set new active panel id
	Panel_ToggleActiveState(&app_file_panel[PANEL_ID_LEFT]);
	Panel_ToggleActiveState(&app_file_panel[PANEL_ID_RIGHT]);
	++app_active_panel_id;
	app_active_panel_id = app_active_panel_id % 2;
}


// scan for connected devices, and return count. Returns -1 if error.
int8_t	App_ScanDevices(void)
{
	uint8_t		drive_num = 0;
	uint8_t		device;
	DIR*		dir;
	char		drive_path[3];
	char*		the_drive_path = drive_path;
	
	// LOGIC: 
	//   F256 can have up to: 1 SD card (device 0:), 2 IEC drives (device 1: and 2:), and a flash drive (3:)
	//     2023/01/19: per Gadget, will eventually support 8 devices, but only 4 named so far, and flash drive is not ready. 
	//     2023/01/23: per Gadget, do you support unit0/1? > " the underlying driver does, but I don't register the second partition in the file-system device table"
	//   to see if a device is connected, we iterate through the possible device #s and try to open the directory file. 
	//     there is probably a better way to do this, but not sure what it is.
	
	app_connected_drive_count = 0;
	
	for (device = DEVICE_LOWEST_DEVICE_NUM; device <= DEVICE_HIGHEST_DEVICE_NUM; device++)
	{
		sprintf(the_drive_path, "%u:", device);
//	Buffer_NewMessage(the_drive_path);
		dir = Kernel_OpenDir(the_drive_path);

		if (dir)
		{
			// this device exists/is connected
			global_connected_device[drive_num] = device;
			++drive_num;
			
			if (Kernel_CloseDir(dir) == -1)
			{
				//Buffer_NewMessage("clsdir fail");
			}
		//sprintf(the_drive_path, "device was ok. device=%u", device);
		//Buffer_NewMessage(the_drive_path);
			//sprintf(global_string_buff1, "%u: open ok", device);
		}
// 		else
// 		{
// 			sprintf(global_string_buff1, "%u: open F", device);
// 		}
// 			Buffer_NewMessage(global_string_buff1);
	}
	
	return drive_num;
}


// initialize various objects - once per run
void App_Initialize(void)
{
	Buffer_Clear();

	// show info about the host F256 and environment, as well as copyright, version of f/manager
	App_LoadOverlay(OVERLAY_SCREEN);
	Screen_ShowAppAboutInfo();
	
	// scan which devices are connected, so we know what panels can access
	Buffer_NewMessage(General_GetString(ID_STR_MSG_SCANNING));
	app_connected_drive_count = App_ScanDevices();

	sprintf(global_string_buff1, General_GetString(ID_STR_MSG_SHOW_DRIVE_COUNT), app_connected_drive_count);
	Buffer_NewMessage(global_string_buff1);
	
	// have screen remember which disk systems are available
	Screen_SetInitialMenuStates(app_connected_drive_count); 

	// left panel always starts as the active one
	app_active_panel_id = PANEL_ID_LEFT;
	
	// if no drives connected, prior to beta 21, we just quit, but now we will continue
	// as user could be using f/manager in the primary flash position, they'll need a 
	// way to get to DOS and SuperBASIC
	if (app_connected_drive_count < 1)
	{
		Buffer_NewMessage(General_GetString(ID_STR_MSG_NO_DRIVES_AVAILABLE));
		
		App_InitializePanelForMemory(PANEL_ID_LEFT, false);	// TODO: replace with macro defines
		App_InitializePanelForMemory(PANEL_ID_RIGHT, true);
	}
	else
	{
		App_InitializePanelForDisk(PANEL_ID_LEFT);
	
		// if we have a second disk device available, set that up in the right panel
		if (app_connected_drive_count > 1)
		{
			App_InitializePanelForDisk(PANEL_ID_RIGHT);
		}
		else
		{
			App_InitializePanelForMemory(PANEL_ID_RIGHT, false);
		}
	}

	// always set left panel active at start, and right panel inactive. 
	app_file_panel[PANEL_ID_LEFT].active_ = true;	// we always start out with left panel being the active one
	app_file_panel[PANEL_ID_RIGHT].active_ = false;	// we always start out with left panel being the active one
}


// initialize the specified panel for use with a disk system
void App_InitializePanelForDisk(uint8_t panel_id)
{
	uint8_t				the_drive_index = panel_id;
	uint8_t				panel_x_offset;
	char				drive_path[3];
	char*				the_drive_path = drive_path;

	App_LoadOverlay(OVERLAY_DISKSYS);
	
	sprintf(the_drive_path, "%u:", global_connected_device[the_drive_index]);

	if ( (app_file_panel[panel_id].root_folder_ = Folder_NewOrReset(app_file_panel[panel_id].root_folder_, global_connected_device[the_drive_index], the_drive_path) ) == NULL)
	{
		Buffer_NewMessage(General_GetString(ID_STR_ERROR_ALLOC_FAIL));
		App_Exit(ERROR_COULD_NOT_CREATE_ROOT_FOLDER_OBJ);
	}

	//DEBUG_OUT(("%s %d: panel folder=%p, panel id=%u, device num=%u, path=%s", __func__ , __LINE__, app_file_panel[panel_id].root_folder_, panel_id, global_connected_device[the_drive_index], the_drive_path));

	panel_x_offset = UI_RIGHT_PANEL_X_DELTA * panel_id; // panel id is either 0 or 1.
	
	Panel_Initialize(
		&app_file_panel[panel_id], 
		PARAM_INITIALIZE_FOR_DISK,
		(UI_LEFT_PANEL_BODY_X1 + panel_x_offset + 1), (UI_VIEW_PANEL_BODY_Y1 + 2), 
		(UI_VIEW_PANEL_BODY_WIDTH - 2), (UI_VIEW_PANEL_BODY_HEIGHT - 3)
	);

	Panel_SetCurrentDevice(&app_file_panel[panel_id], global_connected_device[the_drive_index]);
}


// initialize the specified panel for use with RAM or Flash
void App_InitializePanelForMemory(uint8_t panel_id, bool for_flash)
{
	uint8_t				panel_x_offset;

	App_LoadOverlay(OVERLAY_MEMSYSTEM);
	
	if ( (app_file_panel[panel_id].memory_system_ = MemSys_NewOrReset(app_file_panel[panel_id].memory_system_, for_flash)) == NULL)
	{
		Buffer_NewMessage(General_GetString(ID_STR_ERROR_ALLOC_FAIL));
		App_Exit(ERROR_COULD_NOT_CREATE_OR_RESET_MEMSYS_OBJ);
	}

	panel_x_offset = UI_RIGHT_PANEL_X_DELTA * panel_id; // panel id is either 0 or 1.

	Panel_Initialize(
		&app_file_panel[panel_id], 
		PARAM_INITIALIZE_FOR_MEMORY,
		(UI_LEFT_PANEL_BODY_X1 + panel_x_offset + 1), (UI_VIEW_PANEL_BODY_Y1 + 2), 
		(UI_VIEW_PANEL_BODY_WIDTH - 2), (UI_VIEW_PANEL_BODY_HEIGHT - 3)
	);
}


// handles user input
uint8_t App_MainLoop(void)
{
	uint8_t				user_input;
	uint8_t				new_device_num;
	bool				exit_main_loop = false;
	bool				success;
	WB2KViewPanel*		the_panel;
	
	// main loop
	while (exit_main_loop == false)
	{
		// turn off cursor - seems to turn itself off when kernal detects cursor position has changed. 
		Sys_EnableTextModeCursor(false);
		
		the_panel = &app_file_panel[app_active_panel_id];
		
		//DEBUG: track software stack pointer
		//sprintf(global_string_buffer, "sp: %x%x", *(char*)0x52, *(char*)0x51);
		//Buffer_NewMessage(global_string_buffer);

		do
		{
			// collect info we need for menu enabling/disabling
			if (the_panel->for_disk_ == true)
			{
				App_LoadOverlay(OVERLAY_DISKSYS);
				app_menu_enabler.file_type_ = Folder_GetCurrentFileType(the_panel->root_folder_);
				app_menu_enabler.for_flash_ = false;
				app_menu_enabler.is_kup_ = false;
				app_menu_enabler.is_meatloaf_ = the_panel->root_folder_->is_meatloaf_;
			}
			else
			{
				App_LoadOverlay(OVERLAY_MEMSYSTEM);
				app_menu_enabler.file_type_ = 0;
				app_menu_enabler.for_flash_ = the_panel->memory_system_->is_flash_;
				app_menu_enabler.is_kup_ = MemSys_GetCurrentRowKUPState(the_panel->memory_system_);
				app_menu_enabler.is_meatloaf_ = false;
			}
						
			// ask Screen to establish which menu items should be available (this just keeps this code out of MAIN to maximize heap space)
			app_menu_enabler.for_disk_ = the_panel->for_disk_;
			app_menu_enabler.other_panel_for_disk_ = app_file_panel[(app_active_panel_id + 1) % 2].for_disk_;
			app_menu_enabler.other_panel_for_flash_ = app_file_panel[(app_active_panel_id + 1) % 2].memory_system_->is_flash_;
				// HBD: memory_system_ is null for disk sys. seems to be working ok, but that's random
			app_menu_enabler.other_panel_is_meatloaf_ = app_file_panel[(app_active_panel_id + 1) % 2].root_folder_->is_meatloaf_;
				// HBD: root_folder is null for memory sys. seems to be working ok, but that's random
			App_LoadOverlay(OVERLAY_SCREEN);
			Screen_UpdateMenuStates(&app_menu_enabler);
			
			// ask Screen to draw the appropriate set of menus, only doing those that haven't changed since last round
			Screen_RenderMenu(PARAM_ONLY_RENDER_CHANGED_ITEMS);

			// ask Screen to get user input and vet it against the menu items that are currently enabled
			// only inputs for active menu items will cause an input to be returned here
			user_input = Screen_GetValidUserInput();
			// Get user input and vet it against the menu items that are currently enabled
			// returns ACTION_INVALID_INPUT if the key pressed was for a disabled menu item
			// returns the key pressed if it matched an enabled menu item, or if wasn't a known (to Screen) input. This lets App still allow for cursor keys, etc, which aren't represented by menu items

			//DEBUG_OUT(("%s %d: user_input=%u", __func__ , __LINE__, user_input));
			//sprintf(global_string_buff1, "%s %d: user_input=%u", __func__ , __LINE__, user_input);
			//Buffer_NewMessage(global_string_buff1);
			
			// first switch: for file menu only, and skip if file menu is inactive
			//   slightly inefficient in that it has to go through them all twice, but this is not a performance bottleneck
			//   note: we also put the sort commands here because it doesn't make sense to sort if no files
			switch (user_input)
			{
				case ACTION_INVALID_INPUT:
					//DEBUG_OUT(("%s %d: user hit a key for a disabled menu item", __func__ , __LINE__));
					break;

				case ACTION_SORT_BY_NAME:
					//DEBUG_OUT(("%s %d: Sort by name", __func__ , __LINE__));
					the_panel->sort_compare_function_ = (void*)&File_CompareName;
					Panel_SortAndDisplay(the_panel);
					break;

// 				case ACTION_SORT_BY_DATE:
// 					//DEBUG_OUT(("%s %d: Sort by date", __func__ , __LINE__));
// 					the_panel->sort_compare_function_ = (void*)&File_CompareDate;
// 					Panel_SortAndDisplay(the_panel);
// 					Buffer_NewMessage("Files now sorted by date.");
					break;
// 
				case ACTION_SORT_BY_SIZE:
					//DEBUG_OUT(("%s %d: Sort by size", __func__ , __LINE__));
					the_panel->sort_compare_function_ = (void*)&File_CompareSize;
					Panel_SortAndDisplay(the_panel);
					break;
		
				case ACTION_SORT_BY_TYPE:
					//DEBUG_OUT(("%s %d: Sort by type", __func__ , __LINE__));
					the_panel->sort_compare_function_ = (void*)&File_CompareFileTypeID;
					Panel_SortAndDisplay(the_panel);
					break;
		
				case ACTION_VIEW_AS_HEX:
					//DEBUG_OUT(("%s %d: view as hex", __func__ , __LINE__));
					global_clock_is_visible = false;
					success = Panel_ViewCurrentFile(the_panel, PARAM_VIEW_AS_HEX);
					App_LoadOverlay(OVERLAY_SCREEN);
					Screen_Render();	// the hex view has completely overwritten the screen
					Screen_RenderMenu(PARAM_RENDER_ALL_MENU_ITEMS);
					Panel_RenderContents(&app_file_panel[PANEL_ID_LEFT]);
					Panel_RenderContents(&app_file_panel[PANEL_ID_RIGHT]);
					//sprintf(global_string_buff1, "view as hex success = %u", success);
					//Buffer_NewMessage(global_string_buff1);
					break;
			
				case ACTION_VIEW_AS_TEXT:
					//DEBUG_OUT(("%s %d: view as hex", __func__ , __LINE__));
					global_clock_is_visible = false;
					success = Panel_ViewCurrentFile(the_panel, PARAM_VIEW_AS_TEXT);
					App_LoadOverlay(OVERLAY_SCREEN);
					Screen_Render();	// the hex view has completely overwritten the screen
					Screen_RenderMenu(PARAM_RENDER_ALL_MENU_ITEMS);
					Panel_RenderContents(&app_file_panel[PANEL_ID_LEFT]);
					Panel_RenderContents(&app_file_panel[PANEL_ID_RIGHT]);
					//sprintf(global_string_buff1, "view as text success = %u", success);
					//Buffer_NewMessage(global_string_buff1);
					break;
			
				case ACTION_COPY:
					success = Panel_CopyCurrentFile(the_panel, &app_file_panel[(app_active_panel_id + 1) % 2]);
					//sprintf(global_string_buff1, "copy file success = %u", success);
					//Buffer_NewMessage(global_string_buff1);
					break;
			
				case ACTION_DUPLICATE:
					success = Panel_CopyCurrentFile(the_panel, the_panel);	// to duplicate, we just pass same panel as target.
					//sprintf(global_string_buff1, "copy file success = %u", success);
					//Buffer_NewMessage(global_string_buff1);
					break;
				
				case ACTION_RENAME:
					success = Panel_RenameCurrentFile(the_panel);
					break;
			
				case ACTION_DELETE:
				case ACTION_DELETE_ALT:
					// works for files and dirs
					success = Panel_DeleteCurrentFile(the_panel);
					break;

				case ACTION_SELECT:
				case ACTION_LOAD:
					// if the current file is a directory, open it, and redisplay the panel with the contents
					// if the current file is an exe, run it with pexec. if a font, load it into memory, etc.
					success = Panel_OpenCurrentFileOrFolder(the_panel);					
					break;
					
				case ACTION_FILL_MEMORY:
					success = Panel_FillCurrentBank(the_panel);						
					break;

				case ACTION_CLEAR_MEMORY:
					success = Panel_ClearCurrentBank(the_panel);						
					break;
					
				case ACTION_SEARCH_MEMORY:
					success = Panel_SearchCurrentBank(the_panel);
					break;

				case ACTION_SEARCH_MEMORY_NEXT:
					App_LoadOverlay(OVERLAY_EM);
					success = global_find_next_enabled = EM_SearchMemory(PARAM_START_AFTER_LAST_HIT);					
					break;
					
				case ACTION_SWAP_ACTIVE_PANEL:
					// mark old panel inactive, mark new panel active, set new active panel id
					App_SwapActivePanel();
					App_LoadOverlay(OVERLAY_SCREEN);
					Screen_SwapCopyDirectionIndicator();
					the_panel = &app_file_panel[app_active_panel_id];
					break;

				case ACTION_SWITCH_TO_SD:
				case ACTION_SWITCH_TO_FLOPPY_1:
				case ACTION_SWITCH_TO_FLOPPY_2:
				case ACTION_SWITCH_TO_RAM:
				case ACTION_SWITCH_TO_FLASH:
					// tell panel to connect to the specified drive or mem system
					// all 5 of these actions are tied to number keys, so can turn them into integers easily
					new_device_num = user_input - 48;
					success = Panel_SwitchDevice(the_panel, new_device_num);
					break;
					
				case ACTION_REFRESH_PANEL:
					Panel_Refresh(the_panel);			
					break;
				
				case ACTION_FORMAT_DISK:
					success = Panel_FormatDrive(the_panel);
					if (success)
					{
						Panel_Refresh(the_panel);			
					}
					break;

				case ACTION_LOAD_MEATLOAF_URL:
					success = Panel_OpenMeatloafURL(the_panel);
						
					break;
					
				case ACTION_NEW_FOLDER:
					success = Panel_MakeDir(the_panel);
					break;

				case ACTION_SET_TIME:
					App_LoadOverlay(OVERLAY_SCREEN);	
					General_Strlcpy(global_string_buff1, General_GetString(ID_STR_DLG_SET_CLOCK_TITLE), APP_DIALOG_WIDTH);
					*global_string_buff2 = 0;	// don't provide a default value, show empty string
					global_string_buff2 = Screen_GetStringFromUser(
						global_string_buff1, 
						General_GetString(ID_STR_DLG_SET_CLOCK_BODY), 
						global_string_buff2, 
						14);	// //YY-MM-DD HH-MM = 14
					App_LoadOverlay(OVERLAY_DISKSYS);
					
					if (global_string_buff2 != NULL)
					{
						// user entered a date/time string, now try to parse and save it.
						success = Sys_UpdateRTC(global_string_buff2);
						App_DisplayTime();
					}
					
					break;

				case ACTION_ABOUT:
					Screen_ShowAppAboutInfo();
					break;
					
				case MOVE_UP:
					success = Panel_SelectPrevFile(the_panel);
					//sprintf(global_string_buff1, "prev file selection success = %u", success);
					//Buffer_NewMessage(global_string_buff1);
					break;

				case MOVE_DOWN:
					success = Panel_SelectNextFile(the_panel);
					//DEBUG_OUT(("%s %d: next file selection success = %u", __func__ , __LINE__, success));
					//sprintf(global_string_buff1, "next file selection success = %u", success);
					//Buffer_NewMessage(global_string_buff1);
					break;

				case MOVE_LEFT:
					if (app_active_panel_id == PANEL_ID_RIGHT)
					{
						// mark old panel inactive, mark new panel active, set new active panel id
						App_SwapActivePanel();
						App_LoadOverlay(OVERLAY_SCREEN);
						Screen_SwapCopyDirectionIndicator();
						the_panel = &app_file_panel[app_active_panel_id];
					}
					break;

				case MOVE_RIGHT:
					if (app_active_panel_id == PANEL_ID_LEFT)
					{
						// mark old panel inactive, mark new panel active, set new active panel id
						App_SwapActivePanel();
						App_LoadOverlay(OVERLAY_SCREEN);
						Screen_SwapCopyDirectionIndicator();
						the_panel = &app_file_panel[app_active_panel_id];
					}
					break;
				
				case ACTION_EXIT_TO_BASIC:
					Kernal_RunNamed(global_named_app_basic, 5);	// this will only ever return in an error condition. 
					success = false;
					break;
					
				case ACTION_EXIT_TO_DOS:
					Kernal_RunNamed(global_named_app_dos, 3);	// this will only ever return in an error condition. 
					success = false;
					break;
					
				case ACTION_QUIT:
					App_LoadOverlay(OVERLAY_SCREEN);
					if (Screen_ShowUserTwoButtonDialog(
						General_GetString(ID_STR_DLG_ARE_YOU_SURE), 
						ID_STR_DLG_QUIT_CONFIRM, 
						ID_STR_DLG_YES, 
						ID_STR_DLG_NO
						) == 1)
					{
						exit_main_loop = true;
						continue;
					}
					break;

				default:
					//sprintf(global_string_buff1, "didn't know key %u", user_input);
					//Buffer_NewMessage(global_string_buff1);
					//DEBUG_OUT(("%s %d: didn't know key %u", __func__, __LINE__, user_input));
					//fatal(user_input);
					user_input = ACTION_INVALID_INPUT;
					break;
			}
		} while (user_input == ACTION_INVALID_INPUT);				
	} // while for exit loop
	
	// normal returns all handled above. this is a catch-all
	return 0;
}




/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/


// Draws the progress bar frame on the screen
void App_ShowProgressBar(void)
{
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, PROGRESS_BAR_Y - 1, UI_MIDDLE_AREA_WIDTH, CH_UNDERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, PROGRESS_BAR_Y,     UI_MIDDLE_AREA_WIDTH, CH_SPACE,      MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, PROGRESS_BAR_Y + 1, UI_MIDDLE_AREA_WIDTH, CH_OVERSCORE,  MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
}

// Hides the progress bar frame on the screen
void App_HideProgressBar(void)
{
	Text_FillBox(UI_MIDDLE_AREA_START_X, PROGRESS_BAR_Y - 1, (UI_MIDDLE_AREA_START_X + UI_MIDDLE_AREA_WIDTH - 1), PROGRESS_BAR_Y + 1, CH_SPACE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR);
}


// draws the 'bar' part of the progress bar, according to a % complete passed (0-100 integer)
void App_UpdateProgressBar(uint8_t progress_bar_total)
{
	// logic:
	//  - has access to 40 positions worth of status: 5 characters each with 8 slots
	//  - so 2.5% complete = |, 5% = ||, 25%=||||||||  || (imagine PETSCII) 
	//  - draws spaces for not-yet-reached char spaces
	//  - integer division (with non-signed) goes to floor. so 15/8 = 1 r7. 1 is the number of full blocks, 7 (minus 1) is the index to the partial block char

	uint8_t		i;
	uint8_t		full_blocks;
	uint8_t		the_char_code;
	uint8_t		progress_bar_char_index;

	progress_bar_total = (progress_bar_total > 100) ? 100 : progress_bar_total;
	
	full_blocks = progress_bar_total / PROGRESS_BAR_DIVISOR;
	progress_bar_char_index = progress_bar_total % PROGRESS_BAR_DIVISOR; // remainders will be 0-7: index to app_progress_bar_char[]

	for (i = 0; i < PROGRESS_BAR_WIDTH; i++)
	{
		if (i < full_blocks)
		{
			the_char_code = CH_PROGRESS_BAR_FULL;
		}
		else if (i == full_blocks && progress_bar_char_index > 0)
		{
			the_char_code = app_progress_bar_char[progress_bar_char_index];
		}
		else
		{
			the_char_code = CH_SPACE;
		}

		Text_SetCharAndColorAtXY(PROGRESS_BAR_START_X + i, PROGRESS_BAR_Y, the_char_code, COLOR_PROGRESS_BAR, COLOR_BLACK);
	}
}


// // copy 256b chunks of data between specified 6502 addr and the fixed address range in EM, without bank switching
// // page_num is used to calculate distance from the base EM address
// // set to_em to true to copy from CPU space to EM, or false to copy from EM to specified CPU addr. PARAM_COPY_TO_EM/PARAM_COPY_FROM_EM
// void App_EMDataCopyDMA(uint8_t* cpu_addr, uint8_t page_num, bool to_em)
// {
// 	uint32_t	em_addr;	// physical memory address (20 bit)
// 	uint8_t		zp_em_addr_base;
// 	uint8_t		zp_cpu_addr_base;
// 	
// 
// 	// LOGIC:
// 	//   DMA will be used to copy directly from extended memory: no bank switching takes place
// 	//   sys address is physical machine 20-bit address.
// 	//   sys address is always relative to EM_STORAGE_START_PHYS_ADDR ($28000), based on page_num passed
// 	//     eg, if page_num=0, it is EM_STORAGE_START_PHYS_ADDR, if page_num is 8 it is EM_STORAGE_START_PHYS_ADDR + 8*256
// 	//   cpu address is $0000-$FFFF range that CPU can access
// 	
// 	if (to_em == true)
// 	{
// 		zp_em_addr_base = ZP_TO_ADDR;
// 		zp_cpu_addr_base = ZP_FROM_ADDR;
// 	}
// 	else
// 	{
// 		zp_em_addr_base = ZP_FROM_ADDR;
// 		zp_cpu_addr_base = ZP_TO_ADDR;
// 	}
// 	
// 	// add the offset to the base address for EM data get to the right place for this chunk's copy
// 	em_addr = (uint32_t)EM_STORAGE_START_PHYS_ADDR + (page_num * 256);
// 	
// 	//sprintf(global_string_buff1, "DMA copy page_num=%u, em_addr=%lu", page_num, em_addr);
// 	//Buffer_NewMessage(global_string_buff1);
// 	
// 	// set up the 20-bit address as either to/from
// 	*(uint16_t*)zp_em_addr_base = em_addr & 0xFFFF;
// 	*(uint8_t*)(zp_em_addr_base + 2) = ((uint32_t)EM_STORAGE_START_PHYS_ADDR >> 16) & 0xFF;
// 
// 	// set up the 16-bit address as either to/from
// 	*(char**)zp_cpu_addr_base = (char*)cpu_addr;
// 	*(uint8_t*)(zp_cpu_addr_base + 2) = 0;	// this buffer is in local / CPU memory, so: 0x00 0500 (etc)
// 
// 	// set copy length to 256 bytes
// 	*(char**)ZP_COPY_LEN = (char*)STORAGE_FILE_BUFFER_1_LEN;	
// 	*(uint8_t*)(ZP_COPY_LEN + 2) = 0;
// 
// 	//sprintf(global_string_buff1, "ZP_TO_ADDR=%02x,%02x,%02x; ZP_FROM_ADDR=%02x,%02x,%02x; ZP_COPY_LEN=%02x,%02x,%02x; ", *(uint8_t*)(ZP_TO_ADDR+0), *(uint8_t*)(ZP_TO_ADDR+1), *(uint8_t*)(ZP_TO_ADDR+2), *(uint8_t*)(ZP_FROM_ADDR+0), *(uint8_t*)(ZP_FROM_ADDR+1), *(uint8_t*)(ZP_FROM_ADDR+2), *(uint8_t*)(ZP_COPY_LEN+0), *(uint8_t*)(ZP_COPY_LEN+1), *(uint8_t*)(ZP_COPY_LEN+2));
// 	//Buffer_NewMessage(global_string_buff1);
// 
// 	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);	
//  	Memory_CopyWithDMA();	
// 	Sys_RestoreIOPage();
// }


// copy 256b chunks of data between specified 6502 addr and the fixed address range in EM, using bank switching -- no DMA
// em_bank_num is used to derive the base EM address
// page_num is used to calculate distance from the base EM address
// set to_em to true to copy from CPU space to EM, or false to copy from EM to specified CPU addr. PARAM_COPY_TO_EM/PARAM_COPY_FROM_EM
void App_EMDataCopy(uint8_t* cpu_addr, uint8_t em_bank_num, uint8_t page_num, bool to_em)
{
	//uint32_t	em_addr;			// physical memory address (20 bit)
	uint8_t		em_slot;			// 00-7F are valid slots, but our dedicated EM storage starts at bank $14
	uint8_t*	em_cpu_addr;		// A000-BFFF: the specific target address within the CPU address space that the EM memory has been banked into
	uint8_t		previous_overlay_bank_num;
	

	// LOGIC:
	//   The overlay bank will be temporarily swapped out, and the required EM bank swapped in (tried to swap out kernel#2/IO bank, but froze up)
	//   required bank # can be calculated by taking system address and dividing by 8192. eg, 0x40000 / 0x2000 = bank 0x20
	//   sys address is physical machine 20-bit address.
	//   sys address is relative to em_bank_num, based on page_num passed
	//     eg, if em_bank_num=$14 (EM_STORAGE_START_PHYS_ADDR ($28000=bank 14)) and page_num=0, it is $28000, if page_num is 7 it is $28000 + 7*256
	//   cpu address is $0000-$FFFF range that CPU can access
	
	// add the offset to the base address get to the right place for this chunk's copy
	em_slot = em_bank_num + (page_num / 32);
	//em_addr = (uint32_t)(em_bank_num * 8192) + (page_num * 256));
	
	// CPU addr has to keep recycling space between A000-BFFF, so when chunk num is >32, need to get it back down under 32
	em_cpu_addr = (uint8_t*)((uint16_t)EM_STORAGE_START_CPU_ADDR + ((page_num % 32) * 256));
	
	//sprintf(global_string_buff1, "EM copy page_num=%u, em_slot=%u, em_cpu_addr=%p", page_num, em_slot, em_cpu_addr);
	//Buffer_NewMessage(global_string_buff1);
	
	// map the required EM bank into the overlay bank temporarily
	zp_bank_num = em_slot;
	previous_overlay_bank_num = Memory_SwapInNewBank(EM_STORAGE_START_SLOT);
	
	// copy data to/from
	if (to_em == true)
	{
		memcpy(em_cpu_addr, cpu_addr, 256);
	}
	else
	{
		memcpy(cpu_addr, em_cpu_addr, 256);
	}
	
	// map whatever overlay had been in place, back in place
	zp_bank_num = previous_overlay_bank_num;
	previous_overlay_bank_num = Memory_SwapInNewBank(EM_STORAGE_START_SLOT);	
}


// read the real time clock and display it
void App_DisplayTime(void)
{
	// LOGIC: 
	//   f256jr has a built in real time clock (RTC)
	//   it works like this:
	//     1) you enable it with bit 0 of RND_CTRL
	//     2) you turn on seed mode by setting bit 1 of RND_CTRL to 1.
	//     3) you populate RNDL and RNDH with a seed
	//     4) you turn off see mode by unsetting bit 1 of RND_CTRL.
	//     5) you get random numbers by reading RNDL and RNDH. every time you read them, it repopulates them. 
	//     6) resulting 16 bit number you divide by 65336 (RAND_MAX_FOENIX) to get a number 0-1. 
	//   I will use the real time clock to seed the number generator
	//  The clock should only be visible and updated when the main 2-panel screen is displayed
	
	
	uint8_t		old_rtc_control;
	
	if (global_clock_is_visible != true)
	{
		return;
	}
	
	// need to have vicky registers available
	Sys_SwapIOPage(VICKY_IO_PAGE_REGISTERS);
	
	// stop RTC from updating external registers. Required!
	old_rtc_control = R8(RTC_CONTROL);
	R8(RTC_CONTROL) = old_rtc_control | 0x08; // stop it from updating external registers
	
	// get year/month/day/hours/mins/second from RTC
	// below is subtly wrong, and i dno't care. don't need the datetime in a struct anyway.
	//global_datetime->year = R8(RTC_YEAR) & 0x0F + ((R8(RTC_YEAR) & 0xF0) >> 4) * 10;
	//global_datetime->month = R8(RTC_MONTH) & 0x0F + ((R8(RTC_MONTH) & 0x20) >> 4) * 10;
	//global_datetime->day = R8(RTC_DAY) & 0x0F + ((R8(RTC_DAY) & 0x30) >> 4) * 10;
	//global_datetime->hour = R8(RTC_HOURS) & 0x0F + ((R8(RTC_HOURS) & 0x30) >> 4) * 10;
	//global_datetime->min = R8(RTC_MINUTES) & 0x0F + ((R8(RTC_MINUTES) & 0x70) >> 4) * 10;
	//global_datetime->sec = R8(RTC_SECONDS) & 0x0F + ((R8(RTC_SECONDS) & 0x70) >> 4) * 10;

	sprintf(global_string_buff1, "20%02X-%02X-%02X %02X:%02X", R8(RTC_YEAR), R8(RTC_MONTH), R8(RTC_DAY), R8(RTC_HOURS), R8(RTC_MINUTES));
	
	// restore timer control to what it had been
	R8(RTC_CONTROL) = old_rtc_control;

	Sys_DisableIOBank();
	
	// draw at upper/right edge of screen, just under app title bar.
	Text_DrawStringAtXY(64, 3, global_string_buff1, COLOR_BRIGHT_YELLOW, COLOR_BLACK);
}


// reads in a filename from the filename EM storage and copies to global_retrieved_em_filename
// the_row is a 0-255 index to the filename associated with the file object with row_ property matching the_row
// returns a pointer to the local copy of the string (for compatibility reasons)
char* App_GetFilenameFromEM(uint8_t the_row)
{
	uint8_t		old_bank_under_io;
	uint16_t	addr_loc;
	char*		the_addr;

	// LOGIC:
	//   filenames are in fixed 32-char blocks in a specific EM bank
	//   this EM bank is brought in under the I/O bank at $C000
	//   when populating files from a directory, f/manager trims each to 31 if longer, and copies to EM
	//   no pointer is set up, so we just do math.
	//   row * 32 + 0xC000 is the address for any given filename
	
	// do math
	addr_loc = (0xC000 + FILE_MAX_FILENAME_SIZE * the_row);
	the_addr = (char*)addr_loc;
	
	// Disable the I/O page so we can get to RAM under it
	asm("SEI"); // disable interrupts in case some other process has a role here
	asm("lda $01");	// Stash the current IO page at ZP_OLD_IO_PAGE
	asm("sta %b", ZP_OLD_IO_PAGE);
	R8(MMU_IO_CTRL) = 4; // set only bit 2
	//Sys_DisableIOBank();
	
	// map the string bank into CPU memory space
	zp_bank_num = FILENAME_STORAGE_EM_SLOT;
	old_bank_under_io = Memory_SwapInNewBank(BANK_IO);

	// copy the string to buffer in MAIN space
	memcpy(global_retrieved_em_filename, the_addr, FILE_MAX_FILENAME_SIZE);
	
	Memory_RestorePreviousBank(BANK_IO);
	asm("CLI"); // restore interrupts
	
	// Re-enable the I/O page, which unmaps the string bank from 6502 RAM space
	//Sys_RestoreIOPage();
	asm("lda %b", ZP_OLD_IO_PAGE);	// we stashed the previous IO page at ZP_OLD_IO_PAGE
	asm("sta $01");	// switch back to the previous IO setting

	return global_retrieved_em_filename;
}


// stores the passed filename in the filename EM storage
// the_row is a 0-255 index to the filename associated with the file object with row_ property matching the_row
void App_SetFilenameInEM(const char* the_filename, uint8_t the_row)
{
	uint8_t		old_bank_under_io;
	uint16_t	addr_loc;
	char*		the_addr;

	// LOGIC:
	//   filenames are in fixed 32-char blocks in a specific EM bank
	//   this EM bank is brought in under the I/O bank at $C000
	//   when populating files from a directory, f/manager trims each to 31 if longer, and copies to EM
	//   no pointer is set up, so we just do math.
	//   row * 32 + 0xC000 is the address for any given filename
	
	// do math
	addr_loc = (0xC000 + FILE_MAX_FILENAME_SIZE * the_row);
	the_addr = (char*)addr_loc;
	
	// Disable the I/O page so we can get to RAM under it
	asm("SEI"); // disable interrupts in case some other process has a role here
	asm("lda $01");	// Stash the current IO page at ZP_OLD_IO_PAGE
	asm("sta %b", ZP_OLD_IO_PAGE);
	R8(MMU_IO_CTRL) = 4; // set only bit 2
	//Sys_DisableIOBank();
	
	// bring in the EM bank we want, under the I/O memory
	zp_bank_num = FILENAME_STORAGE_EM_SLOT;
	old_bank_under_io = Memory_SwapInNewBank(BANK_IO);

	// copy the string from MAIN space to EM, using fixed 32-byte len
	memcpy(the_addr, the_filename, FILE_MAX_FILENAME_SIZE);
	
	Memory_RestorePreviousBank(BANK_IO);
	asm("CLI"); // restore interrupts
	
	// Re-enable the I/O page, which unmaps the string bank from 6502 RAM space
	//Sys_RestoreIOPage();
	asm("lda %b", ZP_OLD_IO_PAGE);	// we stashed the previous IO page at ZP_OLD_IO_PAGE
	asm("sta $01");	// switch back to the previous IO setting
}


// Brings the requested overlay into memory
void App_LoadOverlay(uint8_t the_overlay_em_bank_number)
{
	zp_bank_num = the_overlay_em_bank_number;
	Memory_SwapInNewBank(OVERLAY_CPU_BANK);
}


// if ending on error: display error message, wait for user to confirm, and exit
// if no error, just exit
void App_Exit(uint8_t the_error_number)
{
	if (the_error_number != ERROR_NO_ERROR)
	{
		App_LoadOverlay(OVERLAY_SCREEN);
		sprintf(global_string_buff1, General_GetString(ID_STR_MSG_FATAL_ERROR), the_error_number);
		Screen_ShowUserTwoButtonDialog(
			global_string_buff1, 
			ID_STR_MSG_FATAL_ERROR_BODY, 
			ID_STR_DLG_OK, 
			ID_STR_DLG_OK
			);
	}

	// close log file if debugging flags were passed
	#if defined LOG_LEVEL_1 || defined LOG_LEVEL_2 || defined LOG_LEVEL_3 || defined LOG_LEVEL_4 || defined LOG_LEVEL_5
		General_LogCleanUp();
	#endif
	
	// turn cursor back on
	Sys_EnableTextModeCursor(true);
	
	R8(0xD6A2) = 0xDE;
	R8(0xD6A3) = 0xAD;
	R8(0xD6A0) = 0xF0;
	R8(0xD6A0) = 0x00;
	asm("JMP ($FFFC)");
}




int main(void)
{
	kernel_init();

	App_LoadOverlay(OVERLAY_STARTUP);
	
	if (Sys_InitSystem() == false)
	{
		App_Exit(ERROR_COULD_NOT_INIT_SYSTEM);
	}
	
	Sys_SetBorderSize(0, 0); // want all 80 cols and 60 rows!
	
	// initialize the comm buffer - do this before drawing UI or garbage will get written into comms area
	Buffer_Initialize();
	
	// initialize the random number generator embedded in the Vicky
	Startup_InitializeRandomNumGen();
	
	// set up pointers to string data that is in EM
	Startup_LoadString();

	// clear screen and draw logo
	Startup_ShowLogo();

	// initialize the shared dialog box to 2-button appearance
	App_InitializeDialogBox();

	App_LoadOverlay(OVERLAY_SCREEN);
	
	// Do first draw of UI
	Screen_Render();

	App_Initialize();
	
	Panel_Refresh(&app_file_panel[PANEL_ID_LEFT]);
	Panel_Refresh(&app_file_panel[PANEL_ID_RIGHT]);
	
	Keyboard_InitiateMinuteHand();
	App_DisplayTime();
	
	App_MainLoop();
	
	// restore screen, etc.
	App_Exit(ERROR_WEIRD_CANNOT_EXPLAIN);	// ERROR_NO_ERROR
	
	return 0;
}