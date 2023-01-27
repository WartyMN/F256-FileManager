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


/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/

// there are 12 buttons which can be accessed with the same code
#define NUM_BUTTONS					9

#define BUTTON_ID_COPY				0
#define BUTTON_ID_DELETE			1
#define BUTTON_ID_DUPLICATE			2
#define BUTTON_ID_RENAME			3
#define BUTTON_ID_TEXT_VIEW			4
#define BUTTON_ID_HEX_VIEW			5
#define BUTTON_ID_NEXT_DEVICE		6
#define BUTTON_ID_REFRESH			7
#define BUTTON_ID_FORMAT			8

#define DEVICE_ID_UNSET				-1
#define DEVICE_ID_ERROR				-2

#define UI_BUTTON_STATE_DISABLED	0
#define UI_BUTTON_STATE_NORMAL		1
#define UI_BUTTON_STATE_SELECTED	2

#define UI_MIDDLE_AREA_START_X			35
#define UI_MIDDLE_AREA_START_Y			5
#define UI_MIDDLE_AREA_WIDTH			10
#define UI_MIDDLE_AREA_FILE_CMD_Y		(UI_MIDDLE_AREA_START_Y + 3)
#define UI_MIDDLE_AREA_DEV_MENU_Y		16
#define UI_MIDDLE_AREA_DEV_CMD_Y		(UI_MIDDLE_AREA_DEV_MENU_Y + 3)

#define UI_PANEL_INNER_WIDTH			33
#define UI_PANEL_OUTER_WIDTH			(UI_PANEL_INNER_WIDTH + 2)
#define UI_PANEL_INNER_HEIGHT			43
#define UI_PANEL_OUTER_HEIGHT			(UI_PANEL_INNER_HEIGHT + 2)
#define UI_PANEL_TAB_WIDTH				28
#define UI_PANEL_TAB_HEIGHT				3
#define UI_PANEL_FILENAME_OFFSET		1	// from left edge of panel to start of filename
#define UI_PANEL_FILETYPE_OFFSET		21	// from start of filename to start of filesize
#define UI_PANEL_FILESIZE_OFFSET		7	// from start of filesize to start of filetype

#define UI_LEFT_PANEL_TITLE_TAB_X1		0
#define UI_LEFT_PANEL_TITLE_TAB_Y1		3
#define UI_LEFT_PANEL_TITLE_TAB_WIDTH	UI_PANEL_TAB_WIDTH
#define UI_LEFT_PANEL_TITLE_TAB_HEIGHT	UI_PANEL_TAB_HEIGHT
#define UI_LEFT_PANEL_TITLE_TAB_X2		(UI_LEFT_PANEL_TITLE_TAB_X1 + UI_LEFT_PANEL_TITLE_TAB_WIDTH - 1)
#define UI_LEFT_PANEL_TITLE_TAB_Y2		(UI_LEFT_PANEL_TITLE_TAB_Y1 + UI_LEFT_PANEL_TITLE_TAB_HEIGHT - 1)
#define UI_LEFT_PANEL_BODY_X1			0
#define UI_LEFT_PANEL_BODY_Y1			6
#define UI_LEFT_PANEL_BODY_WIDTH		UI_PANEL_OUTER_WIDTH
#define UI_LEFT_PANEL_BODY_HEIGHT		UI_PANEL_OUTER_HEIGHT
#define UI_LEFT_PANEL_BODY_X2			(UI_LEFT_PANEL_BODY_X1 + UI_LEFT_PANEL_BODY_WIDTH - 1)
#define UI_LEFT_PANEL_BODY_Y2			(UI_LEFT_PANEL_BODY_Y1 + UI_LEFT_PANEL_BODY_HEIGHT - 1)

#define UI_RIGHT_PANEL_X_DELTA			45

#define UI_RIGHT_PANEL_TITLE_TAB_X1		(UI_LEFT_PANEL_TITLE_TAB_X1 + UI_RIGHT_PANEL_X_DELTA)
#define UI_RIGHT_PANEL_TITLE_TAB_Y1		(UI_LEFT_PANEL_TITLE_TAB_Y1)
#define UI_RIGHT_PANEL_TITLE_TAB_WIDTH	UI_PANEL_TAB_WIDTH
#define UI_RIGHT_PANEL_TITLE_TAB_HEIGHT	UI_PANEL_TAB_HEIGHT
#define UI_RIGHT_PANEL_TITLE_TAB_X2		(UI_RIGHT_PANEL_TITLE_TAB_X1 + UI_RIGHT_PANEL_TITLE_TAB_WIDTH - 1)
#define UI_RIGHT_PANEL_TITLE_TAB_Y2		(UI_RIGHT_PANEL_TITLE_TAB_Y1 + UI_RIGHT_PANEL_TITLE_TAB_HEIGHT - 1)
#define UI_RIGHT_PANEL_BODY_X1			(UI_LEFT_PANEL_BODY_X1 + UI_RIGHT_PANEL_X_DELTA)
#define UI_RIGHT_PANEL_BODY_Y1			(UI_LEFT_PANEL_BODY_Y1)
#define UI_RIGHT_PANEL_BODY_WIDTH		UI_PANEL_OUTER_WIDTH
#define UI_RIGHT_PANEL_BODY_HEIGHT		UI_PANEL_OUTER_HEIGHT
#define UI_RIGHT_PANEL_BODY_X2			(UI_RIGHT_PANEL_BODY_X1 + UI_RIGHT_PANEL_BODY_WIDTH - 1)
#define UI_RIGHT_PANEL_BODY_Y2			(UI_RIGHT_PANEL_BODY_Y1 + UI_RIGHT_PANEL_BODY_HEIGHT - 1)

#define CH_UNDERSCORE					148		// this is one line up from a pure underscore, but works if text right under it. 0x5f	// '_'
#define CH_OVERSCORE					0x0e	// opposite of '_'

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
	uint8_t		x2_;
	uint8_t		y2_;
	uint8_t		width_;
	uint8_t		string_id_;
	uint8_t		state_;	// 0-disabled, 1-not selected, 2-selected
} UI_Button;

/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/

// swap the copy >>> button for copy <<< and vice versa
void Screen_SwapCopyDirectionIndicator(void);

// populate button objects, etc. no drawing.
void Screen_InitializeUI(void);

// set up screen variables and draw screen for first time
void Screen_Render(void);




#endif /* SCREEN_H_ */
