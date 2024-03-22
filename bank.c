/*
 * bank.c
 *
 *  Created on: Mar 18, 2024
 *      Author: micahbly
 */




/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
#include "app.h"
#include "bank.h"
#include "comm_buffer.h"
#include "general.h"
#include "kernel.h"
#include "keyboard.h"
#include "list_panel.h"
#include "memory.h"
#include "screen.h"
#include "strings.h"
#include "sys.h"
#include "text.h"

// C includes
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// F256 includes
#include "f256.h"


/*****************************************************************************/
/*                               Definitions                                 */
/*****************************************************************************/


/*****************************************************************************/
/*                           File-scope Variables                            */
/*****************************************************************************/

static char*		bank_non_kup_name = "(non-KUP)";
static char*		bank_non_kup_description = "This bank is not a KUP (named) bank";
static char*		bank_kup_component_description = "This bank contains part of a KUP program";

/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/

extern TextDialogTemplate	global_dlg;	// dialog we'll configure and re-use for different purposes
extern char					global_dlg_title[36];	// arbitrary
extern char					global_dlg_body_msg[70];	// arbitrary
extern char					global_dlg_button[3][10];	// arbitrary

extern uint8_t		temp_screen_buffer_char[APP_DIALOG_BUFF_SIZE];	// WARNING HBD: don't make dialog box bigger than will fit!
extern uint8_t		temp_screen_buffer_attr[APP_DIALOG_BUFF_SIZE];	// WARNING HBD: don't make dialog box bigger than will fit!

extern char*		global_string_buff1;
extern char*		global_string_buff2;



/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/



/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/



/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/

// **** CONSTRUCTOR AND DESTRUCTOR *****



// **** CONSTRUCTOR AND DESTRUCTOR *****

// constructor
// Banks are not allocated dynamically, they are fixed array elements of the parent memory system object
// the name and description are, however, allocated so that they can be used even when bank itself is not mapped into CPU space
void Bank_Init(FMBankObject* the_bank, const char* the_name, const char* the_description, bank_type the_bank_type, uint8_t the_bank_num, uint8_t the_row)
{
	if (the_bank_type == BANK_KUP_PRIMARY)
	{
		if ( (the_bank->name_ = General_StrlcpyWithAlloc(the_name, FILE_MAX_FILENAME_SIZE)) == NULL)
		{
			//Buffer_NewMessage("could not allocate memory for the bank name");
			LOG_ERR(("%s %d: could not allocate memory for the bank name", __func__ , __LINE__));
			goto error;
		}
		LOG_ALLOC(("%s %d:	__ALLOC__	the_bank->name_	%p	size	%li", __func__ , __LINE__, the_bank->name_, General_Strnlen(the_bank->name_, FILE_MAX_FILENAME_SIZE) + 1));
	
		if ( (the_bank->description_ = General_StrlcpyWithAlloc(the_description, FILE_MAX_FILENAME_SIZE)) == NULL)
		{
			//Buffer_NewMessage("could not allocate memory for the bank description");
			LOG_ERR(("%s %d: could not allocate memory for the bank description", __func__ , __LINE__));
			goto error;
		}
		LOG_ALLOC(("%s %d:	__ALLOC__	the_bank->description_	%p	size	%li", __func__ , __LINE__, the_bank->description_, General_Strnlen(the_bank->description_, FILE_MAX_FILENAME_SIZE) + 1));

		the_bank->is_kup_ = true;
	}
	else if (the_bank_type == BANK_KUP_SECONDARY)
	{
		// copy the name, but take the fixed description stored in the bank.c file so we don't keep allocating strings
		if ( (the_bank->name_ = General_StrlcpyWithAlloc(the_name, FILE_MAX_FILENAME_SIZE)) == NULL)
		{
			//Buffer_NewMessage("could not allocate memory for the bank name");
			LOG_ERR(("%s %d: could not allocate memory for the bank name", __func__ , __LINE__));
			goto error;
		}
		LOG_ALLOC(("%s %d:	__ALLOC__	the_bank->name_	%p	size	%li", __func__ , __LINE__, the_bank->name_, General_Strnlen(the_bank->name_, FILE_MAX_FILENAME_SIZE) + 1));

		the_bank->description_ = bank_kup_component_description;
		the_bank->is_kup_ = false;
	}
	else
	{
		// assign the fixed name and description stored in the bank.c file so we don't keep allocating strings
		the_bank->name_ = bank_non_kup_name;
		the_bank->description_ = bank_non_kup_description;
		the_bank->is_kup_ = false;
	}

	//DEBUG_OUT(("%s %d: bank %02x: kup=%u, name='%s', desc='%s'", __func__ , __LINE__, the_bank_num, is_kup, the_bank->name_, the_bank->description_));

	// file is brand new: not selected yet.
	the_bank->selected_ = false;

	the_bank->row_ = the_row;
	the_bank->bank_num_ = the_bank_num;
	the_bank->addr_ = (uint32_t)the_bank_num * (uint32_t)8192;

	return;

error:
	if (the_bank) Bank_Destroy(&the_bank);
	DEBUG_OUT(("%s %d: bank %02x: error happened during allocation", __func__ , __LINE__, the_bank_num));
	return;
}


// destructor
// frees all allocated memory associated with the passed object but does not free the bank (no allocation for banks)
void Bank_Destroy(FMBankObject** the_bank)
{

	if (*the_bank == NULL)
	{
		//LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		App_Exit(ERROR_BANK_TO_DESTROY_WAS_NULL);	// crash early, crash often
	}

	// free name and description, but only if they aren't are boilerplate strings
	if ((*the_bank)->name_ != bank_non_kup_name)
	{
		if ((*the_bank)->name_ != NULL)
		{
			LOG_ALLOC(("%s %d:	__FREE__	(*the_bank)->name_	%p	size	%i", __func__ , __LINE__, (*the_bank)->name_, General_Strnlen((*the_bank)->name_, FILE_MAX_FILENAME_SIZE) + 1));
			free((*the_bank)->name_);
			(*the_bank)->name_ = NULL;
		}
	
		if ((*the_bank)->description_ != NULL)
		{
			LOG_ALLOC(("%s %d:	__FREE__	(*the_bank)->description_	%p	size	%i", __func__ , __LINE__, (*the_bank)->description_, General_Strnlen((*the_bank)->description_, FILE_MAX_FILENAME_SIZE) + 1));
			free((*the_bank)->description_);
			(*the_bank)->description_ = NULL;
		}
	}
}



// **** SETTERS *****

// // set files selected/unselected status (no visual change)
// void Bank_SetSelected(FMBankObject* the_bank, bool selected)
// {
// 	if (the_bank == NULL)
// 	{
// 		LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
// 		return;
// 	}
// 	
// 	the_bank->selected_ = selected;
// 
// 	return;
// }


/// updates the icon's size/position information
void Bank_UpdatePos(FMBankObject* the_bank, uint8_t x, int8_t display_row, uint16_t row)
{
	if (the_bank == NULL)
	{
		//LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return;
	}
	
	the_bank->x_ = x;
	the_bank->display_row_ = display_row;
	the_bank->row_ = row;
}





// **** GETTERS *****


// get the selected/not selected state of the bank
bool Bank_IsSelected(FMBankObject* the_bank)
{
	if (the_bank == NULL)
	{
		//LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}

	return the_bank->selected_;
}





// **** FILL AND CLEAR FUNCTIONS *****


// fills the bank with 0s
void Bank_Clear(FMBankObject* the_bank)
{
	Bank_Fill(the_bank, 0);
}


// fills the bank with the passed value
void Bank_Fill(FMBankObject* the_bank, uint8_t the_fill_value)
{
	uint8_t		i;
	char*		the_buffer = (char*)STORAGE_FILE_BUFFER_1;
	
	// LOGIC:
	//   bank is made up of 64 pages of 256b
	//   to fill, we set the desired value in the interbank copy buff, then do 64 copy ops

	memset(the_buffer, the_fill_value, STORAGE_FILE_BUFFER_1_LEN);
	
	for (i = 0; i < 64; i++)
	{
		App_EMDataCopy((uint8_t*)the_buffer, the_bank->bank_num_, i, PARAM_COPY_TO_EM);
	}
	
	sprintf(global_string_buff1, General_GetString(ID_STR_MSG_BANK_FILLED_WITH), the_bank->bank_num_, the_fill_value, the_fill_value);
	Buffer_NewMessage(global_string_buff1);
}


// asks the user to enter a fill value and returns it
// returns a negative number if user aborts, or a 0-255 number.
int16_t Bank_AskForFillValue(void)
{
	int8_t				i;
	uint8_t				the_len;
	uint16_t			the_fill_value = 0;
	bool				success;
	bool				take_char_value = false;

	General_Strlcpy((char*)&global_dlg_title, General_GetString(ID_STR_DLG_FILL_BANK_TITLE), COMM_BUFFER_MAX_STRING_LEN);
	General_Strlcpy((char*)&global_dlg_body_msg, General_GetString(ID_STR_DLG_FILL_BANK_BODY), APP_DIALOG_WIDTH);
	global_string_buff2[0] = 0;	// clear whatever string had been in this buffer before
	
	success = Text_DisplayTextEntryDialog(&global_dlg, (char*)&temp_screen_buffer_char, (char*)&temp_screen_buffer_attr, global_string_buff2, 3); //len("255")=3
	
	// did user enter a value?
	if (success == false)
	{
		return -1;
	}
	
	// user entered a number or a char, use that to fill. 
	
	// LOGIC
	//   if first char user entered is '0' - '9', convert the string to a number.
	//   otherwise, use the first char's byte value as the fill. 
	//   in other words, if user enters "255", fill with FF. If user enters "aBc", fill with "a"
	
	the_len = General_Strnlen(global_string_buff2, 3);
	
	// loop through each char, stopping on the first one that isn't a number.
	for (i = 0; i < the_len; i++)
	{
		if (global_string_buff2[i] > 47 && global_string_buff2[i] < 58)
		{
			the_fill_value = the_fill_value * 10 + (global_string_buff2[i] - 48);
		}
		else
		{
			take_char_value = true;
			continue;
		}
	}
	
	// take the value of the first character entered as the fill value? if not, check if over 255.
	if (take_char_value)
	{
		the_fill_value = (uint16_t)global_string_buff2[0];
	}
	else if (the_fill_value > 255)
	{
		the_fill_value = 255;
	}
	
// 	// did we already give up and assign a fill value based on first char? if not, convert the number
// 	if (the_fill_value == 0)
// 	{
// 		the_fill_value = (uint16_t)(number_digits[0] * 100) + (uint16_t)(number_digits[1] * 10) + (uint16_t)(number_digits[2]);
// 		// note: no attempt made to stop people entering bogus values like "256" or "999". will just cap at 255.
// 		if (the_fill_value > 255)
// 		{
// 			the_fill_value = 255;
// 		}
// 	}
// 	else
// 	{
// 		DEBUG_OUT(("%s %d: fill=%i / %c (first letter accepted as fill val)", __func__ , __LINE__, the_fill_value, the_fill_value));
// 	}
// 	
// 	DEBUG_OUT(("%s %d: fill=%i, digits=%u,%u,%u", __func__ , __LINE__, the_fill_value, number_digits[0], number_digits[1], number_digits[2]));
	
	return (int16_t)the_fill_value;
}




// **** OTHER FUNCTIONS *****


// mark file as selected, and refresh display accordingly
bool Bank_MarkSelected(FMBankObject* the_bank, int8_t y_offset)
{
	// LOGIC: if file was already selected, don't do anything. don't change state, don't change visual appearance

	//DEBUG_OUT(("%s %d: filename: '%s', selected=%i", __func__ , __LINE__, the_bank->file_name_, the_bank->selected_));

	if (the_bank == NULL)
	{
		//LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}

	if (the_bank->selected_ == false)
	{
		the_bank->selected_ = true;

		// re-render with selected state
		Bank_Render(the_bank, the_bank->selected_, y_offset, true);	// if we're here, panel must be active?
	}

	return true;
}

// mark file as un-selected, and refresh display accordingly
bool Bank_MarkUnSelected(FMBankObject* the_bank, int8_t y_offset)
{
	// LOGIC: if file was already un-selected, don't do anything. don't change state, don't change visual appearance

	if (the_bank == NULL)
	{
		//LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return false;
	}

	if (the_bank->selected_ == true)
	{
		the_bank->selected_ = false;

		// re-render with un-selected state
		Bank_Render(the_bank, the_bank->selected_, y_offset, true);	// if we're here, panel must be active?
	}

	return true;
}


// render name, description, address and any other relevant labels at the previously established coordinates
// if as_selected is true, will render with inversed text. Otherwise, will render normally.
// if as_active is true, will render in LIST_ACTIVE_COLOR, otherwise in LIST_INACTIVE_COLOR
void Bank_Render(FMBankObject* the_bank, bool as_selected, int8_t y_offset, bool as_active)
{
	uint8_t	x1;
	uint8_t	x2;
	uint8_t	sizex;
	uint8_t	typex;
	uint8_t	the_color;
	int8_t	y;
	
	// LOGIC:
	//   Panel is responsible for having flowed the content in a way that each bank either has a displayable display_row_ value, or -1.
	//   y_offset is the first displayable row of the parent panel
	
	if (the_bank == NULL)
	{
		//LOG_ERR(("%s %d: passed class object was null", __func__ , __LINE__));
		return;
	}

	if (as_active)
	{
		the_color = LIST_ACTIVE_COLOR;
	}
	else
	{
		the_color = LIST_INACTIVE_COLOR;
	}
	
	x1 = the_bank->x_;
	x2 = the_bank->x_ + (UI_PANEL_INNER_WIDTH - 1);
	typex = x1 + UI_PANEL_BANK_NUM_OFFSET + 1;
	sizex = typex + UI_PANEL_BANK_ADDR_OFFSET + 0; // "bytes" is 5 in len, but we are using 6 digit size, so start one before bytes.
	
	if (the_bank->display_row_ != -1)
	{
		sprintf(global_string_buff1, "%06lx", the_bank->addr_);
		y = the_bank->display_row_ + y_offset;
		Text_FillBox(x1, y, x2, y, CH_SPACE, the_color, APP_BACKGROUND_COLOR);
		Text_DrawStringAtXY( x1, y, the_bank->name_, the_color, APP_BACKGROUND_COLOR);
		Text_DrawStringAtXY( sizex, y, global_string_buff1, the_color, APP_BACKGROUND_COLOR);
		sprintf(global_string_buff1, "%03u", the_bank->bank_num_);
		Text_DrawStringAtXY( typex, y, global_string_buff1, the_color, APP_BACKGROUND_COLOR);
		
		if (as_selected == true)
		{
			Text_InvertBox(x1, y, x2, y);
				
			// show description of the bank in the special status line under the bank panels, above the comms
			Text_FillBox( 0, UI_FULL_PATH_LINE_Y, 79, UI_FULL_PATH_LINE_Y, CH_SPACE, APP_BACKGROUND_COLOR, APP_BACKGROUND_COLOR);
			Text_DrawStringAtXY( 0, UI_FULL_PATH_LINE_Y, the_bank->description_, COLOR_GREEN, APP_BACKGROUND_COLOR);
		}
	}
	else
	{
		//DEBUG_OUT(("%s %d: can't render; x1=%i, x2=%i, x_bound=%i, x_offset=%i (%s)", __func__ , __LINE__, x1, x2, x_bound, x_offset, the_label->text_));
	}
}


// // helper function called by List class's print function: prints one file entry
// void Bank_Print(void* the_payload)
// {
// 	FMBankObject*		this_file = (FMBankObject*)(the_payload);
// 
// 	DEBUG_OUT(("|%-34s|%-1i|%-12lu|%-10s|%-8s|", this_file->file_name_, this_file->selected_, this_file->size_, this_file->datetime_.dat_StrDate, this_file->datetime_.dat_StrTime));
// }


// ***** comparison functions used to compare to list items with Wb2KFileObject payloads

// bool Bank_CompareName(void* first_payload, void* second_payload)
// {
// 	FMBankObject*		file_1 = (FMBankObject*)first_payload;
// 	FMBankObject*		file_2 = (FMBankObject*)second_payload;
// 
// 	if (General_Strncasecmp(file_1->file_name_, file_2->file_name_, FILE_MAX_FILENAME_SIZE) > 0)
// 	{
// 		return true;
// 	}
// 	else
// 	{
// 		return false;
// 	}
// }
