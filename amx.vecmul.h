#pragma once
#include <vector>
#include <immintrin.h>
#include <array>

#include "amx.tile_config.h"
#include "amx.types.h"
#include "amx.transpose.h"


namespace amx {

    // load 16 values into the first row of a tile and compute the product.
	inline float vector_mul_method1(
        const std::vector<BF16>& a,
        const std::vector<BF16>& b)
    {  
        const int n_elements = static_cast<int>(a.size());
        const std::byte* ptr_a = reinterpret_cast<const std::byte*>(a.data());
        const std::byte* ptr_b = reinterpret_cast<const std::byte*>(b.data());

        {
            amx::Tile_config config = { 0 };
            config.palette_id = 1;
            config.start_row = 0;

            config.rows[0] = 1;
            config.colsb[0] = 4;
            config.rows[1] = 1;
            config.colsb[1] = 64;
            config.rows[2] = 16;
            config.colsb[2] = 4;
            _tile_loadconfig(&config);
        }

        _tile_zero(0);

        for (int i = 0; i < n_elements / 32; ++i) {
            const int offset = i << 6;

            // load 16 BF16 values into the first row (note there is space for 32 values)
            _tile_loadd(1, ptr_a + offset, 4); // stride does not matter since we only load one row

            // load 16 BF16 values into the first column
            _tile_loadd(2, ptr_b + offset, 4);
            _tile_dpbf16ps(0, 1, 2);
        }
        float result;
        _tile_stored(0, &result, 4); // stride does not matter, we only save one row
        _tile_release();
        return result;
	}

    // PS_PH  Packed Single to Packed Halve
     // PBH    Packed Brain Half float

     //_mm512_cvtne2ps_pbh FP32 -> BF16    vcvtne2ps2bf16 zmm, zmm, zmm    Convert packed single-precision (32-bit) floating-point elements in two vectors a and b to packed BF16 (16-bit) floating-point elements, and store the results in single vector dst.
     //_mm512_cvtxps_ph    FP32 -> FP16    vcvtps2phx ymm, zmm             Convert packed single-precision (32-bit) floating-point elements in a to packed half-precision (16-bit) floating-point elements, and store the results in dst.  
     //                                    vcvtps2phx ymm, zmm{ er }
     //_mm512_cvtps_ph     FP32 -> FP16    vcvtps2ph ymm, zmm {sae}, imm8  Convert packed single-precision (32-bit) floating-point elements in a to packed half-precision (16-bit) floating-point elements, and store the results in dst. Rounding is done according to the rounding[3:0] parameter,
     //_mm512_cvtph_ps     FP16 -> FP32    vcvtph2ps zmm, ymm   Skylake: NO embedded broadcast.          Convert packed half-precision (16-bit) floating-point elements in a to packed single-precision (32-bit) floating-point elements, and store the results in dst.
     //_mm512_cvtxph_ps    FP16 -> FP32    vcvtph2psx zmm, ymm  SapphireRapids: with embedded broadcast. Convert packed half-precision (16-bit) floating-point elements in a to packed single-precision (32-bit) floating-point elements, and store the results in dst.
     //_mm512_cvtpbh_ps    BF16 -> FP32    SEQUENCE             Convert packed BF16 (16-bit) floating-point elements in a to packed single-precision (32-bit) floating-point elements, and store the results in dst. This intrinsic neither raises any floating point exceptions nor turns sNAN into qNAN.


    // load 256 values into a tile and compute the product.
    template <bool TRANSPOSED, bool CONFIGURED>
    [[nodiscard]] float vector_mul_method2(
        const std::vector<BF16>& a,
        std::vector<BF16>& b,
        std::array<std::byte, 1024>& buf)
    {
        //std::cout << "vector_mul_method2: n_elements " << n_elements << std::endl;
        const int n_elements = static_cast<int>(a.size());
        const std::byte* ptr_a = reinterpret_cast<const std::byte*>(a.data());
        std::byte* ptr_b = reinterpret_cast<std::byte*>(b.data());
        BF16* ptr_tmp2 = reinterpret_cast<BF16*>(buf.data());

        if constexpr (!CONFIGURED) {
            {
                amx::Tile_config config = { 0 };
                config.palette_id = 1;
                config.start_row = 0;
                for (int i = 0; i < 8; ++i) {
                    config.rows[i] = 16;
                    config.colsb[i] = 64;
                }
                _tile_loadconfig(&config);
            }
        }

        _tile_zero(0);

        const int n_blocks = n_elements >> 9;

        int offset = 0;
        int i = n_blocks;

        /*
        for (; i >= 3; i -= 3) {
            _tile_loadd(1, ptr_a + offset + (0 * 1024), 64);
            _tile_loadd(2, ptr_a + offset + (1 * 1024), 64);
            _tile_loadd(3, ptr_a + offset + (2 * 1024), 64);

            if constexpr (TRANSPOSED) {
                _tile_loadd(4, ptr_b + offset + (0 * 1024), 64);
                _tile_loadd(5, ptr_b + offset + (1 * 1024), 64);
                _tile_loadd(6, ptr_b + offset + (2 * 1024), 64);
            }
            else {
                transpose_amx_BF16(reinterpret_cast<const uint16_t*>(ptr_b + offset + (0 * 1024)), ptr_b_t);
                _tile_loadd(4, ptr_b_t, 64);
                transpose_amx_BF16(reinterpret_cast<const uint16_t*>(ptr_b + offset + (1 * 1024)), ptr_b_t);
                _tile_loadd(5, ptr_b_t, 64);
                transpose_amx_BF16(reinterpret_cast<const uint16_t*>(ptr_b + offset + (2 * 1024)), ptr_b_t);
                _tile_loadd(6, ptr_b_t, 64);
            }
            _tile_dpbf16ps(0, 1, 4);
            _tile_dpbf16ps(0, 2, 5);
            _tile_dpbf16ps(0, 3, 6);
            offset += (3 * 1024);
        }
       */
        for (; i > 0; --i)
        {
            _tile_loadd(1, ptr_a + offset, 64);
            if constexpr (TRANSPOSED) {
                _tile_loadd(2, ptr_b + offset, 64);
            }
            else {
                transpose_BF16(reinterpret_cast<BF16*>(ptr_b + offset), ptr_tmp2);
                _tile_loadd(2, ptr_tmp2, 64);
            }
            _tile_dpbf16ps(0, 1, 2);
            offset += 1024;
        }

        float* ptr = reinterpret_cast<float*>(buf.data());
        _tile_stored(0, ptr, 64);

        if constexpr (!CONFIGURED) {
            _tile_release();
        }

        //extract the trace of tile 0
        const __m512i trace_offsets = _mm512_set_epi32(
            (0 * 16) + 0, (1 * 16) + 1, (2 * 16) + 2, (3 * 16) + 3,
            (4 * 16) + 4, (5 * 16) + 5, (6 * 16) + 6, (7 * 16) + 7,
            (8 * 16) + 8, (9 * 16) + 9, (10 * 16) + 10, (11 * 16) + 11,
            (12 * 16) + 12, (13 * 16) + 13, (14 * 16) + 14, (15 * 16) + 15);

        return _mm512_reduce_add_ps(_mm512_i32gather_ps(trace_offsets, ptr, 4));
    }
}