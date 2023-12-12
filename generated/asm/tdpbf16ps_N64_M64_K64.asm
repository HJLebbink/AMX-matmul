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
; extern "C" void tdpbf16ps_N64_M64_K64_asm(void* c, void* a, void* b); //RCX, RDX, R8, R9
GLOBAL tdpbf16ps_N64_M64_K64_asm

SECTION .text
tdpbf16ps_N64_M64_K64_asm:

  ldtilecfg   [config]

  ; n_stores = 16 (min 16); n_loads = 40 (min 32); n_spills 8; n_reuse = 56
  ; ========================================
  ; specification:
  ; C[0][0] += A[0][0] * B[0][0]
  ; C[0][0] += A[1][0] * B[1][0]
  ; C[0][1] += A[0][1] * B[0][0]
  ; C[0][1] += A[1][1] * B[1][0]
  ; C[0][2] += A[0][2] * B[0][0]
  ; C[0][2] += A[1][2] * B[1][0]
  ; C[0][3] += A[0][3] * B[0][0]
  ; C[0][3] += A[1][3] * B[1][0]
  ; C[1][0] += A[0][0] * B[0][1]
  ; C[1][0] += A[1][0] * B[1][1]
  ; C[1][1] += A[0][1] * B[0][1]
  ; C[1][1] += A[1][1] * B[1][1]
  ; C[1][2] += A[0][2] * B[0][1]
  ; C[1][2] += A[1][2] * B[1][1]
  ; C[1][3] += A[0][3] * B[0][1]
  ; C[1][3] += A[1][3] * B[1][1]
  ; C[2][2] += A[1][2] * B[1][2]
  ; C[2][0] += A[1][0] * B[1][2]
  ; C[2][3] += A[1][3] * B[1][2]
  ; C[2][1] += A[1][1] * B[1][2]
  ; C[2][2] += A[0][2] * B[0][2]
  ; C[2][0] += A[0][0] * B[0][2]
  ; C[2][3] += A[0][3] * B[0][2]
  ; C[2][1] += A[0][1] * B[0][2]
  ; C[3][0] += A[0][0] * B[0][3]
  ; C[3][0] += A[1][0] * B[1][3]
  ; C[3][1] += A[0][1] * B[0][3]
  ; C[3][1] += A[1][1] * B[1][3]
  ; C[3][2] += A[0][2] * B[0][3]
  ; C[3][2] += A[1][2] * B[1][3]
  ; C[3][3] += A[0][3] * B[0][3]
  ; C[3][3] += A[1][3] * B[1][3]


  mov         r10d, 64     ; stride is always 64
  ; registers: 0= 1= 2= 3= 4= 5= 6= 7= 
  tileloadd   tmm0, [rdx + r10 + 0*1024]	; load A[0][0]
  tileloaddt1 tmm1, [r8  + r10 + 0*1024]	; load B[0][0]
  tileloaddt1 tmm2, [rcx + r10 + 0*1024]	; load C[0][0]
  tdpbf16ps   tmm2, tmm0, tmm1				; C[0][0] += A[0][0] * B[0][0]
 ;tilestored  [rcx + r10 + 0*1024], tmm2 	; store C[0][0]

  ; registers: 0=A[0][0] 1=B[0][0] 2=C[0][0] 3= 4= 5= 6= 7= 
  tileloadd   tmm3, [rdx + r10 + 4*1024]	; load A[1][0]
  tileloaddt1 tmm4, [r8  + r10 + 4*1024]	; load B[1][0]
 ;tileloaddt1 tmm2, [rcx + r10 + 0*1024]	; load C[0][0]
  tdpbf16ps   tmm2, tmm3, tmm4				; C[0][0] += A[1][0] * B[1][0]
  tilestored  [rcx + r10 + 0*1024], tmm2 	; store C[0][0]

  ; registers: 0=A[0][0] 1=B[0][0] 2= 3=A[1][0] 4=B[1][0] 5= 6= 7= 
  tileloadd   tmm2, [rdx + r10 + 1*1024]	; load A[0][1]
 ;tileloadd   tmm1, [r8  + r10 + 0*1024]	; load B[0][0]
  tileloaddt1 tmm5, [rcx + r10 + 1*1024]	; load C[0][1]
  tdpbf16ps   tmm5, tmm2, tmm1				; C[0][1] += A[0][1] * B[0][0]
 ;tilestored  [rcx + r10 + 1*1024], tmm5 	; store C[0][1]

  ; registers: 0=A[0][0] 1=B[0][0] 2=A[0][1] 3=A[1][0] 4=B[1][0] 5=C[0][1] 6= 7= 
  tileloaddt1 tmm6, [rdx + r10 + 5*1024]	; load A[1][1]
 ;tileloadd   tmm4, [r8  + r10 + 4*1024]	; load B[1][0]
 ;tileloaddt1 tmm5, [rcx + r10 + 1*1024]	; load C[0][1]
  tdpbf16ps   tmm5, tmm6, tmm4				; C[0][1] += A[1][1] * B[1][0]
  tilestored  [rcx + r10 + 1*1024], tmm5 	; store C[0][1]

  ; registers: 0=A[0][0] 1=B[0][0] 2=A[0][1] 3=A[1][0] 4=B[1][0] 5= 6=A[1][1] 7= 
  tileloaddt1 tmm5, [rdx + r10 + 2*1024]	; load A[0][2]
 ;tileloadd   tmm1, [r8  + r10 + 0*1024]	; load B[0][0]
  tileloaddt1 tmm7, [rcx + r10 + 2*1024]	; load C[0][2]
  tdpbf16ps   tmm7, tmm5, tmm1				; C[0][2] += A[0][2] * B[0][0]
 ;tilestored  [rcx + r10 + 2*1024], tmm7 	; store C[0][2]

  ; registers: 0=A[0][0] 1=B[0][0] 2=A[0][1] 3=A[1][0] 4=B[1][0] 5=A[0][2] 6=A[1][1] 7=C[0][2] 
  tileloadd   tmm0, [rdx + r10 + 6*1024]	; load A[1][2]
 ;tileloadd   tmm4, [r8  + r10 + 4*1024]	; load B[1][0]
 ;tileloaddt1 tmm7, [rcx + r10 + 2*1024]	; load C[0][2]
  tdpbf16ps   tmm7, tmm0, tmm4				; C[0][2] += A[1][2] * B[1][0]
  tilestored  [rcx + r10 + 2*1024], tmm7 	; store C[0][2]

  ; registers: 0=A[1][2] 1=B[0][0] 2=A[0][1] 3=A[1][0] 4=B[1][0] 5=A[0][2] 6=A[1][1] 7= 
  tileloaddt1 tmm7, [rdx + r10 + 3*1024]	; load A[0][3]
 ;tileloadd   tmm1, [r8  + r10 + 0*1024]	; load B[0][0]
  tileloaddt1 tmm0, [rcx + r10 + 3*1024]	; load C[0][3]
  tdpbf16ps   tmm0, tmm7, tmm1				; C[0][3] += A[0][3] * B[0][0]
 ;tilestored  [rcx + r10 + 3*1024], tmm0 	; store C[0][3]

  ; registers: 0=C[0][3] 1= 2=A[0][1] 3=A[1][0] 4=B[1][0] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
  tileloadd   tmm1, [rdx + r10 + 7*1024]	; load A[1][3]
 ;tileloadd   tmm4, [r8  + r10 + 4*1024]	; load B[1][0]
 ;tileloaddt1 tmm0, [rcx + r10 + 3*1024]	; load C[0][3]
  tdpbf16ps   tmm0, tmm1, tmm4				; C[0][3] += A[1][3] * B[1][0]
  tilestored  [rcx + r10 + 3*1024], tmm0 	; store C[0][3]

  ; registers: 0= 1=A[1][3] 2=A[0][1] 3=A[1][0] 4= 5=A[0][2] 6=A[1][1] 7=A[0][3] 
  tileloadd   tmm0, [rdx + r10 + 0*1024]	; load A[0][0]
  tileloaddt1 tmm4, [r8  + r10 + 1*1024]	; load B[0][1]
  tileloaddt1 tmm1, [rcx + r10 + 4*1024]	; load C[1][0]
  tdpbf16ps   tmm1, tmm0, tmm4				; C[1][0] += A[0][0] * B[0][1]
 ;tilestored  [rcx + r10 + 4*1024], tmm1 	; store C[1][0]

  ; registers: 0=A[0][0] 1=C[1][0] 2=A[0][1] 3=A[1][0] 4=B[0][1] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm3, [rdx + r10 + 4*1024]	; load A[1][0]
  tileloaddt1 tmm0, [r8  + r10 + 5*1024]	; load B[1][1]
 ;tileloaddt1 tmm1, [rcx + r10 + 4*1024]	; load C[1][0]
  tdpbf16ps   tmm1, tmm3, tmm0				; C[1][0] += A[1][0] * B[1][1]
  tilestored  [rcx + r10 + 4*1024], tmm1 	; store C[1][0]

  ; registers: 0=B[1][1] 1= 2=A[0][1] 3=A[1][0] 4=B[0][1] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm2, [rdx + r10 + 1*1024]	; load A[0][1]
 ;tileloadd   tmm4, [r8  + r10 + 1*1024]	; load B[0][1]
  tileloaddt1 tmm1, [rcx + r10 + 5*1024]	; load C[1][1]
  tdpbf16ps   tmm1, tmm2, tmm4				; C[1][1] += A[0][1] * B[0][1]
 ;tilestored  [rcx + r10 + 5*1024], tmm1 	; store C[1][1]

  ; registers: 0=B[1][1] 1=C[1][1] 2=A[0][1] 3=A[1][0] 4=B[0][1] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm6, [rdx + r10 + 5*1024]	; load A[1][1]
 ;tileloadd   tmm0, [r8  + r10 + 5*1024]	; load B[1][1]
 ;tileloaddt1 tmm1, [rcx + r10 + 5*1024]	; load C[1][1]
  tdpbf16ps   tmm1, tmm6, tmm0				; C[1][1] += A[1][1] * B[1][1]
  tilestored  [rcx + r10 + 5*1024], tmm1 	; store C[1][1]

  ; registers: 0=B[1][1] 1= 2=A[0][1] 3=A[1][0] 4=B[0][1] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm5, [rdx + r10 + 2*1024]	; load A[0][2]
 ;tileloadd   tmm4, [r8  + r10 + 1*1024]	; load B[0][1]
  tileloaddt1 tmm1, [rcx + r10 + 6*1024]	; load C[1][2]
  tdpbf16ps   tmm1, tmm5, tmm4				; C[1][2] += A[0][2] * B[0][1]
 ;tilestored  [rcx + r10 + 6*1024], tmm1 	; store C[1][2]

  ; registers: 0=B[1][1] 1=C[1][2] 2=A[0][1] 3=A[1][0] 4=B[0][1] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
  tileloadd   tmm2, [rdx + r10 + 6*1024]	; load A[1][2]
 ;tileloadd   tmm0, [r8  + r10 + 5*1024]	; load B[1][1]
 ;tileloaddt1 tmm1, [rcx + r10 + 6*1024]	; load C[1][2]
  tdpbf16ps   tmm1, tmm2, tmm0				; C[1][2] += A[1][2] * B[1][1]
  tilestored  [rcx + r10 + 6*1024], tmm1 	; store C[1][2]

  ; registers: 0=B[1][1] 1= 2=A[1][2] 3=A[1][0] 4=B[0][1] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm7, [rdx + r10 + 3*1024]	; load A[0][3]
 ;tileloadd   tmm4, [r8  + r10 + 1*1024]	; load B[0][1]
  tileloaddt1 tmm1, [rcx + r10 + 7*1024]	; load C[1][3]
  tdpbf16ps   tmm1, tmm7, tmm4				; C[1][3] += A[0][3] * B[0][1]
 ;tilestored  [rcx + r10 + 7*1024], tmm1 	; store C[1][3]

  ; registers: 0=B[1][1] 1=C[1][3] 2=A[1][2] 3=A[1][0] 4= 5=A[0][2] 6=A[1][1] 7=A[0][3] 
  tileloadd   tmm4, [rdx + r10 + 7*1024]	; load A[1][3]
 ;tileloadd   tmm0, [r8  + r10 + 5*1024]	; load B[1][1]
 ;tileloaddt1 tmm1, [rcx + r10 + 7*1024]	; load C[1][3]
  tdpbf16ps   tmm1, tmm4, tmm0				; C[1][3] += A[1][3] * B[1][1]
  tilestored  [rcx + r10 + 7*1024], tmm1 	; store C[1][3]

  ; registers: 0= 1= 2=A[1][2] 3=A[1][0] 4=A[1][3] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm2, [rdx + r10 + 6*1024]	; load A[1][2]
  tileloaddt1 tmm0, [r8  + r10 + 6*1024]	; load B[1][2]
  tileloaddt1 tmm1, [rcx + r10 + 10*1024]	; load C[2][2]
  tdpbf16ps   tmm1, tmm2, tmm0				; C[2][2] += A[1][2] * B[1][2]
 ;tilestored  [rcx + r10 + 10*1024], tmm1 	; store C[2][2]

  ; registers: 0=B[1][2] 1=C[2][2] 2=A[1][2] 3=A[1][0] 4=A[1][3] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm3, [rdx + r10 + 4*1024]	; load A[1][0]
 ;tileloadd   tmm0, [r8  + r10 + 6*1024]	; load B[1][2]
  tileloaddt1 tmm2, [rcx + r10 + 8*1024]	; load C[2][0]
  tdpbf16ps   tmm2, tmm3, tmm0				; C[2][0] += A[1][0] * B[1][2]
 ;tilestored  [rcx + r10 + 8*1024], tmm2 	; store C[2][0]

  ; registers: 0=B[1][2] 1=C[2][2] 2=C[2][0] 3=A[1][0] 4=A[1][3] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm4, [rdx + r10 + 7*1024]	; load A[1][3]
 ;tileloadd   tmm0, [r8  + r10 + 6*1024]	; load B[1][2]
  tileloaddt1 tmm3, [rcx + r10 + 11*1024]	; load C[2][3]
  tdpbf16ps   tmm3, tmm4, tmm0				; C[2][3] += A[1][3] * B[1][2]
 ;tilestored  [rcx + r10 + 11*1024], tmm3 	; store C[2][3]

  ; registers: 0=B[1][2] 1=C[2][2] 2=C[2][0] 3=C[2][3] 4=A[1][3] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm6, [rdx + r10 + 5*1024]	; load A[1][1]
 ;tileloadd   tmm0, [r8  + r10 + 6*1024]	; load B[1][2]
  tileloaddt1 tmm4, [rcx + r10 + 9*1024]	; load C[2][1]
  tdpbf16ps   tmm4, tmm6, tmm0				; C[2][1] += A[1][1] * B[1][2]
 ;tilestored  [rcx + r10 + 9*1024], tmm4 	; store C[2][1]

  ; registers: 0= 1=C[2][2] 2=C[2][0] 3=C[2][3] 4=C[2][1] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm5, [rdx + r10 + 2*1024]	; load A[0][2]
  tileloaddt1 tmm0, [r8  + r10 + 2*1024]	; load B[0][2]
 ;tileloaddt1 tmm1, [rcx + r10 + 10*1024]	; load C[2][2]
  tdpbf16ps   tmm1, tmm5, tmm0				; C[2][2] += A[0][2] * B[0][2]
  tilestored  [rcx + r10 + 10*1024], tmm1 	; store C[2][2]

  ; registers: 0=B[0][2] 1= 2=C[2][0] 3=C[2][3] 4=C[2][1] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
  tileloaddt1 tmm1, [rdx + r10 + 0*1024]	; load A[0][0]
 ;tileloadd   tmm0, [r8  + r10 + 2*1024]	; load B[0][2]
 ;tileloaddt1 tmm2, [rcx + r10 + 8*1024]	; load C[2][0]
  tdpbf16ps   tmm2, tmm1, tmm0				; C[2][0] += A[0][0] * B[0][2]
  tilestored  [rcx + r10 + 8*1024], tmm2 	; store C[2][0]

  ; registers: 0=B[0][2] 1=A[0][0] 2= 3=C[2][3] 4=C[2][1] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm7, [rdx + r10 + 3*1024]	; load A[0][3]
 ;tileloadd   tmm0, [r8  + r10 + 2*1024]	; load B[0][2]
 ;tileloaddt1 tmm3, [rcx + r10 + 11*1024]	; load C[2][3]
  tdpbf16ps   tmm3, tmm7, tmm0				; C[2][3] += A[0][3] * B[0][2]
  tilestored  [rcx + r10 + 11*1024], tmm3 	; store C[2][3]

  ; registers: 0=B[0][2] 1=A[0][0] 2= 3= 4=C[2][1] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
  tileloaddt1 tmm2, [rdx + r10 + 1*1024]	; load A[0][1]
 ;tileloadd   tmm0, [r8  + r10 + 2*1024]	; load B[0][2]
 ;tileloaddt1 tmm4, [rcx + r10 + 9*1024]	; load C[2][1]
  tdpbf16ps   tmm4, tmm2, tmm0				; C[2][1] += A[0][1] * B[0][2]
  tilestored  [rcx + r10 + 9*1024], tmm4 	; store C[2][1]

  ; registers: 0= 1=A[0][0] 2=A[0][1] 3= 4= 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm1, [rdx + r10 + 0*1024]	; load A[0][0]
  tileloaddt1 tmm0, [r8  + r10 + 3*1024]	; load B[0][3]
  tileloaddt1 tmm3, [rcx + r10 + 12*1024]	; load C[3][0]
  tdpbf16ps   tmm3, tmm1, tmm0				; C[3][0] += A[0][0] * B[0][3]
 ;tilestored  [rcx + r10 + 12*1024], tmm3 	; store C[3][0]

  ; registers: 0=B[0][3] 1= 2=A[0][1] 3=C[3][0] 4= 5=A[0][2] 6=A[1][1] 7=A[0][3] 
  tileloaddt1 tmm1, [rdx + r10 + 4*1024]	; load A[1][0]
  tileloaddt1 tmm4, [r8  + r10 + 7*1024]	; load B[1][3]
 ;tileloaddt1 tmm3, [rcx + r10 + 12*1024]	; load C[3][0]
  tdpbf16ps   tmm3, tmm1, tmm4				; C[3][0] += A[1][0] * B[1][3]
  tilestored  [rcx + r10 + 12*1024], tmm3 	; store C[3][0]

  ; registers: 0=B[0][3] 1= 2=A[0][1] 3= 4=B[1][3] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm2, [rdx + r10 + 1*1024]	; load A[0][1]
 ;tileloadd   tmm0, [r8  + r10 + 3*1024]	; load B[0][3]
  tileloaddt1 tmm1, [rcx + r10 + 13*1024]	; load C[3][1]
  tdpbf16ps   tmm1, tmm2, tmm0				; C[3][1] += A[0][1] * B[0][3]
 ;tilestored  [rcx + r10 + 13*1024], tmm1 	; store C[3][1]

  ; registers: 0=B[0][3] 1=C[3][1] 2= 3= 4=B[1][3] 5=A[0][2] 6=A[1][1] 7=A[0][3] 
 ;tileloadd   tmm6, [rdx + r10 + 5*1024]	; load A[1][1]
 ;tileloadd   tmm4, [r8  + r10 + 7*1024]	; load B[1][3]
 ;tileloaddt1 tmm1, [rcx + r10 + 13*1024]	; load C[3][1]
  tdpbf16ps   tmm1, tmm6, tmm4				; C[3][1] += A[1][1] * B[1][3]
  tilestored  [rcx + r10 + 13*1024], tmm1 	; store C[3][1]

  ; registers: 0=B[0][3] 1= 2= 3= 4=B[1][3] 5=A[0][2] 6= 7=A[0][3] 
 ;tileloadd   tmm5, [rdx + r10 + 2*1024]	; load A[0][2]
 ;tileloadd   tmm0, [r8  + r10 + 3*1024]	; load B[0][3]
  tileloaddt1 tmm1, [rcx + r10 + 14*1024]	; load C[3][2]
  tdpbf16ps   tmm1, tmm5, tmm0				; C[3][2] += A[0][2] * B[0][3]
 ;tilestored  [rcx + r10 + 14*1024], tmm1 	; store C[3][2]

  ; registers: 0=B[0][3] 1=C[3][2] 2= 3= 4=B[1][3] 5= 6= 7=A[0][3] 
  tileloaddt1 tmm2, [rdx + r10 + 6*1024]	; load A[1][2]
 ;tileloadd   tmm4, [r8  + r10 + 7*1024]	; load B[1][3]
 ;tileloaddt1 tmm1, [rcx + r10 + 14*1024]	; load C[3][2]
  tdpbf16ps   tmm1, tmm2, tmm4				; C[3][2] += A[1][2] * B[1][3]
  tilestored  [rcx + r10 + 14*1024], tmm1 	; store C[3][2]

  ; registers: 0=B[0][3] 1= 2= 3= 4=B[1][3] 5= 6= 7=A[0][3] 
 ;tileloadd   tmm7, [rdx + r10 + 3*1024]	; load A[0][3]
 ;tileloadd   tmm0, [r8  + r10 + 3*1024]	; load B[0][3]
  tileloaddt1 tmm1, [rcx + r10 + 15*1024]	; load C[3][3]
  tdpbf16ps   tmm1, tmm7, tmm0				; C[3][3] += A[0][3] * B[0][3]
 ;tilestored  [rcx + r10 + 15*1024], tmm1 	; store C[3][3]

  ; registers: 0= 1=C[3][3] 2= 3= 4=B[1][3] 5= 6= 7= 
  tileloaddt1 tmm0, [rdx + r10 + 7*1024]	; load A[1][3]
 ;tileloadd   tmm4, [r8  + r10 + 7*1024]	; load B[1][3]
 ;tileloaddt1 tmm1, [rcx + r10 + 15*1024]	; load C[3][3]
  tdpbf16ps   tmm1, tmm0, tmm4				; C[3][3] += A[1][3] * B[1][3]
  tilestored  [rcx + r10 + 15*1024], tmm1 	; store C[3][3]

  tilerelease
  ret

