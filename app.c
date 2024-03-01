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
#include "file.h"
#include "list_panel.h"
#include "folder.h"
#include "general.h"
#include "memory.h"
#include "text.h"
#include "screen.h"
#include "strings.h"
#include "sys.h"

// C includes
#include <stdbool.h>
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




/*****************************************************************************/
/*                          File-scoped Variables                            */
/*****************************************************************************/


static WB2KFolderObject*	app_root_folder[2];
static uint8_t				app_active_panel_id;	// PANEL_ID_LEFT or PANEL_ID_RIGHT
static uint8_t				app_connected_drive_count;

/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

int8_t					global_connected_device[DEVICE_MAX_DEVICE_COUNT];	// will be 8, 9, etc, if connected, or -1 if not. 

WB2KViewPanel			app_file_panel[2];
//WB2KFolderObject		app_root_folder[2];
TextDialogTemplate		global_dlg;	// dialog we'll configure and re-use for different purposes
char					global_dlg_title[36];	// arbitrary
char					global_dlg_body_msg[70];	// arbitrary
char					global_dlg_button[3][10];	// arbitrary

char*					global_string_buff1 = (char*)STORAGE_STRING_BUFFER_1;
char*					global_string_buff2 = (char*)STORAGE_STRING_BUFFER_2;

char					global_temp_path_1_buffer[FILE_MAX_PATHNAME_SIZE];
char					global_temp_path_2_buffer[FILE_MAX_PATHNAME_SIZE] = "";
char*					global_temp_path_1 = global_temp_path_1_buffer;
char*					global_temp_path_2 = global_temp_path_2_buffer;

uint8_t					temp_screen_buffer_char[APP_DIALOG_BUFF_SIZE];	// WARNING HBD: don't make dialog box bigger than will fit!
uint8_t					temp_screen_buffer_attr[APP_DIALOG_BUFF_SIZE];	// WARNING HBD: don't make dialog box bigger than will fit!

extern uint8_t				zp_bank_num;

extern void _OVERLAY_1_LOAD__[], 	_OVERLAY_1_SIZE__[];
extern void _OVERLAY_2_LOAD__[],	_OVERLAY_2_SIZE__[];
// extern void _OVERLAY_CREATE_MAP_LOAD__[], 	_OVERLAY_CREATE_MAP_SIZE__[];
// extern void _OVERLAY_CREATE_LEVEL_LOAD__[],	_OVERLAY_CREATE_LEVEL_SIZE__[];
// extern void _OVERLAY_COMBAT_LOAD__[],		_OVERLAY_COMBAT_SIZE__[];
// extern void _OVERLAY_INVENTORY_LOAD__[], 	_OVERLAY_INVENTORY_SIZE__[];
// extern void _OVERLAY_GAMEOVER_LOAD__[],		_OVERLAY_GAMEOVER_SIZE__[];
// extern void _OVERLAY_NOTICE_BOARD_LOAD__[], _OVERLAY_NOTICE_BOARD_SIZE__[];
// extern void _OVERLAY_CREATE_CAVERN_LOAD__[],_OVERLAY_CREATE_CAVERN_SIZE__[];

Overlay overlay[NUM_OVERLAYS] =
{
	{OVERLAY_1_SLOT, OVERLAY_1_VALUE},
	{OVERLAY_2_SLOT, OVERLAY_2_VALUE},
	{OVERLAY_3_SLOT, OVERLAY_3_VALUE},
	{OVERLAY_4_SLOT, OVERLAY_4_VALUE},
	{OVERLAY_5_SLOT, OVERLAY_5_VALUE},
	{OVERLAY_6_SLOT, OVERLAY_6_VALUE},
	{OVERLAY_7_SLOT, OVERLAY_7_VALUE},
	{OVERLAY_8_SLOT, OVERLAY_8_VALUE},
	{OVERLAY_9_SLOT, OVERLAY_9_VALUE},
};


#pragma zpsym ("zp_bank_num");

/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

// swap the active panels. 
void App_SwapActivePanel(void);

// initialize various objects - once per run
void App_Initialize(void);

// handles user input
uint8_t App_MainLoop(void);

// scan for connected devices, and return count. Returns -1 if error.
int8_t	App_ScanDevices(void);

/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/


// swap the active panels. 
void App_SwapActivePanel(void)
{
	// mark old panel inactive, mark new panel active, set new active panel id
	// TODO
	Panel_ToggleActiveState(&app_file_panel[PANEL_ID_LEFT]);
	Panel_ToggleActiveState(&app_file_panel[PANEL_ID_RIGHT]);
	++app_active_panel_id;
	app_active_panel_id = app_active_panel_id % 2;
// 	sprintf(global_string_buff1, "active panel set to %u", app_active_panel_id);
// 	Buffer_NewMessage(global_string_buff1);
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
	uint8_t				the_drive_index = 0;
	WB2KFileObject*		root_folder_file_left;
	WB2KFileObject*		root_folder_file_right;
	DateTime			this_datetime;
	char				drive_path[3];
	char*				the_drive_path = drive_path;

	Buffer_Clear();

	// show info about the host F256 and environment, as well as copyright, version of f/manager
	App_LoadOverlay(OVERLAY_SCREEN);
	Screen_ShowAboutInfo();

	// set up the dialog template we'll use throughout the app
	global_dlg.x_ = (SCREEN_NUM_COLS - APP_DIALOG_WIDTH)/2;
	global_dlg.y_ = 16;
	global_dlg.width_ = APP_DIALOG_WIDTH;
	global_dlg.height_ = APP_DIALOG_HEIGHT;
	global_dlg.num_buttons_ = 2;
	global_dlg.btn_shortcut_[0] = CH_ESC; // OSF_CH_ESC;
	global_dlg.btn_shortcut_[1] = CH_ENTER;
	global_dlg.btn_is_affirmative_[0] = false;
	global_dlg.btn_is_affirmative_[1] = true;

	global_dlg.title_text_ = global_dlg_title;
	global_dlg.body_text_ = global_dlg_body_msg;
	global_dlg.btn_label_[0] = global_dlg_button[0];
	global_dlg.btn_label_[1] = global_dlg_button[1];

	// scan which devices are connected, so we know what panels can access
	Buffer_NewMessage(General_GetString(ID_STR_MSG_SCANNING));
	app_connected_drive_count = App_ScanDevices();

	sprintf(global_string_buff1, General_GetString(ID_STR_MSG_SHOW_DRIVE_COUNT), app_connected_drive_count);
	Buffer_NewMessage(global_string_buff1);
	
	// if no drives connected, there's nothing for us to do. just quit. 
	if (app_connected_drive_count < 1)
	{
		Buffer_NewMessage(General_GetString(ID_STR_MSG_NO_DRIVES_AVAILABLE));
		App_Exit(ERROR_NO_CONNECTED_DRIVES_FOUND);
	}


	// set up the left panel pointing to the lowest available device
	
	App_LoadOverlay(OVERLAY_FOLDER);
	
	sprintf(the_drive_path, "%u:", global_connected_device[the_drive_index]);

	if ( (root_folder_file_left = File_New(the_drive_path, PARAM_FILE_IS_FOLDER, 0, 0, 0, &this_datetime) ) == NULL)
	{
		App_Exit(ERROR_COULD_NOT_CREATE_ROOT_FOLDER_FILE_LEFT);
	}

	app_active_panel_id = PANEL_ID_LEFT;
	
	if ( (app_root_folder[PANEL_ID_LEFT] = Folder_New(root_folder_file_left, PARAM_MAKE_COPY_OF_FOLDER_FILE, global_connected_device[the_drive_index]) ) == NULL)
	{
		Buffer_NewMessage(General_GetString(ID_STR_ERROR_ALLOC_FAIL));
		App_Exit(ERROR_COULD_NOT_CREATE_ROOT_FOLDER_OBJ_LEFT);
	}

	// we had Folder_New make a copy, so we can free the file object we passed it.
	File_Destroy(&root_folder_file_left);
	
	app_file_panel[PANEL_ID_LEFT].drive_index_ = the_drive_index;


	// set up the right panel pointing to the next lowest available device (will be same if only 1 device)
	
	++the_drive_index;

	// set the 2nd panel to the next device, unless there is only 1 device. in that case, have same device on both sides. 
	if (the_drive_index >= app_connected_drive_count)
	{
		--the_drive_index;
	}
	else
	{
		sprintf(the_drive_path, "%u:", global_connected_device[the_drive_index]);
	}

	if ( (root_folder_file_right = File_New(the_drive_path, PARAM_FILE_IS_FOLDER, 0, 0, 0, &this_datetime) ) == NULL)
	{
		App_Exit(ERROR_COULD_NOT_CREATE_ROOT_FOLDER_FILE_RIGHT);
	}

	App_LoadOverlay(OVERLAY_FOLDER);
	
	if ( (app_root_folder[PANEL_ID_RIGHT] = Folder_New(root_folder_file_right, PARAM_MAKE_COPY_OF_FOLDER_FILE, global_connected_device[the_drive_index]) ) == NULL)
	{
		App_Exit(ERROR_COULD_NOT_CREATE_ROOT_FOLDER_OBJ_RIGHT);
	}

	// we had Folder_New make a copy, so we can free the file object we passed it.
	File_Destroy(&root_folder_file_right);

	app_file_panel[PANEL_ID_RIGHT].drive_index_ = the_drive_index;


	Panel_Initialize(
		&app_file_panel[PANEL_ID_LEFT], 
		app_root_folder[PANEL_ID_LEFT], 
		(UI_LEFT_PANEL_BODY_X1 + 1), (UI_LEFT_PANEL_BODY_Y1 + 2), 
		(UI_LEFT_PANEL_BODY_WIDTH - 2), (UI_LEFT_PANEL_BODY_HEIGHT - 3)
	);
	Panel_Initialize(
		&app_file_panel[PANEL_ID_RIGHT], 
		app_root_folder[PANEL_ID_RIGHT], 
		(UI_RIGHT_PANEL_BODY_X1 + 1), (UI_RIGHT_PANEL_BODY_Y1 + 2), 
		(UI_RIGHT_PANEL_BODY_WIDTH - 2), (UI_RIGHT_PANEL_BODY_HEIGHT - 3)
	);

	Panel_SetCurrentDrive(&app_file_panel[PANEL_ID_LEFT], app_root_folder[PANEL_ID_LEFT]->device_number_);
	Panel_SetCurrentDrive(&app_file_panel[PANEL_ID_RIGHT], app_root_folder[PANEL_ID_RIGHT]->device_number_);
	app_file_panel[PANEL_ID_LEFT].active_ = true;	// we always start out with left panel being the active one
	app_file_panel[PANEL_ID_RIGHT].active_ = false;	// we always start out with left panel being the active one

	//Buffer_NewMessage("Initialization complete.");
}


// handles user input
uint8_t App_MainLoop(void)
{
	uint8_t				user_input;
	bool				exit_main_loop = false;
	bool				success;
	bool				file_menu_active;
	WB2KViewPanel*		the_panel;
	
	// main loop
	while (! exit_main_loop)
	{
		// turn off cursor - seems to turn itself off when kernal detects cursor position has changed. 
		Sys_EnableTextModeCursor(false);
		
		the_panel = &app_file_panel[app_active_panel_id];
		
		//DEBUG: track software stack pointer
		//sprintf(global_string_buffer, "sp: %x%x", *(char*)0x52, *(char*)0x51);
		//Buffer_NewMessage(global_string_buffer);

		do
		{
			// LOGIC:
			//   enable/disable menu buttons -- 
			//   our rule is simple: if active panel has 1 or more files, 1 must be selected, so all file menu items are active
			//   have separate switches for when file menu is active/inactive
			
			// redraw any menu buttons that could change (file menu only at this point)
			file_menu_active = (the_panel->num_rows_ > 0);
			App_LoadOverlay(OVERLAY_SCREEN);
			Screen_DrawFileMenuItems(file_menu_active);

			user_input = getchar();
	
			Screen_DisplayTime();
		
			// first switch: for file menu only, and skip if file menu is inactive
			//   slightly inefficient in that it has to go through them all twice, but this is not a performance bottleneck
			//   note: we also put the sort commands here because it doesn't make sense to sort if no files
			if (file_menu_active)
			{
				switch (user_input)
				{
					case ACTION_SORT_BY_NAME:
						//DEBUG_OUT(("%s %d: Sort by name", __func__ , __LINE__));
						the_panel->sort_compare_function_ = (void*)&File_CompareName;
						Panel_SortFiles(the_panel);
						break;

	// 				case ACTION_SORT_BY_DATE:
	// 					//DEBUG_OUT(("%s %d: Sort by date", __func__ , __LINE__));
	// 					the_panel->sort_compare_function_ = (void*)&File_CompareDate;
	// 					Panel_SortFiles(the_panel);
	// 					Buffer_NewMessage("Files now sorted by date.");
						break;
	// 
					case ACTION_SORT_BY_SIZE:
						//DEBUG_OUT(("%s %d: Sort by size", __func__ , __LINE__));
						the_panel->sort_compare_function_ = (void*)&File_CompareSize;
						Panel_SortFiles(the_panel);
						break;
			
					case ACTION_SORT_BY_TYPE:
						//DEBUG_OUT(("%s %d: Sort by type", __func__ , __LINE__));
						the_panel->sort_compare_function_ = (void*)&File_CompareFileTypeID;
						Panel_SortFiles(the_panel);
						break;
			
					case ACTION_VIEW_AS_HEX:
						//DEBUG_OUT(("%s %d: view as hex", __func__ , __LINE__));
						success = Panel_ViewCurrentFileAsHex(the_panel);	
						App_LoadOverlay(OVERLAY_SCREEN);
						Screen_Render();	// the hex view has completely overwritten the screen
						Panel_RenderContents(&app_file_panel[PANEL_ID_LEFT]);
						Panel_RenderContents(&app_file_panel[PANEL_ID_RIGHT]);
						//sprintf(global_string_buff1, "view as hex success = %u", success);
						//Buffer_NewMessage(global_string_buff1);
						break;
				
					case ACTION_VIEW_AS_TEXT:
						//DEBUG_OUT(("%s %d: view as hex", __func__ , __LINE__));
						success = Panel_ViewCurrentFileAsText(the_panel);	
						App_LoadOverlay(OVERLAY_SCREEN);
						Screen_Render();	// the hex view has completely overwritten the screen
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
						
					default:
						// no need to do any default action: we WANT it to fall through to next switch if none of above happened.
						break;
				}
			}
			
			switch (user_input)
			{
				case ACTION_SWAP_ACTIVE_PANEL:
					// mark old panel inactive, mark new panel active, set new active panel id
					App_SwapActivePanel();
					App_LoadOverlay(OVERLAY_SCREEN);
					Screen_SwapCopyDirectionIndicator();
					the_panel = &app_file_panel[app_active_panel_id];
					break;

				case ACTION_NEXT_DEVICE:
					// tell panel to forget all its files, and repopulate itself from the next drive in the system. 
					success = Panel_SwitchToNextDrive(the_panel, app_connected_drive_count - 1);
					break;
					
				case ACTION_REFRESH_PANEL:
					App_LoadOverlay(OVERLAY_FOLDER);
					Folder_RefreshListing(the_panel->root_folder_);
					Panel_Init(the_panel);			
					break;
				
				case ACTION_FORMAT_DISK:
					success = Panel_FormatDrive(the_panel);
					if (success)
					{
						App_LoadOverlay(OVERLAY_FOLDER);
						Folder_RefreshListing(the_panel->root_folder_);
						Panel_Init(the_panel);			
					}
					break;
					
				case ACTION_NEW_FOLDER:
					success = Panel_MakeDir(the_panel);
					break;

				case ACTION_SET_TIME:
					General_Strlcpy((char*)&global_dlg_title, General_GetString(ID_STR_DLG_SET_CLOCK_TITLE), COMM_BUFFER_MAX_STRING_LEN);
					General_Strlcpy((char*)&global_dlg_body_msg, General_GetString(ID_STR_DLG_SET_CLOCK_BODY), APP_DIALOG_WIDTH);
					global_string_buff2[0] = 0;	// clear whatever string had been in this buffer before
					
					success = Text_DisplayTextEntryDialog(&global_dlg, (char*)&temp_screen_buffer_char, (char*)&temp_screen_buffer_attr, global_string_buff2, 14); //YY-MM-DD HH-MM = 14
					
					if (success)
					{
						// user entered a date/time string, now try to parse and save it.
						success = Sys_UpdateRTC(global_string_buff2);
						Screen_DisplayTime();
					}
					
					break;

				case ACTION_ABOUT:
					Screen_ShowAboutInfo();
					break;
					
				case MOVE_UP:
				case MOVE_UP_ALT:
					success = Panel_SelectPrevFile(the_panel);
					//sprintf(global_string_buff1, "prev file selection success = %u", success);
					//Buffer_NewMessage(global_string_buff1);
					break;

				case MOVE_DOWN:
				case MOVE_DOWN_ALT:
					success = Panel_SelectNextFile(the_panel);
					//sprintf(global_string_buff1, "next file selection success = %u", success);
					//Buffer_NewMessage(global_string_buff1);
					break;

				case MOVE_LEFT:
				case MOVE_LEFT_ALT:
					if (app_active_panel_id == PANEL_ID_RIGHT)
					{
						// mark old panel inactive, mark new panel active, set new active panel id
						App_SwapActivePanel();
						the_panel = &app_file_panel[app_active_panel_id];
					}
					break;

				case MOVE_RIGHT:
				case MOVE_RIGHT_ALT:
					if (app_active_panel_id == PANEL_ID_LEFT)
					{
						// mark old panel inactive, mark new panel active, set new active panel id
						App_SwapActivePanel();
						the_panel = &app_file_panel[app_active_panel_id];
					}
					break;
					
				case ACTION_QUIT:
					General_Strlcpy((char*)&global_dlg_title, General_GetString(ID_STR_DLG_ARE_YOU_SURE), COMM_BUFFER_MAX_STRING_LEN);
					General_Strlcpy((char*)&global_dlg_body_msg, General_GetString(ID_STR_DLG_QUIT_CONFIRM), APP_DIALOG_WIDTH);
					General_Strlcpy((char*)&global_dlg_button[0], General_GetString(ID_STR_DLG_NO), 10);
					General_Strlcpy((char*)&global_dlg_button[1], General_GetString(ID_STR_DLG_YES), 10);
					
					global_dlg.num_buttons_ = 2;
					
					if (Text_DisplayDialog(&global_dlg, (char*)&temp_screen_buffer_char, (char*)&temp_screen_buffer_attr) > 0)
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


// Brings the requested overlay into memory
void App_LoadOverlay(uint8_t the_overlay_id)
{
	zp_bank_num = overlay[the_overlay_id].lut_value_;
	Memory_SwapInNewBank(overlay[the_overlay_id].lut_slot_);
}


// if ending on error: display error message, wait for user to confirm, and exit
// if no error, just exit
void App_Exit(uint8_t the_error_number)
{
	if (the_error_number != ERROR_NO_ERROR)
	{
		sprintf(global_string_buff1, General_GetString(ID_STR_MSG_FATAL_ERROR), the_error_number);
		General_Strlcpy((char*)&global_dlg_title, global_string_buff1, MAX_STRING_COMP_LEN);
		General_Strlcpy((char*)&global_dlg_body_msg, General_GetString(ID_STR_MSG_FATAL_ERROR_BODY), APP_DIALOG_WIDTH);
		General_Strlcpy((char*)&global_dlg_button[0], General_GetString(ID_STR_DLG_OK), 10);
		
		global_dlg.num_buttons_ = 2;
		
		Text_DisplayDialog(&global_dlg, (char*)&temp_screen_buffer_char, (char*)&temp_screen_buffer_attr);
	}

	// free some last things. not sure if this matters, as we're about to exit, but... 
	Folder_Destroy(&app_root_folder[0]);
	Folder_Destroy(&app_root_folder[1]);
	
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

	if (Sys_InitSystem() == false)
	{
		App_Exit(0);
	}
	
	Sys_SetBorderSize(0, 0); // want all 80 cols and 60 rows!

	App_LoadOverlay(OVERLAY_SCREEN);
	
	// initialize the random number generator embedded in the Vicky
	Startup_InitializeRandomNumGen();
	
	// set up pointers to string data that is in EM
	App_LoadStrings();

	// clear screen and draw logo
	Screen_ShowLogo();
	
	// initialize the comm buffer - do this before drawing UI or garbage will get written into comms area
	Buffer_Initialize();
	
	// Initialize screen structures and do first draw
	Screen_InitializeUI();
	Screen_Render();

	App_Initialize();
	
	Panel_Init(&app_file_panel[PANEL_ID_LEFT]);

	if (app_connected_drive_count > 1)
	{
		Panel_Init(&app_file_panel[PANEL_ID_RIGHT]);
	}
	
	App_LoadOverlay(OVERLAY_SCREEN);
	Screen_DisplayTime();
	
	App_MainLoop();
	
	// restore screen, etc.
	App_Exit(ERROR_NO_ERROR);
	
	return 0;
}