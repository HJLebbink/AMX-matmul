#pragma once
#include <immintrin.h>
#include <vector>
#include <iostream> //std::cout
#include <random>

#include "amx.vecmul.h"
#include "tools.timing.h"
#include <array>
#include <cstddef>
#include <iomanip>
#include "amx.tile_config.h"
#include "amx.transpose.h"
#include "amx.types.h"
#include "generated/asm/amx.asm.h"

namespace amx {

    inline long long mymin(long long a, long long b) {
        if (a < b) {
            return a;
        }
        return b;
    }

    inline void vecmul_example() {

        constexpr int n_elements = 5 * 512; // needs to be multiple of 32 due to the convert below
        std::vector<float> a = std::vector<float>(n_elements);
        std::vector<float> b = std::vector<float>(n_elements);
        int seed = 0;

        // init content
        {
            if (false) {
                float v = 0.0f;
                for (int i = 0; i < n_elements; ++i) {
                    a[i] = v;
                    b[i] = v;
                    v += 0.01f;
                    // 1 in BF16 = 16256 = 0x3F80
                    // 2 in BF20 = 16384 = 0x4000
                }
            }
            else { // random content
                std::random_device rd;
                if (seed == 0) {
                    seed = rd();
                }
                std::mt19937 gen(seed);
                std::uniform_real_distribution<> dist(0.0, 1.0);

                for (int i = 0; i < n_elements; ++i) {
                    a[i] = static_cast<float>(dist(rd));
                    b[i] = static_cast<float>(dist(rd));
                }
            }
        }

        // compute expected reference value for FP32 and FP64
        float c_exp_fp32 = 0;
        double c_exp_fp64 = 0;

        {
            for (int i = 0; i < n_elements; ++i) {
                c_exp_fp32 += a[i] * b[i];
                c_exp_fp64 += static_cast<double>(a[i]) * static_cast<double>(b[i]);
            }
        }

        std::vector<BF16> a_bf16 = std::vector<BF16>(n_elements, static_cast<BF16>(0));
        std::vector<BF16> b_bf16 = std::vector<BF16>(n_elements, static_cast<BF16>(0));

        std::vector<FP16> a_fp16 = std::vector<FP16>(n_elements, static_cast<FP16>(0));
        std::vector<FP16> b_fp16 = std::vector<FP16>(n_elements, static_cast<FP16>(0));

        // convert FP32 to BF16 and FP16
        {
            const std::byte* ptr_a_src = reinterpret_cast<std::byte*>(a.data());
            const std::byte* ptr_b_src = reinterpret_cast<std::byte*>(b.data());

            std::byte* a_bf16_ptr = reinterpret_cast<std::byte*>(a_bf16.data());
            std::byte* b_bf16_ptr = reinterpret_cast<std::byte*>(b_bf16.data());
            std::byte* a_fp16_ptr = reinterpret_cast<std::byte*>(a_fp16.data());
            std::byte* b_fp16_ptr = reinterpret_cast<std::byte*>(b_fp16.data());

            const int n_blocks = n_elements / 32;

            for (int i = 0; i < n_blocks; i++) {
                const int offset_src = i << 7;
                const int offset_dst = i << 6;

                const __m512 a0 = _mm512_load_ps(ptr_a_src + offset_src);
                const __m512 a1 = _mm512_load_ps(ptr_a_src + offset_src + 64);
                const __m512 b0 = _mm512_load_ps(ptr_b_src + offset_src);
                const __m512 b1 = _mm512_load_ps(ptr_b_src + offset_src + 64);
                { // FP32 to BF16
                    _mm512_store_ph(a_bf16_ptr + offset_dst, _mm512_cvtne2ps_pbh(a1, a0));
                    _mm512_store_ph(b_bf16_ptr + offset_dst, _mm512_cvtne2ps_pbh(b1, b0));
                }
                { // FP32 to FP16
                    _mm256_store_ph(a_fp16_ptr + offset_dst, _mm512_cvtxps_ph(a0));
                    _mm256_store_ph(a_fp16_ptr + offset_dst + 32, _mm512_cvtxps_ph(a1));
                    _mm256_store_ph(b_fp16_ptr + offset_dst, _mm512_cvtxps_ph(b0));
                    _mm256_store_ph(b_fp16_ptr + offset_dst + 32, _mm512_cvtxps_ph(b1));
                }
            }
        }

        // calculate the expected rounding errors
        {
            std::vector<float> a2_bf16 = std::vector<float>(n_elements, 0.f);
            std::vector<float> a2_fp16 = std::vector<float>(n_elements, 0.f);
            std::vector<float> b2_bf16 = std::vector<float>(n_elements, 0.f);
            std::vector<float> b2_fp16 = std::vector<float>(n_elements, 0.f);

            std::byte* a2_bf16_ptr = reinterpret_cast<std::byte*>(a2_bf16.data());
            std::byte* a2_fp16_ptr = reinterpret_cast<std::byte*>(a2_fp16.data());
            std::byte* b2_bf16_ptr = reinterpret_cast<std::byte*>(b2_bf16.data());
            std::byte* b2_fp16_ptr = reinterpret_cast<std::byte*>(b2_fp16.data());

            const std::byte* a_bf16_ptr = reinterpret_cast<std::byte*>(a_bf16.data());
            const std::byte* a_fp16_ptr = reinterpret_cast<std::byte*>(a_fp16.data());
            const std::byte* b_bf16_ptr = reinterpret_cast<std::byte*>(b_bf16.data());
            const std::byte* b_fp16_ptr = reinterpret_cast<std::byte*>(b_fp16.data());

            // PS_PH  Packed Single to Packed Halve
            // PBH    Packed Brain Half float

            //_mm512_cvtne2ps_pbh FP32 -> BF16    vcvtne2ps2bf16 zmm, zmm, zmm    Convert packed single-precision (32-bit) floating-point elements in two vectors a and b to packed BF16 (16-bit) floating-point elements, and store the results in single vector dst.
            //_mm512_cvtxps_ph    FP32 -> FP16    vcvtps2phx ymm, zmm             Convert packed single-precision (32-bit) floating-point elements in a to packed half-precision (16-bit) floating-point elements, and store the results in dst.  
            //                                    vcvtps2phx ymm, zmm{ er }
            //_mm512_cvtps_ph     FP32 -> FP16    vcvtps2ph ymm, zmm {sae}, imm8  Convert packed single-precision (32-bit) floating-point elements in a to packed half-precision (16-bit) floating-point elements, and store the results in dst. Rounding is done according to the rounding[3:0] parameter,
            //_mm512_cvtph_ps     FP16 -> FP32    vcvtph2ps zmm, ymm   Skylake: NO embedded broadcast.          Convert packed half-precision (16-bit) floating-point elements in a to packed single-precision (32-bit) floating-point elements, and store the results in dst.
            //_mm512_cvtxph_ps    FP16 -> FP32    vcvtph2psx zmm, ymm  SapphireRapids: with embedded broadcast. Convert packed half-precision (16-bit) floating-point elements in a to packed single-precision (32-bit) floating-point elements, and store the results in dst.
            //_mm512_cvtpbh_ps    BF16 -> FP32    SEQUENCE             Convert packed BF16 (16-bit) floating-point elements in a to packed single-precision (32-bit) floating-point elements, and store the results in dst. This intrinsic neither raises any floating point exceptions nor turns sNAN into qNAN.

            for (int i = 0; i < n_elements / 16; i++) {
                const int offset_src = i << 5;
                const int offset_dst = i << 6;
                {
                    const __m256h av = _mm256_load_ph(a_bf16_ptr + offset_src);
                    const __m512 av2 = _mm512_cvtpbh_ps(av); // BF16 -> FP32
                    _mm512_store_ps(a2_bf16_ptr + offset_dst, av2);
                }
                {
                    const __m256h av = _mm256_load_ph(a_fp16_ptr + offset_src);
                    const __m512 av2 = _mm512_cvtxph_ps(av); // FP16 -> FP32
                    _mm512_store_ps(a2_fp16_ptr + offset_dst, av2);
                }
                {
                    const __m256h bv = _mm256_load_ph(b_bf16_ptr + offset_src);
                    const __m512 bv2 = _mm512_cvtpbh_ps(bv); // BF16 -> FP32
                    _mm512_store_ps(b2_bf16_ptr + offset_dst, bv2);
                }
                {
                    const __m256h bv = _mm256_load_ph(b_fp16_ptr + offset_src);
                    const __m512 bv2 = _mm512_cvtxph_ps(bv); // FP16 -> FP32
                    _mm512_store_ps(b2_fp16_ptr + offset_dst, bv2);
                }
            }
            if (false) {
                for (int i = 0; i < n_elements; ++i) {
                    std::cout << i << ": " << std::setprecision(15) << a[i] << " " << a2_bf16[i] << " " << a2_fp16[i] << "; " << b[i] << " " << b2_bf16[i] << " " << b2_fp16[i] << std::endl;
                }
            }

            double c_obs_bf16 = 0;
            double c_obs_fp16 = 0;

            for (int i = 0; i < n_elements; ++i) {
                c_obs_bf16 += static_cast<double>(a2_bf16[i]) * static_cast<double>(b2_bf16[i]);
                c_obs_fp16 += static_cast<double>(a2_fp16[i]) * static_cast<double>(b2_fp16[i]);
            }
            std::cout << "vector length " << n_elements << std::endl;
            std::cout << "expected value FP64 " << std::setprecision(15) << c_exp_fp64 << std::endl;
            std::cout << "expected value FP32 " << c_exp_fp32 << std::endl;
            std::cout << "expected accumulated rounding error FP16 " << (c_obs_fp16 - c_exp_fp64) << std::endl;
            std::cout << "expected accumulated rounding error BF16 " << (c_obs_bf16 - c_exp_fp64) << std::endl;
        }

        // use fused multiply add with half precision floating point FP16
        float c_obs_fp16 = 0;
        {
            const std::byte* ptr_a = reinterpret_cast<std::byte*>(a_fp16.data());
            const std::byte* ptr_b = reinterpret_cast<std::byte*>(b_fp16.data());

            __m512h r = _mm512_setzero_ph();
            for (int i = 0; i < n_elements / 32; ++i) {
                const int offset = i << 6;
                const __m512h av = _mm512_load_ph(ptr_a + offset);
                const __m512h bv = _mm512_load_ph(ptr_b + offset);
                r = _mm512_fmadd_ph(av, bv, r);
            }
            __m512 r0 = _mm512_cvtxph_ps(_mm512_extracti32x8_epi32(r, 0));
            __m512 r1 = _mm512_cvtxph_ps(_mm512_extracti32x8_epi32(r, 1));
            __m512 r2 = _mm512_add_ps(r0, r1);
            c_obs_fp16 = _mm512_reduce_add_ps(r2);
        }

        // use fused multiply add with Brain floating point BF16
        float c_obs_bf16 = 0;
        {
            const std::byte* ptr_a = reinterpret_cast<std::byte*>(a_bf16.data());
            const std::byte* ptr_b = reinterpret_cast<std::byte*>(b_bf16.data());

            __m512 r = _mm512_setzero_ps();
            for (int i = 0; i < n_elements / 32; ++i) {
                const int offset = i << 6;
                const __m512bh av = _mm512_load_ph(ptr_a + offset);
                const __m512bh bv = _mm512_load_ph(ptr_b + offset);
                r = _mm512_dpbf16_ps(r, av, bv);
            }
            c_obs_bf16 = _mm512_reduce_add_ps(r);
        }

        const float c_obs_amx32 = vector_mul_method1(a_bf16, b_bf16);

        float c_obs_amx32_asm = 0;
        assembly::vecmul_bf16_asm(&c_obs_amx32_asm, a_bf16.data(), b_bf16.data(), n_elements);

        // use AMX with BF16, never faster than the FMA with FP16, but for completeness.
        //const float c_obs2 = vector_mul_method1(a_bf16, b_bf16);
        float c_obs_amx512 = 0;
        {
            std::array<std::byte, 1024> buf;
            c_obs_amx512 = vector_mul_method2<false, false>(a_bf16, b_bf16, buf);
        }

        // do something with the results
        std::cout << std::endl;
        std::cout << "FP64 ref        " << std::setprecision(14) << c_exp_fp64 << std::endl;
        std::cout << "FP32 ref        " << c_exp_fp32 << std::endl;
        std::cout << "FP16            " << c_obs_fp16 << "; error " << (c_obs_fp16 - c_exp_fp32) << std::endl;
        std::cout << "BF16            " << c_obs_bf16 << "; error " << (c_obs_bf16 - c_exp_fp32) << std::endl;

        std::cout << "BF16 AMX-32     " << c_obs_amx32 << "; error " << (c_obs_amx32 - c_exp_fp32) << std::endl;
        std::cout << "BF16 AMX-32 ASM " << c_obs_amx32_asm << "; error " << (c_obs_amx32_asm - c_exp_fp32) << std::endl;
        std::cout << "BF16 AMX-512    " << c_obs_amx512 << "; error " << (c_obs_amx512 - c_exp_fp32) << std::endl;
        std::cout << std::endl;
    }

    void vecmul_example_speed()
    {
        constexpr int max_n_elements = 10 * 512;
        constexpr int n_experiments = 100000;

        std::cout << "minimal values of " << n_experiments << " runs" << std::endl;
        std::cout << "n_elements\tfp64\tfp32\tfp16\tbf16\tbf16 (amx32)\tbf16 (amx32 asm)\tbf16 (amx512)" << std::endl;

        for (int n_elements = 512; n_elements < max_n_elements; n_elements += 512) {

            const std::vector<float> a = std::vector<float>(n_elements, 0.0f);
            const std::vector<float> b = std::vector<float>(n_elements, 0.0f);

            long long speed_fp64 = 0x0FFF'FFFF'FFFF'FFFF;
            long long speed_fp32 = 0x0FFF'FFFF'FFFF'FFFF;
            long long speed_fp16 = 0x0FFF'FFFF'FFFF'FFFF;
            long long speed_bf16 = 0x0FFF'FFFF'FFFF'FFFF;
            long long speed_bf16_amx_16 = 0x0FFF'FFFF'FFFF'FFFF;
            long long speed_bf16_amx_16_asm = 0x0FFF'FFFF'FFFF'FFFF;
            long long speed_bf16_amx_512 = 0x0FFF'FFFF'FFFF'FFFF;

            // calc FP64
            if (true) {
                const std::vector<double> a_fp64 = std::vector<double>(n_elements, 0.0);
                const std::vector<double> b_fp64 = std::vector<double>(n_elements, 0.0);

                const std::byte* ptr_a = reinterpret_cast<const std::byte*>(a_fp64.data());
                const std::byte* ptr_b = reinterpret_cast<const std::byte*>(b_fp64.data());

                for (int run = 0; run < n_experiments; ++run) {
                    volatile double c_exp_fp64 = 0;
                    ::tools::timing::reset_and_start_timer();

                    __m512d r = _mm512_setzero_pd();
                    for (int i = 0; i < n_elements / 8; ++i) {
                        const int offset = i << 6;
                        const __m512d av = _mm512_load_pd(ptr_a + offset);
                        const __m512d bv = _mm512_load_pd(ptr_b + offset);
                        r = _mm512_fmadd_pd(av, bv, r);
                    }
                    c_exp_fp64 = _mm512_reduce_add_pd(r);
                    speed_fp64 = mymin(speed_fp64, ::tools::timing::get_elapsed_cycles());
                }
            }

            // calc FP32
            if (true) {
                const std::byte* ptr_a = reinterpret_cast<const std::byte*>(a.data());
                const std::byte* ptr_b = reinterpret_cast<const std::byte*>(b.data());

                for (int run = 0; run < n_experiments; ++run) {
                    volatile float c_exp_fp32 = 0;
                    ::tools::timing::reset_and_start_timer();
                    __m512 r = _mm512_setzero_ps();
                    for (int i = 0; i < n_elements / 16; ++i) {
                        const int offset = i << 6;
                        const __m512 av = _mm512_load_ps(ptr_a + offset);
                        const __m512 bv = _mm512_load_ps(ptr_b + offset);
                        r = _mm512_fmadd_ps(av, bv, r);
                    }
                    c_exp_fp32 = _mm512_reduce_add_ps(r);
                    speed_fp32 = mymin(speed_fp32, ::tools::timing::get_elapsed_cycles());
                }
            }

            // use data with only zeros...
            const std::vector<std::byte> a_bf16 = std::vector<std::byte>(n_elements * 2, static_cast<std::byte>(0));
            const std::vector<std::byte> b_bf16 = std::vector<std::byte>(n_elements * 2, static_cast<std::byte>(0));
            const std::vector<std::byte> a_fp16 = std::vector<std::byte>(n_elements * 2, static_cast<std::byte>(0));
            const std::vector<std::byte> b_fp16 = std::vector<std::byte>(n_elements * 2, static_cast<std::byte>(0));

            // calc FP16
            if (true) {
                const std::byte* ptr_a = a_fp16.data();
                const std::byte* ptr_b = b_fp16.data();

                for (int run = 0; run < n_experiments; ++run) {
                    volatile float c_obs1 = 0;
                    ::tools::timing::reset_and_start_timer();

                    __m512h r = _mm512_setzero_ph();
                    for (int i = 0; i < n_elements / 32; ++i) {
                        const int offset = i << 6;
                        const __m512h av = _mm512_loadu_ph(ptr_a + offset);
                        const __m512h bv = _mm512_loadu_ph(ptr_b + offset);
                        r = _mm512_fmadd_ph(av, bv, r); // Sapphire rapids: lat 4; thr 0.5
                    }
                    __m512 r0 = _mm512_cvtxph_ps(_mm512_extracti32x8_epi32(r, 0));
                    __m512 r1 = _mm512_cvtxph_ps(_mm512_extracti32x8_epi32(r, 1));
                    c_obs1 = _mm512_reduce_add_ps(_mm512_add_ps(r0, r1));

                    speed_fp16 = mymin(speed_fp16, ::tools::timing::get_elapsed_cycles());
                }
            }

            // calc BF16
            if (true) {
                const std::byte* ptr_a = a_bf16.data();
                const std::byte* ptr_b = b_bf16.data();

                for (int run = 0; run < n_experiments; ++run) {
                    volatile float c_obs1 = 0;
                    ::tools::timing::reset_and_start_timer();

                    __m512 r = _mm512_setzero_ps();
                    for (int i = 0; i < n_elements / 32; ++i) {
                        const int offset = i << 6;
                        const __m512bh av = _mm512_loadu_ph(ptr_a + offset);
                        const __m512bh bv = _mm512_loadu_ph(ptr_b + offset);
                        r = _mm512_dpbf16_ps(r, av, bv); //Sapphire rapids: lat ?; thr ?
                    }
                    c_obs1 = _mm512_reduce_add_ps(r);

                    speed_bf16 = mymin(speed_bf16, ::tools::timing::get_elapsed_cycles());
                }
            }

            // calc AMX BF16 16 elements
            if (true) {
                const std::byte* ptr_a = a_bf16.data();
                const std::byte* ptr_b = b_bf16.data();

                amx::Tile_config config = { 0 };
                {
                    config.palette_id = 1;
                    config.start_row = 0;

                    config.rows[0] = 1;
                    config.colsb[0] = 4;
                    config.rows[1] = 1;
                    config.colsb[1] = 64;
                    config.rows[2] = 16;
                    config.colsb[2] = 4;
                }
                _tile_loadconfig(&config);

                for (int run = 0; run < n_experiments; ++run) {
                    float c_obs2 = 0;
                    ::tools::timing::reset_and_start_timer();

                    _tile_zero(0);

                    for (int i = 0; i < n_elements / 32; ++i) {
                        const int offset = i << 6;

                        // load 16 BF16 values into the first row (note there is space for 32 values)
                        _tile_loadd(1, ptr_a + offset, 4); // stride does not matter since we only load one row

                        // load 16 BF16 values into the first column
                        _tile_loadd(2, ptr_b + offset, 4);
                        _tile_dpbf16ps(0, 1, 2);
                    }
                    _tile_stored(0, &c_obs2, 4); // stride does not matter, we only save one row
                    speed_bf16_amx_16 = mymin(speed_bf16_amx_16, ::tools::timing::get_elapsed_cycles());
                }

                _tile_release();
            }

            // calc AMX BF16 16 elements asm
            if (true) {
                for (int run = 0; run < n_experiments; ++run) {
                    float c_obs3 = 0;
                    ::tools::timing::reset_and_start_timer();
                    assembly::vecmul_bf16_asm(&c_obs3, a_bf16.data(), b_bf16.data(), n_elements);
                    speed_bf16_amx_16_asm = mymin(speed_bf16_amx_16_asm, ::tools::timing::get_elapsed_cycles());
                }
            }

            // calc AMX BF16 512 elements
            if (true) {
                const std::byte* ptr_a = a_bf16.data();
                const std::byte* ptr_b = b_bf16.data();

                std::array<std::byte, 1024> buf;
                float* ptr_tmp1 = reinterpret_cast<float*>(buf.data());
                BF16* ptr_tmp2 = reinterpret_cast<BF16*>(buf.data());
                
                {
                    amx::Tile_config config = { 0 };
                    config.palette_id = 1;
                    config.start_row = 0;

                    config.rows[0] = 16;
                    config.colsb[0] = 64;
                    config.rows[1] = 16;
                    config.colsb[1] = 64;
                    config.rows[2] = 16;
                    config.colsb[2] = 64;
                    _tile_loadconfig(&config);
                }

                constexpr bool TRANSPOSED = true; // true means that the data is assumed to be transposed

                const __m512i trace_offsets = _mm512_set_epi32(
                    (0 * 16) + 0, (1 * 16) + 1, (2 * 16) + 2, (3 * 16) + 3,
                    (4 * 16) + 4, (5 * 16) + 5, (6 * 16) + 6, (7 * 16) + 7,
                    (8 * 16) + 8, (9 * 16) + 9, (10 * 16) + 10, (11 * 16) + 11,
                    (12 * 16) + 12, (13 * 16) + 13, (14 * 16) + 14, (15 * 16) + 15);

                _tile_zero(0);

                const int n_blocks = n_elements >> 9;

                for (int run = 0; run < n_experiments; ++run) {
                    int offset = 0;
                    int i = n_blocks;
                    volatile float c_obs2 = 0;
                    ::tools::timing::reset_and_start_timer();

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
                            transpose_amx_BF16(reinterpret_cast<const uint16_t*>(ptr_b + offset + (0 * 1024)), ptr_tmp2);
                            _tile_loadd(4, ptr_tmp2, 64);
                            transpose_amx_BF16(reinterpret_cast<const uint16_t*>(ptr_b + offset + (1 * 1024)), ptr_tmp2);
                            _tile_loadd(5, ptr_tmp2, 64);
                            transpose_amx_BF16(reinterpret_cast<const uint16_t*>(ptr_b + offset + (2 * 1024)), ptr_tmp2);
                            _tile_loadd(6, ptr_tmp2, 64);
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
                            transpose_BF16(reinterpret_cast<const BF16*>(ptr_b + offset), ptr_tmp2);
                            _tile_loadd(2, ptr_tmp2, 64);
                        }
                        _tile_dpbf16ps(0, 1, 2);
                        offset += 1024;
                    }
                    _tile_stored(0, ptr_tmp1, 64);
                    
                    {
                        //extract the trace of tile 0
                        c_obs2 = _mm512_reduce_add_ps(_mm512_i32gather_ps(trace_offsets, ptr_tmp1, 4));
                    }
                    speed_bf16_amx_512 = mymin(speed_bf16_amx_512, ::tools::timing::get_elapsed_cycles());
                }
                _tile_release();
            }

            std::cout << 
                n_elements << "\t\t" << 
                speed_fp64 << "\t" << 
                speed_fp32 << "\t" << 
                speed_fp16 << "\t" << 
                speed_bf16 << "\t" <<
                speed_bf16_amx_16 << "\t\t" <<
                speed_bf16_amx_16_asm << "\t\t\t" <<
                speed_bf16_amx_512 << "\t\t" << std::endl;
        }
    }
}