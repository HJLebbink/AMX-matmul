#pragma once
#include <cstdint>
#include <bit>
#include <immintrin.h>
#include <sstream>
#include <string>

#define INOUT


namespace amx {

    using MatrixKey = uint32_t;

    inline constexpr MatrixKey make_matrix_key_A(int col, int row) noexcept {
        return col | (row << 8) | (0b001 << 16);
    }

    inline constexpr MatrixKey make_matrix_key_B(int col, int row) noexcept {
        return col | (row << 8) | (0b010 << 16);
    }

    inline constexpr MatrixKey make_matrix_key_C(int col, int row) noexcept {
        return col | (row << 8) | (0b100 << 16);
    }

    inline constexpr int get_col(MatrixKey key) noexcept {
        return key & 0xFF;
    }
    inline constexpr int get_row(MatrixKey key) noexcept {
        return (key >> 8) & 0xFF;
    }

    inline [[nodiscard]] std::string to_string_matrix_key(MatrixKey key) {
        if (key == 0) {
            return "";
        }
        std::stringstream ss;
        switch (key >> 16) {
        case 0b001: ss << "A"; break;
        case 0b010: ss << "B"; break;
        case 0b100: ss << "C"; break;
        default: ss << "?";
        }
        ss << "[" << get_col(key) << "][" << get_row(key) << "]";
        return ss.str();
    }

    inline [[nodiscard]] constexpr bool has_type_C(MatrixKey key) noexcept {
        return (key >> 16) == 0b100;
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


	using BF16 = uint16_t;
	using FP16 = uint16_t;
    using FP32 = float;
    using Int32 = int;
    using Int8 = int8_t;
    using Uint8 = uint8_t;
    using Int32 = int32_t;

    inline constexpr [[nodiscard]] float bf16_to_float(BF16 v) noexcept {
        return std::bit_cast<float>(static_cast<uint32_t>(v) << 16);
    }

    inline constexpr [[nodiscard]] BF16 float_to_bf16(float v) noexcept {
        return static_cast<uint16_t>(std::bit_cast<uint32_t>(v) >> 16);
    }

    inline [[nodiscard]] float fp16_to_float(FP16 v) noexcept {
        __m128i x = _mm_undefined_si128();
        _mm_store_ph(&v, x);
        return _mm_cvtph_ps(x).m128_f32[0]; //Skylake
    }

    template <typename T>
    [[nodiscard]] constexpr int calc_n_cols() noexcept {
        if constexpr (sizeof(T) == 1) {
            return 64;
        }
        else if constexpr (sizeof(T) == 2) {
            return 32;
        }
        else if constexpr (sizeof(T) == 4) {
            return 16;
        }
        else {
            __debugbreak();
        }
    }

    template <typename T>
    [[nodiscard]] constexpr int calc_n_cols_tile(int n_cols) noexcept {
        if constexpr (sizeof(T) == 1) {
            return (n_cols >> 6) + (((n_cols & 0b0011'1111) == 0) ? 0 : 1);
        } else if constexpr (sizeof(T) == 2) {
            return (n_cols >> 5) + (((n_cols & 0b0001'1111) == 0) ? 0 : 1);
        }
        else {
            return (n_cols >> 4) + (((n_cols & 0b0000'1111) == 0) ? 0 : 1);
        }
    }

    constexpr inline void test_n_cols_tile() {
        {
            static_assert(calc_n_cols_tile<int8_t>(0 * 64) == 0);

            static_assert(calc_n_cols_tile<int8_t>((0 * 64) + 1) == 1);
            static_assert(calc_n_cols_tile<int8_t>((1 * 64) + 0) == 1);

            static_assert(calc_n_cols_tile<int8_t>((1 * 64) + 1) == 2);
            static_assert(calc_n_cols_tile<int8_t>((2 * 64) + 0) == 2);

            static_assert(calc_n_cols_tile<int8_t>((2 * 64) + 1) == 3);
            static_assert(calc_n_cols_tile<int8_t>((3 * 64) + 0) == 3);
        }
        {
            static_assert(calc_n_cols_tile<BF16>(0 * 32) == 0);

            static_assert(calc_n_cols_tile<BF16>((0 * 32) + 1) == 1);
            static_assert(calc_n_cols_tile<BF16>((1 * 32) + 0) == 1);

            static_assert(calc_n_cols_tile<BF16>((1 * 32) + 1) == 2);
            static_assert(calc_n_cols_tile<BF16>((2 * 32) + 0) == 2);

            static_assert(calc_n_cols_tile<BF16>((2 * 32) + 1) == 3);
            static_assert(calc_n_cols_tile<BF16>((3 * 32) + 0) == 3);
        }
        {
            static_assert(calc_n_cols_tile<float>(0 * 16) == 0);

            static_assert(calc_n_cols_tile<float>((0 * 16) + 1) == 1);
            static_assert(calc_n_cols_tile<float>((1 * 16) + 0) == 1);

            static_assert(calc_n_cols_tile<float>((1 * 16) + 1) == 2);
            static_assert(calc_n_cols_tile<float>((2 * 16) + 0) == 2);

            static_assert(calc_n_cols_tile<float>((2 * 16) + 1) == 3);
            static_assert(calc_n_cols_tile<float>((3 * 16) + 0) == 3);
        }
    }

    inline
    [[nodiscard]] constexpr int calc_n_rows_tile(int n_rows) noexcept {
        return (n_rows >> 4) + (((n_rows & 0b0000'1111) == 0) ? 0 : 1);
    }

    constexpr inline void test_n_rows_tile() {
        static_assert(calc_n_rows_tile(0 * 16) == 0);

        static_assert(calc_n_rows_tile((0 * 16) + 1) == 1);
        static_assert(calc_n_rows_tile((1 * 16) + 0) == 1);

        static_assert(calc_n_rows_tile((1 * 16) + 1) == 2);
        static_assert(calc_n_rows_tile((2 * 16) + 0) == 2);

        static_assert(calc_n_rows_tile((2 * 16) + 1) == 3);
        static_assert(calc_n_rows_tile((3 * 16) + 0) == 3);
    }
}