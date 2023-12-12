#pragma once
#include <cstdint>
#include <immintrin.h>

#include "amx.types.h"

namespace amx {

    inline void transpose(const uint32_t* src, uint32_t* dst)
    {
        if (true) {
            const __m512i one = _mm512_set1_epi32(1);
            __m512i offset = _mm512_setr_epi32(
                0 * 16, 1 * 16, 2 * 16, 3 * 16,
                4 * 16, 5 * 16, 6 * 16, 7 * 16,
                8 * 16, 9 * 16, 10 * 16, 11 * 16,
                12 * 16, 13 * 16, 14 * 16, 15 * 16);

            for (int i = 0; i < 16; ++i) {
                _mm512_i32scatter_epi32(dst, offset, _mm512_load_epi32(src), 4);
                src += 16;
                offset = _mm512_add_epi32(offset, one);
            }

            //TODO make a version with an unrolled loop
        }
        else {
            int offset_src = 0;
            for (int j = 0; j < 16; ++j) {
                for (int i = 0; i < 16; ++i) {
                    //std::cout << "dst[" << ((i * 16) + j) << "] = src[" << offset_src << "]" << std::endl;
                    dst[(i * 16) + j] = src[offset_src];
                    offset_src++;
                }
            }
        }
    }

    inline void transpose_Int8(const Int8* src, Int8* dst) {
        const uint32_t* src2 = reinterpret_cast<const uint32_t*>(src);
        uint32_t* dst2 = reinterpret_cast<uint32_t*>(dst);
        transpose(src2, dst2);
    }

    inline void transpose_BF16(const BF16* src, BF16* dst) {
        if (true) {
            const uint32_t* src2 = reinterpret_cast<const uint32_t*>(src);
            uint32_t* dst2 = reinterpret_cast<uint32_t*>(dst);
            transpose(src2, dst2);
        }
        else {
            int offset_src = 0;
            for (int j = 0; j < 32; j += 2) {
                for (int i = 0; i < 16; ++i) {
                    dst[(i * 32) + j + 0] = src[offset_src + 0];
                    dst[(i * 32) + j + 1] = src[offset_src + 1];
                    offset_src += 2;
                }
            }
        }
    }

    inline void transpose_FP32(const FP32* src, FP32* dst) {
        const uint32_t* src2 = reinterpret_cast<const uint32_t*>(src);
        uint32_t* dst2 = reinterpret_cast<uint32_t*>(dst);
        transpose(src2, dst2);
    }
}
