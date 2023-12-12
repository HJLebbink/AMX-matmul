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
; extern "C" void tdpbf16ps_N32_M32_K64_asm(void* c, void* a, void* b); //RCX, RDX, R8, R9
GLOBAL tdpbf16ps_N32_M32_K64_asm

SECTION .text
tdpbf16ps_N32_M32_K64_asm:

  ldtilecfg   [config]

  ; n_stores = 4 (min 4); n_loads = 12 (min 12); n_spills 0; n_reuse = 12
  ; ========================================
  ; specification:
  ; C[0][1] += A[0][1] * B[0][0]
  ; C[0][0] += A[0][0] * B[0][0]
  ; C[1][0] += A[0][0] * B[0][1]
  ; C[1][1] += A[0][1] * B[0][1]
  ; C[0][0] += A[1][0] * B[1][0]
  ; C[0][1] += A[1][1] * B[1][0]
  ; C[1][0] += A[1][0] * B[1][1]
  ; C[1][1] += A[1][1] * B[1][1]


  mov         r10d, 64     ; stride is always 64
  ; registers: 0= 1= 2= 3= 4= 5= 6= 7= 
  tileloaddt1 tmm0, [rdx + r10 + 1*1024]	; load A[0][1]
  tileloaddt1 tmm1, [r8  + r10 + 0*1024]	; load B[0][0]
  tileloaddt1 tmm2, [rcx + r10 + 1*1024]	; load C[0][1]
  tdpbf16ps   tmm2, tmm0, tmm1				; C[0][1] += A[0][1] * B[0][0]
 ;tilestored  [rcx + r10 + 1*1024], tmm2 	; store C[0][1]

  ; registers: 0=A[0][1] 1=B[0][0] 2=C[0][1] 3= 4= 5= 6= 7= 
  tileloaddt1 tmm3, [rdx + r10 + 0*1024]	; load A[0][0]
 ;tileloadd   tmm1, [r8  + r10 + 0*1024]	; load B[0][0]
  tileloaddt1 tmm4, [rcx + r10 + 0*1024]	; load C[0][0]
  tdpbf16ps   tmm4, tmm3, tmm1				; C[0][0] += A[0][0] * B[0][0]
 ;tilestored  [rcx + r10 + 0*1024], tmm4 	; store C[0][0]

  ; registers: 0=A[0][1] 1= 2=C[0][1] 3=A[0][0] 4=C[0][0] 5= 6= 7= 
 ;tileloadd   tmm3, [rdx + r10 + 0*1024]	; load A[0][0]
  tileloaddt1 tmm1, [r8  + r10 + 1*1024]	; load B[0][1]
  tileloaddt1 tmm5, [rcx + r10 + 2*1024]	; load C[1][0]
  tdpbf16ps   tmm5, tmm3, tmm1				; C[1][0] += A[0][0] * B[0][1]
 ;tilestored  [rcx + r10 + 2*1024], tmm5 	; store C[1][0]

  ; registers: 0=A[0][1] 1=B[0][1] 2=C[0][1] 3= 4=C[0][0] 5=C[1][0] 6= 7= 
 ;tileloadd   tmm0, [rdx + r10 + 1*1024]	; load A[0][1]
 ;tileloadd   tmm1, [r8  + r10 + 1*1024]	; load B[0][1]
  tileloaddt1 tmm3, [rcx + r10 + 3*1024]	; load C[1][1]
  tdpbf16ps   tmm3, tmm0, tmm1				; C[1][1] += A[0][1] * B[0][1]
 ;tilestored  [rcx + r10 + 3*1024], tmm3 	; store C[1][1]

  ; registers: 0= 1= 2=C[0][1] 3=C[1][1] 4=C[0][0] 5=C[1][0] 6= 7= 
  tileloaddt1 tmm0, [rdx + r10 + 2*1024]	; load A[1][0]
  tileloaddt1 tmm1, [r8  + r10 + 2*1024]	; load B[1][0]
 ;tileloaddt1 tmm4, [rcx + r10 + 0*1024]	; load C[0][0]
  tdpbf16ps   tmm4, tmm0, tmm1				; C[0][0] += A[1][0] * B[1][0]
  tilestored  [rcx + r10 + 0*1024], tmm4 	; store C[0][0]

  ; registers: 0=A[1][0] 1=B[1][0] 2=C[0][1] 3=C[1][1] 4= 5=C[1][0] 6= 7= 
  tileloaddt1 tmm4, [rdx + r10 + 3*1024]	; load A[1][1]
 ;tileloadd   tmm1, [r8  + r10 + 2*1024]	; load B[1][0]
 ;tileloaddt1 tmm2, [rcx + r10 + 1*1024]	; load C[0][1]
  tdpbf16ps   tmm2, tmm4, tmm1				; C[0][1] += A[1][1] * B[1][0]
  tilestored  [rcx + r10 + 1*1024], tmm2 	; store C[0][1]

  ; registers: 0=A[1][0] 1= 2= 3=C[1][1] 4=A[1][1] 5=C[1][0] 6= 7= 
 ;tileloadd   tmm0, [rdx + r10 + 2*1024]	; load A[1][0]
  tileloaddt1 tmm1, [r8  + r10 + 3*1024]	; load B[1][1]
 ;tileloaddt1 tmm5, [rcx + r10 + 2*1024]	; load C[1][0]
  tdpbf16ps   tmm5, tmm0, tmm1				; C[1][0] += A[1][0] * B[1][1]
  tilestored  [rcx + r10 + 2*1024], tmm5 	; store C[1][0]

  ; registers: 0= 1=B[1][1] 2= 3=C[1][1] 4=A[1][1] 5= 6= 7= 
 ;tileloadd   tmm4, [rdx + r10 + 3*1024]	; load A[1][1]
 ;tileloadd   tmm1, [r8  + r10 + 3*1024]	; load B[1][1]
 ;tileloaddt1 tmm3, [rcx + r10 + 3*1024]	; load C[1][1]
  tdpbf16ps   tmm3, tmm4, tmm1				; C[1][1] += A[1][1] * B[1][1]
  tilestored  [rcx + r10 + 3*1024], tmm3 	; store C[1][1]

  tilerelease
  ret

