/*
 * memory.h
 *
 *  Created on: December 4, 2022
 *      Author: micahbly
 */

#ifndef MEMORY_H_
#define MEMORY_H_




/* about this class
 *
 * this header represents a set of assembly functions in memory.asm
 * the functions in this header file are all related to moving memory between physical F256jr RAM and MMU-mapped 6502 RAM
 * these functions neeed to be in the MAIN segment so they are always available
 * all functions that modify the LUT will reset it to its original configurate before exiting
 *
 */

/*****************************************************************************/
/*                                Includes                                   */
/*****************************************************************************/

#include "app.h"


/*****************************************************************************/
/*                            Macro Definitions                              */
/*****************************************************************************/

#define PARAM_FOR_ATTR_MEM	true	// param for functions updating VICKY screen memory: make it affect color/attribute memory
#define PARAM_FOR_CHAR_MEM	false	// param for functions updating VICKY screen memory: make it affect character memory

#define ZP_BANK_SLOT		0x10	// zero-page address holding the LUT slot to be modified (0-7) (eg, if 0, will be $08,if 1, $09, etc.)
#define ZP_BANK_NUM			0x11	// zero-page address holding the new LUT bank# to be set in the ZP_BANK_SLOT
#define ZP_OLD_BANK_NUM		0x12	// zero-page address holding the original LUT bank # before being changed
#define ZP_X				0x13	// zero-page address we will use for passing X coordinate to assembly routines
#define ZP_Y				0x14	// zero-page address we will use for passing Y coordinate to assembly routines
#define ZP_SCREEN_ID		0x15	// zero-page address we will use for passing the screen ID to assembly routines
#define ZP_PHYS_ADDR_LO		0x16	// zero-page address pointing to a 20-bit physical memory address
#define ZP_PHYS_ADDR_MI		0x17	// zero-page address pointing to a 20-bit physical memory address
#define ZP_PHYS_ADDR_HI		0x18	// zero-page address pointing to a 20-bit physical memory address
#define ZP_CPU_ADDR_LO		0x19	// zero-page address pointing to a 16-bit address in 6502 memory space (virtual 64k)
#define ZP_CPU_ADDR_HI		0x1A	// zero-page address pointing to a 16-bit address in 6502 memory space (virtual 64k)
#define ZP_TEMP_1			0x1B	// zero-page address we will use for temp variable storage in assembly routines
#define ZP_TEMP_2			0x1C	// zero-page address we will use for temp variable storage in assembly routines
#define ZP_TEMP_3			0x1D	// zero-page address we will use for temp variable storage in assembly routines
#define ZP_TEMP_4			0x1E	// zero-page address we will use for temp variable storage in assembly routines
#define ZP_OTHER_PARAM		0x1F	// zero-page address we will use for communicating 1 byte to/from assembly routines
#define ZP_OLD_IO_PAGE		0x20	// zero-page address holding the original IO page # before being changed



/*****************************************************************************/
/*                               Enumerations                                */
/*****************************************************************************/


/*****************************************************************************/
/*                                 Structs                                   */
/*****************************************************************************/


/*****************************************************************************/
/*                             Global Variables                              */
/*****************************************************************************/


/*****************************************************************************/
/*                       Public Function Prototypes                          */
/*****************************************************************************/

// call to a routine in memory.asm that modifies the MMU LUT to bring the specified bank of physical memory into the CPU's RAM space
// set zp_bank_num before calling.
// returns the slot that had been mapped previously
uint8_t __fastcall__ Memory_SwapInNewBank(uint8_t the_bank_slot);

// call to a routine in memory.asm that modifies the MMU LUT to bring the back the previously specified bank of physical memory into the CPU's RAM space
// relies on a previous routine having set ZP_OLD_BANK_NUM. Should be called after Memory_SwapInNewBank(), when finished with the new bank
// set zp_bank_num before calling.
void __fastcall__ Memory_RestorePreviousBank(uint8_t the_bank_slot);

// call to a routine in memory.asm that returns whatever is currently mapped in the specified MMU slot
// set zp_bank_num before calling.
// returns the slot that had been mapped previously
uint8_t __fastcall__ Memory_GetMappedBankNum(void);

// call to a routine in memory.asm that writes an illegal opcode followed by address of debug buffer
// that is a simple to the f256jr emulator to write the string at the debug buffer out to the console
void __fastcall__ Memory_DebugOut(void);



#endif /* MEMORY_H_ */
