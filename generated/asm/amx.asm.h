#pragma once
#include <cstdint>

#include "../../amx.types.h"

namespace amx::assembly {

	extern "C" void vecmul_bf16_asm(void* c, const void* a, const void* b, int n_elements);


	extern "C" void tdpbssd_N16_M16_K64_asm(void* c, const void* a, const void* b);
	extern "C" void tdpbssd_N16_M16_K128_asm(void* c, const void* a, const void* b);
	extern "C" void tdpbssd_N32_M32_K64_asm(void* c, const void* a, const void* b);
	extern "C" void tdpbssd_N32_M32_K128_asm(void* c, const void* a, const void* b);
	extern "C" void tdpbssd_N64_M64_K64_asm(void* c, const void* a, const void* b);

	extern "C" void tdpbf16ps_N16_M16_K32_no_AMX_asm(float* c, const amx::BF16 * a, const amx::BF16 * b);
	extern "C" void tdpbf16ps_N16_M16_K32_no_AMX_old_asm(float* c, const amx::BF16 * a, const amx::BF16 * b);

	extern "C" void tdpbf16ps_N16_M16_K32_asm(float* c, const amx::BF16 * a, const amx::BF16 * b);
	extern "C" void tdpbf16ps_N16_M16_K32_no_config_asm(float* c, const amx::BF16 * a, const amx::BF16 * b);
	extern "C" void tdpbf16ps_N32_M32_K64_asm(float* c, const amx::BF16 * a, const amx::BF16 * b);
	extern "C" void tdpbf16ps_N64_M64_K64_asm(float* c, const amx::BF16 * a, const amx::BF16 * b);
	extern "C" void tdpbf16ps_N128_M128_K128_asm(float* c, const amx::BF16 * a, const amx::BF16 * b);
	extern "C" void tdpbf16ps_N256_M256_K256_asm(float* c, const amx::BF16 * a, const amx::BF16 * b);
	extern "C" void tdpbf16ps_N512_M512_K512_asm(float* c, const amx::BF16 * a, const amx::BF16 * b);
}
