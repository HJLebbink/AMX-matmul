BITS     64
ALIGN	 8, nop

SECTION .data align=16

;   struct Tile_config {
;        uint8_t palette_id;
;        uint8_t start_row;
;        uint8_t reserved_0[14];
;        uint16_t colsb[8];
;        uint16_t reserved_1[8];
;        uint8_t rows[8];
;        uint8_t reserved_2[8];

config:	db 1,0,  
		db 0,0,0,0,0,0,0,0,0,0,0,0,0,0 ; reserved_0
		dw 4,64,4,0,0,0,0,0            ; colsb[8];
		dw 0,0,0,0,0,0,0,0             ; reserved_1
		db 1,1,16,0,0,0,0,0            ; rows[8];
		db 0,0,0,0,0,0,0,0             ; reserved_2

;----------------------------------------------------------------------------
; extern "C" void vecmul_bf16_asm(void *c, void* a, void* b, int length); //RCX, RDX, R8, R9
GLOBAL vecmul_bf16_asm

SECTION .text
vecmul_bf16_asm:

	shr         r9d, 5            ; divide by 32, we will be processing 16 elements in a tile
	jz          exit

	mov         r10d, 4           ; set STRIDE to 4 bytes
	ldtilecfg   [config]

	tilezero    tmm0

	ALIGN 32
loop:

	tileloadd   tmm1, [rdx + r10]       ; stride does not matter since we only load one row
	tileloadd   tmm2, [r8  + r10]
	
	tdpbf16ps   tmm0, tmm1, tmm2
	;tdpfp16ps   tmm0, tmm1, tmm2    ; AMX-FP16 (Granite Rapids)

	add         rdx, 64
	add         r8,  64

	dec         r9d
	jnz         loop

	tilestored  [rcx + r10], tmm0
	tilerelease	

exit:
	ret
