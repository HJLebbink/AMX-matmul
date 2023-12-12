
.intel_syntax
BITS     64
ALIGN	 8, nop

SECTION .data

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
		dw 64,64,64,0,0,0,0,0          ; colsb[8];
		dw 0,0,0,0,0,0,0,0             ; reserved_1
		db 16,16,16,0,0,0,0,0          ; rows[8];
		db 0,0,0,0,0,0,0,0             ; reserved_2

;----------------------------------------------------------------------------
; extern "C" void tdpbf16ps_N16_M16_K32_asm(float *c, BF16* a, BF16* b); //RCX, RDX, R8, R9
GLOBAL tdpbf16ps_N16_M16_K32_asm

SECTION .text
tdpbf16ps_N16_M16_K32_asm:

 ; C[0][0] += A[0][0] * B[0][0]

	mov         r10d, 64 ; set STRIDE to 64
	ldtilecfg   [config]

	tileloadd   tmm0, [rcx + r10] ; load C[0][0]
	tileloadd   tmm1, [rdx + r10] ; load A[0][0]
	tileloadd   tmm2, [r8 + r10]  ; load B[0][0]
	tdpbf16ps   tmm0, tmm1, tmm2  ; C[0][0] += A[0][0] * B[0][0]
	tilestored  [rcx + r10], tmm0 ; store C[0][0]
	tilerelease	
	ret

;----------------------------------------------------------------------------
; extern "C" void tdpbf16ps_N16_M16_K32_no_config_asm(float *c, BF16* a, BF16* b); //RCX, RDX, R8, R9
GLOBAL tdpbf16ps_N16_M16_K32_no_config_asm

SECTION .text
tdpbf16ps_N16_M16_K32_no_config_asm:

 ; C[0][0] += A[0][0] * B[0][0]

	mov         r10d, 64 ; set STRIDE to 64

	tileloadd   tmm0, [rcx + r10] ; load C[0][0]
	tileloadd   tmm1, [rdx + r10] ; load A[0][0]
	tileloadd   tmm2, [r8 + r10]  ; load B[0][0]
	tdpbf16ps   tmm0, tmm1, tmm2  ; C[0][0] += A[0][0] * B[0][0]
	tilestored  [rcx + r10], tmm0 ; store C[0][0]
	ret
