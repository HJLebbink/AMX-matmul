#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <immintrin.h>
#include <iosfwd>
#include <iostream>
#include <ostream>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "amx.print.h"
#include "amx.tile_config.h"
#include "amx.transpose.h"
#include "amx.types.h"
#include "tools.timing.h"
#include <limits>


 // PS_PH  Packed Single to Packed Halve
 // PBH    Packed Brain Half float


 //_mm512_cvtneps_pbh  FP32 -> BF16    vcvtneps2bf16 ymm, zmm          Convert packed single-precision (32-bit) floating-point elements in a to packed BF16 (16-bit) floating-point elements, and store the results in dst.
 //_mm512_cvtne2ps_pbh FP32 -> BF16    vcvtne2ps2bf16 zmm, zmm, zmm    Convert packed single-precision (32-bit) floating-point elements in two vectors a and b to packed BF16 (16-bit) floating-point elements, and store the results in single vector dst.
 
 //_mm512_cvtxps_ph    FP32 -> FP16    vcvtps2phx ymm, zmm             Convert packed single-precision (32-bit) floating-point elements in a to packed half-precision (16-bit) floating-point elements, and store the results in dst.  
 //                                    vcvtps2phx ymm, zmm{ er }
 //_mm512_cvtps_ph     FP32 -> FP16    vcvtps2ph ymm, zmm {sae}, imm8  Convert packed single-precision (32-bit) floating-point elements in a to packed half-precision (16-bit) floating-point elements, and store the results in dst. Rounding is done according to the rounding[3:0] parameter,
 //_mm512_cvtps_pd     FP32 -> FP64    vcvtps2pd zmm, ymm              Convert packed single-precision (32-bit) floating-point elements in a to packed double-precision (64-bit) floating-point elements, and store the results in dst.

 //_mm512_cvtph_ps     FP16 -> FP32    vcvtph2ps zmm, ymm   Skylake: NO embedded broadcast.          Convert packed half-precision (16-bit) floating-point elements in a to packed single-precision (32-bit) floating-point elements, and store the results in dst.
 //_mm512_cvtxph_ps    FP16 -> FP32    vcvtph2psx zmm, ymm  SapphireRapids: with embedded broadcast. Convert packed half-precision (16-bit) floating-point elements in a to packed single-precision (32-bit) floating-point elements, and store the results in dst.
 //_mm512_cvtph_pd     FP16 -> FP64    vcvtph2pd zmm, xmm   Convert packed half-precision (16-bit) floating-point elements in a to packed double-precision (64-bit) floating-point elements, and store the results in dst.

 //_mm512_cvtpd_ph     FP64 -> FP16    vcvtpd2ph xmm, zmm   Convert packed double-precision (64-bit) floating-point elements in a to packed half-precision (16-bit) floating-point elements, and store the results in dst.
 //_mm512_cvtpd_pslo   FP64 -> FP32    vcvtpd2ps zmm, zmm   

 //_mm512_cvtpbh_ps    BF16 -> FP32    SEQUENCE             Convert packed BF16 (16-bit) floating-point elements in a to packed single-precision (32-bit) floating-point elements, and store the results in dst. This intrinsic neither raises any floating point exceptions nor turns sNAN into qNAN.


namespace amx::example {

	using BF16 = uint16_t;
	constexpr inline int MyVectorLength = 16 * 32;


	template <int LENGTH>
	void fill_random(
		INOUT std::vector<std::array<BF16, LENGTH>>& data, 
		const float min_value = 0.f, 
		const float max_value = 1.f, 
		int seed = 0
	) {
		std::random_device rd;
		std::mt19937 gen((seed == 0) ? rd() : seed);
		std::uniform_real_distribution<> dist(min_value, max_value);

		for (int pos = 0; pos < static_cast<int>(data.size()); ++pos) {
			BF16* ptr = data[pos].data();
			for (int j = 0; j < MyVectorLength; ++j) {
				ptr[j] = amx::float_to_bf16(static_cast<float>(dist(rd)));
			}
		}
	}

	template <int LENGTH>
	std::string pretty_print(
		const std::array<BF16, LENGTH>& vector, 
		bool colour = false, 
		amx::tools::PrintType pt = amx::tools::PrintType::bf16
	) {
		std::stringstream ss;
		int counter = 0;
		ss << ((colour) ? "\u001b[0m" : ""); // reset colour
		for (int row = 0; row < 1; ++row) {
			for (int col = 0; col < LENGTH; ++col) {
				ss << amx::tools::pretty_print_value(vector[col], col, colour, pt);
			}
			ss << std::endl;
		}
		return ss.str();
	}

	template <int LENGTH>
	std::array<double, LENGTH> vector_cast_FP64(const std::array<BF16, LENGTH>& vector) {
		std::array<double, LENGTH> result;
		for (int i = 0; i < LENGTH; ++i) {
			result[i] = static_cast<double>(bf16_to_float(vector[i]));
		}
		return result;
	}

	template <int LENGTH>
	std::vector<std::array<double, LENGTH>> data_cast_FP64(const std::vector<std::array<BF16, LENGTH>>& data) {
		std::vector<std::array<double, LENGTH>> result;
		for (int i = 0; i < static_cast<int>(data.size()); ++i) {
			result.push_back(vector_cast_FP64(data[i]));
		}
		return result;
	}

	template <int LENGTH>
	float vec_mul_ref(const std::array<BF16, LENGTH>& a, const std::array<BF16, LENGTH>& b) {
		float c = 0;
		for (int i = 0; i < MyVectorLength; ++i) {
			c += amx::bf16_to_float(a[i]) * amx::bf16_to_float(b[i]);
		}
		return c;
	}

	template <int LENGTH>
	std::tuple<int, float> find_minimum_ref(
		const std::array<BF16, LENGTH>& needle, 
		const std::vector<std::array<BF16, LENGTH>>& data
	) {
		float minimum = std::numeric_limits<float>::max();
		int least_idx = -1;

		for (int i = 0; i < static_cast<int>(data.size()); ++i) {
			const float y = vec_mul_ref(needle, data[i]);

			if (y < minimum) {
				minimum = y;
				least_idx = i;
			}
		}
		return std::make_tuple(least_idx, minimum);
	}

	template <int LENGTH>
	std::tuple<int, float> find_minimum_load_BF16_fma_BF16(
		const std::array<BF16, LENGTH>& needle,
		const std::vector<std::array<BF16, LENGTH>>& data
	) {
		float minimum = std::numeric_limits<float>::max();
		int least_idx = -1;
		const std::byte* ptr_a = reinterpret_cast<const std::byte*>(needle.data());

		for (int i = 0; i < static_cast<int>(data.size()); ++i) {
			const std::byte* ptr_b = reinterpret_cast<const std::byte*>(data[i].data());

			__m512 r = _mm512_setzero_ps();
			for (int j = 0; j < LENGTH / 32; ++j) {
				const int offset = j << 6;
				const __m512bh av = _mm512_load_ph(ptr_a + offset);
				const __m512bh bv = _mm512_load_ph(ptr_b + offset);
				r = _mm512_dpbf16_ps(r, av, bv);
			}
			const float y = _mm512_reduce_add_ps(r);

			if (y < minimum) {
				minimum = y;
				least_idx = i;
			}
		}
		return std::make_tuple(least_idx, minimum);
	}

	template <int LENGTH>
	std::tuple<int, float> find_minimum_load_FP64_fma_BF16(
		const std::array<double, LENGTH>& needle,
		const std::vector<std::array<double, LENGTH>>& data
	) {
		float minimum = std::numeric_limits<float>::max();
		int least_idx = -1;
		const std::byte* ptr_a = reinterpret_cast<const std::byte*>(needle.data());

		for (int i = 0; i < static_cast<int>(data.size()); ++i) {
			const std::byte* ptr_b = reinterpret_cast<const std::byte*>(data[i].data());
			float y = 0;

			if (false) {
				__m512 r = _mm512_setzero_ps();
				for (int j = 0; j < LENGTH / 8; ++j) {
					const int offset = j << 6;

					__m512d a1a = _mm512_loadu_pd(ptr_a + offset + (0 * 64));
					__m512d a1b = _mm512_loadu_pd(ptr_a + offset + (1 * 64));
					__m512d a1c = _mm512_loadu_pd(ptr_a + offset + (2 * 64));
					__m512d a1d = _mm512_loadu_pd(ptr_a + offset + (3 * 64));

					__m256 a2a = _mm512_cvtpd_ps(a1a);
					__m256 a2b = _mm512_cvtpd_ps(a1b);
					__m256 a2c = _mm512_cvtpd_ps(a1c);
					__m256 a2d = _mm512_cvtpd_ps(a1d);

					__m512 a3a = _mm512_insertf32x8(_mm512_castps256_ps512(a2a), a2b, 1);
					__m512 a3b = _mm512_insertf32x8(_mm512_castps256_ps512(a2c), a2d, 1);

					__m512d b1a = _mm512_loadu_pd(ptr_b + offset + (0 * 64));
					__m512d b1b = _mm512_loadu_pd(ptr_b + offset + (1 * 64));
					__m512d b1c = _mm512_loadu_pd(ptr_b + offset + (2 * 64));
					__m512d b1d = _mm512_loadu_pd(ptr_b + offset + (3 * 64));

					__m256 b2a = _mm512_cvtpd_ps(b1a);
					__m256 b2b = _mm512_cvtpd_ps(b1b);
					__m256 b2c = _mm512_cvtpd_ps(b1c);
					__m256 b2d = _mm512_cvtpd_ps(b1d);

					__m512 b3a = _mm512_insertf32x8(_mm512_castps256_ps512(b2a), b2b, 1);
					__m512 b3b = _mm512_insertf32x8(_mm512_castps256_ps512(b2c), b2d, 1);

					const __m512bh av = _mm512_cvtne2ps_pbh(a3a, a3b);
					const __m512bh bv = _mm512_cvtne2ps_pbh(b3a, b3b);
					r = _mm512_dpbf16_ps(r, av, bv);
				}
				y = _mm512_reduce_add_ps(r);
			}
			else {
				__m128 r = _mm_setzero_ps();
				for (int j = 0; j < LENGTH / 32; ++j) {
					const int offset = j << 6;

					__m512d a1a = _mm512_loadu_pd(ptr_a + offset);
					__m256 a2a = _mm512_cvtpd_ps(a1a);

					__m512d b1a = _mm512_loadu_pd(ptr_b + offset);
					__m256 b2a = _mm512_cvtpd_ps(b1a);

					const __m128bh av = _mm256_cvtneps_pbh(a2a);
					const __m128bh bv = _mm256_cvtneps_pbh(b2a);
					r = _mm_dpbf16_ps(r, av, bv);
				}
				y = _mm512_reduce_add_ps(_mm512_castps128_ps512(r));
			}

			if (y < minimum) {
				minimum = y;
				least_idx = i;
			}
		}
		return std::make_tuple(least_idx, minimum);
	}

	template <int LENGTH>
	std::tuple<int, float> find_minimum_load_BF16_fma_FP64(
		const std::array<BF16, LENGTH>& needle,
		const std::vector<std::array<BF16, LENGTH>>& data
	) {
		float minimum = std::numeric_limits<float>::max();
		int least_idx = -1;
		const std::byte* ptr_a = reinterpret_cast<const std::byte*>(needle.data());

		for (int i = 0; i < static_cast<int>(data.size()); ++i) {
			const std::byte* ptr_b = reinterpret_cast<const std::byte*>(data[i].data());

			__m512d r = _mm512_setzero_pd();
			for (int j = 0; j < LENGTH / 8; ++j) {
				const int offset = j << 4;
				const __m512d av = _mm512_cvtps_pd(_mm256_cvtpbh_ps(_mm_loadu_ph(ptr_a + offset)));
				const __m512d bv = _mm512_cvtps_pd(_mm256_cvtpbh_ps(_mm_loadu_ph(ptr_b + offset)));
				r = _mm512_fmadd_pd(av, bv, r);
			}
			const float y = static_cast<float>(_mm512_reduce_add_pd(r));

			if (y < minimum) {
				minimum = y;
				least_idx = i;
			}
		}
		return std::make_tuple(least_idx, minimum);
	}

	template <int LENGTH>
	std::tuple<int, float> find_minimum_load_FP64_fma_FP64(
		const std::array<double, LENGTH>& needle,
		const std::vector<std::array<double, LENGTH>>& data
	) {
		float minimum = std::numeric_limits<float>::max();
		int least_idx = -1;
		const std::byte* ptr_a = reinterpret_cast<const std::byte*>(needle.data());

		for (int i = 0; i < static_cast<int>(data.size()); ++i) {
			const std::byte* ptr_b = reinterpret_cast<const std::byte*>(data[i].data());

			__m512d r = _mm512_setzero_pd();
			for (int j = 0; j < LENGTH / 8; ++j) {
				const int offset = j << 6;
				const __m512d av = _mm512_load_pd(ptr_a + offset);
				const __m512d bv = _mm512_load_pd(ptr_b + offset);
				r = _mm512_fmadd_pd(av, bv, r);
			}
			const float y = static_cast<float>(_mm512_reduce_add_pd(r));

			if (y < minimum) {
				minimum = y;
				least_idx = i;
			}
		}
		return std::make_tuple(least_idx, minimum);
	}

	inline std::tuple<int, float> find_minimum_load_BF16_amx_BF16(
		const std::array<BF16, 512>& needle, 
		const std::vector<std::array<BF16, 512>>& data
	) {
		float minimum = std::numeric_limits<float>::max();
		int least_idx = -1;

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

		const __m512i trace_offsets = _mm512_set_epi32(
			(0 * 16) + 0, (1 * 16) + 1, (2 * 16) + 2, (3 * 16) + 3,
			(4 * 16) + 4, (5 * 16) + 5, (6 * 16) + 6, (7 * 16) + 7,
			(8 * 16) + 8, (9 * 16) + 9, (10 * 16) + 10, (11 * 16) + 11,
			(12 * 16) + 12, (13 * 16) + 13, (14 * 16) + 14, (15 * 16) + 15);

		std::array<BF16, 512> buf;
		BF16* buf_ptr = buf.data();

		transpose_BF16(needle.data(), buf_ptr);
		_tile_loadd(1, buf_ptr, 64);

		for (int i = 0; i < static_cast<int>(data.size()); ++i) {
			_tile_loadd(2, data[i].data(), 64);
			_tile_zero(0);
			_tile_dpbf16ps(0, 1, 2);
			_tile_stored(0, buf_ptr, 64);

			const float y = _mm512_reduce_add_ps(_mm512_i32gather_ps(trace_offsets, buf_ptr, 4));

			if (y < minimum) {
				minimum = y;
				least_idx = i;
			}
		}
		_tile_release();
		return std::make_tuple(least_idx, minimum);
	}

	inline std::tuple<int, float> find_minimum_load_BF16_amx_BF16_x16(
		const std::array<std::array<BF16, 512>, 16>& needle16,
		const std::vector<std::array<BF16, 512>>& data
	) {
		float minimum = std::numeric_limits<float>::max();
		int least_idx = -1;

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

		const __m512i trace_offsets = _mm512_set_epi32(
			(0 * 16) + 0, (1 * 16) + 1, (2 * 16) + 2, (3 * 16) + 3,
			(4 * 16) + 4, (5 * 16) + 5, (6 * 16) + 6, (7 * 16) + 7,
			(8 * 16) + 8, (9 * 16) + 9, (10 * 16) + 10, (11 * 16) + 11,
			(12 * 16) + 12, (13 * 16) + 13, (14 * 16) + 14, (15 * 16) + 15);

		// to get started, copy A and B to continues tmp buffer
		std::array<BF16, 512*16> buf_data, buf_needle;
		BF16* buf_data_ptr = buf_data.data();
		BF16* buf_needle_ptr = buf_needle.data();


		if (true) {
			std::cout << "data before: " << std::endl;
			for (int j = 0; j < 16; ++j) {
				for (int i = 0; i < 512; ++i) {
					std::cout << bf16_to_float(needle16[j][i]) << " ";
				}
				std::cout << std::endl;
			}
			std::cout << std::endl;

		}
		if (false) {
			std::cout << "needle before: " << std::endl;
			for (int j = 0; j < 16; ++j) {
				for (int i = 0; i < 512; ++i) {
					std::cout << bf16_to_float(data[j][i]) << " ";
				}
				std::cout << std::endl;
			}
			std::cout << std::endl;
		}

		if (true) { // copy data
			int offset = 0;
			for (int j = 0; j < 16; ++j) {
				std::memcpy(buf_data_ptr + offset, data[j].data(), 1024);
				offset += 512;
			}
		}
		if (true) { // copy needle
			int offset = 0;
			for (int j = 0; j < 16; ++j) {
				transpose_BF16(needle16[j].data(), buf_needle_ptr + offset);
				offset += 512;
			}
		}

		if (true) {
			std::cout << "data: " << std::endl;
			int idx = 0;
			for (int j = 0; j < 16; ++j) {
				for (int i = 0; i < 512; ++i) {
					std::cout << bf16_to_float(buf_data[idx]) << " ";
					idx++;
				}
				std::cout << std::endl;
			}
			std::cout << std::endl;

		}
		if (false) {
			std::cout << "needle: " << std::endl;
			int idx = 0;
			for (int j = 0; j < 16; ++j) {
				for (int i = 0; i < 512; ++i) {
					std::cout << bf16_to_float(buf_needle[idx]) << " ";
					idx++;
				}
				std::cout << std::endl;
			}
			std::cout << std::endl;
		}

		//_tile_loadd(1, buf_needle_ptr, 1024);
		if (false) { // print content of tile 1
			std::array<BF16, 512> tmp;
			_tile_stored(1, tmp.data(), 64);
			int idx = 0;
			for (int row = 0; row < 16; ++row) {
				for (int col = 0; col < 32; ++col) {
					std::cout << bf16_to_float(tmp[idx]) << " ";
					idx++;
				}
				std::cout << std::endl;
			}
		}

		const size_t n_elements = data.size();

		for (int i = 0; i < n_elements; ++i) {

			_tile_zero(0);
			int offset = 0;
			for (int j = 0; j < 16; ++j) {

				_tile_loadd(1, buf_needle_ptr + offset, 1024);

				//_tile_loadd(2, ptr_a, 64);

				_tile_dpbf16ps(0, 1, 2);
				offset += 64;
			}

			_tile_stored(0, buf_data_ptr, 64);

			const float y = _mm512_reduce_add_ps(_mm512_i32gather_ps(trace_offsets, buf_data_ptr, 4));

			if (y < minimum) {
				minimum = y;
				least_idx = i;
			}
		}
		_tile_release();
		return std::make_tuple(least_idx, minimum);
	}

	inline void vector_mul() {
		constexpr int n_runs = 20;
		constexpr int vector_length = 512;
		constexpr int n_vectors_in_data = 10000;

		std::vector<std::array<BF16, vector_length>> data;
		std::array<BF16, vector_length> needle;
		std::array<std::array<BF16, vector_length>, 16> needle16;

		// initialize data
		{
			if (true) {
				data = std::vector<std::array<BF16, vector_length>>(16);

				for (int j = 0; j < 16; ++j) {
					const BF16 v = amx::float_to_bf16(float(j));
					for (int i = 0; i < vector_length; ++i) {
						data[j][i] = v;
						needle16[j][i] = v;
					}
				}
				needle = needle16[0];
			}
			else {
				data = std::vector<std::array<BF16, vector_length>>(n_vectors_in_data);
				fill_random(data);
				std::vector<std::array<BF16, vector_length>> tmp(16);
				fill_random(tmp);
				needle = tmp[0];
				for (int i = 0; i < 16; ++i) {
					needle16[i] = tmp[i];
				}
			}
			//std::cout << "data: " << pretty_print(data[0], true);
			//std::cout << "needle: " << pretty_print(needle, true);
		}

		uint64_t fastest_load_BF16_ref = std::numeric_limits<uint64_t>::max();
		uint64_t fastest_load_BF16_fma_BF16 = std::numeric_limits<uint64_t>::max();
		uint64_t fastest_load_FP64_fma_BF16 = std::numeric_limits<uint64_t>::max();
		uint64_t fastest_load_FP64_fma_FP64 = std::numeric_limits<uint64_t>::max();
		uint64_t fastest_load_BF16_fma_FP64 = std::numeric_limits<uint64_t>::max();
		uint64_t fastest_load_BF16_amx_BF16 = std::numeric_limits<uint64_t>::max();
		uint64_t fastest_load_BF16_amx2_BF16 = std::numeric_limits<uint64_t>::max();

		int result_load_BF16_ref = 0;
		int result_load_FP64_fma_FP64 = 0;
		int result_load_BF16_fma_FP64 = 0;
		int result_load_FP64_fma_BF16 = 0;
		int result_load_BF16_fma_BF16 = 0;
		int result_load_BF16_amx_BF16 = 0;
		int result_load_BF16_amx2_BF16 = 0;

		float minimum_load_BF16_ref = 0;
		float minimum_load_FP64_fma_FP64 = 0;
		float minimum_load_BF16_fma_FP64 = 0;
		float minimum_load_FP64_fma_BF16 = 0;
		float minimum_load_BF16_fma_BF16 = 0;
		float minimum_load_BF16_amx_BF16 = 0;
		float minimum_load_BF16_amx2_BF16 = 0;

		const auto needle_FP64 = vector_cast_FP64(needle);
		const auto data_FP64 = data_cast_FP64(data);


		// load BF16, reference CPP (FP32)
		for (int run = 0; run < n_runs; ++run) {
			::tools::timing::reset_and_start_timer();
			auto [idx, minimum] = find_minimum_ref(needle, data);
			const uint64_t cycle_per_update = ::tools::timing::get_elapsed_cycles();

			if (cycle_per_update < fastest_load_BF16_ref) {
				fastest_load_BF16_ref = cycle_per_update;
			}
			result_load_BF16_ref = idx;
			minimum_load_BF16_ref = minimum;
		}

		// load FP64, fma FP64
		for (int run = 0; run < n_runs; ++run) {
			::tools::timing::reset_and_start_timer();
			auto [idx, minimum] = find_minimum_load_FP64_fma_FP64(needle_FP64, data_FP64);
			const uint64_t cycle_per_update = ::tools::timing::get_elapsed_cycles();

			if (cycle_per_update < fastest_load_FP64_fma_FP64) {
				fastest_load_FP64_fma_FP64 = cycle_per_update;
			}
			result_load_FP64_fma_FP64 = idx;
			minimum_load_FP64_fma_FP64 = minimum;
		}

		// load BF16, fma FP64
		for (int run = 0; run < n_runs; ++run) {
			::tools::timing::reset_and_start_timer();
			auto [idx, minimum] = find_minimum_load_BF16_fma_FP64(needle, data);
			const uint64_t cycle_per_update = ::tools::timing::get_elapsed_cycles();

			if (cycle_per_update < fastest_load_BF16_fma_FP64) {
				fastest_load_BF16_fma_FP64 = cycle_per_update;
			}
			result_load_BF16_fma_FP64 = idx;
			minimum_load_BF16_fma_FP64 = minimum;
		}

		// load FP64, fma BF16
		for (int run = 0; run < n_runs; ++run) {
			::tools::timing::reset_and_start_timer();
			auto [idx, minimum] = find_minimum_load_FP64_fma_BF16(needle_FP64, data_FP64);
			const uint64_t cycle_per_update = ::tools::timing::get_elapsed_cycles();

			if (cycle_per_update < fastest_load_FP64_fma_BF16) {
				fastest_load_FP64_fma_BF16 = cycle_per_update;
			}
			result_load_FP64_fma_BF16 = idx;
			minimum_load_FP64_fma_BF16 = minimum;
		}

		// load BF16, fma BF16
		for (int run = 0; run < n_runs; ++run) {
			::tools::timing::reset_and_start_timer();
			auto [idx, minimum] = find_minimum_load_BF16_fma_BF16(needle, data);
			const uint64_t cycle_per_update = ::tools::timing::get_elapsed_cycles();

			if (cycle_per_update < fastest_load_BF16_fma_BF16) {
				fastest_load_BF16_fma_BF16 = cycle_per_update;
			}
			result_load_BF16_fma_BF16 = idx;
			minimum_load_BF16_fma_BF16 = minimum;
		}

		// load BF16, AMX BF16
		for (int run = 0; run < n_runs; ++run) {
			::tools::timing::reset_and_start_timer();
			auto [idx, minimum] = find_minimum_load_BF16_amx_BF16(needle, data);
			const uint64_t cycle_per_update = ::tools::timing::get_elapsed_cycles();

			if (cycle_per_update < fastest_load_BF16_amx_BF16) {
				fastest_load_BF16_amx_BF16 = cycle_per_update;
			}
			result_load_BF16_amx_BF16 = idx;
			minimum_load_BF16_amx_BF16 = minimum;
		}

		// load BF16, AMX BF16
		for (int run = 0; run < n_runs; ++run) {
			::tools::timing::reset_and_start_timer();
			auto [idx, minimum] = find_minimum_load_BF16_amx_BF16_x16(needle16, data);
			const uint64_t cycle_per_update = ::tools::timing::get_elapsed_cycles();

			if (cycle_per_update < fastest_load_BF16_amx2_BF16) {
				fastest_load_BF16_amx2_BF16 = cycle_per_update;
			}
			result_load_BF16_amx2_BF16 = idx;
			minimum_load_BF16_amx2_BF16 = minimum;
		}

		std::cout << "load BF16 ref (FP32): result " << result_load_BF16_ref << "; minimum " << minimum_load_BF16_ref << std::endl;
		std::cout << "load FP64 fma FP64  : result " << result_load_FP64_fma_FP64 << "; minimum " << minimum_load_FP64_fma_FP64 << std::endl;
		std::cout << "load BF16 fma FP64  : result " << result_load_BF16_fma_FP64 << "; minimum " << minimum_load_BF16_fma_FP64 << std::endl;
		std::cout << "load FP64 fma BF16  : result " << result_load_FP64_fma_BF16 << "; minimum " << minimum_load_FP64_fma_BF16 << std::endl;
		std::cout << "load BF16 fma BF16  : result " << result_load_BF16_fma_BF16 << "; minimum " << minimum_load_BF16_fma_BF16 << std::endl;
		std::cout << "load BF16 amx BF16  : result " << result_load_BF16_amx_BF16 << "; minimum " << minimum_load_BF16_amx_BF16 << std::endl;
		std::cout << "load BF16 amx2 BF16 : result " << result_load_BF16_amx_BF16 << "; minimum " << minimum_load_BF16_amx_BF16 << std::endl;

		std::cout << std::endl;
		std::cout << "load BF16 ref CPP  : " << fastest_load_BF16_ref << " cycles" << std::endl;
		std::cout << "load FP64 fma FP64 : " << fastest_load_FP64_fma_FP64 << " cycles" << std::endl;
		std::cout << "load BF16 fma FP64 : " << fastest_load_BF16_fma_FP64 << " cycles" << std::endl;
		std::cout << "load FP64 fma BF16 : " << fastest_load_FP64_fma_BF16 << " cycles" << std::endl;
		std::cout << "load BF16 fma BF16 : " << fastest_load_BF16_fma_BF16 << " cycles" << std::endl;
		std::cout << "load BF16 amx BF16 : " << fastest_load_BF16_amx_BF16 << " cycles" << std::endl;
		std::cout << "load BF16 amx2 BF16: " << fastest_load_BF16_amx2_BF16/16 << " cycles" << std::endl;
	}
}