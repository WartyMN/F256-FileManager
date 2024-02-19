/*
 * list_panel.h
 *
 *  Created on: Sep 4, 2020
 *      Author: micahbly
 *
 *  This is a huge cut-down of the Amiga WorkBench2000 code, for F256 f/manager and B128 f/manager
 *    8-bit version started Jan 12, 2023
 */

#ifndef LIST_PANEL_H_
#define LIST_PANEL_H_



/* about this class: WB2KListPanel
 *
 * This handles
 *
 *** things this class needs to be able to do
 *
 *** things objects of this class have
 *
 *
 */


/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

#include "folder.h"
#include "list.h"
#include "app.h"

/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/

// macro values for the Panel_CheckForMouseSelection function parameters
#define DO_NOT_MARK_AS_SELECTED		0
#define MARK_AS_SELECTED			1

#define PANEL_LIST_MAX_ROWS			144	// hack... most files we can display. 8bit note: this is IEC max files/dir. FAT32 can do 255 with longnames on


/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/



/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/


typedef struct WB2KViewPanel
{
	WB2KFolderObject*	root_folder_;
	uint8_t				x_;		// this set of position data is for the panel within the window
	uint8_t				y_;
	uint8_t				width_;
	uint8_t				height_;
	uint8_t				num_rows_;							// for any mode, number of rows used
	uint8_t				content_top_;						// for column mode, need to track our own content top position
	uint8_t				device_number_;						// For CBM, the drive #. eg, 8 or 9. 
	int8_t				drive_index_;						// reference to index to global_connected_device array. -1 if no device.
// 	uint8_t				col_width_[PANEL_LIST_NUM_COLS];	// for list mode, the widths of each column. Can vary by window width
// 	uint8_t				col_highest_visible_;				// the last column that should be rendered by File_RenderLabel
// 	bool				col_show_[PANEL_LIST_NUM_COLS];		// for list mode, whether a given column will render. Can vary by window width
// 	uint8_t				col_x_[PANEL_LIST_NUM_COLS];		// for list mode, starting x position of each column
	bool				active_;	// keep 1 panel as the active one. non-active panels get grayed out highlights
	void*				sort_compare_function_;
} WB2KViewPanel;


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

void test_dir_stuff (void);
bool printdir (char *newdir);

/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/


// **** CONSTRUCTOR AND DESTRUCTOR *****

// (re)initializer: does not allocate. takes a valid panel and resets it to starting values (+ those passed)
void Panel_Initialize(WB2KViewPanel* the_panel, WB2KFolderObject* root_folder, uint8_t x, uint8_t y, uint8_t width, uint8_t height);

// Forget all its files, and repopulate from the next drive in the system. 
// max_drive_num is the highest available connected drive in the system. an index to global_connected_device array.
bool Panel_SwitchToNextDrive(WB2KViewPanel* the_panel, uint8_t max_drive_num);



// **** SETTERS *****

// sets the current device number (CBM drive number, eg, 8-9-10-11 or FNX drive num, eg 0, 1, or 2) the panel is using
// does not refresh. Repopulate to do that.
void Panel_SetCurrentDrive(WB2KViewPanel* the_panel, uint8_t the_device_num);

// tells the panel to toggle its active/inactive state, and redraw its title appropriately
void Panel_ToggleActiveState(WB2KViewPanel* the_panel);


// **** GETTERS *****

// returns true if the folder in the panel has any currently selected files/folders
bool Panel_HasSelections(WB2KViewPanel* the_panel);

// returns number of currently selected files in this panel
uint16_t Panel_GetCountSelectedFiles(WB2KViewPanel* the_panel);

// // return the root folder
// WB2KFolderObject* Panel_GetRootFolder(WB2KViewPanel* the_panel);



// **** OTHER FUNCTIONS *****

// create a new folder in the current one
bool Panel_MakeDir(WB2KViewPanel* the_panel);

// format the specified drive
// DANGER WILL ROBINSON!
bool Panel_FormatDrive(WB2KViewPanel* the_panel);

// initialize a new panel and get directory listing or info view data
bool Panel_Init(WB2KViewPanel* the_panel);

// check if the passed X coordinate is owned by this panel. returns true if x is between x_ and x_ + width_
bool Panel_OwnsX(WB2KViewPanel* the_panel, int16_t x);

// // For mouse drag only: check to see if a folder is at the coordinates passed (in case user will drop them here)
// // if a folder is under the mouse, it will set that folder as the global drop target
// // returns true if any folder is under the mouse pointer
// bool Panel_CheckForMouseOverFolder(WB2KViewPanel* the_panel, MouseTracker* the_mouse, bool highlight_if_folder);
// 
// // check to see if an already-selected file is under the mouse pointer
// bool Panel_CheckForAlreadySelectedIconUnderMouse(WB2KViewPanel* the_panel, MouseTracker* the_mouse_tracker);
// 
// // check to see if any files were selected at the coordinates passed.
// // returns 0 if nothing selected, 1 if 1 file, 2 if multiple files, or 3 if one folder (using enums)
// IconSelectionResult Panel_CheckForMouseSelection(WB2KViewPanel* the_panel, MouseTracker* the_mouse_tracker, bool do_selection, bool highlight_if_folder);

// rename the currently selected file
bool Panel_RenameCurrentFile(WB2KViewPanel* the_panel);

// delete the currently selected file
bool Panel_DeleteCurrentFile(WB2KViewPanel* the_panel);

// Launch current file if EXE, or load font if FNT
bool Panel_LoadCurrentFile(WB2KViewPanel* the_panel);

// attempt to open the current file, if it is a directory, and display it in the same panel
bool Panel_OpenCurrentFileFolder(WB2KViewPanel* the_panel);

// copy the currently selected file to the other panel
bool Panel_CopyCurrentFile(WB2KViewPanel* the_panel, WB2KViewPanel* the_other_panel);

// show the contents of the currently selected file as a hex dump
bool Panel_ViewCurrentFileAsHex(WB2KViewPanel* the_panel);

// show the contents of the currently selected file as text
bool Panel_ViewCurrentFileAsText(WB2KViewPanel* the_panel);

// change file selection - user did cursor up
// returns false if action was not possible (eg, you were at top of list already)
bool Panel_SelectPrevFile(WB2KViewPanel* the_panel);

// change file selection - user did cursor down
// returns false if action was not possible (eg, you were at bottom of list already)
bool Panel_SelectNextFile(WB2KViewPanel* the_panel);

// select or unselect 1 file by row id
bool Panel_SetFileSelectionByRow(WB2KViewPanel* the_panel, uint16_t the_row, bool do_selection);

// de-select all files
bool Panel_UnSelectAllFiles(WB2KViewPanel* the_panel);

// Performs an "Open" action on any files in the panel that are marked as selected
bool Panel_OpenSelectedFiles(WB2KViewPanel* the_panel);

// move every currently selected file into the specified folder
// returns -1 in event of error, or count of files moved
int16_t Panel_MoveSelectedFiles(WB2KViewPanel* the_panel, WB2KFolderObject* the_target_folder);

// repositions the all elements of the display (without re-rendering it, and without recalculating available height/width), calling the appropriate internal function for list view, icon view, or column view
void Panel_ReflowContent(WB2KViewPanel* the_panel);

// clears the panel in preparation for re-rendering it
void Panel_ClearDisplay(WB2KViewPanel* the_panel);

// display the main contents of a panel (excludes list header, if any)
void Panel_RenderContents(WB2KViewPanel* the_panel);

// sorts the file list by date/name/etc, then calls the panel to renew its view.
// TODO: consider adding a boolean "do reflow". 
void Panel_SortFiles(WB2KViewPanel* the_panel);



// TEMPORARY DEBUG FUNCTIONS



#endif /* LIST_PANEL_H_ */
