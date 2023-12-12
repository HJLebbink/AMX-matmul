BITS     64
ALIGN	 8, nop

;----------------------------------------------------------------------------
; extern "C" void tdpbf16ps_N16_M16_K32_no_AMX_old_asm(float *c, BF16* a, BF16* b); //RCX, RDX, R8, R9
GLOBAL tdpbf16ps_N16_M16_K32_no_AMX_old_asm

SECTION .text
tdpbf16ps_N16_M16_K32_no_AMX_old_asm:

	; load the complete B matrix
	vmovups     zmm15, [r8 + (0 * 64)]  
	vmovups     zmm16, [r8 + (1 * 64)]  
	vmovups     zmm17, [r8 + (2 * 64)]    
	vmovups     zmm18, [r8 + (3 * 64)]  
	vmovups     zmm19, [r8 + (4 * 64)]    
	vmovups     zmm20, [r8 + (5 * 64)]  
	vmovups     zmm21, [r8 + (6 * 64)]   
	vmovups     zmm22, [r8 + (7 * 64)]  
	vmovups     zmm23, [r8 + (8 * 64)]  
	vmovups     zmm24, [r8 + (9 * 64)]   
	vmovups     zmm25, [r8 + (10 * 64)]   
	vmovups     zmm26, [r8 + (11 * 64)]  
	vmovups     zmm27, [r8 + (12 * 64)]  
	vmovups     zmm28, [r8 + (13 * 64)]  
	vmovups     zmm29, [r8 + (14 * 64)]  
	vmovups     zmm30, [r8 + (15 * 64)]  

	mov         eax, 16
loop:	
	vmovups     zmm31, [rcx]   ; load one row of C matrix

	vdpbf16ps   zmm31, zmm15, dword [rdx + (0 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm16, dword [rdx + (1 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm17, dword [rdx + (2 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm18, dword [rdx + (3 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm19, dword [rdx + (4 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm20, dword [rdx + (5 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm21, dword [rdx + (6 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm22, dword [rdx + (7 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm23, dword [rdx + (8 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm24, dword [rdx + (9 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm25, dword [rdx + (10 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm26, dword [rdx + (11 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm27, dword [rdx + (12 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm28, dword [rdx + (13 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm29, dword [rdx + (14 * 4)]{1to4}
	vdpbf16ps   zmm31, zmm30, dword [rdx + (15 * 4)]{1to4}
	add         rdx, 64

	vmovups     [rcx], zmm31  ; store one row of C matrix
	add         rcx, 64

	dec         eax
	jnz         loop
	ret
