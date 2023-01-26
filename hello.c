// test file for compiling for f256jr

/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

// project includes
// #include "sys.h"
// #include "general.h"
// #include "text.h"
// #include "memory.h"
// #include "app.h"

// C includes
// #include <stdbool.h>
#include <stdio.h>
// #include <stdlib.h>
#include <string.h>
// #include <stdint.h>
#include <stdlib.h>

// cc65 includes
//#include <conio.h>

// char*			global_string[10];
// 
// extern uint8_t	zp_bank_num;
// extern uint8_t	zp_old_bank_num;
// 
// #pragma zpsym ("zp_bank_num");
// #pragma zpsym ("zp_old_bank_num");

/*****************************************************************************/
/*                       Private Function Prototypes                         */
/*****************************************************************************/


/*****************************************************************************/
/*                       Private Function Definitions                        */
/*****************************************************************************/



/*****************************************************************************/
/*                        Public Function Definitions                        */
/*****************************************************************************/


int main(int argc, char* argv[])
{
char*	somestuff;
unsigned int delayx;

if ( (somestuff = (char*)calloc(123, sizeof(char)) ) == NULL)
{
	puts("SUCCESS!!!\n\n\n");
}
else
{
	puts("FAIL!\n\n\n");
}

for (delayx=0; delayx < 65535; ++delayx)
{
	// nothing.
}

puts("hello");
return 0;

// 	uint8_t		the_char = 0;
// 	uint8_t		i;
// 	uint8_t*	vram_loc = (uint8_t*)0xc000;
// 
// 	uint8_t	original_lut_setting[8];
// 	
// 	// Swap I/O Page 2 into bank 6	
// 	asm("lda #$02");
// 	asm("sta $0001");
// 
// 	// put some junk on the screen
// 	asm("lda #66");
// 	asm("sta $c004");
// 	*vram_loc++ = 33;
// 	*vram_loc++ = 34;
// 	*vram_loc++ = 65;
// 	*vram_loc++ = 66;
// 	*vram_loc++ = 67;
// 	*vram_loc++ = 68;
// 	
// 	
// 	// test some LK sys routines
// 	// adjust MMU mapping so that banks 0-5 are $0000 - $BFFF (the kernal may have set them to something else)
// 	for (i = 0; i < 4; i++)
// 	{
// 		zp_bank_num = i;
// 		Memory_SwapInNewBank(i);
// 		original_lut_setting[i] = zp_old_bank_num;	// capture LUT settings so we can restore before giving back to DOS/etc. 
// 	}
// 
// 	// still here? 
// 	vram_loc += 80;
// 	*vram_loc++ = 69;
// 	*vram_loc++ = 70;
// 	*vram_loc++ = 71;
// 
// 	if (Sys_InitSystem() == false)
// 	{
// 		exit(0);
// 	}
// 	
// 	// clear screen, set text mode (no overlay), turn off visual cursor
// 
// 	Sys_SetBorderSize(23, 31);
// 	DEBUG_OUT(("hello: border set to 23,31"));
// 	//asm("STP"); // force a break
// 	Sys_EnableTextModeCursor(false);
// 	DEBUG_OUT(("hello: cursor disabled"));
// 	Text_ClearScreen(FG_COLOR_WHITE, BG_COLOR_BLACK);
// 	DEBUG_OUT(("hello: screen cleared"));
// 	//asm("STP"); // force a break
// 
// 	// test text rendering
// 	{
// 		char*	a_string = "Hello, Hoomans! The Lich King sends you greetings and chilly wishes.";
// 		char*	b_string = "All who read this are doomed to die! So come to the Lair first for some fun!";
// 		char*	counting_string = "123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 ";
// 		
// 		Text_DrawStringAtXY(
// 			0, 2, 
// 			a_string,
// 			COLOR_BRIGHT_WHITE, COLOR_BLACK
// 		);
// 		Text_DrawStringAtXY(
// 			0, 3, 
// 			b_string,
// 			COLOR_BLACK, COLOR_BRIGHT_YELLOW
// 		);
// 		Text_DrawStringAtXY(
// 			0, 26, 
// 			counting_string,
// 			COLOR_BLACK, COLOR_BRIGHT_CYAN
// 		);
// 		
// 		DEBUG_OUT(("hello: text message written to 0,25r"));
// 	}
// 	
// 	// Swap I/O Page 2 back out in bank 6	
// 	asm("lda #$02");
// 	asm("sta $0001");
// 
// 	// restore MMU mappings to what they were on startup
// 	for (i = 0; i < 4; i++)
// 	{
// 		zp_bank_num = i;
// 		Memory_SwapInNewBank(original_lut_setting[i]);
// 	}
// 	
// 	return 0;
}

// #pragma code-name ("SHIMCODE")
// 
// void jump_to_startup(void)
// {
// 	asm("jmp $0210");
// }
