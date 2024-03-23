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
#include "file.h"
#include "general.h"
#include "kernel.h"
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

static File_Panel		panel[NUM_PANELS];
static UI_Button		uibutton[NUM_BUTTONS];
 
static uint8_t			screen_titlebar[UI_BYTE_SIZE_OF_APP_TITLEBAR] = 
{
	148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,141,142,143,144,145,146,147,32,102,47,109,97,110,97,103,101,114,32,0x46,0x32,0x35,0x36,32,140,139,138,137,136,135,134,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,
};


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

extern bool					global_started_from_flash;		// tracks whether app started from flash or from disk
extern bool					global_clock_is_visible;		// tracks whether or not the clock should be drawn. set to false when not showing main 2-panel screen.

extern char*				global_string[NUM_STRINGS];
extern char*				global_string_buff1;
extern char*				global_string_buff2;

extern uint8_t				zp_bank_num;
extern uint8_t				io_bank_value_kernel;	// stores value for the physical bank pointing to C000-DFFF whenever we change it, so we can restore it.

#pragma zpsym ("zp_bank_num");


/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

void Screen_DrawUI(void);

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
	Text_CopyMemBoxLinearBuffer((uint8_t*)&screen_titlebar, 0, 0, 79, 2, SCREEN_COPY_TO_SCREEN, SCREEN_FOR_TEXT_CHAR);
	Text_FillBoxAttrOnly(0, 0, 79, 0, APP_ACCENT_COLOR, APP_BACKGROUND_COLOR);
	Text_FillBoxAttrOnly(0, 2, 79, 2, APP_ACCENT_COLOR, APP_BACKGROUND_COLOR);
	Text_InvertBox(48, 1, 54, 1);	// right-hand side vertical bars need to be inversed to grow from thin to fat


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

		// draw file list head rows
// 		x1 += UI_PANEL_FILENAME_OFFSET;
// 		++y1;
// 		Text_DrawStringAtXY(x1, y1, General_GetString(ID_STR_LBL_FILENAME), LIST_HEADER_COLOR, PANEL_BACKGROUND_COLOR);
// 		x1 += UI_PANEL_FILETYPE_OFFSET;
// 		Text_DrawStringAtXY(x1, y1, General_GetString(ID_STR_LBL_FILETYPE), LIST_HEADER_COLOR, PANEL_BACKGROUND_COLOR);
// 		x1 += UI_PANEL_FILESIZE_OFFSET;
// 		Text_DrawStringAtXY(x1, y1, General_GetString(ID_STR_LBL_FILESIZE), LIST_HEADER_COLOR, PANEL_BACKGROUND_COLOR);
	}
	
	// draw permanently enabled buttons. 
	// see Screen_DrawFileMenuItems() for the file menu items - they need to be redrawn during main loop
	for (i = FIRST_PERMSTATE_BUTTON; i <= LAST_PERMSTATE_BUTTON; i++)
	{
		x1 = uibutton[i].x1_;
		y1 = uibutton[i].y1_;
		Text_DrawHLine(UI_MIDDLE_AREA_START_X, y1, UI_MIDDLE_AREA_WIDTH, CH_SPACE, MENU_FOREGROUND_COLOR, MENU_BACKGROUND_COLOR, CHAR_AND_ATTR);
		Text_DrawStringAtXY(x1, y1, General_GetString(uibutton[i].string_id_), MENU_FOREGROUND_COLOR, MENU_BACKGROUND_COLOR);
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


// redraw file menu buttons in activated/inactivated state as appropriate
// device buttons are always activated, so are only drawn once
void Screen_DrawFileMenuItems(bool as_active)
{
	uint8_t		i;
	uint8_t		x1;
	uint8_t		y1;
	uint8_t		text_color;

	text_color = (as_active == true ? MENU_FOREGROUND_COLOR : MENU_INACTIVE_COLOR);
	
	// draw buttons
	for (i = FIRST_ACTIVATING_BUTTON; i <= LAST_ACTIVATING_BUTTON; i++)
	{
		//text_color = (uibutton[i].active_ == true ? MENU_FOREGROUND_COLOR : MENU_INACTIVE_COLOR);
		x1 = uibutton[i].x1_;
		y1 = uibutton[i].y1_;
		Text_DrawHLine(UI_MIDDLE_AREA_START_X, y1, UI_MIDDLE_AREA_WIDTH, CH_SPACE, text_color, MENU_BACKGROUND_COLOR, CHAR_AND_ATTR);
		Text_DrawStringAtXY(x1, y1, General_GetString(uibutton[i].string_id_), text_color, MENU_BACKGROUND_COLOR);
	}
}


// swap the copy >>> button for copy <<< and vice versa
void Screen_SwapCopyDirectionIndicator(void)
{
	uint8_t		x1;
	uint8_t		y1;

	if (uibutton[BUTTON_ID_COPY].string_id_ == ID_STR_FILE_COPY_RIGHT)
	{
		uibutton[BUTTON_ID_COPY].string_id_ = ID_STR_FILE_COPY_LEFT;
	}
	else
	{
		uibutton[BUTTON_ID_COPY].string_id_ = ID_STR_FILE_COPY_RIGHT;
	}

	x1 = uibutton[BUTTON_ID_COPY].x1_;
	y1 = uibutton[BUTTON_ID_COPY].y1_;
	Text_DrawStringAtXY(x1, y1, General_GetString(uibutton[BUTTON_ID_COPY].string_id_), MENU_FOREGROUND_COLOR, MENU_BACKGROUND_COLOR);
}


// populate button objects, etc. no drawing.
void Screen_InitializeUI(void)
{
	panel[PANEL_ID_LEFT].id_ = PANEL_ID_LEFT;
	panel[PANEL_ID_LEFT].device_id_ = DEVICE_ID_UNSET;
	panel[PANEL_ID_LEFT].x1_ = UI_LEFT_PANEL_BODY_X1;
	panel[PANEL_ID_LEFT].y1_ = UI_VIEW_PANEL_BODY_Y1;
	panel[PANEL_ID_LEFT].width_ = UI_VIEW_PANEL_BODY_WIDTH;
	panel[PANEL_ID_LEFT].x2_ = UI_LEFT_PANEL_BODY_X2;
	panel[PANEL_ID_LEFT].y2_ = UI_VIEW_PANEL_BODY_Y2;

	panel[PANEL_ID_RIGHT].id_ = PANEL_ID_RIGHT;
	panel[PANEL_ID_RIGHT].device_id_ = DEVICE_ID_UNSET;
	panel[PANEL_ID_RIGHT].x1_ = UI_RIGHT_PANEL_BODY_X1;
	panel[PANEL_ID_RIGHT].y1_ = UI_VIEW_PANEL_BODY_Y1;
	panel[PANEL_ID_RIGHT].width_ = UI_VIEW_PANEL_BODY_WIDTH;
	panel[PANEL_ID_RIGHT].x2_ = UI_RIGHT_PANEL_BODY_X2;
	panel[PANEL_ID_RIGHT].y2_ = UI_VIEW_PANEL_BODY_Y2;

	// set up the buttons - DEVICE actions
	uibutton[BUTTON_ID_DEV_SD_CARD].id_ = BUTTON_ID_DEV_SD_CARD;
	uibutton[BUTTON_ID_DEV_SD_CARD].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_DEV_SD_CARD].y1_ = UI_MIDDLE_AREA_DEV_CMD_Y;
	uibutton[BUTTON_ID_DEV_SD_CARD].string_id_ = ID_STR_DEV_SD;
	//uibutton[BUTTON_ID_DEV_SD_CARD].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_DEV_FLOPPY_1].id_ = BUTTON_ID_DEV_FLOPPY_1;
	uibutton[BUTTON_ID_DEV_FLOPPY_1].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_DEV_FLOPPY_1].y1_ = UI_MIDDLE_AREA_DEV_CMD_Y + 1;
	uibutton[BUTTON_ID_DEV_FLOPPY_1].string_id_ = ID_STR_DEV_FLOPPY_1;
	//uibutton[BUTTON_ID_DEV_FLOPPY_1].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_DEV_FLOPPY_2].id_ = BUTTON_ID_DEV_FLOPPY_2;
	uibutton[BUTTON_ID_DEV_FLOPPY_2].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_DEV_FLOPPY_2].y1_ = UI_MIDDLE_AREA_DEV_CMD_Y + 2;
	uibutton[BUTTON_ID_DEV_FLOPPY_2].string_id_ = ID_STR_DEV_FLOPPY_2;
	//uibutton[BUTTON_ID_DEV_FLOPPY_2].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_DEV_RAM].id_ = BUTTON_ID_DEV_RAM;
	uibutton[BUTTON_ID_DEV_RAM].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_DEV_RAM].y1_ = UI_MIDDLE_AREA_DEV_CMD_Y + 3;
	uibutton[BUTTON_ID_DEV_RAM].string_id_ = ID_STR_DEV_RAM;
	//uibutton[BUTTON_ID_DEV_RAM].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_DEV_FLASH].id_ = BUTTON_ID_DEV_FLASH;
	uibutton[BUTTON_ID_DEV_FLASH].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_DEV_FLASH].y1_ = UI_MIDDLE_AREA_DEV_CMD_Y + 4;
	uibutton[BUTTON_ID_DEV_FLASH].string_id_ = ID_STR_DEV_FLASH;
	//uibutton[BUTTON_ID_DEV_FLASH].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_REFRESH].id_ = BUTTON_ID_REFRESH;
	uibutton[BUTTON_ID_REFRESH].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_REFRESH].y1_ = UI_MIDDLE_AREA_DEV_CMD_Y + 5;
	uibutton[BUTTON_ID_REFRESH].string_id_ = ID_STR_DEV_REFRESH_LISTING;
	//uibutton[BUTTON_ID_REFRESH].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_FORMAT].id_ = BUTTON_ID_FORMAT;
	uibutton[BUTTON_ID_FORMAT].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_FORMAT].y1_ = UI_MIDDLE_AREA_DEV_CMD_Y + 6;
	uibutton[BUTTON_ID_FORMAT].string_id_ = ID_STR_DEV_FORMAT;
	//uibutton[BUTTON_ID_FORMAT].state_ = UI_BUTTON_STATE_DISABLED;

	// set up the buttons - DIRECTORY actions
	uibutton[BUTTON_ID_MAKE_DIR].id_ = BUTTON_ID_MAKE_DIR;
	uibutton[BUTTON_ID_MAKE_DIR].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_MAKE_DIR].y1_ = UI_MIDDLE_AREA_DIR_CMD_Y;
	uibutton[BUTTON_ID_MAKE_DIR].string_id_ = ID_STR_DEV_MAKE_DIR;
	//uibutton[BUTTON_ID_MAKE_DIR].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_SORT_BY_TYPE].id_ = BUTTON_ID_SORT_BY_TYPE;
	uibutton[BUTTON_ID_SORT_BY_TYPE].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_SORT_BY_TYPE].y1_ = UI_MIDDLE_AREA_DIR_CMD_Y + 1;
	uibutton[BUTTON_ID_SORT_BY_TYPE].string_id_ = ID_STR_DEV_SORT_BY_TYPE;
	//uibutton[BUTTON_ID_SORT_BY_TYPE].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_SORT_BY_NAME].id_ = BUTTON_ID_SORT_BY_NAME;
	uibutton[BUTTON_ID_SORT_BY_NAME].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_SORT_BY_NAME].y1_ = UI_MIDDLE_AREA_DIR_CMD_Y + 2;
	uibutton[BUTTON_ID_SORT_BY_NAME].string_id_ = ID_STR_DEV_SORT_BY_NAME;
	//uibutton[BUTTON_ID_SORT_BY_NAME].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_SORT_BY_SIZE].id_ = BUTTON_ID_SORT_BY_SIZE;
	uibutton[BUTTON_ID_SORT_BY_SIZE].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_SORT_BY_SIZE].y1_ = UI_MIDDLE_AREA_DIR_CMD_Y + 3;
	uibutton[BUTTON_ID_SORT_BY_SIZE].string_id_ = ID_STR_DEV_SORT_BY_SIZE;
	//uibutton[BUTTON_ID_SORT_BY_SIZE].state_ = UI_BUTTON_STATE_DISABLED;

	// set up the buttons - FILE actions
	uibutton[BUTTON_ID_COPY].id_ = BUTTON_ID_COPY;
	uibutton[BUTTON_ID_COPY].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_COPY].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y;
	uibutton[BUTTON_ID_COPY].string_id_ = ID_STR_FILE_COPY_RIGHT;
	//uibutton[BUTTON_ID_COPY].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_DELETE].id_ = BUTTON_ID_DELETE;
	uibutton[BUTTON_ID_DELETE].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_DELETE].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y + 1;
	uibutton[BUTTON_ID_DELETE].string_id_ = ID_STR_FILE_DELETE;
	//uibutton[BUTTON_ID_DELETE].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_DUPLICATE].id_ = BUTTON_ID_DUPLICATE;
	uibutton[BUTTON_ID_DUPLICATE].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_DUPLICATE].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y + 2;
	uibutton[BUTTON_ID_DUPLICATE].string_id_ = ID_STR_FILE_DUP;
	//uibutton[BUTTON_ID_DUPLICATE].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_RENAME].id_ = BUTTON_ID_RENAME;
	uibutton[BUTTON_ID_RENAME].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_RENAME].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y + 3;
	uibutton[BUTTON_ID_RENAME].string_id_ = ID_STR_FILE_RENAME;
	//uibutton[BUTTON_ID_RENAME].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_TEXT_VIEW].id_ = BUTTON_ID_TEXT_VIEW;
	uibutton[BUTTON_ID_TEXT_VIEW].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_TEXT_VIEW].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y + 4;
	uibutton[BUTTON_ID_TEXT_VIEW].string_id_ = ID_STR_FILE_TEXT_PREVIEW;
	//uibutton[BUTTON_ID_TEXT_VIEW].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_HEX_VIEW].id_ = BUTTON_ID_HEX_VIEW;
	uibutton[BUTTON_ID_HEX_VIEW].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_HEX_VIEW].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y + 5;
	uibutton[BUTTON_ID_HEX_VIEW].string_id_ = ID_STR_FILE_HEX_PREVIEW;
	//uibutton[BUTTON_ID_HEX_VIEW].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_LOAD].id_ = BUTTON_ID_LOAD;
	uibutton[BUTTON_ID_LOAD].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_LOAD].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y + 6;
	uibutton[BUTTON_ID_LOAD].string_id_ = ID_STR_FILE_LOAD;
	//uibutton[BUTTON_ID_LOAD].state_ = UI_BUTTON_STATE_DISABLED;

	// set up the buttons - APP actions
	uibutton[BUTTON_ID_SET_CLOCK].id_ = BUTTON_ID_SET_CLOCK;
	uibutton[BUTTON_ID_SET_CLOCK].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_SET_CLOCK].y1_ = UI_MIDDLE_AREA_APP_CMD_Y;
	uibutton[BUTTON_ID_SET_CLOCK].string_id_ = ID_STR_APP_SET_CLOCK;
	//uibutton[BUTTON_ID_SET_CLOCK].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_ABOUT].id_ = BUTTON_ID_ABOUT;
	uibutton[BUTTON_ID_ABOUT].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_ABOUT].y1_ = UI_MIDDLE_AREA_APP_CMD_Y + 1;
	uibutton[BUTTON_ID_ABOUT].string_id_ = ID_STR_APP_ABOUT;
	//uibutton[BUTTON_ID_ABOUT].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_EXIT_TO_BASIC].id_ = BUTTON_ID_EXIT_TO_BASIC;
	uibutton[BUTTON_ID_EXIT_TO_BASIC].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_EXIT_TO_BASIC].y1_ = UI_MIDDLE_AREA_APP_CMD_Y + 2;
	uibutton[BUTTON_ID_EXIT_TO_BASIC].string_id_ = ID_STR_APP_EXIT_TO_BASIC;
	//uibutton[BUTTON_ID_QUIT].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_EXIT_TO_DOS].id_ = BUTTON_ID_EXIT_TO_DOS;
	uibutton[BUTTON_ID_EXIT_TO_DOS].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_EXIT_TO_DOS].y1_ = UI_MIDDLE_AREA_APP_CMD_Y + 3;
	uibutton[BUTTON_ID_EXIT_TO_DOS].string_id_ = ID_STR_APP_EXIT_TO_DOS;
	//uibutton[BUTTON_ID_QUIT].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_QUIT].id_ = BUTTON_ID_QUIT;
	uibutton[BUTTON_ID_QUIT].x1_ = UI_MIDDLE_AREA_START_X;
	uibutton[BUTTON_ID_QUIT].y1_ = UI_MIDDLE_AREA_APP_CMD_Y + 4;
	uibutton[BUTTON_ID_QUIT].string_id_ = ID_STR_APP_QUIT;
	//uibutton[BUTTON_ID_QUIT].state_ = UI_BUTTON_STATE_DISABLED;
}


// set up screen variables and draw screen for first time
void Screen_Render(void)
{
	global_clock_is_visible = true;
	Text_ClearScreen(APP_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);
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


// display information about f/manager
void Screen_ShowAppAboutInfo(void)
{
	// give credit for pexec flash loader, if we started from flash and not disk
	if (global_started_from_flash == true)
	{
		Buffer_NewMessage(General_GetString(ID_STR_ABOUT_FLASH_LOADER));
	}
	
	// show app name, version, and credit
	sprintf(global_string_buff1, General_GetString(ID_STR_ABOUT_FMANAGER), CH_COPYRIGHT, MAJOR_VERSION, MINOR_VERSION, UPDATE_VERSION);
	Buffer_NewMessage(global_string_buff1);
	
	// also show current bytes free
	sprintf(global_string_buff1, General_GetString(ID_STR_N_BYTES_FREE), _heapmemavail());
	Buffer_NewMessage(global_string_buff1);
}



