/*
 * list_panel.c
 *
 *  Created on: Sep 4, 2020
 *      Author: micahbly
 *
 *  This is a huge cut-down of the Amiga WorkBench2000 code, for F256 f/manager and B128 f/manager
 *    8-bit version started Jan 12, 2023
 */





/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "list_panel.h"
#include "api.h"
#include "app.h"
#include "comm_buffer.h"
#include "debug.h"
#include "file.h"
#include "folder.h"
#include "general.h"
#include "kernel.h" // most kernel calls are covered by stdio.h etc, but mkfs was not, so added this header file
#include "keyboard.h"
#include "list.h"
#include "memory.h"
#include "overlay_em.h"
#include "screen.h"
#include "strings.h"
#include "sys.h"
#include "text.h"

// C includes
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <device.h>
#include <cc65.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

// F256 includes
#include "f256.h"



/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/



/*****************************************************************************/
/*                          File-scoped Variables                            */
/*****************************************************************************/


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

extern char*		global_named_app_basic;

extern char*		global_string_buff1;
extern char*		global_string_buff2;

extern char*		global_temp_path_1;
extern char*		global_temp_path_2;

extern bool					global_find_next_enabled;

extern int8_t				global_connected_device[DEVICE_MAX_DEVICE_COUNT];	// will be 8, 9, etc, if connected, or -1 if not..
extern char*				global_search_phrase;
extern char*				global_search_phrase_human_readable;
extern uint8_t				global_search_phrase_len;

extern TextDialogTemplate	global_dlg;	// dialog we'll configure and re-use for different purposes
extern char					global_dlg_title[36];	// arbitrary
extern char					global_dlg_body_msg[70];	// arbitrary
extern char					global_dlg_button[3][10];	// arbitrary
extern uint8_t				temp_screen_buffer_char[APP_DIALOG_BUFF_SIZE];	// WARNING HBD: don't make dialog box bigger than will fit!
extern uint8_t				temp_screen_buffer_attr[APP_DIALOG_BUFF_SIZE];	// WARNING HBD: don't make dialog box bigger than will fit!

extern uint8_t				zp_bank_num;
extern uint16_t				zp_len;
extern uint8_t				zp_x_cnt;
extern uint8_t				zp_y_cnt;
extern uint8_t				zp_search_loc_byte;
extern uint8_t				zp_search_loc_page;
extern uint8_t				zp_search_loc_bank;

#pragma zpsym ("zp_bank_num");
#pragma zpsym ("zp_len");
#pragma zpsym ("zp_x_cnt");
#pragma zpsym ("zp_y_cnt");
#pragma zpsym ("zp_search_loc_byte");
#pragma zpsym ("zp_search_loc_page");
#pragma zpsym ("zp_search_loc_bank");



extern struct call_args args; // in gadget's version of f256 lib, this is allocated and initialized with &args in crt0. 

/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

// calculate and set positions for the panel's files, when viewed as list
// call this when window is first opened, or when window size changes, or when list of files is changed.
// this does not actually render any icons, it just calculates where they should be in the window's bitmap.
// note: this also sets/resets the surface's required_inner_width_ property (logical internal width vs physical internal width)
void Panel_ReflowContentForDisk(WB2KViewPanel* the_panel);

// calculate and set positions for the panel's memory banks, when viewed as list
// call this when window is first opened, or when window size changes, or when list of files is changed.
// this does not actually render any icons, it just calculates where they should be in the window's bitmap.
// note: this also sets/resets the surface's required_inner_width_ property (logical internal width vs physical internal width)
void Panel_ReflowContentForMemory(WB2KViewPanel* the_panel);


/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/



// calculate and set positions for the panel's files, when viewed as list
// call this when window is first opened, or when window size changes, or when list of files is changed.
// this does not actually render any icons, it just calculates where they should be in the window's bitmap.
// note: this also sets/resets the surface's required_inner_width_ property (logical internal width vs physical internal width)
void Panel_ReflowContentForDisk(WB2KViewPanel* the_panel)
{
	WB2KList*	the_item;
	uint8_t		num_rows;
	uint8_t		max_rows = PANEL_LIST_MAX_ROWS;
	uint8_t		num_files = 0;
	uint16_t	row;
	int8_t		display_row;
	uint8_t		first_viz_row = the_panel->content_top_;
	uint8_t		last_viz_row = first_viz_row + the_panel->height_ - 1;
	
	// LOGIC:
	//   we will scroll as needed vertically
	//   the panel's content_top is 0 when unscrolled (initial position)
	//   a file's y position is calculated based on content top, first row of (inner) panel, and the row # of the file
	//   when scrolled down, any file's that have scrolled up out of the panel will have y positions < top of panel
	//   negative positions will happen for longer directories. 
	
	App_LoadOverlay(OVERLAY_DISKSYS);
	
	num_files = Folder_GetCountFiles(the_panel->root_folder_);

	// see how many rows and V space we need by taking # of files (do NOT include space for a header row: that row is part of different spacer)
	num_rows = num_files;
	
	if (num_rows > max_rows)
	{
		LOG_WARN(("%s %d: this folder is showing %u files, which is more than max of %u", __func__ , __LINE__, num_files, max_rows));
	}
	
	//sprintf(global_string_buff1, "num_files=%u, num_rows=%u", num_files, num_rows);
	//Buffer_NewMessage(global_string_buff1);

	// if there are no files in the folder the panel is showing, we can stop here
	if (num_files == 0)
	{
		the_panel->num_rows_ = 0;
		LOG_INFO(("%s %d: this folder ('%s') shows a file count of 0", __func__ , __LINE__, the_panel->root_folder_->folder_file_->file_name_));
		return;
	}
	
	// set the x and y positions of every file
	// for labels, if the column isn't to be shown, set it's y property to -1
	the_item = *(the_panel->root_folder_->list_);

	// no files?
	if ( the_item == NULL )
	{
		//sprintf(global_string_buff1, "this folder ('%s') shows a file count of %u but file list seems to be empty!", the_panel->root_folder_->folder_file_->file_name_, num_files);
		//Buffer_NewMessage(global_string_buff1);
		LOG_ERR(("%s %d: this folder ('%s') shows a file count of %u but file list seems to be empty!", __func__ , __LINE__, the_panel->root_folder_->folder_file_->file_name_, num_files));
		App_Exit(ERROR_NO_FILES_IN_FILE_LIST); // crash early, crash often
	}
	
	for (row = 0; row < num_rows && the_item; row++)
	{
		WB2KFileObject*	this_file;
		
		if (row >= first_viz_row && row <= last_viz_row)
		{
			display_row = row - first_viz_row;
		}
		else
		{
			display_row = -1;
		}

		this_file = (WB2KFileObject*)(the_item->payload_);

		// store the icon's x, y, and rect info so we can use it for mouse detection
		File_UpdatePos(this_file, the_panel->x_, display_row, row);

		//sprintf(global_string_buff1, "file '%s' display row=%i, row=%u, first_viz_row=%u", this_file->file_name_, display_row, row, first_viz_row);
		//Buffer_NewMessage(global_string_buff1);
		
		// get next node
		the_item = the_item->next_item_;
	}
	//printf("width checker found %i files; num_cols: %i, col=%i\n", num_files, num_cols, col);

	// remember number of rows used
	the_panel->num_rows_ = num_rows;
}


// calculate and set positions for the panel's memory banks, when viewed as list
// call this when window is first opened, or when window size changes, or when list of files is changed.
// this does not actually render any icons, it just calculates where they should be in the window's bitmap.
// note: this also sets/resets the surface's required_inner_width_ property (logical internal width vs physical internal width)
void Panel_ReflowContentForMemory(WB2KViewPanel* the_panel)
{
	uint16_t	row;
	int8_t		display_row;
	uint8_t		first_viz_row = the_panel->content_top_;
	uint8_t		last_viz_row = first_viz_row + the_panel->height_ - 1;
	
	// LOGIC:
	//   we will scroll as needed vertically
	//   the panel's content_top is 0 when unscrolled (initial position)
	//   a file's y position is calculated based on content top, first row of (inner) panel, and the row # of the file
	//   when scrolled down, any file's that have scrolled up out of the panel will have y positions < top of panel
	//   negative positions will happen for longer directories. 
	
	// see how many rows and V space we need by taking # of files (do NOT include space for a header row: that row is part of different spacer)

	App_LoadOverlay(OVERLAY_MEMSYSTEM);
	
	for (row = 0; row < MEMORY_BANK_COUNT; row++)
	{
		FMBankObject*	this_bank;
		
		if (row >= first_viz_row && row <= last_viz_row)
		{
			display_row = row - first_viz_row;
		}
		else
		{
			display_row = -1;
		}

		this_bank = &the_panel->memory_system_->bank_[row];

		// store the icon's x, y, and rect info so we can use it for mouse detection
		Bank_UpdatePos(this_bank, the_panel->x_, display_row, row);
	}

	// remember number of rows used
	the_panel->num_rows_ = MEMORY_BANK_COUNT;
}


// // on change to size of parent surface, redefine dimensions of the panel as appropriate for list mode
// // define the width of each list column, and whether or not there is enough space to render it.
// void Panel_UpdateSizeListMode(WB2KViewPanel* the_panel, uint8_t x, uint8_t y, uint8_t width, uint8_t height)
// {
// 	// LOGIC:
// 	//   As window gets too narrow, we shrink filename column, then start dropping columns entirely
// 	//   Priority for displaying columns is: ALWAYS: icon and name; type > date > size
// 
// 	bool		scroller_reset_needed = false;
// 
// 	// first check if new height/width/x/y are different. if not, stop here. 
// 	if (the_panel->x_ == x && the_panel->y_ == y && the_panel->width_ == width && the_panel->height_ == height)
// 	{
// 		return;
// 	}
// 	
// 	// check if height changed: if so, reset content top to 0, because we probably have to reflow
// 	if (the_panel->height_ != height)
// 	{
// 		the_panel->content_top_ = 0;
// 		scroller_reset_needed = true;
// 	}
// 	
// 	// accept new size/pos data
// 	the_panel->width_ = width;
// 	the_panel->height_ = height;
// 	the_panel->x_ = x;
// 	the_panel->y_ = y;
// 
// // 	// set our required inner width (and publish to parent surface as these are identical in case of list mode)
// // 	the_panel->required_inner_width_ = required_h_space;
// // 	
// // 	if (the_panel->my_parent_surface_->required_inner_width_ != the_panel->required_inner_width_)
// // 	{
// // 		// inform window it should recalculate the scrollbar sliders
// // 		the_panel->my_parent_surface_->required_inner_width_ = the_panel->required_inner_width_;
// // 		scroller_reset_needed = true;
// // 	}
// // 
// // 	// reset scrollers if either required inner height/width changed, or if available height/width changed
// // 	//   (whatever is appropriate to this view mode)
// // 	if (scroller_reset_needed)
// // 	{
// // 		Window_ResetScrollbars(the_panel->my_parent_surface_);
// // 	}
// 	
// 	return;
// }

// display the title of the panel only
// inverses the title if the panel is active, draws it normally if inactive
void Panel_RenderTitleOnly(WB2KViewPanel* the_panel);




/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/



// **** CONSTRUCTOR AND DESTRUCTOR *****


// (re)initializer: does not allocate. takes a valid panel and resets it to starting values (+ those passed)
void Panel_Initialize(WB2KViewPanel* the_panel, bool for_disk, uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
	// deal with disk vs memory differences
	if (for_disk)
	{
		the_panel->memory_system_ = NULL;
		the_panel->sort_compare_function_ = (void*)&File_CompareName;
	}
	else
	{
		the_panel->root_folder_ = NULL;
	
		if (the_panel->memory_system_->is_flash_)
		{
			the_panel->device_number_ = DEVICE_FLASH;
		}
		else
		{
			the_panel->device_number_ = DEVICE_RAM;
		}
	}
	
	// some common attributes
	the_panel->for_disk_ = for_disk;
	the_panel->x_ = x;
	the_panel->y_ = y;
	the_panel->width_ = width;
	the_panel->height_ = height;
	the_panel->content_top_ = 0;
	the_panel->num_rows_ = 0;

	//DEBUG_OUT(("%s %d: filename='%s'", __func__ , __LINE__, the_panel->root_folder_->folder_file_->file_name_));

	return;
}


// Forget all its files, and repopulate from the specified disk or memory system
bool Panel_SwitchDevice(WB2KViewPanel* the_panel, device_number the_device)
{
	char		path_buff[3];
	bool		for_flash;
	bool		was_disk;
	
	// LOGIC:
	//   we don't technically need to free the folder and file list when switching from file view to mem view, or vice versa
	//     it will get reset next time we load a file view (or bank view)
	//   however, we don't have enough memory available, so practically, we have to destroy the folder or memsys each switchover
	//     do not need to destroy when going file to file, or memsys to memsys. it has resets for that. 
	
	the_panel->device_number_ = the_device;
	
	// capture state of panel before switch, so we can check if we switched from disk to memsys or vice versa
	was_disk = the_panel->for_disk_;

	if (the_device < DEVICE_MAX_DISK_DEVICE)
	{
		the_panel->for_disk_ = true;
	}
	else
	{
		the_panel->for_disk_ = false;
	}

	if ( was_disk == true && the_panel->for_disk_ == false)
	{
		// switched from disk to memsys: free folder-related memory
		App_LoadOverlay(OVERLAY_DISKSYS);
		Folder_Destroy(&the_panel->root_folder_);
	}
	else if ( was_disk == false && the_panel->for_disk_ == true)
	{
		// switched from memsys to disk: free memsys-related memory
		App_LoadOverlay(OVERLAY_MEMSYSTEM);
		MemSys_Destroy(&the_panel->memory_system_);
	}

	if (the_panel->for_disk_ == true)
	{
		the_panel->sort_compare_function_ = (void*)&File_CompareName;

		sprintf(path_buff, "%d:", the_device);
		
		App_LoadOverlay(OVERLAY_DISKSYS);

		if ( (the_panel->root_folder_ = Folder_NewOrReset(the_panel->root_folder_, the_device, path_buff)) == NULL)
		{
			LOG_ERR(("%s %d: could not free the panel's root folder", __func__ , __LINE__));
			App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
		}
	}
	else
	{
		App_LoadOverlay(OVERLAY_MEMSYSTEM);

		if (the_device == DEVICE_RAM)
		{
			for_flash = false;
		}
		else
		{
			for_flash = true;
		}
		
		if ( (the_panel->memory_system_ = MemSys_NewOrReset(the_panel->memory_system_, for_flash)) == NULL)
		{
			Buffer_NewMessage(General_GetString(ID_STR_ERROR_ALLOC_FAIL));
			App_Exit(ERROR_COULD_NOT_CREATE_OR_RESET_MEMSYS_OBJ);
		}
	}	
	
	Panel_Refresh(the_panel);
	
	return true;
}




// **** SETTERS *****


// sets the current device number (CBM device number, eg, 8-9-10-11 or FNX drive num, eg 0, 1, or 2) the panel is using
// does not refresh. Repopulate to do that.
void Panel_SetCurrentDevice(WB2KViewPanel* the_panel, device_number the_device_num)
{
	the_panel->device_number_ = the_device_num;
}


// tells the panel to toggle its active/inactive state, and redraw its title appropriately
void Panel_ToggleActiveState(WB2KViewPanel* the_panel)
{
	if (the_panel->active_ == true)
	{
		the_panel->active_ = false;
	}
	else
	{
		the_panel->active_ = true;
	}
	
	Panel_RenderTitleOnly(the_panel);
	Panel_RenderContents(the_panel);
}


// **** GETTERS *****


// // returns true if the folder in the panel has any currently selected files/folders
// bool Panel_HasSelections(WB2KViewPanel* the_panel)
// {
// 	if (the_panel == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_PANEL_WAS_NULL); // crash early, crash often
// 	}
// 	
// 	return Folder_HasSelections(the_panel->root_folder_);
// }


// // returns number of currently selected files in this panel
// uint16_t Panel_GetCountSelectedFiles(WB2KViewPanel* the_panel)
// {
// 	if (the_panel == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_PANEL_WAS_NULL); // crash early, crash often
// 	}
// 	
// 	return Folder_GetCountSelectedFiles(the_panel->root_folder_);
// }


// // return the root folder
// WB2KFolderObject* Panel_GetRootFolder(WB2KViewPanel* the_panel)
// {
// 	if (the_panel == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_PANEL_WAS_NULL); // crash early, crash often
// 	}
// 	
// 	return the_panel->root_folder_;
// }



// **** OTHER FUNCTIONS *****


// create a new folder in the current one
bool Panel_MakeDir(WB2KViewPanel* the_panel)
{
	bool				success;
	uint8_t				current_path_len;
	uint8_t				available_len;
	uint8_t				temp_dialog_width;

	temp_dialog_width = global_dlg.width_ - 2;
	
	// calculate the max length of the folder the user can enter, based on max path len - current len - 1 for separator
	current_path_len = General_Strnlen(the_panel->root_folder_->file_path_, FILE_MAX_PATHNAME_SIZE);
	available_len = FILE_MAX_PATHNAME_SIZE - current_path_len - 1;

	// we are hard limited by max window width at this point. 80-2 for box chars.
	if (available_len > temp_dialog_width)
	{
		available_len = temp_dialog_width;
	}

// 	sprintf(global_string_buff1, "current_path_len=%u, available_len=%u", current_path_len, available_len);
// 	Buffer_NewMessage(global_string_buff1);
// 	sprintf(global_string_buff1, "temp_dialog_width=%u, orig_dialog_width=%u", temp_dialog_width, orig_dialog_width);
// 	Buffer_NewMessage(global_string_buff1);
	
	General_Strlcpy((char*)&global_dlg_title, General_GetString(ID_STR_DLG_NEW_FOLDER_TITLE), 36);
	General_Strlcpy((char*)&global_dlg_body_msg, General_GetString(ID_STR_DLG_ENTER_NEW_FOLDER_NAME), 70);
	global_string_buff2[0] = 0;	// clear whatever string had been in this buffer before
	
	success = Text_DisplayTextEntryDialog(&global_dlg, (char*)&temp_screen_buffer_char, (char*)&temp_screen_buffer_attr, global_string_buff2, available_len, APP_ACCENT_COLOR, APP_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);

	// did user enter a name?
	if (success == false)
	{
		return false;
	}

	General_CreateFilePathFromFolderAndFile(global_temp_path_1, the_panel->root_folder_->file_path_, global_string_buff2);

	success = Kernal_MkDir(global_temp_path_1, the_panel->device_number_);
	
	if (success == false)
	{
		return false;
	}
	
	// renew file listing
	Panel_Refresh(the_panel);			

	return success;
}


// format the specified drive
// DANGER WILL ROBINSON!
bool Panel_FormatDrive(WB2KViewPanel* the_panel)
{
	int8_t				result_code;

	App_LoadOverlay(OVERLAY_SCREEN);
	if (Screen_ShowUserTwoButtonDialog(
		General_GetString(ID_STR_DLG_FORMAT_TITLE), 
		ID_STR_DLG_ARE_YOU_SURE, 
		ID_STR_DLG_YES, 
		ID_STR_DLG_NO
		) != 1)
	{
		return false;
	}

	// leaving dialog title still at format?, pull it up again with text input field so user can enter new disk name
	General_Strlcpy(global_string_buff1, General_GetString(ID_STR_DLG_ENTER_NEW_NAME), 70);
	*global_string_buff2 = 0;
	
	global_string_buff2 = Screen_GetStringFromUser(General_GetString(ID_STR_DLG_FORMAT_TITLE), global_string_buff1, global_string_buff2, FILE_MAX_FILENAME_SIZE);

	// did user enter a name?
	if (global_string_buff2 == NULL)
	{
		return false;
	}

	Buffer_NewMessage(General_GetString(ID_STR_MSG_FORMATTING));
	
	if ( (result_code = mkfs(global_string_buff2, the_panel->device_number_)) < 0)
	{		
		Buffer_NewMessage(General_GetString(ID_STR_ERROR_GENERIC_DISK));
		LOG_INFO(("%s %d: Kernel reported error formatting drive %u: %i", __func__ , __LINE__, the_panel->device_number_, result_code));
		return false;
	}

	Buffer_NewMessage(General_GetString(ID_STR_MSG_DONE));
	
	return true;
}


// Reset a view panel display properties and renew the listing
bool Panel_Refresh(WB2KViewPanel* the_panel)
{
	uint8_t		the_error_code;

	if (the_panel == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_PANEL_WAS_NULL); // crash early, crash often
	}

	//DEBUG_OUT(("%s %d: the_panel->device_number_=%u, for_disk=%u", __func__ , __LINE__, the_panel->device_number_, the_panel->for_disk_));

	// reset the first visible row to 0 from whatever it might have been if user had scrolled down
	the_panel->content_top_ = 0;

	if (the_panel->for_disk_ == true)
	{
		Buffer_NewMessage(General_GetString(ID_STR_MSG_READING_DIR));
		
		App_LoadOverlay(OVERLAY_DISKSYS);
		
		// have root folder clear out its list of files
		Folder_DestroyAllFiles(the_panel->root_folder_);

		// have root folder populate its list of files
		if ( (the_error_code = Folder_PopulateFiles(the_panel->root_folder_)) > ERROR_NO_ERROR)
		{		
			LOG_INFO(("%s %d: Root folder reported that file population failed with error %u", __func__ , __LINE__, the_error_code));
			//sprintf(global_string_buff1, "pop err %u", the_error_code);
			//Buffer_NewMessage(global_string_buff1);
			
			//Panel_ClearDisplay(the_panel);	// clear out the list, visually at least
			return false;
		}
	}
	else
	{
		App_LoadOverlay(OVERLAY_MEMSYSTEM);
		MemSys_ResetAllBanks(the_panel->memory_system_);
		MemSys_PopulateBanks(the_panel->memory_system_);
	}

	// sort and display contents
	Panel_SortAndDisplay(the_panel);
		
	return true;
}


// // check to see if an already-selected file is under the mouse pointer
// bool Panel_CheckForAlreadySelectedIconUnderMouse(WB2KViewPanel* the_panel, MouseTracker* the_mouse)
// {
// 	// LOGIC:
// 	//   iterate through all files in the panel's list
// 	//   check the coords passed for intersection with the icon's x, y, height, width. same for label
// 
// 	WB2KList*	the_item;
// 	
// 	if (the_panel == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME); // crash early, crash often
// 	}
// 
// 	// define the selection area based on pointer + pointer radius (we won't be in lasso mode if this function is called)
// 	Mouse_UpdateSelectionRectangle(the_mouse, the_panel->my_parent_surface_->content_left_, the_panel->content_top_);
// 
// 	the_item = *(the_panel->root_folder_->list_);
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject* this_file = (WB2KFileObject*)(the_item->payload_);
// 		WB2KLabel* this_label = this_file->label_[PANEL_LIST_NAME_COL_NUM];
// 
// 		if ( Mouse_DetectOverlap(the_mouse, this_label->rect_) || Mouse_DetectOverlap(the_mouse, this_file->icon_rect_) )
// 		{
// 			if (this_file->selected_)
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


// // check if the passed X coordinate is owned by this panel. returns true if x is between x_ and x_ + width_
// bool Panel_OwnsX(WB2KViewPanel* the_panel, int16_t x)
// {
// 	if (the_panel == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_PANEL_WAS_NULL); // crash early, crash often
// 	}
// 
// 	if (x >= the_panel->x_ && x <= (the_panel->x_ + the_panel->width_))
// 	{
// 		return true;
// 	}
// 	
// 	return false;
// }


// // For mouse drag only: check to see if a folder is at the coordinates passed (in case user will drop them here)
// // if a folder is under the mouse, it will set that folder as the global drop target
// // returns true if any folder is under the mouse pointer
// bool Panel_CheckForMouseOverFolder(WB2KViewPanel* the_panel, MouseTracker* the_mouse, bool highlight_if_folder)
// {
// 	// LOGIC:
// 	//   iterate through all files in the panel's list
// 	//   check the coords passed for intersection with the icon's x, y, height, width. same for label
// 
// 	WB2KList*			the_item;
// 	uint16_t		x_bound;
// 	uint16_t		y_bound;
// 	
// 	if (the_panel == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME); // crash early, crash often
// 	}
// 
// 	x_bound = the_panel->x_ + the_panel->width_;
// 	y_bound = the_panel->y_ + the_panel->height_;
// 
// 	// define the selection area based on lasso coords (if lasso mode) or just pointer + pointer radius
// 	Mouse_UpdateSelectionRectangle(the_mouse, the_panel->my_parent_surface_->content_left_, the_panel->content_top_);
// 
// 	the_item = *(the_panel->root_folder_->list_);
// 
// 	if (the_item == NULL)
// 	{
// 		return false;
// 	}
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);
// 		WB2KLabel*			this_label = this_file->label_[PANEL_LIST_NAME_COL_NUM];
// 
// 		if ( Mouse_DetectOverlap(the_mouse, this_label->rect_) || Mouse_DetectOverlap(the_mouse, this_file->icon_rect_) )
// 		{
// 			if (this_file->is_directory_)
// 			{
// 				// mouse is dragged over a folder. if this folder is already selected, it means it is part of the drag. prevent it from being ALSO selected as the drag target.
// 				if (this_file->selected_)
// 				{
// 					return false;
// 				}
// 				
// 				if (highlight_if_folder)
// 				{
// 					// remember the last (=only) folder found, on global level
// 					if (File_MarkAsDropTarget(this_file, the_panel->my_parent_surface_->content_left_, the_panel->content_top_, x_bound, y_bound) != false)
// 					{
// 						FileMover_SetTargetFolderFile(global_app->file_mover_, the_panel->my_parent_surface_, the_panel, this_file);
// 					}
// 				}
// 
// 				return true;
// 			}
// 		}
// 		
// 		the_item = the_item->next_item_;
// 	}
// 
// 	return false;
// }


// // check to see if any files were selected at the coordinates passed.
// // returns 0 if nothing selected, 1 if 1 file, 2 if multiple files, or 3 if one folder (using enums)
// // if do_selection is true, and icon(s) are found, the icons will be flagged as selected and re-rendered with selection appearance
// // if highlight_if_folder is true, and ONE folder is found, that folder will get re-rendered as selected (but not marked as selected)
// IconSelectionResult Panel_CheckForMouseSelection(WB2KViewPanel* the_panel, MouseTracker* the_mouse, bool do_selection, bool highlight_if_folder)
// {
// 	// LOGIC:
// 	//   iterate through all files in the panel's list
// 	//   check the coords passed for intersection with the icon's x, y, height, width. same for label
// 	//   if do_selection is TRUE, then the goal is to select icons. it will set source for FileMover, and clear target.
// 	//   if do_selection is FALSE, then the goal is to see if we are dragging something onto another folder. it will set TARGET for FileMover
// 
// 	int					num_files_selected = 0;
// 	WB2KList*			the_item;
// 	uint16_t		x_bound;
// 	uint16_t		y_bound;
// 	bool				at_least_one_folder_selected = false;
// 	
// 	if (the_panel == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_DEFINE_ME); // crash early, crash often
// 	}
// 
// 	x_bound = the_panel->x_ + the_panel->width_;
// 	y_bound = the_panel->y_ + the_panel->height_;
// 
// 	// define the selection area based on lasso coords (if lasso mode) or just pointer + pointer radius
// 	Mouse_UpdateSelectionRectangle(the_mouse, the_panel->my_parent_surface_->content_left_, the_panel->content_top_);
// 
// 	the_item = *(the_panel->root_folder_->list_);
// 
// 	if (the_item == NULL)
// 	{
// 		return selection_error;
// 	}
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);
// 		WB2KLabel*			this_label = this_file->label_[PANEL_LIST_NAME_COL_NUM];
// 
// 		if ( Mouse_DetectOverlap(the_mouse, this_label->rect_) || Mouse_DetectOverlap(the_mouse, this_file->icon_rect_) )
// 		{
// 			++num_files_selected;
// 
// 			if (this_file->is_directory_)
// 			{
// 				// this folder is not part of the drag selection, so ok to let it be the drag target
// 				at_least_one_folder_selected = true;
// 			}			
// 
// 			// mark file as selected?
// 			if (do_selection)
// 			{
// 				if (File_MarkSelected(this_file, the_panel->my_parent_surface_->content_left_, the_panel->content_top_, x_bound, y_bound, the_panel->col_highest_visible_) == false)
// 				{
// 					// the passed file was null. do anything?
// 					LOG_ERR(("%s %d: couldn't mark file '%s' as selected", __func__ , __LINE__, this_file->file_name_));
// 					App_Exit(ERROR_DEFINE_ME);
// 				}
// 			}
// 		}
// 
// 		the_item = the_item->next_item_;
// 	}
// 
// 
// 	// now proceed with rest of return logic
// 	if (num_files_selected == 0)
// 	{
// 		// if this was not a check to see if we were dropping on a folder, then we have selected nothing, so clear any existing FileMover source
// 		if (do_selection)
// 		{
// 			FileMover_ClearSource(global_app->file_mover_);
// 		}
// 		
// 		return selection_none;
// 	}
// 	else
// 	{
// 		FileMover_SetSource(global_app->file_mover_, the_panel->my_parent_surface_, the_panel, the_panel->root_folder_);
// 		
// 		if (num_files_selected == 1)
// 		{
// 			if (at_least_one_folder_selected)
// 			{
// 				return selection_single_folder;
// 			}
// 			else
// 			{
// 				return selection_single_file;
// 			}
// 		}
// 		else
// 		{
// 			return selection_multiple_file;
// 		}
// 	}
// }

	
// fill the currently selected memory bank with a value supplied by the user
bool Panel_FillCurrentBank(WB2KViewPanel* the_panel)
{
	App_LoadOverlay(OVERLAY_MEMSYSTEM);

	// check if user would be writing over f/manager RAM
	if (MemSys_BankIsWriteable(the_panel->memory_system_) == false)
	{
		Buffer_NewMessage(General_GetString(ID_STR_ERROR_ATTEMPT_TO_OVERWRITE_FM_RAM));
		return false;
	}
	
	return (MemSys_FillCurrentBank(the_panel->memory_system_));
}						

	
// fill the currently selected memory bank with zeros
bool Panel_ClearCurrentBank(WB2KViewPanel* the_panel)
{
	App_LoadOverlay(OVERLAY_MEMSYSTEM);

	// check if user would be writing over f/manager RAM
	if (MemSys_BankIsWriteable(the_panel->memory_system_) == false)
	{
		Buffer_NewMessage(General_GetString(ID_STR_ERROR_ATTEMPT_TO_OVERWRITE_FM_RAM));
		return false;
	}
	
	return (MemSys_ClearCurrentBank(the_panel->memory_system_));
}						


// rename the currently selected file
bool Panel_RenameCurrentFile(WB2KViewPanel* the_panel)
{
	WB2KFileObject*		the_file;
	bool				success;
	char*				new_file_name;
	
	App_LoadOverlay(OVERLAY_DISKSYS);
	
	the_file = Folder_GetCurrentFile(the_panel->root_folder_);

	if (the_file == NULL)
	{
		return false;
	}

	//sprintf(global_string_buff1, "file to rename='%s'", the_file->file_name_);
	//Buffer_NewMessage(global_string_buff1);

	sprintf(global_string_buff1, General_GetString(ID_STR_DLG_RENAME_TITLE), the_file->file_name_);

	// copy the current file name into the edit buffer so user can edit
	General_Strlcpy(global_string_buff2, the_file->file_name_, FILE_MAX_FILENAME_SIZE);

	App_LoadOverlay(OVERLAY_SCREEN);
	new_file_name = Screen_GetStringFromUser(global_string_buff1, General_GetString(ID_STR_DLG_ENTER_NEW_NAME), global_string_buff2, FILE_MAX_FILENAME_SIZE);
	App_LoadOverlay(OVERLAY_DISKSYS);
	
	if (new_file_name == NULL)
	{
		//sprintf(global_string_buff1, "user canceled out of rename dialog");
		//Buffer_NewMessage(global_string_buff1);
		return false;
	}
	
	General_CreateFilePathFromFolderAndFile(global_temp_path_1, the_panel->root_folder_->file_path_, the_file->file_name_);
	General_CreateFilePathFromFolderAndFile(global_temp_path_2, the_panel->root_folder_->file_path_, global_string_buff2);

// 	sprintf(global_string_buff1, "new path='%s'", global_temp_path_1);
// 	Buffer_NewMessage(global_string_buff1);
// 	sprintf(global_string_buff1, "new filename='%s'", global_string_buff2);
// 	Buffer_NewMessage(global_string_buff1);

	strcpy(global_string_buff1, global_string_buff2); // get a copy of the filename because it won't be available after rename
	
	success = File_Rename(the_file, global_string_buff2, global_temp_path_1, global_temp_path_2);
	
	if (success == false)
	{
		return false;
	}
	
	// renew file listing
	Panel_RenderContents(the_panel);
	
// 	sprintf(global_string_buff2, General_GetString(ID_STR_MSG_RENAME_SUCCESS), global_string_buff1, the_file->file_name_);
// 	Buffer_NewMessage(global_string_buff2);
	
	return success;
}


// Launch current file if EXE, or load font if FNT, open directory if dir, etc.
bool Panel_OpenCurrentFileOrFolder(WB2KViewPanel* the_panel)
{
	WB2KFileObject*		the_file;
	bool				success;

	if (the_panel->for_disk_ == true)
	{
		App_LoadOverlay(OVERLAY_DISKSYS);
		
		the_file = Folder_GetCurrentFile(the_panel->root_folder_);
	
		if (the_file == NULL)
		{
			return false;
		}
		
		General_CreateFilePathFromFolderAndFile(global_temp_path_1, the_panel->root_folder_->file_path_, the_file->file_name_);
		
		if (the_file->file_type_ == _CBM_T_DIR)
		{
			if (the_panel->root_folder_->is_meatloaf_ == false)
			{
				// normal "go up" behavior pretty straightforward: just feed it the path.
				if ( (the_panel->root_folder_ = Folder_NewOrReset(the_panel->root_folder_, the_panel->device_number_, global_temp_path_1)) == NULL )
				{
					LOG_ERR(("%s %d: could not reset the panel's root folder", __func__ , __LINE__));
					App_Exit(ERROR_DEFINE_ME);	// crash early, crash often
				}
			}
			else
			{
				// meatloaf go up is different: to go "up", it will accept "_" (<- on C-64), or "..", but you have to "load" it as if loading a file. 
				//   This is the mechanism MEATLOAF appears to key off of. Nothing will be loaded, it will "fail" from f/m point of view, but meatloaf will have changed dirs
				//   for a regular file, just pass the filename, and "load" it. 
				//   for anything, it needs the "1:" or "2:" prefix, or microkernel will not do it. 
				// then force f/manager to refresh
				
				// try to change directory by "loading" the file. 
				sprintf(global_temp_path_1, "%u:%s", the_panel->root_folder_->device_number_, the_file->file_name_);
				success = File_LoadFileToEM(global_temp_path_1, EM_STORAGE_START_PHYS_BANK_NUM);
				
				//sprintf(global_string_buff1, "Trying to change meatloaf dirs with '%s'...", global_temp_path_1);
				//Buffer_NewMessage(global_string_buff1);
			}
	
			Panel_Refresh(the_panel);
			success = true;
		}
		else if (the_file->file_type_ == FNX_FILETYPE_FONT)
		{
			success = File_ReadFontData(global_temp_path_1);
		}
		else if (the_file->file_type_ == FNX_FILETYPE_EXE || the_file->file_type_ == FNX_FILETYPE_IMAGE)
		{
			// this works because pexec can display images as well as log executables
			success = Kernal_LoadApp(global_temp_path_1, NULL);
		}
		else if (the_file->file_type_ == FNX_FILETYPE_MUSIC)
		{
			global_temp_path_2 = General_GetString(ID_STR_APP_PATH_MOD_PLAYER);
			
			if (File_CheckForFile(global_temp_path_2, ID_STR_ERROR_NO_MOD_PLAYER) == true)
			{
				success = Kernal_LoadApp(global_temp_path_2, global_temp_path_1);
			}
			else
			{
				success = false;
			}
		}
		else if (the_file->file_type_ == FNX_FILETYPE_MIDI)
		{
			global_temp_path_2 = General_GetString(ID_STR_APP_PATH_MIDI_PLAYER);
			
			if (File_CheckForFile(global_temp_path_2, ID_STR_ERROR_NO_MIDI_PLAYER) == true)
			{
				success = Kernal_LoadApp(global_temp_path_2, global_temp_path_1);
			}
			else
			{
				success = false;
			}
		}
		else if (the_file->file_type_ == FNX_FILETYPE_TEXT)
		{
			global_temp_path_2 = General_GetString(ID_STR_APP_PATH_TEXT_EDITOR);
			
			if (File_CheckForFile(global_temp_path_2, ID_STR_ERROR_NO_TEXT_EDITOR) == true)
			{
				success = Kernal_LoadApp(global_temp_path_2, global_temp_path_1);
			}
			else
			{
				success = false;
			}
		}
		else if (the_file->file_type_ == FNX_FILETYPE_BASIC)
		{
			// until SuperBASIC will accept a file path, only thing we can do is load file into $28000, tell user to type "XGO" once basic loads, then switch to basic.
			success = File_LoadFileToEM(global_temp_path_1, EM_STORAGE_START_PHYS_BANK_NUM);
			
			if (success)
			{
				Buffer_NewMessage(General_GetString(ID_STR_MSG_BASIC_LOAD_INSTRUCTIONS));
				Keyboard_GetChar();
							
				//success = Kernal_RunBASIC();
				Kernal_RunNamed(global_named_app_basic, 5);	// this will only ever return in an error condition. 
				success = false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		App_LoadOverlay(OVERLAY_MEMSYSTEM);
		success = MemSys_ExecuteCurrentRow(the_panel->memory_system_);
	}

	
	return success;
}


// delete the currently selected file
bool Panel_DeleteCurrentFile(WB2KViewPanel* the_panel)
{
	WB2KFileObject*		the_file;
	int16_t				the_current_row;
	bool				success;
	char				delete_file_name_buff[FILE_MAX_FILENAME_SIZE];
	char*				delete_file_name = delete_file_name_buff;
	
	App_LoadOverlay(OVERLAY_DISKSYS);
	
	the_current_row = Folder_GetCurrentRow(the_panel->root_folder_);
	
	if (the_current_row < 0)
	{
		return false;
	}
	
	the_file = Folder_FindFileByRow(the_panel->root_folder_, the_current_row);
	strcpy(delete_file_name, the_file->file_name_);
	sprintf(global_string_buff1, General_GetString(ID_STR_DLG_DELETE_TITLE), delete_file_name);

	App_LoadOverlay(OVERLAY_SCREEN);

	if (Screen_ShowUserTwoButtonDialog(
		global_string_buff1, 
		ID_STR_DLG_ARE_YOU_SURE, 
		ID_STR_DLG_YES, 
		ID_STR_DLG_NO
		) != 1)
	{
		return false;
	}

	App_LoadOverlay(OVERLAY_DISKSYS);
	General_CreateFilePathFromFolderAndFile(global_temp_path_1, the_panel->root_folder_->file_path_, the_file->file_name_);

	success = File_Delete(global_temp_path_1, the_file->is_directory_);
	
	if (success == false)
	{
		if (the_file->is_directory_)
		{
			Buffer_NewMessage(General_GetString(ID_STR_MSG_DELETE_DIR_FAILURE));
		}
		else
		{
			Buffer_NewMessage(General_GetString(ID_STR_MSG_DELETE_FILE_FAILURE));
		}
		
		return false;
	}
	
	// renew file listing
	Panel_Refresh(the_panel);

	// try to select the file that was selected before the deleted one
	Panel_SetFileSelectionByRow(the_panel, the_current_row, true);
	
	// now send the message
	sprintf(global_string_buff1, General_GetString(ID_STR_MSG_DELETE_SUCCESS), delete_file_name);
	Buffer_NewMessage(global_string_buff1);
	
	return success;
}


// copy the currently selected file to the other panel
bool Panel_CopyCurrentFile(WB2KViewPanel* the_panel, WB2KViewPanel* the_other_panel)
{
	uint8_t				i;
	uint8_t				src_bank_num;
	uint8_t				dst_bank_num;
	uint32_t			percent_read;
	bool				success = false;
	FILE*				the_target_handle;
	WB2KFileObject*		the_file;
	char*				the_name;
	uint8_t*			the_buffer = (uint8_t*)STORAGE_FILE_BUFFER_1;

	// for BSAVE

	// possible scenarios:
	//   1. both panels are disk systems. copy disk to disk
	//   2. src panel is disk, target panel is memory: copy file from disk to memory, starting with selected bank in other panel
	//   3. src panel is memory, other panel is disk: copy 8192 bytes from current bank to a new file in the other panel. ask for filename
	//   4. src panel is memory, other panel is memory: copy 8192 bytes from current bank to other panel's selected bank
	
	if (the_panel->for_disk_ == false)
	{
		App_LoadOverlay(OVERLAY_MEMSYSTEM);
		src_bank_num = MemSys_GetCurrentBankNum(the_panel->memory_system_);
	}
	
	if (the_other_panel->for_disk_ == false)
	{
		App_LoadOverlay(OVERLAY_MEMSYSTEM);
		dst_bank_num = MemSys_GetCurrentBankNum(the_other_panel->memory_system_);
	}
	
	if (the_panel->for_disk_ == true && the_other_panel->for_disk_ == true)
	{
		// copy a file from disk to disk
		App_LoadOverlay(OVERLAY_DISKSYS);
		success = Folder_CopyCurrentFile(the_panel->root_folder_, the_other_panel->root_folder_);
		
		if (success)
		{
			Buffer_NewMessage(General_GetString(ID_STR_MSG_DONE));
		}
		else
		{
			Buffer_NewMessage(General_GetString(ID_STR_ERROR_GENERIC_DISK));
		}
	}
	else if (the_panel->for_disk_ == true && the_other_panel->for_disk_ == false)
	{
		// load a file from disk into memory
		App_LoadOverlay(OVERLAY_DISKSYS);
		the_file = Folder_GetCurrentFile(the_panel->root_folder_);
		General_CreateFilePathFromFolderAndFile(global_temp_path_1, the_panel->root_folder_->file_path_, the_file->file_name_);
		success = File_LoadFileToEM(global_temp_path_1, dst_bank_num);
	}
	else if (the_panel->for_disk_ == false && the_other_panel->for_disk_ == true)
	{
		// copy memory bank to file on disk

		// set up a 'what's the file name?' dialog box
		General_Strlcpy(global_string_buff1, General_GetString(ID_STR_DLG_COPY_TO_FILE_TITLE), 70);
		
		// copy the current bank name into the edit buffer so user can edit
		sprintf(global_string_buff2, "Bank_%02X.bin", src_bank_num);
		
		App_LoadOverlay(OVERLAY_SCREEN);	
		the_name = Screen_GetStringFromUser(global_string_buff1, General_GetString(ID_STR_DLG_ENTER_FILE_NAME), global_string_buff2, FILE_MAX_FILENAME_SIZE);
		App_LoadOverlay(OVERLAY_DISKSYS);
		
		if (the_name == NULL)
		{
			return false;
		}

		General_CreateFilePathFromFolderAndFile(global_temp_path_2, the_other_panel->root_folder_->file_path_, the_name);
	
		// get a target handle for writing
		if ( (the_target_handle = Folder_GetTargetHandleForWriting(global_temp_path_2)) == NULL)
		{
			return false;
		}

		// prepare to use progress bar
		App_ShowProgressBar();

		// loop until all 8192 bytes of source bank have been witten out, writing STORAGE_FILE_BUFFER_1_LEN bytes per loop (sized to available buffer)
		for (i = 0; i < PAGES_PER_BANK; i++)
		{
			App_EMDataCopy(the_buffer, src_bank_num, i, PARAM_COPY_FROM_EM);

			fwrite(the_buffer, 1, STORAGE_FILE_BUFFER_1_LEN, the_target_handle);
			
			percent_read = ((uint32_t)i * 100) / (uint32_t)PAGES_PER_BANK;
			
			App_UpdateProgressBar((uint8_t)percent_read);		
		}
		
		fclose(the_target_handle);

		// clear the progress bar
		App_HideProgressBar();
		
		success = true;
	}
	else if (the_panel->for_disk_ == false && the_other_panel->for_disk_ == false)
	{
		// copy memory bank to memory bank
		
		// prevent user from copying from same bank to same bank
		if (src_bank_num == dst_bank_num)
		{
			// can't copy to/from same memory bank!
			Buffer_NewMessage(General_GetString(ID_STR_ERROR_ATTEMPT_COPY_BANK_TO_ITSELF));
			return false;
		}
		
		// check if user would be writing over f/manager RAM
		if (MemSys_BankIsWriteable(the_other_panel->memory_system_) == false)
		{
			Buffer_NewMessage(General_GetString(ID_STR_ERROR_ATTEMPT_TO_OVERWRITE_FM_RAM));
			return false;
		}
				
		// ok, safe to proceed
		for (i = 0; i < PAGES_PER_BANK; i++)
		{
			App_EMDataCopy(the_buffer, src_bank_num, i, PARAM_COPY_FROM_EM);
			App_EMDataCopy(the_buffer, dst_bank_num, i, PARAM_COPY_TO_EM);
		}

		success = true;
	}
	
	Panel_Refresh(the_other_panel);
	// refresh this panel too, in case both panels are pointing at same folder and user really just did a duplicate operation
	// only do this though if they are same panel, because it makes selected file lose focus.
	if (the_other_panel == the_panel)
	{
		Panel_Refresh(the_panel);
	}
	
	return success;
}


// show the contents of the currently selected file using the selected type of viewer
// the_viewer_type is one of the predefined macro param values (PARAM_VIEW_AS_HEX, PARAM_VIEW_AS_TEXT, etc.)
bool Panel_ViewCurrentFile(WB2KViewPanel* the_panel, uint8_t the_viewer_type)
{
	int16_t				the_current_row;
	uint8_t				num_pages;
	uint8_t				bank_num;
	WB2KFileObject*		the_file;
	bool				success;
	char*				the_name;
	
	if (the_panel->for_disk_ == true)
	{
		App_LoadOverlay(OVERLAY_DISKSYS);	
		the_current_row = Folder_GetCurrentRow(the_panel->root_folder_);
	}
	else
	{
		App_LoadOverlay(OVERLAY_MEMSYSTEM);
		the_current_row = MemSys_GetCurrentRow(the_panel->memory_system_);
	}
	
	if (the_current_row < 0)
	{
		return false;
	}
	
	if (the_panel->for_disk_ == true)
	{
		the_file = Folder_FindFileByRow(the_panel->root_folder_, the_current_row);
		General_CreateFilePathFromFolderAndFile(global_temp_path_1, the_panel->root_folder_->file_path_, the_file->file_name_);
		the_name = the_file->file_name_;
		num_pages = the_file->size_/256;
		bank_num = EM_STORAGE_START_PHYS_BANK_NUM;
		success = File_LoadFileToEM(global_temp_path_1, bank_num);
	}
	else
	{
		// for view memory, no question of success: it is already there always
		the_name = the_panel->memory_system_->bank_[the_current_row].name_;
		num_pages = PAGES_PER_BANK;	// 32 pages per bank (8192/256=32)
		bank_num = the_panel->memory_system_->bank_[the_current_row].bank_num_;
		success = true;
	}
	
	if (success)
	{
		App_LoadOverlay(OVERLAY_EM);
		
		if (the_viewer_type == PARAM_VIEW_AS_HEX)
		{
			EM_DisplayAsHex(bank_num, num_pages, the_name);
		}
		else
		{
			EM_DisplayAsText(bank_num, num_pages, the_name);
		}
	}
	else
	{
		Buffer_NewMessage(General_GetString(ID_STR_ERROR_FAIL_VIEW_FILE));
	}
	
	return success;
}


// change file selection - user did cursor up
// returns false if action was not possible (eg, you were at top of list already)
bool Panel_SelectPrevFile(WB2KViewPanel* the_panel)
{
	int16_t		the_current_row;
	
	if (the_panel->for_disk_ == true)
	{
		App_LoadOverlay(OVERLAY_DISKSYS);	
		the_current_row = Folder_GetCurrentRow(the_panel->root_folder_);
	}
	else
	{
		App_LoadOverlay(OVERLAY_MEMSYSTEM);
		the_current_row = MemSys_GetCurrentRow(the_panel->memory_system_);
	}
	
	if (--the_current_row < 0)
	{
		return false;
	}
	
	return Panel_SetFileSelectionByRow(the_panel, the_current_row, true);
}


// change file selection - user did cursor down
// returns false if action was not possible (eg, you were at bottom of list already)
bool Panel_SelectNextFile(WB2KViewPanel* the_panel)
{
	int16_t		the_current_row;
	uint16_t	the_item_count;
	
	if (the_panel->for_disk_ == true)
	{
		App_LoadOverlay(OVERLAY_DISKSYS);
		the_current_row = Folder_GetCurrentRow(the_panel->root_folder_);
		the_item_count = Folder_GetCountFiles(the_panel->root_folder_);
	}
	else
	{
		App_LoadOverlay(OVERLAY_MEMSYSTEM);
		the_current_row = MemSys_GetCurrentRow(the_panel->memory_system_);
		the_item_count = MEMORY_BANK_COUNT;
	}	
	
	if (++the_current_row == the_item_count)
	{
		// we're already on the last file
		return false;
	}
	
	return Panel_SetFileSelectionByRow(the_panel, the_current_row, true);
}


// select or unselect 1 file by row id
bool Panel_SetFileSelectionByRow(WB2KViewPanel* the_panel, uint16_t the_row, bool do_selection)
{
	// LOGIC:
	//   iterate through all files in the panel's list
	//   if do_selection is TRUE, then the goal is to mark the file at that row as selected
	//   if do_selection is FALSE, then the goal is to mark the file at that row as unselected.

	bool				scroll_needed = false;
	bool				success;
	uint8_t				content_top = the_panel->content_top_;
	WB2KFileObject*		the_file;
	FMBankObject*		the_bank;
	
	if (the_panel == NULL)
	{
		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_SET_FILE_SEL_BY_ROW_PANEL_WAS_NULL); // crash early, crash often
	}
	
	if (the_panel->for_disk_ == true)
	{
		App_LoadOverlay(OVERLAY_DISKSYS);
		the_file = Folder_SetFileSelectionByRow(the_panel->root_folder_, the_row, do_selection, the_panel->y_);
		success = (the_file != NULL);
	}
	else
	{
		App_LoadOverlay(OVERLAY_MEMSYSTEM);
		the_bank = MemSys_SetBankSelectionByRow(the_panel->memory_system_, the_row, do_selection, the_panel->y_, the_panel->active_);	
		success = (the_bank != NULL);
	}
	
	// is the newly selected file visible? If not, scroll to make it visible
	if (success)
	{
		if (the_row >= (content_top + the_panel->height_))
		{
			// row is off the bottom of the screen. 
			// to make it visible, increase content_top and reflow and re-render panel
			scroll_needed = true;
			++the_panel->content_top_;
			Text_SetXY(the_panel->x_, UI_VIEW_PANEL_SCROLL_UP_START);
			zp_y_cnt = UI_VIEW_PANEL_SCROLL_CNT;
			Text_ScrollTextUp(UI_PANEL_INNER_WIDTH);

		}
		else if (the_row < content_top)
		{
			scroll_needed = true;
			--the_panel->content_top_;
			Text_SetXY(the_panel->x_, UI_VIEW_PANEL_SCROLL_DN_START);
			zp_y_cnt = UI_VIEW_PANEL_SCROLL_CNT;
			Text_ScrollTextDown(UI_PANEL_INNER_WIDTH);
		}

		if (scroll_needed == true)
		{
			Panel_ReflowContent(the_panel);
	
			if (the_panel->for_disk_ == true)
			{
				File_Render(the_file, File_IsSelected(the_file), the_panel->y_, the_panel->active_);
			}
			else
			{
				Bank_Render(the_bank, Bank_IsSelected(the_bank), the_panel->y_, the_panel->active_);
			}
		}
	}
	
	return success;
}


// // de-select all files
// bool Panel_UnSelectAllFiles(WB2KViewPanel* the_panel)
// {
// 	// LOGIC:
// 	//   iterate through all files in the folder's list
// 	//   for any file that is listed as selected, instruct it to de-select itself
// 	//   NOTE: this_filetype is used purely to double-check that we have a valid list node. Remove that code if/when I figure out how to only get valid list nodes
// 
// 	WB2KList*		the_item;
// 	uint16_t	x_bound;
// 
// 	if (the_panel == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_PANEL_WAS_NULL); // crash early, crash often
// 	}
// 
// 	if (the_panel->root_folder_ == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object had a null root folder", __func__ , __LINE__));
// 		App_Exit(ERROR_PANEL_ROOT_FOLDER_WAS_NULL); // crash early, crash often
// 	}
// 
// 	x_bound = the_panel->x_ + the_panel->width_;
// 
// 	the_item = *(the_panel->root_folder_->list_);
// 
// 	if (the_item == NULL)
// 	{
// 		return false;
// 	}
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject* this_file = (WB2KFileObject*)(the_item->payload_);
// 		//printf("Folder_UnSelectAllFiles: file %s selected=%i\n", this_file->file_name_, this_file->selected_);
// 
// 		if (File_MarkUnSelected(this_file, the_panel->y_) == false)
// 		{
// 			// the passed file was null. do anything?
// 		}
// 
// 		the_item = the_item->next_item_;
// 	}
// 
// 	return true;
// }


// // Performs an "Open" action on any files in the panel that are marked as selected
// bool Panel_OpenSelectedFiles(WB2KViewPanel* the_panel)
// {
// 	// LOGIC:
// 	//   iterate through all files in the panel's list
// 	//   for any file that is listed as selected, instruct it to open itself
// 	//   NOTE: this_filetype is used purely to double-check that we have a valid list node. Remove that code if/when I figure out how to only get valid list nodes
// 
// 	WB2KList*	the_item;
// 
// 	if (the_panel == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_PANEL_WAS_NULL); // crash early, crash often
// 	}
// 
// 	if (the_panel->root_folder_ == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object had a null root folder", __func__ , __LINE__));
// 		App_Exit(ERROR_PANEL_ROOT_FOLDER_WAS_NULL); // crash early, crash often
// 	}
// 
// 	the_item = *(the_panel->root_folder_->list_);
// 
// 	if (the_item == NULL)
// 	{
// 		return false;
// 	}
// 
// 	while (the_item != NULL)
// 	{
// 		WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);
// 		WB2KFolderObject*	the_root_folder;
// 
// 		if (File_IsSelected(this_file) == true)
// 		{
// 			// LOGIC:
// 			//    if folder, open in new file browser window
// 			//    if file, perform some open action that makes sense
// 			if (File_IsFolder(this_file) == true)
// 			{
// 				WB2KWindow*			new_surface;
// 
// 				//if ( (the_root_folder = Folder_GetRootFolderNEW(this_file)) == NULL)
// 				if ( (the_root_folder = Folder_GetRootFolder(this_file->file_path_, this_file->rport_, this_file->datetime_.dat_Stamp)) == NULL)
// 				{
// 					LOG_ERR(("%s %d: Unable to create a folder object for '%s'", __func__ , __LINE__, this_file->file_path_));
// 					goto error;
// 				}
// 
// 				//DEBUG_OUT(("%s %d: root folder's filename='%s', is_disk=%i", __func__ , __LINE__, this_file->file_name_, the_root_folder->is_disk_));
// 
// 				// create a new surface
// 				new_surface = App_GetOrCreateWindow(global_app, the_root_folder, the_panel->view_mode_, this_file, the_panel->my_parent_surface_);
// 
// 				if ( new_surface == NULL )
// 				{
// 					LOG_ERR(("%s %d: Couldn't open a new WB surface", __func__ , __LINE__));
// 					Folder_Destroy(&the_root_folder);
// 					goto error;
// 				}
// 			}
// 			else
// 			{
// 				if (File_Open(this_file) == false)
// 				{
// 					LOG_ERR(("%s %d: Couldn't open the selected file '%s' using WBStartup", __func__ , __LINE__, this_file->file_name_));
// 					goto error;
// 				}
// 			}
// 		}
// 		the_item = the_item->next_item_;
// 	}
// 
// 	return true;
// 	
// error:
// 	return false;
// }


// // move every currently selected file into the specified folder
// // returns -1 in event of error, or count of files moved
// int16_t Panel_MoveSelectedFiles(WB2KViewPanel* the_panel, WB2KFolderObject* the_target_folder)
// {
// 	if (the_panel == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		App_Exit(ERROR_PANEL_WAS_NULL); // crash early, crash often
// 	}
// 
// 	if (the_target_folder == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed the_target_folder was null", __func__ , __LINE__));
// 		App_Exit(ERROR_PANEL_TARGET_FOLDER_WAS_NULL); // crash early, crash often
// 	}
// 
// 	return Folder_MoveSelectedFiles(the_panel->root_folder_, the_target_folder);
// }


// calculate and set positions for the panel's files, when viewed as list
// call this when window is first opened, or when window size changes, or when list of files is changed.
// this does not actually render any icons, it just calculates where they should be in the window's bitmap.
// note: this also sets/resets the surface's required_inner_width_ property (logical internal width vs physical internal width)
void Panel_ReflowContent(WB2KViewPanel* the_panel)
{
	if (the_panel->for_disk_ == true)
	{
		Panel_ReflowContentForDisk(the_panel);
	}
	else
	{
		Panel_ReflowContentForMemory(the_panel);
	}
}


// clears the panel in preparation for re-rendering it
void Panel_ClearDisplay(WB2KViewPanel* the_panel)
{
	// LOGIC:
	//   for panel in a backdrop window, just set entire thing to a simple pattern
	//   for panel in a regular window, set everything to background color
	
	Text_FillBox(the_panel->x_ + 0, the_panel->y_ + 0, the_panel->x_ + the_panel->width_ - 1, the_panel->y_ + the_panel->height_ - 1, CH_SPACE, LIST_ACTIVE_COLOR, APP_BACKGROUND_COLOR);
	
	return;

}


// display the main contents of a panel (excludes list header, if any)
// call this whenever window needs to be redrawn
// Panel_ReflowContent must be called (at least once) before calling this
// this routine only renders, it does not do any positioning of icons
void Panel_RenderContents(WB2KViewPanel* the_panel)
{
	// clear the panel. if this is a refresh, it isn't guaranteed panel UI was just drawn. eg, file was deleted. 
	Text_FillBox(
		the_panel->x_, the_panel->y_, 
		the_panel->x_ + (UI_PANEL_INNER_WIDTH - 1), the_panel->y_ + (UI_PANEL_INNER_HEIGHT - 2), 
		CH_SPACE, LIST_ACTIVE_COLOR, APP_BACKGROUND_COLOR
	);

	// draw file list head rows
	App_LoadOverlay(OVERLAY_SCREEN);
	Screen_DrawPanelHeader(the_panel->x_, the_panel->for_disk_);
	
	// call on container to render its contents
	if (the_panel->for_disk_ == true)
	{
		WB2KList*	the_item;

		App_LoadOverlay(OVERLAY_DISKSYS);
		
		the_item = *(the_panel->root_folder_->list_);
	
		while (the_item != NULL)
		{
			WB2KFileObject*		this_file = (WB2KFileObject*)(the_item->payload_);
			
			File_Render(this_file, File_IsSelected(this_file), the_panel->y_, the_panel->active_);
			
			the_item = the_item->next_item_;
		}
	}
	else
	{
		uint8_t		row;
		
		App_LoadOverlay(OVERLAY_MEMSYSTEM);
		
		for (row = 0; row < MEMORY_BANK_COUNT; row++)
		{
			FMBankObject*	this_bank;

			this_bank = &the_panel->memory_system_->bank_[row];
			Bank_Render(this_bank, Bank_IsSelected(this_bank), the_panel->y_, the_panel->active_);
		}
	}
	
	// draw folder title in the top tab
	Panel_RenderTitleOnly(the_panel);

	return;
}


// display the title of the panel only
// inverses the title if the panel is active, draws it normally if inactive
void Panel_RenderTitleOnly(WB2KViewPanel* the_panel)
{
	uint8_t		back_color;
	uint8_t		fore_color;

	if (the_panel->active_ == true)
	{
		fore_color = PANEL_BACKGROUND_COLOR;
		back_color = LIST_ACTIVE_COLOR;
	}
	else
	{
		fore_color = LIST_INACTIVE_COLOR;
		back_color = PANEL_BACKGROUND_COLOR;
	}
	
	Text_FillBox(the_panel->x_, the_panel->y_ - 3, the_panel->x_ + (UI_PANEL_TAB_WIDTH - 3), the_panel->y_ - 3, CH_SPACE, fore_color, back_color);

	if (the_panel->for_disk_ == true)
	{
		Text_DrawStringAtXY( the_panel->x_, the_panel->y_ - 3, the_panel->root_folder_->folder_file_->file_name_, fore_color, back_color);
	}
	else if (the_panel->device_number_ == DEVICE_RAM)
	{
		Text_DrawStringAtXY( the_panel->x_, the_panel->y_ - 3, "RAM", fore_color, back_color);
	}
	else
	{
		Text_DrawStringAtXY( the_panel->x_, the_panel->y_ - 3, "Flash", fore_color, back_color);
	}
}


// sorts the file list by date/name/etc, then calls the panel to renew its view.
// TODO: consider adding a boolean "do reflow". 
void Panel_SortAndDisplay(WB2KViewPanel* the_panel)
{
	WB2KFileObject*		the_current_file;
	
	// LOGIC: 
	//   the panel has a concept of currently selected row. this is used to determine bounds for cursor up/down file selection
	//   after sort, the files will visually be in the right order, but current row won't necessarily match up any more
	//   so we get a reference the current file, then sort, then get that file's (possibly) new row number, and use it for current row
	
	if (the_panel->for_disk_ == true)
	{
		App_LoadOverlay(OVERLAY_DISKSYS);
		
		the_current_file = Folder_GetCurrentFile(the_panel->root_folder_);
		
		List_InitMergeSort(the_panel->root_folder_->list_, the_panel->sort_compare_function_);

		Panel_ReflowContent(the_panel);
		Panel_RenderContents(the_panel);
		
		// now re-set the folder's idea of what the current file is
		if (the_current_file != NULL)
		{
			Folder_SetCurrentRow(the_panel->root_folder_, the_current_file->row_);
		}
		else
		{
			Folder_SetCurrentRow(the_panel->root_folder_, 1);
		}
		
		// have screen function draw the sort triangle in the right place (doing it there to save space in MAIN)
		App_LoadOverlay(OVERLAY_SCREEN);
		Screen_UpdateSortIcons(the_panel->x_, the_panel->sort_compare_function_);
		
		// set or unset the visual indicator for Meatloaf mode
		Screen_UpdateMeatloafIcon(the_panel->x_, the_panel->root_folder_->is_meatloaf_);
	}
	else
	{
		// we never sort memory banks, so don't need to know current row, change sort icons, etc.
		//MemSys_SetCurrentRow(the_panel->memory_system_, 0); // for memory bank, first row is always the selected one when displaying from scratch
		Panel_ReflowContent(the_panel);
		Panel_RenderContents(the_panel);
		
		// always unset the visual indicator for Meatloaf mode in case it had been set. never true for memory
		App_LoadOverlay(OVERLAY_SCREEN);
		Screen_UpdateMeatloafIcon(the_panel->x_, false);
		
		App_LoadOverlay(OVERLAY_MEMSYSTEM);
		MemSys_SetBankSelectionByRow(the_panel->memory_system_, 0, PARAM_MARK_SELECTED, the_panel->y_, the_panel->active_);
	}
}


// initiate a memory search at the start of the currently selected bank
bool Panel_SearchCurrentBank(WB2KViewPanel* the_panel)
{
	uint8_t		the_bank_num;
	char*		search_phrase;
	
	if (the_panel->for_disk_ == true)
	{
		return false;
	}

	App_LoadOverlay(OVERLAY_MEMSYSTEM);
	the_bank_num = MemSys_GetCurrentBankNum(the_panel->memory_system_);
	
	// set up a 'enter search search phrase' dialog box
	General_Strlcpy(global_string_buff1, General_GetString(ID_STR_DLG_SEARCH_BANK_TITLE), 70);

	// copy the previous human-readable version of global search phrase into a temp buffer
	General_Strlcpy(global_string_buff2, global_search_phrase_human_readable, MAX_SEARCH_PHRASE_LEN + 1);

	// ask user what they want to search for, showing them the previous thing they searched for, if any		
	App_LoadOverlay(OVERLAY_SCREEN);	
	search_phrase = Screen_GetStringFromUser(global_string_buff1, General_GetString(ID_STR_DLG_SEARCH_BANK_BODY), global_string_buff2, MAX_SEARCH_PHRASE_LEN);
	
	if (search_phrase == NULL)
	{
		global_search_phrase_len = 0;
		*global_search_phrase = 0;
		return false;
	}

	// get a copy of the phrase as  entered, to keep as the human-readable version. for hex bytes, this matters. for normal strings, it will be same thing user entered.
	General_Strlcpy(global_search_phrase_human_readable, search_phrase, MAX_SEARCH_PHRASE_LEN + 1);
	
	// Process user entry to see if they typed in a direct search phrase, or enter a string of numbers
	//   LOGIC:
	//      if user start phrase with "#", then assume it will be string of hex numbers. these need to be converted to raw bytes.
	//      if search phrase didn't start with #, then it will be left alone and just the len returned
	
	global_search_phrase_len = ScreenEvaluateUserStringForHexSeries(&search_phrase);

	// prepare for search
	memcpy(global_search_phrase, search_phrase, global_search_phrase_len);
	*(uint8_t*)ZP_SEARCH_LOC_BYTE = 0;	// start at begining of page
	*(uint8_t*)ZP_SEARCH_LOC_PAGE = 0;	// start at first page in bank
	*(uint8_t*)ZP_SEARCH_LOC_BANK = the_bank_num;	// start at the currently selected bank
	
	//DEBUG_OUT(("%s %d: ZP_SEARCH_LOC_BYTE=%x, ZP_SEARCH_LOC_PAGE=%x, ZP_SEARCH_LOC_BANK=%x, phrase='%s', len=%u", __func__ , __LINE__, zp_search_loc_byte, zp_search_loc_page, zp_search_loc_bank, search_phrase, global_search_phrase_len));
	
	App_LoadOverlay(OVERLAY_EM);	

	if ( (global_find_next_enabled = EM_SearchMemory(PARAM_START_FROM_THIS_BANK)) == false)
	{
		//DEBUG_OUT(("%s %d: nothing found", __func__ , __LINE__));
		return false;
	}
	
	// a match was found. user has already been informed
	// NOTE: do not need to remember this location in order to "find next" can work from here: zp1-3 were updated already	
	
	return true;
}


// ask user for a Meatloaf URL they want to open as a directory
bool Panel_OpenMeatloafURL(WB2KViewPanel* the_panel)
{
	bool				success;
	char*				the_name;
	
	if (the_panel->for_disk_ == false)
	{
		return false;
	}

	if (the_panel->root_folder_->is_meatloaf_ == false)
	{
		return false;
	}

	// LOGIC:
	//   putting this in list panel doesn't accomplish much,
	//   but cc65 was acting weird when thsi was in MainLoop, maybe that switch got too big for it or something. dunno. 
	//   more stable with this in a different place. 
	
// 	General_Strlcpy((char*)&global_dlg_title, General_GetString(ID_STR_DLG_MEATLOAF_URL_TITLE), COMM_BUFFER_MAX_STRING_LEN);
// 	General_Strlcpy((char*)&global_dlg_body_msg, General_GetString(ID_STR_DLG_MEATLOAF_URL_BODY), APP_DIALOG_WIDTH);
	General_Strlcpy((char*)global_string_buff2, General_GetString(ID_STR_DLG_MEATLOAF_DEFAULT_URL), FILE_MAX_FILENAME_SIZE);	// pre-set the foenix app store
	
// 	success = Text_DisplayTextEntryDialog(&global_dlg, (char*)&temp_screen_buffer_char, (char*)&temp_screen_buffer_attr, global_string_buff2, FILE_MAX_FILENAME_SIZE, APP_ACCENT_COLOR, APP_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);


	General_Strlcpy(global_string_buff1, General_GetString(ID_STR_DLG_MEATLOAF_URL_TITLE), APP_DIALOG_WIDTH);
		
	App_LoadOverlay(OVERLAY_SCREEN);	
	the_name = Screen_GetStringFromUser(
		global_string_buff1, 
		General_GetString(ID_STR_DLG_MEATLOAF_URL_BODY), 
		global_string_buff2, 
		FILE_MAX_FILENAME_SIZE);
	App_LoadOverlay(OVERLAY_DISKSYS);
	
	if (the_name == NULL)
	{
		return false;
	}

	// user entered a URL, now try to "load" it. It will be in global_string_buff2
	sprintf(global_temp_path_1, "%u:%s", the_panel->root_folder_->device_number_, global_string_buff2);
	App_LoadOverlay(OVERLAY_DISKSYS);
	File_LoadFileToEM(global_temp_path_1, EM_STORAGE_START_PHYS_BANK_NUM);
	Panel_Refresh(the_panel);


// 	if (success == true)
// 	{
// 		// user entered a URL, now try to "load" it.							
// 		sprintf(global_temp_path_1, "%u:%s", the_panel->root_folder_->device_number_, global_string_buff2);
// 		App_LoadOverlay(OVERLAY_DISKSYS);
// 		File_LoadFileToEM(global_temp_path_1, EM_STORAGE_START_PHYS_BANK_NUM);
// 		Panel_Refresh(the_panel);
// 	}
	
	return success;
}



// // Re-select the current file.
// // typically after a sort, so that panel's current row gets reset to match new row the previously selected file had
// // returns false if action was not possible
// bool Panel_ReSelectCurrentFile(WB2KViewPanel* the_panel)
// {
// 	int16_t		the_current_row;
// 	
// 	App_LoadOverlay(OVERLAY_DISKSYS);
// 	
// 	the_current_row = Folder_GetCurrentRow(the_panel->root_folder_);
// 	
// 	if (--the_current_row < 0)
// 	{
// 		return false;
// 	}
// 	
// 	return Panel_SetFileSelectionByRow(the_panel, the_current_row, true);
// }



