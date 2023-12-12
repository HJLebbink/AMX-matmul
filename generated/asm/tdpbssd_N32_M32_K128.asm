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

config:	
    db 1,0,  
	db 0,0,0,0,0,0,0,0,0,0,0,0,0,0 ; reserved_0
	dw 64,64,64,64,64,64,64,64     ; colsb[8];
	dw 0,0,0,0,0,0,0,0             ; reserved_1
	db 16,16,16,16,16,16,16,16     ; rows[8];
	db 0,0,0,0,0,0,0,0             ; reserved_2

;----------------------------------------------------------------------------
; extern "C" void tdpbssd_N32_M32_K128_asm(void *c, void* a, void* b); //RCX, RDX, R8, R9
GLOBAL tdpbssd_N32_M32_K128_asm

SECTION .text
tdpbssd_N32_M32_K128_asm:

 ; C[0][0] += A[0][0] * B[0][0]
 ; C[0][1] += A[0][1] * B[0][0]
 ; C[1][0] += A[0][0] * B[0][1]
 ; C[1][1] += A[0][1] * B[0][1]

 ; C[0][0] += A[1][0] * B[1][0]
 ; C[0][1] += A[1][1] * B[1][0]
 ; C[1][0] += A[1][0] * B[1][1] 
 ; C[1][1] += A[1][1] * B[1][1]

  ldtilecfg   [config]
  mov         r10d, 64 ; set STRIDE to 64

  tileloadd   tmm0, [rcx + 2*r10 + 0]			; load C[0][0]
  tileloadd   tmm1, [rcx + 2*r10 + 2*1024]		; load C[0][1]
  tileloadd   tmm2, [rcx + 2*r10 + 64]			; load C[1][0]
  tileloadd   tmm3, [rcx + 2*r10 + 2*1024+64]	; load C[1][1]

  tileloadd   tmm4, [rdx + 2*r10 + 0]			; load A[0][0]
  tileloadd   tmm5, [rdx + 2*r10 + 2*1024]		; load A[0][1]
  tileloadd   tmm6, [r8  + 2*r10 + 0]			; load B[0][0]
  tileloadd   tmm7, [r8  + 2*r10 + 2*1024]      ; load B[0][1]

  tdpbssd     tmm0, tmm4, tmm6		    		; C[0][0] += A[0][0] * B[0][0]
  tdpbssd     tmm1, tmm5, tmm6		    		; C[0][1] += A[0][1] * B[0][0]
  tdpbssd     tmm2, tmm4, tmm7		    		; C[1][0] += A[0][0] * B[0][1]
  tdpbssd     tmm3, tmm5, tmm7		    		; C[1][1] += A[0][1] * B[0][1]

  tileloadd   tmm4, [rdx + 2*r10 + 64]          ; load A[1][0]
  tileloadd   tmm5, [rdx + 2*r10 + 2*1024+64]   ; load A[1][1]
  tileloadd   tmm6, [r8  + 2*r10 + 64]		    ; load B[1][0]  
  tileloadd   tmm7, [r8  + 2*r10 + 2*1024+64]   ; load B[1][1]

  tdpbssd     tmm0, tmm4, tmm6		    		; C[0][0] += A[1][0] * B[1][0]
  tdpbssd     tmm1, tmm5, tmm6		    		; C[0][1] += A[1][1] * B[1][0]
  tdpbssd     tmm2, tmm4, tmm7		    		; C[1][0] += A[1][0] * B[1][1]
  tdpbssd     tmm3, tmm5, tmm7		    		; C[1][1] += A[1][1] * B[1][1]

  tilestored  [rcx + 2*r10 + 0], tmm0			; store C[0][0]
  tilestored  [rcx + 2*r10 + 2*1024], tmm1		; store C[0][1]
  tilestored  [rcx + 2*r10 + 64], tmm2			; store C[1][0]
  tilestored  [rcx + 2*r10 + 2*1024+64], tmm3	; store C[1][1]

  tilerelease	
  ret
