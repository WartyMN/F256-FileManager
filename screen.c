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
#include "general.h"
#include "text.h"
#include "strings.h"

// C includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// F256 includes
#include <f256.h>




/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/

#define UI_BYTE_SIZE_OF_APP_TITLEBAR	240	// 3 x 80 rows for the title at top

/*****************************************************************************/
/*                           File-scope Variables                            */
/*****************************************************************************/

static File_Panel		panel[NUM_PANELS];
static UI_Button		uibutton[NUM_BUTTONS];
 
static uint8_t			app_titlebar[UI_BYTE_SIZE_OF_APP_TITLEBAR] = 
{
	148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,141,142,143,144,145,146,147,32,102,47,109,97,110,97,103,101,114,32,106,114,32,140,139,138,137,136,135,134,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,153,
};

/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/


/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/

void Screen_DrawUI(void);

/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/

void Screen_DrawUI(void)
{
	uint8_t		i;
	uint8_t		x1;
	uint8_t		y1;
	uint8_t		x2;
	uint8_t		y2;
	
	Text_ClearScreen(APP_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);
	
	// draw the title bar at top. 3x80
	Text_CopyMemBox((uint8_t*)&app_titlebar, 0, 0, 79, 2, SCREEN_COPY_TO_SCREEN, SCREEN_FOR_TEXT_CHAR); // NOTE: this only works because copying from "start of screen" at app_titlebar.
	Text_FillBoxAttrOnly(0, 0, 79, 0, APP_ACCENT_COLOR, APP_BACKGROUND_COLOR);
// 	Text_FillBoxAttrOnly(0, 1, 79, 1, APP_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);
	Text_FillBoxAttrOnly(0, 2, 79, 2, APP_ACCENT_COLOR, APP_BACKGROUND_COLOR);
	Text_InvertBox(47, 1, 53, 1);	// right-hand side vertical bars need to be inversed to grow from thin to fat


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
	
	// draw buttons
	for (i = 0; i < NUM_BUTTONS; i++)
	{
		x1 = uibutton[i].x1_;
		y1 = uibutton[i].y1_;
		x2 = uibutton[i].x2_;
		y2 = uibutton[i].y2_;
		Text_DrawHLine(UI_MIDDLE_AREA_START_X, y1, UI_MIDDLE_AREA_WIDTH, CH_SPACE, MENU_FOREGROUND_COLOR, MENU_BACKGROUND_COLOR, CHAR_AND_ATTR);
		Text_DrawStringAtXY(x1, y1, General_GetString(uibutton[i].string_id_), MENU_FOREGROUND_COLOR, MENU_BACKGROUND_COLOR);
	}
		
	// draw file menu
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_START_Y, UI_MIDDLE_AREA_WIDTH, CH_UNDERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
//	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_START_Y + 1, UI_MIDDLE_AREA_WIDTH, CH_SPACE, APP_FOREGROUND_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
	Text_DrawStringAtXY(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_START_Y + 1, General_GetString(ID_STR_MENU_FILE), MENU_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_START_Y + 2, UI_MIDDLE_AREA_WIDTH, CH_OVERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
		
	// draw device menu
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_DEV_MENU_Y, UI_MIDDLE_AREA_WIDTH, CH_UNDERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
	//Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_DEV_MENU_Y + 1, UI_MIDDLE_AREA_WIDTH, CH_SPACE, APP_FOREGROUND_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);
	Text_DrawStringAtXY(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_DEV_MENU_Y + 1, General_GetString(ID_STR_MENU_DEVICE), MENU_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);
	Text_DrawHLine(UI_MIDDLE_AREA_START_X, UI_MIDDLE_AREA_DEV_MENU_Y + 2, UI_MIDDLE_AREA_WIDTH, CH_OVERSCORE, MENU_ACCENT_COLOR, APP_BACKGROUND_COLOR, CHAR_AND_ATTR);

	// also draw the comms area
	Buffer_DrawCommunicationArea();
	Buffer_RefreshDisplay();
}

/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/


// populate button objects, etc. no drawing.
void Screen_InitializeUI(void)
{
	panel[PANEL_ID_LEFT].id_ = PANEL_ID_LEFT;
	panel[PANEL_ID_LEFT].device_id_ = DEVICE_ID_UNSET;
	panel[PANEL_ID_LEFT].x1_ = UI_LEFT_PANEL_BODY_X1;
	panel[PANEL_ID_LEFT].y1_ = UI_LEFT_PANEL_BODY_Y1;
	panel[PANEL_ID_LEFT].width_ = UI_LEFT_PANEL_BODY_WIDTH;
	panel[PANEL_ID_LEFT].x2_ = UI_LEFT_PANEL_BODY_X2;
	panel[PANEL_ID_LEFT].y2_ = UI_LEFT_PANEL_BODY_Y2;

	panel[PANEL_ID_RIGHT].id_ = PANEL_ID_RIGHT;
	panel[PANEL_ID_RIGHT].device_id_ = DEVICE_ID_UNSET;
	panel[PANEL_ID_RIGHT].x1_ = UI_RIGHT_PANEL_BODY_X1;
	panel[PANEL_ID_RIGHT].y1_ = UI_RIGHT_PANEL_BODY_Y1;
	panel[PANEL_ID_RIGHT].width_ = UI_RIGHT_PANEL_BODY_WIDTH;
	panel[PANEL_ID_RIGHT].x2_ = UI_RIGHT_PANEL_BODY_X2;
	panel[PANEL_ID_RIGHT].y2_ = UI_RIGHT_PANEL_BODY_Y2;

	// set up the buttons - file actions
	uibutton[BUTTON_ID_COPY].id_ = BUTTON_ID_COPY;
	uibutton[BUTTON_ID_COPY].x1_ = 35;
	uibutton[BUTTON_ID_COPY].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y;
	uibutton[BUTTON_ID_COPY].x2_ = 44;
	uibutton[BUTTON_ID_COPY].y2_ = (6 + 0);
	uibutton[BUTTON_ID_COPY].width_ = 10;
	uibutton[BUTTON_ID_COPY].string_id_ = ID_STR_FILE_COPY_RIGHT;
	uibutton[BUTTON_ID_COPY].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_DELETE].id_ = BUTTON_ID_DELETE;
	uibutton[BUTTON_ID_DELETE].x1_ = 35;
	uibutton[BUTTON_ID_DELETE].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y + 1;
	uibutton[BUTTON_ID_DELETE].x2_ = 44;
	uibutton[BUTTON_ID_DELETE].y2_ = (9);
	uibutton[BUTTON_ID_DELETE].width_ = 10;
	uibutton[BUTTON_ID_DELETE].string_id_ = ID_STR_FILE_DELETE;
	uibutton[BUTTON_ID_DELETE].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_DUPLICATE].id_ = BUTTON_ID_DUPLICATE;
	uibutton[BUTTON_ID_DUPLICATE].x1_ = 35;
	uibutton[BUTTON_ID_DUPLICATE].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y + 2;
	uibutton[BUTTON_ID_DUPLICATE].x2_ = 44;
	uibutton[BUTTON_ID_DUPLICATE].y2_ = (12 + 0);
	uibutton[BUTTON_ID_DUPLICATE].width_ = 10;
	uibutton[BUTTON_ID_DUPLICATE].string_id_ = ID_STR_FILE_DUP;
	uibutton[BUTTON_ID_DUPLICATE].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_RENAME].id_ = BUTTON_ID_RENAME;
	uibutton[BUTTON_ID_RENAME].x1_ = 35;
	uibutton[BUTTON_ID_RENAME].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y + 3;
	uibutton[BUTTON_ID_RENAME].x2_ = 44;
	uibutton[BUTTON_ID_RENAME].y2_ = (15 + 0);
	uibutton[BUTTON_ID_RENAME].width_ = 10;
	uibutton[BUTTON_ID_RENAME].string_id_ = ID_STR_FILE_RENAME;
	uibutton[BUTTON_ID_RENAME].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_TEXT_VIEW].id_ = BUTTON_ID_TEXT_VIEW;
	uibutton[BUTTON_ID_TEXT_VIEW].x1_ = 35;
	uibutton[BUTTON_ID_TEXT_VIEW].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y + 4;
	uibutton[BUTTON_ID_TEXT_VIEW].x2_ = 44;
	uibutton[BUTTON_ID_TEXT_VIEW].y2_ = (18 + 0);
	uibutton[BUTTON_ID_TEXT_VIEW].width_ = 10;
	uibutton[BUTTON_ID_TEXT_VIEW].string_id_ = ID_STR_FILE_TEXT_PREVIEW;
	uibutton[BUTTON_ID_TEXT_VIEW].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_HEX_VIEW].id_ = BUTTON_ID_HEX_VIEW;
	uibutton[BUTTON_ID_HEX_VIEW].x1_ = 35;
	uibutton[BUTTON_ID_HEX_VIEW].y1_ = UI_MIDDLE_AREA_FILE_CMD_Y + 5;
	uibutton[BUTTON_ID_HEX_VIEW].x2_ = 44;
	uibutton[BUTTON_ID_HEX_VIEW].y2_ = (21 + 0);
	uibutton[BUTTON_ID_HEX_VIEW].width_ = 10;
	uibutton[BUTTON_ID_HEX_VIEW].string_id_ = ID_STR_FILE_HEX_PREVIEW;
	uibutton[BUTTON_ID_HEX_VIEW].state_ = UI_BUTTON_STATE_DISABLED;

	// set up the buttons - device actions
	uibutton[BUTTON_ID_NEXT_DEVICE].id_ = BUTTON_ID_NEXT_DEVICE;
	uibutton[BUTTON_ID_NEXT_DEVICE].x1_ = 35;
	uibutton[BUTTON_ID_NEXT_DEVICE].y1_ = UI_MIDDLE_AREA_DEV_CMD_Y;
	uibutton[BUTTON_ID_NEXT_DEVICE].x2_ = 44;
	uibutton[BUTTON_ID_NEXT_DEVICE].y2_ = (9);
	uibutton[BUTTON_ID_NEXT_DEVICE].width_ = 10;
	uibutton[BUTTON_ID_NEXT_DEVICE].string_id_ = ID_STR_DEV_NEXT;
	uibutton[BUTTON_ID_NEXT_DEVICE].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_REFRESH].id_ = BUTTON_ID_REFRESH;
	uibutton[BUTTON_ID_REFRESH].x1_ = 35;
	uibutton[BUTTON_ID_REFRESH].y1_ = UI_MIDDLE_AREA_DEV_CMD_Y + 1;
	uibutton[BUTTON_ID_REFRESH].x2_ = 44;
	uibutton[BUTTON_ID_REFRESH].y2_ = (6 + 0);
	uibutton[BUTTON_ID_REFRESH].width_ = 10;
	uibutton[BUTTON_ID_REFRESH].string_id_ = ID_STR_DEV_REFRESH_LISTING;
	uibutton[BUTTON_ID_REFRESH].state_ = UI_BUTTON_STATE_DISABLED;

	uibutton[BUTTON_ID_FORMAT].id_ = BUTTON_ID_FORMAT;
	uibutton[BUTTON_ID_FORMAT].x1_ = 35;
	uibutton[BUTTON_ID_FORMAT].y1_ = UI_MIDDLE_AREA_DEV_CMD_Y + 2;
	uibutton[BUTTON_ID_FORMAT].x2_ = 44;
	uibutton[BUTTON_ID_FORMAT].y2_ = (12 + 0);
	uibutton[BUTTON_ID_FORMAT].width_ = 10;
	uibutton[BUTTON_ID_FORMAT].string_id_ = ID_STR_DEV_FORMAT;
	uibutton[BUTTON_ID_FORMAT].state_ = UI_BUTTON_STATE_DISABLED;
}


// set up screen variables and draw screen for first time
void Screen_Render(void)
{
	Text_ClearScreen(APP_FOREGROUND_COLOR, APP_BACKGROUND_COLOR);
	Screen_DrawUI();
}

