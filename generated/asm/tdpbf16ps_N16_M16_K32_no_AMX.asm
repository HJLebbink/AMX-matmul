BITS     64
ALIGN	 8, nop

;----------------------------------------------------------------------------
; extern "C" void tdpbf16ps_N16_M16_K32_no_AMX_asm(float *c, BF16* a, BF16* b); //RCX, RDX, R8, R9
GLOBAL tdpbf16ps_N16_M16_K32_no_AMX_asm

SECTION .text
tdpbf16ps_N16_M16_K32_no_AMX_asm:

	; load the complete B matrix
	vmovups     zmm12, [r8 + (0 * 64)]  
	vmovups     zmm13, [r8 + (1 * 64)]  
	vmovups     zmm14, [r8 + (2 * 64)]    
	vmovups     zmm15, [r8 + (3 * 64)]  
	vmovups     zmm16, [r8 + (4 * 64)]    
	vmovups     zmm17, [r8 + (5 * 64)]  
	vmovups     zmm18, [r8 + (6 * 64)]   
	vmovups     zmm19, [r8 + (7 * 64)]  
	vmovups     zmm20, [r8 + (8 * 64)]  
	vmovups     zmm21, [r8 + (9 * 64)]   
	vmovups     zmm22, [r8 + (10 * 64)]   
	vmovups     zmm23, [r8 + (11 * 64)]  
	vmovups     zmm24, [r8 + (12 * 64)]  
	vmovups     zmm25, [r8 + (13 * 64)]  
	vmovups     zmm26, [r8 + (14 * 64)]  
	vmovups     zmm27, [r8 + (15 * 64)]  

	mov         eax, 4
loop:	
	vmovups     zmm28, [rcx + (0 * 64)]   ; load one row of C matrix
	vmovups     zmm29, [rcx + (1 * 64)]   ; load one row of C matrix
	vmovups     zmm30, [rcx + (2 * 64)]   ; load one row of C matrix
	vmovups     zmm31, [rcx + (3 * 64)]   ; load one row of C matrix

	vdpbf16ps   zmm28, zmm12, dword [rdx + (0 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm12, dword [rdx + (0 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm12, dword [rdx + (0 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm12, dword [rdx + (0 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm13, dword [rdx + (1 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm13, dword [rdx + (1 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm13, dword [rdx + (1 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm13, dword [rdx + (1 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm14, dword [rdx + (2 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm14, dword [rdx + (2 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm14, dword [rdx + (2 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm14, dword [rdx + (2 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm15, dword [rdx + (3 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm15, dword [rdx + (3 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm15, dword [rdx + (3 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm15, dword [rdx + (3 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm16, dword [rdx + (4 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm16, dword [rdx + (4 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm16, dword [rdx + (4 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm16, dword [rdx + (4 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm17, dword [rdx + (5 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm17, dword [rdx + (5 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm17, dword [rdx + (5 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm17, dword [rdx + (5 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm18, dword [rdx + (6 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm18, dword [rdx + (6 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm18, dword [rdx + (6 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm18, dword [rdx + (6 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm19, dword [rdx + (7 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm19, dword [rdx + (7 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm19, dword [rdx + (7 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm19, dword [rdx + (7 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm20, dword [rdx + (8 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm20, dword [rdx + (8 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm20, dword [rdx + (8 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm20, dword [rdx + (8 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm21, dword [rdx + (9 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm21, dword [rdx + (9 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm21, dword [rdx + (9 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm21, dword [rdx + (9 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm22, dword [rdx + (10 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm22, dword [rdx + (10 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm22, dword [rdx + (10 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm22, dword [rdx + (10 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm23, dword [rdx + (11 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm23, dword [rdx + (11 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm23, dword [rdx + (11 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm23, dword [rdx + (11 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm24, dword [rdx + (12 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm24, dword [rdx + (12 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm24, dword [rdx + (12 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm24, dword [rdx + (12 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm25, dword [rdx + (13 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm25, dword [rdx + (13 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm25, dword [rdx + (13 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm25, dword [rdx + (13 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm26, dword [rdx + (14 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm26, dword [rdx + (14 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm26, dword [rdx + (14 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm26, dword [rdx + (14 * 4) + (3 * 64)]{1to4}

	vdpbf16ps   zmm28, zmm27, dword [rdx + (15 * 4) + (0 * 64)]{1to4}
	vdpbf16ps   zmm29, zmm27, dword [rdx + (15 * 4) + (1 * 64)]{1to4}
	vdpbf16ps   zmm30, zmm27, dword [rdx + (15 * 4) + (2 * 64)]{1to4}
	vdpbf16ps   zmm31, zmm27, dword [rdx + (15 * 4) + (3 * 64)]{1to4}

	add         rdx, 4*64

	vmovups     [rcx + (0 * 64)], zmm28  ; store one row of C matrix
	vmovups     [rcx + (1 * 64)], zmm29  ; store one row of C matrix
	vmovups     [rcx + (2 * 64)], zmm30  ; store one row of C matrix
	vmovups     [rcx + (3 * 64)], zmm31  ; store one row of C matrix

	add         rcx, 4*64

	dec         eax
	jnz         loop
	ret
