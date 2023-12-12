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
; extern "C" void tdpbssd_N64_M64_K64_asm(void *c, void* a, void* b); //RCX, RDX, R8, R9
GLOBAL tdpbssd_N64_M64_K64_asm

SECTION .text
tdpbssd_N64_M64_K64_asm:

 ; C[0][0] += A[0][0] * B[0][0]
 ; C[0][1] += A[0][1] * B[0][0]
 ; C[0][2] += A[0][2] * B[0][0]
 ; C[0][3] += A[0][3] * B[0][0]
 ; C[1][0] += A[0][0] * B[0][1]
 ; C[1][1] += A[0][1] * B[0][1]
 ; C[1][2] += A[0][2] * B[0][1]
 ; C[1][3] += A[0][3] * B[0][1]
 ; C[2][0] += A[0][0] * B[0][2]
 ; C[2][1] += A[0][1] * B[0][2]
 ; C[2][2] += A[0][2] * B[0][2]
 ; C[2][3] += A[0][3] * B[0][2]
 ; C[3][0] += A[0][0] * B[0][3]
 ; C[3][1] += A[0][1] * B[0][3]
 ; C[3][2] += A[0][2] * B[0][3]
 ; C[3][3] += A[0][3] * B[0][3]

  mov         r10, 64     ; set STRIDE to 64
  ldtilecfg   [config]

  tileloadd   tmm0, [rdx + 1*r10 + 0]	; load A[0][0]
  tileloadd   tmm1, [rdx + 1*r10 + 1024]	; load A[0][1]
  tileloadd   tmm2, [rdx + 1*r10 + 2048]	; load A[0][2]
  tileloadd   tmm3, [rdx + 1*r10 + 3072]	; load A[0][3]

  tileloadd   tmm4, [r8 + 1*r10 + 0]	; load B[0][0]

  tileloadd   tmm5, [rcx + 4*r10 + 0]		; load C[0][0]
  tdpbf16ps     tmm5, tmm0, tmm4		    	; C[0][0] += A[0][0] * B[0][0]
  tilestored  [rcx + 4*r10 + 0], tmm5		; store C[0][0]

  tileloadd   tmm5, [rcx + 4*r10 + 4096]		; load C[0][1]
  tdpbf16ps     tmm5, tmm1, tmm4		    	; C[0][1] += A[0][1] * B[0][0]
  tilestored  [rcx + 4*r10 + 4096], tmm5		; store C[0][1]

  tileloadd   tmm5, [rcx + 4*r10 + 8192]		; load C[0][2]
  tdpbf16ps     tmm5, tmm2, tmm4		    	; C[0][2] += A[0][2] * B[0][0]
  tilestored  [rcx + 4*r10 + 8192], tmm5		; store C[0][2]

  tileloadd   tmm5, [rcx + 4*r10 + 12288]		; load C[0][3]
  tdpbf16ps     tmm5, tmm3, tmm4		    	; C[0][3] += A[0][3] * B[0][0]
  tilestored  [rcx + 4*r10 + 12288], tmm5		; store C[0][3]

  tileloadd   tmm4, [r8 + 1*r10 + 1024]	; load B[0][1]

  tileloadd   tmm5, [rcx + 4*r10 + 64]		; load C[1][0]
  tdpbf16ps     tmm5, tmm0, tmm4		    	; C[1][0] += A[0][0] * B[1][0]
  tilestored  [rcx + 4*r10 + 64], tmm5		; store C[1][0]

  tileloadd   tmm5, [rcx + 4*r10 + 4160]		; load C[1][1]
  tdpbf16ps     tmm5, tmm1, tmm4		    	; C[1][1] += A[0][1] * B[1][0]
  tilestored  [rcx + 4*r10 + 4160], tmm5		; store C[1][1]

  tileloadd   tmm5, [rcx + 4*r10 + 8256]		; load C[1][2]
  tdpbf16ps     tmm5, tmm2, tmm4		    	; C[1][2] += A[0][2] * B[1][0]
  tilestored  [rcx + 4*r10 + 8256], tmm5		; store C[1][2]

  tileloadd   tmm5, [rcx + 4*r10 + 12352]		; load C[1][3]
  tdpbf16ps     tmm5, tmm3, tmm4		    	; C[1][3] += A[0][3] * B[1][0]
  tilestored  [rcx + 4*r10 + 12352], tmm5		; store C[1][3]

  tileloadd   tmm4, [r8 + 1*r10 + 2048]	; load B[0][2]

  tileloadd   tmm5, [rcx + 4*r10 + 128]		; load C[2][0]
  tdpbf16ps     tmm5, tmm0, tmm4		    	; C[2][0] += A[0][0] * B[2][0]
  tilestored  [rcx + 4*r10 + 128], tmm5		; store C[2][0]

  tileloadd   tmm5, [rcx + 4*r10 + 4224]		; load C[2][1]
  tdpbf16ps     tmm5, tmm1, tmm4		    	; C[2][1] += A[0][1] * B[2][0]
  tilestored  [rcx + 4*r10 + 4224], tmm5		; store C[2][1]

  tileloadd   tmm5, [rcx + 4*r10 + 8320]		; load C[2][2]
  tdpbf16ps     tmm5, tmm2, tmm4		    	; C[2][2] += A[0][2] * B[2][0]
  tilestored  [rcx + 4*r10 + 8320], tmm5		; store C[2][2]

  tileloadd   tmm5, [rcx + 4*r10 + 12416]		; load C[2][3]
  tdpbf16ps     tmm5, tmm3, tmm4		    	; C[2][3] += A[0][3] * B[2][0]
  tilestored  [rcx + 4*r10 + 12416], tmm5		; store C[2][3]

  tileloadd   tmm4, [r8 + 1*r10 + 3072]	; load B[0][3]

  tileloadd   tmm5, [rcx + 4*r10 + 192]		; load C[3][0]
  tdpbf16ps     tmm5, tmm0, tmm4		    	; C[3][0] += A[0][0] * B[3][0]
  tilestored  [rcx + 4*r10 + 192], tmm5		; store C[3][0]

  tileloadd   tmm5, [rcx + 4*r10 + 4288]		; load C[3][1]
  tdpbf16ps     tmm5, tmm1, tmm4		    	; C[3][1] += A[0][1] * B[3][0]
  tilestored  [rcx + 4*r10 + 4288], tmm5		; store C[3][1]

  tileloadd   tmm5, [rcx + 4*r10 + 8384]		; load C[3][2]
  tdpbf16ps     tmm5, tmm2, tmm4		    	; C[3][2] += A[0][2] * B[3][0]
  tilestored  [rcx + 4*r10 + 8384], tmm5		; store C[3][2]

  tileloadd   tmm5, [rcx + 4*r10 + 12480]		; load C[3][3]
  tdpbf16ps     tmm5, tmm3, tmm4		    	; C[3][3] += A[0][3] * B[3][0]
  tilestored  [rcx + 4*r10 + 12480], tmm5		; store C[3][3]

  tilerelease
  ret

