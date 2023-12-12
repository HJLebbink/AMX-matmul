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
		dw 64,64,64,64,64,0,0,0          ; colsb[8];
		dw 0,0,0,0,0,0,0,0             ; reserved_1
		db 16,16,16,16,16,0,0,0          ; rows[8];
		db 0,0,0,0,0,0,0,0             ; reserved_2

;----------------------------------------------------------------------------
; extern "C" void tdpbssd_N16_M16_K128_asm(void *c, void* a, void* b); //RCX, RDX, R8, R9
GLOBAL tdpbssd_N16_M16_K128_asm

SECTION .text
tdpbssd_N16_M16_K128_asm:

 ; C[0][0] += A[0][0] * B[0][0]
 ; C[0][0] += A[1][0] * B[1][0]
 
  ldtilecfg   [config]
  mov         r10d, 64 ; set STRIDE to 64

  tileloadd   tmm0, [rcx + r10 + 0]      ; load C[0][0]

  tileloadd   tmm1, [rdx + 2*r10 + 0]    ; load A[0][0]
  tileloadd   tmm3, [rdx + 2*r10 + 64]   ; load A[1][0]
  tileloadd   tmm2, [r8  + 2*r10 + 0]    ; load B[0][0]
  tileloadd   tmm4, [r8  + 2*r10 + 64]   ; load B[1][0]

  tdpbssd     tmm0, tmm1, tmm2           ; C[0][0] += A[0][0] * B[0][0]
  tdpbssd     tmm0, tmm3, tmm4           ; C[0][0] += A[1][0] * B[1][0]

  tilestored  [rcx + r10], tmm0          ; store C[0][0]
  tilerelease	
  ret
