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
;	.export _Memory_Copy
	.export _Memory_CopyWithDMA
;	.export _Memory_FillWithDMA
	.export _Memory_DebugOut

; ZP_LK exports:
	.exportzp	_zp_bank_slot
	.exportzp	_zp_bank_num
	.exportzp	_zp_old_bank_num
	.exportzp	_zp_to_addr
	.exportzp	_zp_from_addr
	.exportzp	_zp_copy_len
;	.exportzp	_zp_x
;	.exportzp	_zp_y
;	.exportzp	_zp_screen_id
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

	.exportzp	_global_string_buffer
	.exportzp	_global_string_buffer2
	
BSOUT				= $ffd2	;// Editor ROM routine to write character to screen
GETIN				= $ffe4	;// Editor ROM routine to wait for a key to be pressed


; F256 DMA addresses and bit values

DMA_CTRL = $DF00		; DMA Control Register
DMA_CTRL_START = $80	; Start the DMA operation
DMA_CTRL_FILL = $04		; Do a FILL operation (if off, will do COPY)
DMA_CTRL_2D = $02		; Use 2D copy/fill
DMA_CTRL_ENABLE = $01	; Enable the DMA engine

DMA_STATUS = $DF01		; DMA status register (Read Only)
DMA_STAT_BUSY = $80		; DMA engine is busy with an operation

DMA_FILL_VAL = $DF01	; Byte value to use for fill operations
DMA_SRC_ADDR = $DF04	; Source address (system bus - 3 byte)
DMA_DST_ADDR = $DF08	; Destination address (system bus - 3 byte)
DMA_COUNT = $DF0C		; Number of bytes to fill or copy


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

; -- ZErOPAGE_LK starts at $10

_zp_bank_slot:			.res 1	; $10
_zp_bank_num:			.res 1
_zp_old_bank_num:		.res 1
_zp_to_addr:			.res 3
_zp_from_addr:			.res 3
_zp_copy_len:			.res 3
_zp_phys_addr_lo:		.res 1
_zp_phys_addr_med:		.res 1
_zp_phys_addr_hi:		.res 1
_zp_cpu_addr_lo:		.res 1
_zp_cpu_addr_hi:		.res 1	; $20
_zp_temp_1:				.res 1
_zp_temp_2:				.res 1
_zp_temp_3:				.res 1
_zp_temp_4:				.res 1
_zp_other_byte:			.res 1
_zp_old_io_page:		.res 1	;-- $26
;_zp_x:					.res 1
;_zp_y:					.res 1
;_zp_screen_id:			.res 1

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




; ---------------------------------------------------------------
; void __fastcall__ Memory_Copy(void)
; ---------------------------------------------------------------
;// call to a routine in memory.asm that copies specified number of bytes from src to dst
;// set zp_to_addr, zp_from_addr, zp_copy_len before calling.
;// credit: http://6502.org/source/general/memory_move.html


;.segment	"OVERLAY_NOTICE_BOARD"
;
;.proc	_Memory_Copy: near
;
;.segment	"OVERLAY_NOTICE_BOARD"
;
;MOVEUP:  LDX _zp_copy_len		; the last byte must be moved first
;         CLC         			; start at the final pages of FROM and TO
;         TXA
;         ADC _zp_from_addr+1
;         STA _zp_from_addr+1
;         CLC
;         TXA
;         ADC _zp_to_addr+1
;         STA _zp_to_addr+1
;         INX         			; allows the use of BNE after the DEX below
;         LDY _zp_copy_len+1
;         BEQ MU3
;         DEY          			; move bytes on the last page first
;         BEQ MU2
;MU1:     LDA (_zp_from_addr),Y
;         STA (_zp_to_addr),Y
;         DEY
;         BNE MU1
;MU2:     LDA (_zp_from_addr),Y 	; handle Y = 0 separately
;         STA (_zp_to_addr),Y
;MU3:     DEY
;         DEC _zp_from_addr+1   	; move the next page (if any)
;         DEC _zp_to_addr+1
;         DEX
;         BNE MU1
;         RTS
;
;.endproc




; ---------------------------------------------------------------
; void __fastcall__ Memory_CopyWithDMA(void)
; ---------------------------------------------------------------
;// call to a routine in memory.asm that copies specified number of bytes from src to dst
;// set zp_to_addr, zp_from_addr, zp_copy_len before calling.
;// this version uses the F256's DMA capabilities to copy, so addresses can be 24 bit (system memory, not CPU memory)
;// in other words, no need to page either dst or src into CPU space


.segment	"CODE"

.proc	_Memory_CopyWithDMA: near

.segment	"CODE"

			SEI					; disable interrupts

			; Wait for VBlank period
LINE_NO = 261*2  ; 240+21
        	LDA #<LINE_NO
        	LDX #>LINE_NO
wait1:
        	CPX $D01B
       	 	BEQ wait1
wait2:
        	cmp $D01A
        	CMP wait2

wait3:
        	CPX $D01B
        	BNE wait3
wait4:
        	CMP $D01A
        	BNE wait4


			STZ DMA_CTRL			; Turn off the DMA engine

			NOP						; random experimenting with trying to prevent timing issue
			NOP
			NOP
			NOP
			NOP
			
			; Enable the DMA engine and set it up for a (1D) copy operation:
			LDA #DMA_CTRL_ENABLE
			STA DMA_CTRL

			NOP						; random experimenting with trying to prevent timing issue
			NOP
			NOP
			NOP
			NOP
			
			;Source address (3 byte):
			LDA _zp_from_addr
			STA DMA_SRC_ADDR
			LDA _zp_from_addr+1
			STA DMA_SRC_ADDR+1
			LDA _zp_from_addr+2
			AND #$07
			STA DMA_SRC_ADDR+2

			;Destination address (3 byte):
			LDA _zp_to_addr
			STA DMA_DST_ADDR
			LDA _zp_to_addr+1
			STA DMA_DST_ADDR+1
			LDA _zp_to_addr+2
			AND #$07
			STA DMA_DST_ADDR+2

			; Num bytes to copy
			LDA _zp_copy_len
			STA DMA_COUNT
			LDA _zp_copy_len+1
			STA DMA_COUNT+1
			LDA _zp_copy_len+2
			STA DMA_COUNT+2

			; flip the START flag to trigger the DMA operation
			LDA DMA_CTRL
			ORA #DMA_CTRL_START
			STA DMA_CTRL
			; wait for it to finish

wait_dma:	LDA DMA_STATUS
			BMI wait_dma            ; Wait until DMA is not busy 
			
			NOP
			NOP
			NOP
			NOP
			NOP

			STZ DMA_CTRL			; Turn off the DMA engine
			
			NOP
			NOP
			NOP
			NOP
			NOP
			
			CLI						; re-enable interrupts
			
			RTS
.endproc


; ---------------------------------------------------------------
; void __fastcall__ Memory_FillWithDMA(void)
; ---------------------------------------------------------------
;// call to a routine in memory.asm that fills the specified number of bytes to the dst
;// set zp_to_addr, zp_copy_len to num bytes to fill, and zp_other_byte to the fill value before calling.
;// this version uses the F256's DMA capabilities to fill, so addresses can be 24 bit (system memory, not CPU memory)
;// in other words, no need to page either dst into CPU space


;.segment	"CODE"
;
;.proc	_Memory_FillWithDMA: near
;
;.segment	"CODE"
;
;			SEI					; disable interrupts
;
;LINE_NO = 261*2  ; 240+21
;        lda #<LINE_NO
;        ldx #>LINE_NO
;wait1:
;        cpx $D01B
;        beq wait1
;wait2:
;        cmp $D01A
;        beq wait2
;
;wait3:
;        cpx $D01B
;        bne wait3
;wait4:
;        cmp $D01A
;        bne wait4
;
;			STZ DMA_CTRL			; Turn off the DMA engine
;			
;			; Enable the DMA engine and set it up for a FILL operation:
;			LDA #DMA_CTRL_FILL | DMA_CTRL_ENABLE
;			STA DMA_CTRL
;
;			; the fill value
;            lda _zp_other_byte
;            sta DMA_FILL_VAL
;            
;			;Destination address (3 byte):
;			LDA _zp_to_addr
;			STA DMA_DST_ADDR
;			LDA _zp_to_addr+1
;			STA DMA_DST_ADDR+1
;			LDA _zp_to_addr+2
;			AND #$07
;			STA DMA_DST_ADDR+2
;
;			; Num bytes to fill
;			LDA _zp_copy_len
;			STA DMA_COUNT
;			LDA _zp_copy_len+1
;			STA DMA_COUNT+1
;			LDA _zp_copy_len+2
;			STA DMA_COUNT+2
;
;			; flip the START flag to trigger the DMA operation
;			LDA DMA_CTRL
;			ORA #DMA_CTRL_START
;			STA DMA_CTRL
;			; wait for it to finish
;
;wait_dma:	LDA DMA_STATUS
;			BMI wait_dma            ; Wait until DMA is not busy 
;			
;			NOP
;			NOP
;			NOP
;			NOP
;			NOP
;			NOP
;			
;			NOP
;			NOP
;
;			NOP
;			NOP
;			
;			CLI						; re-enable interrupts
;			
;			RTS
;.endproc



