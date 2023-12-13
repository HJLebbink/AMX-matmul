#pragma once
#include <cstdint>
#include <iostream> //std::cout
#include <string>

#include "amx.amx_matrix.h"
#include "amx.matrix.h"
#include "amx.print.h"
#include "amx.test_data.h"
#include "amx.tile.h"
#include "amx.tmul.ref.h"
#include "amx.tmul.spr.h"
#include "amx.transpose.h"
#include "amx.types.h"

#include "generated/asm/amx.asm.h"

namespace amx::test {

	constexpr inline float EQUALITY_PRECISION = 0.001f;

	// use test data 1 and compare ref with amx
	inline void test1_matmul_example1() 
	{
		AmxMatrix<Int8> A = AmxMatrix<Int8>(64, 16);
		AmxMatrix<Int8> B = AmxMatrix<Int8>(64, 16);
		AmxMatrix<Int32> C_exp = AmxMatrix<Int32>(64, 16);
		AmxMatrix<Int32> C_obs = AmxMatrix<Int32>(64, 16);

		// load data
		{
			const auto td = createTD1<int8_t, int32_t>();

			A.clear();
			for (int col = 0; col < td.src1_.get_n_cols(); ++col) {
				for (int row = 0; row < td.src1_.get_n_rows(); ++row) {
					A.set(col, row, static_cast<Int8>(td.src1_.get(col, row)));
				}
			}
			B.clear();
			for (int col = 0; col < td.src2_.get_n_cols(); ++col) {
				for (int row = 0; row < td.src2_.get_n_rows(); ++row) {
					B.set(col, row, static_cast<Int8>(td.src2_.get(col, row)));
				}
			}
			C_exp.clear();
			for (int col = 0; col < td.exp_.get_n_cols(); ++col) {
				for (int row = 0; row < td.exp_.get_n_rows(); ++row) {
					C_exp.set(col, row, static_cast<Int32>(td.exp_.get(col, row)));
				}
			}
		}

		// compute stuff
		if (true) {
			C_obs.clear();
			tmul::ref::tdpbssd_intel_doc(C_obs, A, B);
			if (C_obs == C_exp) {
				std::cout << "OK: test1_matmul_tile_example1: tdpbssd_intel_doc" << std::endl;
			}
			else {
				std::cout << "test1_matmul_tile_example1: tdpbssd_intel_doc failure:" << std::endl;
				std::cout << "A: " << A.pretty_print(true) << std::endl;
				std::cout << "B: " << B.pretty_print(true) << std::endl;
				std::cout << "C_exp: " << C_exp.pretty_print(true) << std::endl;
				std::cout << "C_obs: " << C_obs.pretty_print(true) << std::endl;
			}
		}
		if (true) {
			C_obs.clear();
			tmul::spr::tdpbssd_intrin_amx(C_obs, A, B);
			if (C_obs == C_exp) {
				std::cout << "OK: test1_matmul_tile_example1: tdpbssd_intrin_amx" << std::endl;
			}
			else {
				std::cout << "test1_matmul_tile_example1: tdpbssd_intrin_amx failure:" << std::endl;
				std::cout << "A: " << A.pretty_print(true) << std::endl;
				std::cout << "B: " << B.pretty_print(true) << std::endl;
				std::cout << "C_exp: " << C_exp.pretty_print(true) << std::endl;
				std::cout << "C_obs: " << C_obs.pretty_print(true) << std::endl;
			}
		}
	}

	inline void test_correctness_1x1x1_tiles_tdpbf16ps(const int n_experiments)
	{
		constexpr int N = 16;
		constexpr int M = 16;
		constexpr int K = 32;

		// m[col][row]: C[N][M] += A[K][M] * B[N][K]

		AmxMatrix<BF16> A = AmxMatrix<BF16>(K, M);
		AmxMatrix<BF16> B = AmxMatrix<BF16>(N, K);
		AmxMatrix<BF16> Bt = AmxMatrix<BF16>(K, N);
		AmxMatrix<FP32> C_ref = AmxMatrix<FP32>(N, M);
		AmxMatrix<FP32> C_obs = AmxMatrix<FP32>(N, M);

		Tile<BF16> tA = A.get_tile(0, 0);
		Tile<BF16> tB = B.get_tile(0, 0);
		Tile<BF16> tBt = Bt.get_tile(0, 0);
		Tile<FP32> tC_ref = C_ref.get_tile(0, 0);
		Tile<FP32> tC_obs = C_obs.get_tile(0, 0);

		for (int i = 0; i < n_experiments; ++i)
		{
			// init data
			{
				if (true) {
					fill_random(A);
					fill_random(B);
				}
				else {
					A.clear();
					B.clear();

					for (int j = 0; j < K; ++j) {
						const BF16 v = float_to_bf16(static_cast<float>(j) + 1.f);
						tA.set(j, 1, v);
						tB.set(j, 1, v);
					}
				}
				transpose_BF16(tB.data(), tBt.data());

				if (false) {
					std::cout << "tA: " << tA.pretty_print(true, tools::PrintType::bf16) << std::endl;
					std::cout << "tB: " << tB.pretty_print(true, tools::PrintType::bf16) << std::endl;
					std::cout << "tB_t: " << tBt.pretty_print(true, tools::PrintType::bf16) << std::endl;
				}
			}

			// compute stuff
			{
				tC_ref.clear();
				tmul::ref::tdpbf16ps_intel_doc(tC_ref, tA, tBt);

				if (false) {
					std::cout << "expected one position with 11440" << std::endl;
					std::cout << "tA: " << tA.pretty_print(true, tools::PrintType::bf16) << std::endl;
					std::cout << "tB_t: " << tBt.pretty_print(true, tools::PrintType::bf16) << std::endl;
					std::cout << "tC_ref: " << tC_ref.pretty_print(true, tools::PrintType::dec) << std::endl;
				}
				if (true) {
					tC_obs.clear();
					tmul::spr::tdpbf16ps_intrin_amx(tC_obs, tA, tBt);

					if (false) {
						std::cout << "expected one position with 11440" << std::endl;
						std::cout << "tC_ref: " << tC_obs.pretty_print(true, tools::PrintType::dec) << std::endl;
					}

					if (!approx_equal(tC_ref, tC_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_1x1x1_tiles_tdpbf16ps: tdpbf16ps_intrin_amx (experiment " << i << ")" << std::endl;
						std::cout << "tC_ref: " << tC_ref.pretty_print(true) << std::endl;
						std::cout << "tC_obs: " << tC_obs.pretty_print(true) << std::endl;
						subtract(tC_obs, tC_ref);
						std::cout << "delta:  " << tC_obs.pretty_print(true) << std::endl;
						__debugbreak();
						std::cout << "delta:  " << tC_obs.pretty_print(true) << std::endl;
						return;
					}
				}
				if (true) {
					tC_obs.clear();
					assembly::tdpbf16ps_N16_M16_K32_asm(tC_obs.data(), tA.data(), tBt.data());

					if (!approx_equal(tC_ref, tC_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_1x1x1_tiles_tdpbf16ps: tdpbf16ps_N16_M16_K32_asm (experiment " << i << ")" << std::endl;
						std::cout << "tC_ref: " << tC_ref.pretty_print(true) << std::endl;
						std::cout << "tC_obs: " << tC_obs.pretty_print(true) << std::endl;
						subtract(tC_obs, tC_ref);
						std::cout << "delta:  " << tC_obs.pretty_print(true) << std::endl;
						return;
					}
				}
				if (true) {
					tC_obs.clear();
					assembly::tdpbf16ps_N16_M16_K32_no_AMX_asm(tC_obs.data(), tA.data(), tBt.data());

					if (!approx_equal(tC_ref, tC_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_1x1x1_tiles_tdpbf16ps: tdpbf16ps_N16_M16_K32_no_AMX_asm (experiment " << i << ")" << std::endl;
						std::cout << "tC_ref: " << tC_ref.pretty_print(true) << std::endl;
						std::cout << "tC_obs: " << tC_obs.pretty_print(true) << std::endl;
						subtract(tC_obs, tC_ref);
						std::cout << "delta:  " << tC_obs.pretty_print(true) << std::endl;
						return;
					}
				}
				if (true) {
					C_obs.clear();
					tmul::ref::tdpbf16ps_intel_doc(C_obs, A, Bt);

					if (!approx_equal(C_ref, C_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_1x1x1_tiles_tdpbf16ps: tdpbf16ps_intel_doc (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print(true) << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print(true) << std::endl;
						return;
					}
				}
				if (true) {
					C_obs.clear();
					tmul::spr::tdpbf16ps_intrin_amx(C_obs, A, Bt);

					if (!approx_equal(C_ref, C_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_1x1x1_tiles_tdpbf16ps: tdpbf16ps_intrin_amx (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print(true) << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print(true) << std::endl;
						return;
					}
				}
				if (true) {
					C_obs.clear();
					tmul::spr::tdpbf16ps_intrin_amx2(C_obs, A, Bt);

					if (!approx_equal(C_ref, C_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_1x1x1_tiles_tdpbf16ps: tdpbf16ps_intrin_amx2 (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print(true) << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print(true) << std::endl;
						return;
					}
				}
				if (true) {
					C_obs.clear();
					tmul::spr::tdpbf16ps_intrin_amx3(C_obs, A, Bt);

					if (!approx_equal(C_ref, C_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_1x1x1_tiles_tdpbf16ps: tdpbf16ps_intrin_amx3 (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print(true) << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print(true) << std::endl;
						return;
					}
				}
				if (true) {
					C_obs.clear();
					tmul::spr::tdpbf16ps_asm_no_amx(C_obs, A, Bt);

					if (!approx_equal(C_ref, C_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_1x1x1_tiles_tdpbf16ps: tdpbf16ps_asm_no_amx (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print(true) << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print(true) << std::endl;
						return;
					}
				}
			}
		}
		std::cout << "OK: test_1x1x1_tiles_tdpbf16ps (n_experiments " << n_experiments << ")" << std::endl;
	}

	inline void test_correctness_2x2x2_tiles_tdpbf16ps(const int n_experiments)
	{
		constexpr int N = 2 * 16;
		constexpr int M = 2 * 16;
		constexpr int K = 2 * 32;

		// m[col][row]: C[N][M] += A[K][M] * B[N][K]
		AmxMatrix<BF16> A = AmxMatrix<BF16>(K, M);
		AmxMatrix<BF16> Bt = AmxMatrix<BF16>(K, N);
		AmxMatrix<FP32> C_ref = AmxMatrix<FP32>(N, M);
		AmxMatrix<FP32> C_obs = AmxMatrix<FP32>(N, M);

		for (int i = 0; i < n_experiments; ++i)
		{
			// init data
			{
				if (true) {
					fill_random(A);
					fill_random(Bt);
				}
				else {
					A.clear();
					Bt.clear();

					for (int j = 0; j < K; ++j) {
						const BF16 v = float_to_bf16(j + 1.f);
						A.set(j, 1, v);
						Bt.set(j, 1, v);
					}
				}

				if (false) {
					std::cout << "A: " << A.pretty_print(true, tools::PrintType::bf16) << std::endl;
					std::cout << "Bt: " << Bt.pretty_print(true, tools::PrintType::bf16) << std::endl;
				}
			}

			// compute stuff
			{
				C_ref.clear();
				tmul::ref::tdpbf16ps_intel_doc(C_ref, A, Bt);

				if (false) {
					std::cout << "A: " << A.pretty_print(true, tools::PrintType::bf16) << std::endl;
					std::cout << "Bt: " << Bt.pretty_print(true, tools::PrintType::bf16) << std::endl;
					std::cout << "C_ref: " << C_ref.pretty_print(true, tools::PrintType::dec) << std::endl;
				}
				if (true) {
					C_obs.clear();
					tmul::spr::tdpbf16ps_intrin_amx(C_obs, A, Bt);

					if (!approx_equal(C_ref, C_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_2x2x2_tiles_tdpbf16ps: tdpbf16ps_intrin_amx (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print() << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print() << std::endl;
						return;
					}
				}
				if (true) {
					C_obs.clear();
					tmul::spr::tdpbf16ps_intrin_amx2(C_obs, A, Bt);

					if (!approx_equal(C_ref, C_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_2x2x2_tiles_tdpbf16ps: tdpbf16ps_intrin_amx2 (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print() << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print() << std::endl;
						return;
					}
				}
				if (true) {
					C_obs.clear();
					tmul::spr::tdpbf16ps_intrin_amx3(C_obs, A, Bt);

					if (!approx_equal(C_ref, C_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_2x2x2_tiles_tdpbf16ps: tdpbf16ps_intrin_amx3 (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print() << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print() << std::endl;
						return;
					}
				}
				if (true) {
					C_obs.clear();
					tmul::spr::tdpbf16ps_asm_no_amx(C_obs, A, Bt);

					if (!approx_equal(C_ref, C_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_2x2x2_tiles_tdpbf16ps: tdpbf16ps_asm_no_amx<false> (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print() << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print() << std::endl;
						return;
					}
				}
				if (true) {
					C_obs.clear();
					assembly::tdpbf16ps_N32_M32_K64_asm(C_obs.data(), A.data(), Bt.data());

					if (false) {
						std::cout << "A: " << A.pretty_print(true, tools::PrintType::bf16) << std::endl;
						std::cout << "Bt: " << Bt.pretty_print(true, tools::PrintType::bf16) << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print() << std::endl;
					}

					if (!approx_equal(C_ref, C_obs, EQUALITY_PRECISION)) {
						std::cout << "ERROR: test_2x2x2_tiles_tdpbf16ps: tdpbf16ps_N32_M32_K64_asm (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print() << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print() << std::endl;
						return;
					}
				}
			}
		}
		std::cout << "OK: test_2x2x2_tiles_tdpbf16ps (n_experiments " << n_experiments << ")" << std::endl;
	}

	inline void test_correctness_1x1x1_tiles_tdpbssd(const int n_experiments)
	{
		constexpr int N = 16;
		constexpr int M = 16;
		constexpr int K = 64;

		// m[col][row]: C[N][M] += A[K][M] * B[N][K]

		AmxMatrix<Int8> A = AmxMatrix<Int8>(K, M);
		AmxMatrix<Int8> B = AmxMatrix<Int8>(M, K);
		AmxMatrix<Int8> Bt = AmxMatrix<Int8>(K, M);
		AmxMatrix<Int32> C_ref = AmxMatrix<Int32>(N, M);
		AmxMatrix<Int32> C_obs = AmxMatrix<Int32>(N, M);

		Tile<Int8> tA = A.get_tile(0, 0);
		Tile<Int8> tB = B.get_tile(0, 0);
		Tile<Int8> tBt = Bt.get_tile(0, 0);
		Tile<Int32> tC_ref = C_ref.get_tile(0, 0);
		Tile<Int32> tC_obs = C_obs.get_tile(0, 0);

		for (int i = 0; i < n_experiments; ++i)
		{
			// init data
			{
				if (true) {
					fill_random(A);
					fill_random(B);
				}
				else {
					A.clear();
					B.clear();

					for (int j = 0; j < K; ++j) {
						const Int8 v = static_cast<Int8>(j);
						tA.set(j, 1, v);
						tB.set(j, 1, v);
					}
				}
				transpose_Int8(tB.data(), tBt.data());

				if (false) {
					std::cout << "tA: " << tA.pretty_print(true, tools::PrintType::hex) << std::endl;
					std::cout << "tB: " << tB.pretty_print(true, tools::PrintType::hex) << std::endl;
					std::cout << "tB_t: " << tBt.pretty_print(true, tools::PrintType::hex) << std::endl;
				}
			}

			// compute stuff
			{
				tC_ref.clear();
				tmul::ref::tdpbssd_intel_doc(tC_ref, tA, tBt);

				if (false) {
					std::cout << "expected one position with 85344" << std::endl;
					std::cout << "tA: " << tA.pretty_print(true, tools::PrintType::hex) << std::endl;
					std::cout << "tB_t: " << tBt.pretty_print(true, tools::PrintType::hex) << std::endl;
					std::cout << "tC_ref: " << tC_ref.pretty_print(true, tools::PrintType::dec) << std::endl;
				}
				if (true) {
					tC_obs.clear();
					tmul::spr::tdpbssd_intrin_amx(tC_obs, tA, tBt);

					if (false) {
						std::cout << "expected one position with 85344" << std::endl;
						std::cout << "tC_ref: " << tC_obs.pretty_print(true, tools::PrintType::dec) << std::endl;
					}

					if (tC_ref != tC_obs) {
						std::cout << "ERROR: test_1x1x1_tiles_tdpbssd: tdpbf16ps_intrin_amx (experiment " << i << ")" << std::endl;
						std::cout << "tC_ref: " << tC_ref.pretty_print(true, tools::PrintType::dec) << std::endl;
						std::cout << "tC_obs: " << tC_obs.pretty_print(true, tools::PrintType::dec) << std::endl;
						subtract(tC_obs, tC_ref);
						std::cout << "delta:  " << tC_obs.pretty_print(true) << std::endl;
						return;
					}
				}
				//if (true) {
				//	tC_obs.clear();
				//	assembly::tdpbssd_N16_M16_K32_asm(tC_obs.data(), tA.data(), tBt.data());

				//	if (tC_ref != tC_obs) {
				//		std::cout << "ERROR: test_1x1x1_tiles_tdpbssd: tdpbssd_N16_M16_K32_asm (experiment " << i << ")" << std::endl;
				//		std::cout << "tC_ref: " << tC_ref.pretty_print(true) << std::endl;
				//		std::cout << "tC_obs: " << tC_obs.pretty_print(true) << std::endl;
				//		return;
				//	}
				//}
				//if (true) {
				//	tC_obs.clear();
				//	assembly::tdpbssd_N16_M16_K32_no_AMX_asm(tC_obs.data(), tA.data(), tBt.data());

				//	if (tC_ref != tC_obs) {
				//		std::cout << "ERROR: test_1x1x1_tiles_tdpbssd: tdpbssd_N16_M16_K32_no_AMX_asm (experiment " << i << ")" << std::endl;
				//		std::cout << "tC_ref: " << tC_ref.pretty_print(true) << std::endl;
				//		std::cout << "tC_obs: " << tC_obs.pretty_print(true) << std::endl;
				//		return;
				//	}
				//}
				if (true) {
					C_obs.clear();
					tmul::ref::tdpbssd_intel_doc(C_obs, A, Bt);

					if (C_ref != C_obs) {
						std::cout << "ERROR: test_1x1x1_tiles_tdpbssd: tdpbssd_intel_doc(Matrix) (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print(true) << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print(true) << std::endl;
						subtract(C_obs, C_ref);
						std::cout << "delta:  " << tC_obs.pretty_print(true) << std::endl;
						__debugbreak();
						return;
					}
				}
				if (true) {
					C_obs.clear();
					tmul::spr::tdpbssd_intrin_amx(C_obs, A, Bt);

					if (C_ref != C_obs) {
						std::cout << "ERROR: test_1x1x1_tiles_tdpbssd: tdpbssd_intrin_amx (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print(true) << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print(true) << std::endl;
						subtract(C_obs, C_ref);
						std::cout << "delta:  " << tC_obs.pretty_print(true) << std::endl;
						return;
					}
				}
	//			if (true) {
	//				C_obs.clear();
	//				tmul::spr::tdpbssd_asm_no_amx<false>(C_obs, A, Bt);

	//				if (C_ref != C_obs) {
	//					std::cout << "ERROR: test_1x1x1_tiles_tdpbssd: tdpbssd_asm_no_amx<false> (experiment " << i << ")" << std::endl;
	//					std::cout << "C_ref: " << C_ref.pretty_print(true) << std::endl;
	//					std::cout << "C_obs: " << C_obs.pretty_print(true) << std::endl;
	//					return;
	//				}
	//			}
	//			if (true) {
	//				C_obs.clear();
	//				tmul::spr::tdpbssd_asm_no_amx<true>(C_obs, A, Bt);

	//				if (C_ref != C_obs) {
	//					std::cout << "ERROR: test_1x1x1_tiles_tdpbssd: tdpbssd_asm_no_amx<true> (experiment " << i << ")" << std::endl;
	//					std::cout << "C_ref: " << C_ref.pretty_print(true) << std::endl;
	//					std::cout << "C_obs: " << C_obs.pretty_print(true) << std::endl;
	//					return;
	//				}
	//			}
			}
		}
		std::cout << "OK: test_1x1x1_tiles_tdpbssd (n_experiments " << n_experiments << ")" << std::endl;
	}

	inline void test_correctness_2x2x2_tiles_tdpbssd(const int n_experiments)
	{
		constexpr int N = 2 * 16;
		constexpr int M = 2 * 16;
		constexpr int K = 2 * 32;

		// m[col][row]: C[N][M] += A[K][M] * B[N][K]
		AmxMatrix<Int8> A = AmxMatrix<Int8>(K, M);
		AmxMatrix<Int8> Bt = AmxMatrix<Int8>(K, N);
		AmxMatrix<Int32> C_ref = AmxMatrix<Int32>(N, M);
		AmxMatrix<Int32> C_obs = AmxMatrix<Int32>(N, M);

		for (int i = 0; i < n_experiments; ++i)
		{
			// init data
			{
				if (true) {
					fill_random(A);
					fill_random(Bt);
				}
				else {
					A.clear();
					Bt.clear();

					for (int j = 0; j < K; ++j) {
						A.set(j, 1, static_cast<Int8>(j));
						Bt.set(j, 1, static_cast<Int8>(j));
					}
				}

				if (false) {
					std::cout << "A: " << A.pretty_print(true, tools::PrintType::bf16) << std::endl;
					std::cout << "Bt: " << Bt.pretty_print(true, tools::PrintType::bf16) << std::endl;
				}
			}

			// compute stuff
			{
				C_ref.clear();
				tmul::ref::tdpbssd_intel_doc(C_ref, A, Bt);

				if (false) {
					std::cout << "A: " << A.pretty_print(true, tools::PrintType::hex) << std::endl;
					std::cout << "Bt: " << Bt.pretty_print(true, tools::PrintType::hex) << std::endl;
					std::cout << "C_ref: " << C_ref.pretty_print(true, tools::PrintType::dec) << std::endl;
				}
				if (true) {
					C_obs.clear();
					tmul::spr::tdpbssd_intrin_amx(C_obs, A, Bt);

					if (C_ref != C_obs) {
						std::cout << "ERROR: test_2x2x2_tiles_tdpbssd: tdpbssd_intrin_amx (experiment " << i << ")" << std::endl;
						std::cout << "C_ref: " << C_ref.pretty_print() << std::endl;
						std::cout << "C_obs: " << C_obs.pretty_print() << std::endl;
						return;
					}
				}
				//if (true) {
				//	C_obs.clear();
				//	tmul::spr::tdpbssd_asm_no_amx<false>(C_obs, A, Bt);

				//	if (C_ref != C_obs) {
				//		std::cout << "ERROR: test_2x2x2_tiles_tdpbssd: tdpbf16ps_asm_no_amx<false> (experiment " << i << ")" << std::endl;
				//		std::cout << "C_ref: " << C_ref.pretty_print() << std::endl;
				//		std::cout << "C_obs: " << C_obs.pretty_print() << std::endl;
				//		return;
				//	}
				//}
				//if (true) {
				//	C_obs.clear();
				//	tmul::spr::tdpbssd_asm_no_amx<true>(C_obs, A, Bt);

				//	if (C_ref != C_obs) {
				//		std::cout << "ERROR: test_2x2x2_tiles_tdpbssd: tdpbssd_asm_no_amx<true> (experiment " << i << ")" << std::endl;
				//		std::cout << "C_ref: " << C_ref.pretty_print() << std::endl;
				//		std::cout << "C_obs: " << C_obs.pretty_print() << std::endl;
				//		return;
				//	}
				//}
				//if (true) {
				//	C_obs.clear();
				//	assembly::tdpbssd_N32_M32_K64_asm(C_obs.data(), A.data(), Bt.data());

				//	if (false) {
				//		std::cout << "A: " << A.pretty_print(true, tools::PrintType::bf16) << std::endl;
				//		std::cout << "Bt: " << Bt.pretty_print(true, tools::PrintType::bf16) << std::endl;
				//		std::cout << "C_obs: " << C_obs.pretty_print() << std::endl;
				//	}

				//	if (C_ref != C_obs) {
				//		std::cout << "ERROR: test_2x2x2_tiles_tdpbssd: tdpbssd_N32_M32_K64_asm (experiment " << i << ")" << std::endl;
				//		std::cout << "C_ref: " << C_ref.pretty_print() << std::endl;
				//		std::cout << "C_obs: " << C_obs.pretty_print() << std::endl;
				//		return;
				//	}
				//}
			}
		}
		std::cout << "OK: test_2x2x2_tiles_tdpbssd (n_experiments " << n_experiments << ")" << std::endl;
	}
}