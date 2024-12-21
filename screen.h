/*
 * screen.h
 *
 *  Created on: Jan 11, 2023
 *      Author: micahbly
 */

#ifndef SCREEN_H_
#define SCREEN_H_

/* about this class
 *
 *** things a screen needs to be able to do
 *
 * draw the file manager screen
 * draw individual buttons
 * update visual state of individual buttons
 *
 *** things a screen has
 *
 *
 */

/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

//#include <string.h>

#include "app.h"
#include "text.h"
#include <stdint.h>


/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/

#define PARAM_ONLY_RENDER_CHANGED_ITEMS		true	// parameter for Screen_RenderMenu
#define PARAM_RENDER_ALL_MENU_ITEMS			false	// parameter for Screen_RenderMenu

// there are 12 buttons which can be accessed with the same code
#define NUM_BUTTONS					27

// DEVICE actions
#define BUTTON_ID_DEV_SD_CARD		0
#define BUTTON_ID_DEV_FLOPPY_1		(BUTTON_ID_DEV_SD_CARD + 1)
#define BUTTON_ID_DEV_FLOPPY_2		(BUTTON_ID_DEV_FLOPPY_1 + 1)
#define BUTTON_ID_DEV_RAM			(BUTTON_ID_DEV_FLOPPY_2 + 1)
#define BUTTON_ID_DEV_FLASH			(BUTTON_ID_DEV_RAM + 1)
#define BUTTON_ID_REFRESH			(BUTTON_ID_DEV_FLASH + 1)
#define BUTTON_ID_FORMAT			(BUTTON_ID_REFRESH + 1)
#define BUTTON_ID_MAKE_DIR			(BUTTON_ID_FORMAT + 1)
#define BUTTON_ID_SORT_BY_TYPE		(BUTTON_ID_MAKE_DIR + 1)
#define BUTTON_ID_SORT_BY_NAME		(BUTTON_ID_SORT_BY_TYPE + 1)
#define BUTTON_ID_SORT_BY_SIZE		(BUTTON_ID_SORT_BY_NAME + 1)

// FILE actions
#define BUTTON_ID_COPY				(BUTTON_ID_SORT_BY_SIZE + 1)
#define BUTTON_ID_DELETE			(BUTTON_ID_COPY + 1)
#define BUTTON_ID_DUPLICATE			(BUTTON_ID_DELETE + 1)
#define BUTTON_ID_RENAME			(BUTTON_ID_DUPLICATE + 1)
// FILE & BANK actions
#define BUTTON_ID_TEXT_VIEW			(BUTTON_ID_RENAME + 1)
#define BUTTON_ID_HEX_VIEW			(BUTTON_ID_TEXT_VIEW + 1)
#define BUTTON_ID_LOAD				(BUTTON_ID_HEX_VIEW + 1)

// memory bank buttons
#define BUTTON_ID_BANK_FILL			(BUTTON_ID_LOAD + 1)
#define BUTTON_ID_BANK_CLEAR		(BUTTON_ID_BANK_FILL + 1)
#define BUTTON_ID_BANK_FIND			(BUTTON_ID_BANK_CLEAR + 1)
#define BUTTON_ID_BANK_FIND_NEXT	(BUTTON_ID_BANK_FIND + 1)

// app menu buttons
#define BUTTON_ID_SET_CLOCK			(BUTTON_ID_BANK_FIND_NEXT + 1)
#define BUTTON_ID_ABOUT				(BUTTON_ID_SET_CLOCK + 1)
#define BUTTON_ID_EXIT_TO_BASIC		(BUTTON_ID_ABOUT + 1)
#define BUTTON_ID_EXIT_TO_DOS		(BUTTON_ID_EXIT_TO_BASIC + 1)
#define BUTTON_ID_QUIT				(BUTTON_ID_EXIT_TO_DOS + 1)

#define UI_BUTTON_STATE_INACTIVE	false
#define UI_BUTTON_STATE_ACTIVE		true

#define UI_BUTTON_STATE_UNCHANGED	false
#define UI_BUTTON_STATE_CHANGED		true

#define DEVICE_ID_UNSET				-1
#define DEVICE_ID_ERROR				-2

#define UI_MIDDLE_AREA_START_X			35
#define UI_MIDDLE_AREA_START_Y			4
#define UI_MIDDLE_AREA_WIDTH			10

#define UI_MIDDLE_AREA_DEV_MENU_Y		(UI_MIDDLE_AREA_START_Y + 2)
#define UI_MIDDLE_AREA_DEV_CMD_Y		(UI_MIDDLE_AREA_DEV_MENU_Y + 3)

#define UI_MIDDLE_AREA_DIR_MENU_Y		(UI_MIDDLE_AREA_DEV_CMD_Y + 8)
#define UI_MIDDLE_AREA_DIR_CMD_Y		(UI_MIDDLE_AREA_DIR_MENU_Y + 3)

#define UI_MIDDLE_AREA_FILE_MENU_Y		(UI_MIDDLE_AREA_DIR_CMD_Y + 5)
#define UI_MIDDLE_AREA_FILE_CMD_Y		(UI_MIDDLE_AREA_FILE_MENU_Y + 3)

#define UI_MIDDLE_AREA_APP_MENU_Y		(UI_MIDDLE_AREA_FILE_CMD_Y + 12)
#define UI_MIDDLE_AREA_APP_CMD_Y		(UI_MIDDLE_AREA_APP_MENU_Y + 3)

#define UI_PANEL_INNER_WIDTH			33
#define UI_PANEL_OUTER_WIDTH			(UI_PANEL_INNER_WIDTH + 2)
#define UI_PANEL_INNER_HEIGHT			42
#define UI_PANEL_OUTER_HEIGHT			(UI_PANEL_INNER_HEIGHT + 2)
#define UI_PANEL_TAB_WIDTH				28
#define UI_PANEL_TAB_HEIGHT				3

#define UI_PANEL_FILENAME_OFFSET		1	// from left edge of panel to start of filename
#define UI_PANEL_FILETYPE_OFFSET		23	// from start of filename to start of filesize
#define UI_PANEL_FILESIZE_OFFSET		5	// from start of filesize to start of filetype
#define UI_PANEL_BANK_NUM_OFFSET		21	// from start of bank name to start of bank number
#define UI_PANEL_BANK_ADDR_OFFSET		5	// from start of bank number to start of address

#define UI_PANEL_FILENAME_SORT_OFFSET	(UI_PANEL_FILENAME_OFFSET + 3)	// from start of col header to pos right of it for sort icon
#define UI_PANEL_FILETYPE_SORT_OFFSET	(UI_PANEL_FILENAME_SORT_OFFSET + UI_PANEL_FILETYPE_OFFSET)	// from start of col header to pos right of it for sort icon
#define UI_PANEL_FILESIZE_SORT_OFFSET	(UI_PANEL_FILETYPE_SORT_OFFSET + UI_PANEL_FILESIZE_OFFSET)	// from start of col header to pos right of it for sort icon

#define UI_VIEW_PANEL_TITLE_TAB_Y1		3
#define UI_VIEW_PANEL_TITLE_TAB_HEIGHT	UI_PANEL_TAB_HEIGHT
#define UI_VIEW_PANEL_TITLE_TAB_Y2		(UI_VIEW_PANEL_TITLE_TAB_Y1 + UI_VIEW_PANEL_TITLE_TAB_HEIGHT - 1)
#define UI_VIEW_PANEL_HEADER_Y			(UI_VIEW_PANEL_TITLE_TAB_Y2 + 2)
#define UI_VIEW_PANEL_BODY_Y1			6
#define UI_VIEW_PANEL_BODY_HEIGHT		UI_PANEL_OUTER_HEIGHT
#define UI_VIEW_PANEL_BODY_Y2			(UI_VIEW_PANEL_BODY_Y1 + UI_VIEW_PANEL_BODY_HEIGHT - 1)

#define UI_VIEW_PANEL_BODY_WIDTH		UI_PANEL_OUTER_WIDTH
#define UI_LEFT_PANEL_TITLE_TAB_X1		0
#define UI_LEFT_PANEL_TITLE_TAB_WIDTH	UI_PANEL_TAB_WIDTH
#define UI_LEFT_PANEL_TITLE_TAB_X2		(UI_LEFT_PANEL_TITLE_TAB_X1 + UI_LEFT_PANEL_TITLE_TAB_WIDTH - 1)
#define UI_LEFT_PANEL_BODY_X1			0
#define UI_LEFT_PANEL_BODY_X2			(UI_LEFT_PANEL_BODY_X1 + UI_VIEW_PANEL_BODY_WIDTH - 1)

#define UI_RIGHT_PANEL_X_DELTA			45

#define UI_RIGHT_PANEL_TITLE_TAB_X1		(UI_LEFT_PANEL_TITLE_TAB_X1 + UI_RIGHT_PANEL_X_DELTA)
#define UI_RIGHT_PANEL_TITLE_TAB_WIDTH	UI_PANEL_TAB_WIDTH
#define UI_RIGHT_PANEL_TITLE_TAB_X2		(UI_RIGHT_PANEL_TITLE_TAB_X1 + UI_RIGHT_PANEL_TITLE_TAB_WIDTH - 1)
#define UI_RIGHT_PANEL_BODY_X1			(UI_LEFT_PANEL_BODY_X1 + UI_RIGHT_PANEL_X_DELTA)
#define UI_RIGHT_PANEL_BODY_X2			(UI_RIGHT_PANEL_BODY_X1 + UI_VIEW_PANEL_BODY_WIDTH - 1)

#define UI_FULL_PATH_LINE_Y				(UI_VIEW_PANEL_BODY_Y2 + 1)	// row, not in any boxes, under file panels, above comms panel, for showing full path of a file.

#define UI_VIEW_PANEL_SCROLL_CNT		(UI_PANEL_INNER_HEIGHT-2)	// number of rows to scroll
#define UI_VIEW_PANEL_SCROLL_UP_START	(UI_VIEW_PANEL_BODY_Y1 + 3)	// y start pos when scrolling up
#define UI_VIEW_PANEL_SCROLL_DN_START	(UI_VIEW_PANEL_BODY_Y1 + UI_PANEL_INNER_HEIGHT)	// y start pos when scrolling down

#define UI_COPY_PROGRESS_Y				(UI_MIDDLE_AREA_FILE_CMD_Y)
#define UI_COPY_PROGRESS_LEFTMOST		(UI_MIDDLE_AREA_START_X + 3)
#define UI_COPY_PROGRESS_RIGHTMOST		(UI_COPY_PROGRESS_LEFTMOST + 5)

#define CH_PROGRESS_BAR_SOLID_CH1		134		// for drawing progress bars that use solid bars, this is the first char (least filled in)
#define CH_PROGRESS_BAR_CHECKER_CH1		207		// for drawing progress bars that use checkerboard bars, this is the first char (least filled in)
#define CH_INVERSE_SPACE				7		// useful for progress bars as a slot fully used up
#define CH_CHECKERBOARD					199		// useful for progress bars as a slot fully used up
#define CH_UNDERSCORE					148		// this is one line up from a pure underscore, but works if text right under it. 0x5f	// '_'
#define CH_OVERSCORE					0x0e	// opposite of '_'
#define CH_SORT_ICON					248		// downward disclosure triangle in f256 fonts


/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/

/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/

typedef struct File_Panel
{
	uint8_t		id_;
	int8_t		device_id_;	// 8, 9, etc. -1 if not set/error
	uint8_t		x1_;
	uint8_t		y1_;
	uint8_t		x2_;
	uint8_t		y2_;
	uint8_t		width_;
	uint8_t		is_active_;	// is this the active panel?
} File_Panel;

typedef struct UI_Button
{
	uint8_t		id_;
	uint8_t		x1_;
	uint8_t		y1_;
	uint8_t		string_id_;
	bool		active_;	// 0-disabled/inactive, 1-enabled/active
	bool		changed_;	// set to true when the active/inactive state has changed compared to previous render. set to false after rendering
	uint8_t		key_;		// the keyboard code (foenix ascii) for the key that activates the menu
} UI_Button;

typedef struct UI_Menu_Enabler_Info
{
	uint8_t		file_type_;
	bool		for_disk_;	
	bool		for_flash_;
	bool		is_kup_;
	bool		other_panel_for_disk_;	
	bool		other_panel_for_flash_;
} UI_Menu_Enabler_Info;

/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/

// swap the copy >>> button for copy <<< and vice versa
void Screen_SwapCopyDirectionIndicator(void);

// set up screen variables and draw screen for first time
void Screen_Render(void);

// Sets active/inactive for menu items whose active state only needs to be set once, on app startup
// does not render
void Screen_SetInitialMenuStates(uint8_t num_disk_systems);

// Get user input and vet it against the menu items that are currently enabled
// returns 0 if the key pressed was for a disabled menu item
// returns the key pressed if it matched an enabled menu item, or if wasn't a known (to Screen) input. This lets App still allow for cursor keys, etc, which aren't represented by menu items
uint8_t Screen_GetValidUserInput(void);

// determine which menu items should active, which inactive
// sets inactive/active, and flags any that changed since last evaluation
// does not render
void Screen_UpdateMenuStates(UI_Menu_Enabler_Info* the_enabling_info);

// renders the menu items, as either active or inactive, as appropriate. 
// active/inactive and changed/not changed must previously have been set
// if sparse_render is true, only those items that have a different enable decision since last render will be re-rendered. Set sparse_render to false if drawing menu for first time or after clearing screen, etc. 
void Screen_RenderMenu(bool sparse_render);

// have screen function draw the sort triangle in the right place
void Screen_UpdateSortIcons(uint8_t the_panel_x, void* the_sort_compare_function);

// have screen function an icon for meatloaf mode, or clear it
void Screen_UpdateMeatloafIcon(uint8_t the_panel_x, bool meatloaf_mode);

// display information about f/manager
void Screen_ShowAppAboutInfo(void);

// draw just the 3 column headers in the specified panel
// if for_disk is true, will use name/type/size. if false, will use name/bank num/addr
void Screen_DrawPanelHeader(uint8_t panel_id, bool for_disk);

// show user a dialog and have them enter a string
// if a prefilled string is not needed, set starter_string to an empty string
// set max_len to the maximum number of bytes/characters that should be collected from user
// returns NULL if user cancels out of dialog, or returns a path to the string the user provided
char* Screen_GetStringFromUser(char* dialog_title, char* dialog_body, char* starter_string, uint8_t max_len);

// show user a 2 button confirmation dialog and have them click a button
// returns true if user selected the "positive" button, or false if they selected the "negative" button
bool Screen_ShowUserTwoButtonDialog(char* dialog_title, uint8_t dialog_body_string_id, uint8_t positive_btn_label_string_id, uint8_t negative_btn_label_string_id);

// utility function for checking user input for either normal string or series of numbers
// if preceded by "#" will check for list of 2-digit hex numbers. eg, (#FF,AA,01,00,EE).
// will convert to bytes and terminate with 0. In example above, it will return 5 as the len. 
// either way, will return the length of the set of characters that should be thought of as one unit. 
uint8_t ScreenEvaluateUserStringForHexSeries(char** the_string);


#endif /* SCREEN_H_ */
