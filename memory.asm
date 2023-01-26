; native assembly code. see memory.h for the C interface.


;	.fopt	compiler,"cc65 v 2.18 - Git 8b5a2f13"
	.setcpu	"65C02"
	.smart	on
	.autoimport	on
	.case	on
	.debuginfo	off
	.importzp	sp, sreg, regsave, regbank
	.importzp	tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
	.macpack	longbranch
	
; import from lich king .c
;	.import _some_variable

; export to lich king .c
	.export	_Memory_SwapInNewBank
	.export	_Memory_RestorePreviousBank
	.export _Memory_GetMappedBankNum
	.export _Memory_DebugOut

; ZP_LK exports:
	.exportzp	_zp_bank_slot
	.exportzp	_zp_bank_num
	.exportzp	_zp_old_bank_num
	.exportzp	_zp_x
	.exportzp	_zp_y
	.exportzp	_zp_screen_id
	.exportzp	_zp_phys_addr_lo
	.exportzp	_zp_phys_addr_med
	.exportzp	_zp_phys_addr_hi
	.exportzp	_zp_cpu_addr_lo
	.exportzp	_zp_cpu_addr_hi
	.exportzp	_zp_temp_1
	.exportzp	_zp_temp_2
	.exportzp	_zp_temp_3
	.exportzp	_zp_temp_4
	.exportzp	_zp_other_byte

	.exportzp	_global_buffer_vis
	.exportzp	_global_first_map_appearance
	.exportzp	_global_player_last_action
	.exportzp	_global_player
	.exportzp	_global_string_buffer
	.exportzp	_global_string_buffer2
	
BSOUT				= $ffd2	;// Editor ROM routine to write character to screen
GETIN				= $ffe4	;// Editor ROM routine to wait for a key to be pressed

;zp_bank_slot		= $10
;zp_bank_num			= zp_bank_slot + 1
;zp_old_bank_num		= zp_bank_num + 1
;zp_x				= zp_old_bank_num + 1
;zp_y				= zp_x + 1
;zp_screen_id		= zp_y + 1
;zp_phys_addr_lo		= zp_y + 1
;zp_phys_addr_med	= zp_phys_addr_lo + 1
;zp_phys_addr_hi		= zp_phys_addr_med + 1
;zp_cpu_addr_lo		= zp_cpu_addr_lo + 1
;zp_cpu_addr_hi		= zp_cpu_addr_lo + 1
;zp_temp_1			= zp_cpu_addr_hi + 1
;zp_temp_2			= zp_temp_1 + 1
;zp_temp_3			= zp_temp_2 + 1
;zp_temp_4			= zp_temp_3 + 1
;zp_other_byte		= zp_temp_4 + 1


screen_id_player		= 0
screen_id_help			= 1
screen_id_inventory		= 2
screen_id_splash		= 3
screen_id_gameover_lost	= 4
screen_id_gameover_won	= 5
screen_id_master_map	= 6
screen_id_dungeon_map	= 7
screen_id_tinker		= 8


.segment "ZEROPAGE_LK" : zeropage

_zp_bank_slot:			.res 1;
_zp_bank_num:			.res 1;
_zp_old_bank_num:		.res 1;
_zp_x:					.res 1;
_zp_y:					.res 1;
_zp_screen_id:			.res 1;
_zp_phys_addr_lo:		.res 1;
_zp_phys_addr_med:		.res 1;
_zp_phys_addr_hi:		.res 1;
_zp_cpu_addr_lo:		.res 1;
_zp_cpu_addr_hi:		.res 1;
_zp_temp_1:				.res 1;
_zp_temp_2:				.res 1;
_zp_temp_3:				.res 1;
_zp_temp_4:				.res 1;
_zp_other_byte:			.res 1;
_zp_old_io_page:		.res 1;

_global_buffer_vis:				.res 1;
_global_first_map_appearance:	.res 1;
_global_player_last_action:		.res 1;
_global_player:					.res 2;
_global_string_buffer:			.res 2;
_global_string_buffer2:			.res 2;
	
	
; ---------------------------------------------------------------
; uint8_t __fastcall__ Memory_SwapInNewBank(uint8_t the_bank_slot)
; ---------------------------------------------------------------
;// call to a routine in memory.asm that modifies the MMU LUT to bring the specified bank of physical memory into the CPU's RAM space
;// set zp_bank_num before calling.
;// returns the slot that had been mapped previously

.segment	"CODE"

.proc	_Memory_SwapInNewBank: near

.segment	"CODE"

	SEI						; disable IRQs just in case one hits in the middle if MMU mapping
	
	TAX						; get the lut slot (0-7) we want to remap

							
.ifdef _SIMULATOR_			; emulator seems to start with LUT0, but kernel on machine with lut3. not sure why emulator is different
	LDA #$80				; edit mode (bit 7) + edit lut #4 (bits 4-5 both on) + active lut stays as #4 (bits 0-1 on)
.else
	LDA #$B3
.endif
	STA $0000				; make the change

	LDA $0008,x				; before modifying the current map, get the current value of the bank we're about to remap
	STA _zp_old_bank_num

	LDA _zp_bank_num		; get the target physical bank # back
	STA $0008,x				; Set the System bank to use for this bank

.ifdef _SIMULATOR_			; emulator seems to start with LUT0, but kernel on machine with lut3. not sure why emulator is different
	LDA #$00				; Select LUT#0 as active, turn off editing
.else
	LDA #$33				; Select LUT#3 as active, turn off editing
.endif
	STA $0000
	
	; do the return. cc65 requires functions return a 16 bit value!
	LDX #00
	LDA _zp_old_bank_num

	CLI						; safe to reenable IRQs now
	
	RTS

.endproc



; ---------------------------------------------------------------
; void __fastcall__ Memory_RestorePreviousBank(uint8_t the_bank_slot)
; ---------------------------------------------------------------
;// call to a routine in memory.asm that modifies the MMU LUT to bring the back the previously specified bank of physical memory into the CPU's RAM space
;// relies on a previous routine having set ZP_OLD_BANK_NUM. Should be called after Memory_SwapInNewBank(), when finished with the new bank

.segment	"CODE"

.proc	_Memory_RestorePreviousBank: near

.segment	"CODE"

	SEI						; disable IRQs just in case one hits in the middle if MMU mapping
	
	TAX						; get the lut slot (0-7) we want to remap

.ifdef _SIMULATOR_			; emulator seems to start with LUT0, but kernel on machine with lut3. not sure why emulator is different
	LDA #$80				; edit mode (bit 7) + edit lut #4 (bits 4-5 both on) + active lut stays as #4 (bits 0-1 on)
.else
	LDA #$B3
.endif
	STA $0000				; make the change

	LDA _zp_old_bank_num	; get the previously mapped physical bank # back
	STA $0008,x				; Set the System bank to use for this bank

.ifdef _SIMULATOR_			; emulator seems to start with LUT0, but kernel on machine with lut3. not sure why emulator is different
	LDA #$00				; Select LUT#0 as active, turn off editing
.else
	LDA #$33				; Select LUT#3 as active, turn off editing
.endif
	STA $0000				

	CLI						; safe to reenable IRQs now

	RTS

.endproc



; ---------------------------------------------------------------
; uint8_t __fastcall__ Memory_GetMappedBankNum(void)
; ---------------------------------------------------------------
;// call to a routine in memory.asm that returns whatever is currently mapped in the specified MMU slot
;// set zp_bank_num before calling.
;// returns the slot that had been mapped previously

.segment	"CODE"

.proc	_Memory_GetMappedBankNum: near

.segment	"CODE"

	SEI						; disable IRQs just in case one hits in the middle if MMU mapping
	
	TAX						; get the lut slot (0-7) we want to remap

							
.ifdef _SIMULATOR_			; emulator seems to start with LUT0, but kernel on machine with lut3. not sure why emulator is different
	LDA #$80				; edit mode (bit 7) + edit lut #4 (bits 4-5 both on) + active lut stays as #4 (bits 0-1 on)
.else
	LDA #$B3
.endif
	STA $0000				; make the change

	LDA $0008,x				; get the current value of the bank we're about to remap
	
	; do the return. cc65 requires functions return a 16 bit value!
	LDX #00

	CLI						; safe to reenable IRQs now

	RTS

.endproc


; ---------------------------------------------------------------
; void __fastcall__ Memory_DebugOut(void)
; ---------------------------------------------------------------
;// call to a routine in memory.asm that writes an illegal opcode followed by address of debug buffer
;// that is a simple to the f256jr emulator to write the string at the debug buffer out to the console

.segment	"CODE"

.proc	_Memory_DebugOut: near

.segment	"CODE"

	.byte $FC				; illegal opcode that to Paul's JR emulator means "next 2 bytes are address of a string I should write to console"
	.byte $00;
	.byte $03;				; we're using $0300 hard coded as a location for the moment.

	RTS

.endproc



