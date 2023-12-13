#pragma once
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include "amx.amx_matrix.h"
#include "amx.tile.h"
#include "amx.tmul.ref.h"
#include "amx.tmul.spr.h"
#include "amx.types.h"

#include "generated/asm/amx.asm.h"
#include "tools.timing.h"


namespace amx::benchmark {

	inline void tdpbf16ps_1tile_N16_M16_K32(int n_benchmarks = 10000)
	{
		uint64_t fastest_cpp = 100000000000;
		uint64_t fastest_no_amx = 100000000000;
		uint64_t fastest_dym_amx = 100000000000;
		uint64_t fastest_asm_amx = 100000000000;

		alignas(64) std::array<BF16, 512> buf_a, buf_b;
		alignas(64) std::array<float, 256> buf_c;

		//C[N][M] += A[K][M] * B[N][K]
		Tile<BF16> A = Tile<BF16>(buf_a.data());
		Tile<BF16> Bt = Tile<BF16>(buf_b.data());
		Tile<FP32> C = Tile<FP32>(buf_c.data());

		A.clear(); //should be fill_random()
		Bt.clear();

		for (int k = 0; k < n_benchmarks; ++k)
		{
			{ // benchmark cpp
				::tools::timing::reset_and_start_timer();
				tmul::ref::tdpbf16ps_intel_doc(C, A, Bt);
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_cpp) {
					fastest_cpp = elapsed;
				}
			}
			{ // benchmark sapphire rapids but no AMX
				::tools::timing::reset_and_start_timer();
				assembly::tdpbf16ps_N16_M16_K32_no_AMX_asm(C.data(), A.data(), Bt.data());
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_no_amx) {
					fastest_no_amx = elapsed;
				}
			}
			{ // benchmark dynamic intrinsics with AMX
				::tools::timing::reset_and_start_timer();
				tmul::spr::tdpbf16ps_intrin_amx(C, A, Bt);
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_dym_amx) {
					fastest_dym_amx = elapsed;
				}
			}
			{ // benchmark compiled asm with AMX
				::tools::timing::reset_and_start_timer();
				assembly::tdpbf16ps_N16_M16_K32_asm(C.data(), A.data(), Bt.data());
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_asm_amx) {
					fastest_asm_amx = elapsed;
				}
			}
		}

		const int n_computations = 16 * 16 * 32;

		std::cout << "Best results of " << n_benchmarks << " benchmark runs: N16_M16_K32: number of operations " << n_computations << " (BF16 c+=a*b)" << std::endl;
		std::cout << "dynamic ref CPP: " << fastest_cpp << " cycles; " << (static_cast<float>(fastest_cpp) / n_computations) << " cycle per operation" << std::endl;
		std::cout << "dynamic  NO AMX: " << fastest_no_amx << " cycles; " << (static_cast<float>(fastest_no_amx) / n_computations) << " cycle per operation" << std::endl;
		std::cout << "dynamic     AMX: " << fastest_dym_amx << " cycles; " << (static_cast<float>(fastest_dym_amx) / n_computations) << " cycle per operation" << std::endl;
		std::cout << "compiled    AMX: " << fastest_asm_amx << " cycles; " << (static_cast<float>(fastest_asm_amx) / n_computations) << " cycle per operation" << std::endl;
		std::cout << std::endl;
	}

	inline void tdpbf16ps(int N, int M, int K, int n_benchmarks = 10000)
	{
		uint64_t fastest_cpp = 100000000000;
		//uint64_t fastest_icl = 100000000000;
		uint64_t fastest_no_amx = 100000000000;
		uint64_t fastest_no_amx_old = 100000000000;
		uint64_t fastest_dyn_amx3 = 100000000000;
		uint64_t fastest_dyn_amx2 = 100000000000;
		uint64_t fastest_dym_amx = 100000000000;
		uint64_t fastest_asm_amx = 100000000000;

		//C[N][M] += A[K][M] * B[N][K]
		AmxMatrix<BF16> A = AmxMatrix<BF16>(K, M); 
		AmxMatrix<BF16> Bt = AmxMatrix<BF16>(K, N); //B[N][K]
		AmxMatrix<FP32> C = AmxMatrix<FP32>(N, M);

		fill_random(A);
		fill_random(Bt);

		for (int k = 0; k < n_benchmarks; ++k)
		{
			if (true) {
				::tools::timing::reset_and_start_timer();
				tmul::ref::tdpbf16ps_intel_doc(C, A, Bt);
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_cpp) {
					fastest_cpp = elapsed;
				}
			}
			if (true) { // benchmark sapphire rapids but no AMX
				::tools::timing::reset_and_start_timer();
				tmul::spr::tdpbf16ps_asm_no_amx_old(C, A, Bt);
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_no_amx_old) {
					fastest_no_amx_old = elapsed;
				}
			}
			if (true) { // benchmark sapphire rapids but no AMX
				::tools::timing::reset_and_start_timer();
				tmul::spr::tdpbf16ps_asm_no_amx(C, A, Bt);
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_no_amx) {
					fastest_no_amx = elapsed;
				}
			}
			if (true) { // benchmark sapphire rapids AMX not optimized
				::tools::timing::reset_and_start_timer();
				tmul::spr::tdpbf16ps_intrin_amx3(C, A, Bt);
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_dyn_amx3) {
					fastest_dyn_amx3 = elapsed;
				}
			}
			if (true) { // benchmark sapphire rapids AMX not optimized
				::tools::timing::reset_and_start_timer();
				tmul::spr::tdpbf16ps_intrin_amx2(C, A, Bt);
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_dyn_amx2) {
					fastest_dyn_amx2 = elapsed;
				}
			}
			if (true) { // benchmark dynamic intrinsics with AMX
				::tools::timing::reset_and_start_timer();
				tmul::spr::tdpbf16ps_intrin_amx(C, A, Bt);
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_dym_amx) {
					fastest_dym_amx = elapsed;
				}
			}
			if (true) { // benchmark compiled asm with AMX
				if ((N == 16) && (M == 16) && (K == 32)) {
					::tools::timing::reset_and_start_timer();
					assembly::tdpbf16ps_N16_M16_K32_asm(C.data(), A.data(), Bt.data());
				}
				else if ((N == 64) && (M == 64) && (K == 64)) {
					::tools::timing::reset_and_start_timer();
					assembly::tdpbf16ps_N64_M64_K64_asm(C.data(), A.data(), Bt.data());
				}
				else if ((N == 128) && (M == 128) && (K == 128)) {
					::tools::timing::reset_and_start_timer();
					assembly::tdpbf16ps_N128_M128_K128_asm(C.data(), A.data(), Bt.data());
				}
				else if ((N == 256) && (M == 256) && (K == 256)) {
					::tools::timing::reset_and_start_timer();
					assembly::tdpbf16ps_N256_M256_K256_asm(C.data(), A.data(), Bt.data());
				}
				//else if ((N == 512) && (M == 512) && (K == 512)) {
				//	::tools::timing::reset_and_start_timer();
				//	assembly::tdpbf16ps_N512_M512_K512_asm(C.data(), A.data(), Bt.data());
				//}
				else {
					::tools::timing::reset_and_start_timer();
					//TODO
				}
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_asm_amx) {
					fastest_asm_amx = elapsed;
				}
			}
		}

		const long long n_computations = static_cast<long long>(N) * static_cast<long long>(M) * static_cast<long long>(K);

		std::cout << "Best results of " << n_benchmarks << " benchmark runs: N"<< N <<"_M"<<M<<"_K" <<K << ": number of operations " << n_computations << " (BF16 c+=a*b)" << std::endl;
		std::cout << "dynamic    ref CPP: " << fastest_cpp << " cycles; " << (static_cast<float>(fastest_cpp) / n_computations) << " cycle per operation" << std::endl;
		//std::cout << "dynamic        ICL: " << fastest_icl << " cycles; " << (static_cast<float>(fastest_icl) / n_computations) << " cycle per operation"<< std::endl;
		std::cout << "dynamic NO AMX old: " << fastest_no_amx_old << " cycles; " << (static_cast<float>(fastest_no_amx_old) / n_computations) << " cycle per operation" << std::endl;
		std::cout << "dynamic SPR NO AMX: " << fastest_no_amx << " cycles; " << (static_cast<float>(fastest_no_amx) / n_computations) << " cycle per operation; " << (static_cast<float>(fastest_no_amx) / fastest_no_amx) << " faster" << std::endl;
		std::cout << "dynamic    (3) AMX: " << fastest_dyn_amx3 << " cycles; " << (static_cast<float>(fastest_dyn_amx3) / n_computations) << " cycle per operation; " << (static_cast<float>(fastest_no_amx) / fastest_dyn_amx3) << " faster" << std::endl;
		std::cout << "dynamic    (2) AMX: " << fastest_dyn_amx2 << " cycles; " << (static_cast<float>(fastest_dyn_amx2) / n_computations) << " cycle per operation; " << (static_cast<float>(fastest_no_amx) / fastest_dyn_amx2) << " faster" << std::endl;
		std::cout << "dynamic        AMX: " << fastest_dym_amx << " cycles; " << (static_cast<float>(fastest_dym_amx) / n_computations) << " cycle per operation; " << (static_cast<float>(fastest_no_amx) / fastest_dym_amx) << " faster" << std::endl;
		std::cout << "compiled       AMX: " << fastest_asm_amx << " cycles; " << (static_cast<float>(fastest_asm_amx) / n_computations) << " cycle per operation; " << (static_cast<float>(fastest_no_amx) / fastest_asm_amx) << " faster" << std::endl;
		std::cout << std::endl;
	}

	inline void tdpbssd(int N, int M, int K, int n_benchmarks = 10000)
	{
		uint64_t fastest_cpp = 100000000000;
		uint64_t fastest_icl = 100000000000;
		uint64_t fastest_no_amx = 100000000000;
		uint64_t fastest_dyn_amx_not_optimized = 100000000000;
		uint64_t fastest_dym_amx = 100000000000;
		uint64_t fastest_asm_amx = 100000000000;

		//C[N][M] += A[K][M] * B[N][K]
		AmxMatrix<Int8> A = AmxMatrix<Int8>(K, M);
		AmxMatrix<Int8> Bt = AmxMatrix<Int8>(K, N); //B[N][K]
		AmxMatrix<Int32> C = AmxMatrix<Int32>(N, M);

		const int64_t size = N * M * K;
		constexpr int64_t threshold = 100 * 64 * 64 * 64;

		fill_random(A);
		fill_random(Bt);

		for (int k = 0; k < n_benchmarks; ++k)
		{
			if (true) {
				if (size <= threshold) { // benchmark cpp
					::tools::timing::reset_and_start_timer();
					tmul::ref::tdpbssd_intel_doc(C, A, Bt);
					const auto elapsed = ::tools::timing::get_elapsed_cycles();
					if (elapsed < fastest_cpp) {
						fastest_cpp = elapsed;
					}
				}
				else {
					fastest_cpp = 0;
				}
			}
			//if (true) { // benchmark sapphire rapids but no AMX
			//	::tools::timing::reset_and_start_timer();
			//	tmul::spr::tdpbssd_asm_no_amx<false>(C, A, Bt);
			//	const auto elapsed = ::tools::timing::get_elapsed_cycles();
			//	if (elapsed < fastest_no_amx) {
			//		fastest_no_amx = elapsed;
			//	}
			//}
			//if (true) { // benchmark sapphire rapids AMX not optimized
			//	::tools::timing::reset_and_start_timer();
			//	tmul::spr::tdpbssd_asm_no_amx<true>(C, A, Bt);
			//	const auto elapsed = ::tools::timing::get_elapsed_cycles();
			//	if (elapsed < fastest_dyn_amx_not_optimized) {
			//		fastest_dyn_amx_not_optimized = elapsed;
			//	}
			//}
			if (true) { // benchmark dynamic intrinsics with AMX
				::tools::timing::reset_and_start_timer();
				tmul::spr::tdpbssd_intrin_amx(C, A, Bt);
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_dym_amx) {
					fastest_dym_amx = elapsed;
				}
			}
			if (true) { // benchmark compiled asm with AMX
				if ((N == 16) && (M == 16) && (K == 64)) {
					::tools::timing::reset_and_start_timer();
					assembly::tdpbssd_N16_M16_K64_asm(C.data(), A.data(), Bt.data());
				}
				else if ((N == 64) && (M == 64) && (K == 64)) {
					::tools::timing::reset_and_start_timer();
					assembly::tdpbssd_N64_M64_K64_asm(C.data(), A.data(), Bt.data());
				}
				//else if ((N == 128) && (M == 128) && (K == 128)) {
				//	::tools::timing::reset_and_start_timer();
				//	assembly::tdpbssd_N128_M128_K128_asm(C.data(), A.data(), Bt.data());
				//}
				//else if ((N == 256) && (M == 256) && (K == 256)) {
				//	::tools::timing::reset_and_start_timer();
				//	assembly::tdpbssd_N256_M256_K256_asm(C.data(), A.data(), Bt.data());
				//}
				else {
					//TODO
				}
				const auto elapsed = ::tools::timing::get_elapsed_cycles();
				if (elapsed < fastest_asm_amx) {
					fastest_asm_amx = elapsed;
				}
			}
		}

		const int n_computations = N * M * K;

		std::cout << "Best results of " << n_benchmarks << " benchmark runs: N" << N << "_M" << M << "_K" << K << ": number of operations " << n_computations << " (Int8 c+=a*b)" << std::endl;
		std::cout << "dynamic    ref CPP: " << fastest_cpp << " cycles; " << (static_cast<float>(fastest_cpp) / n_computations) << " cycle per operation" << std::endl;
		std::cout << "dynamic        ICL: " << fastest_icl << " cycles; " << (static_cast<float>(fastest_icl) / n_computations) << " cycle per operation" << std::endl;
		std::cout << "dynamic SPR NO AMX: " << fastest_no_amx << " cycles; " << (static_cast<float>(fastest_no_amx) / n_computations) << " cycle per operation; " << (static_cast<float>(fastest_no_amx) / fastest_no_amx) << " faster" << std::endl;
		std::cout << "dynamic notopt AMX: " << fastest_dyn_amx_not_optimized << " cycles; " << (static_cast<float>(fastest_dyn_amx_not_optimized) / n_computations) << " cycle per operation; " << (static_cast<float>(fastest_no_amx) / fastest_dyn_amx_not_optimized) << " faster" << std::endl;
		std::cout << "dynamic        AMX: " << fastest_dym_amx << " cycles; " << (static_cast<float>(fastest_dym_amx) / n_computations) << " cycle per operation; " << (static_cast<float>(fastest_no_amx) / fastest_dym_amx) << " faster" << std::endl;
		std::cout << "compiled       AMX: " << fastest_asm_amx << " cycles; " << (static_cast<float>(fastest_asm_amx) / n_computations) << " cycle per operation; " << (static_cast<float>(fastest_no_amx) / fastest_asm_amx) << " faster" << std::endl;
		std::cout << std::endl;
	}

	inline void benchmark_to_file(const std::string& filename, std::vector<int>& dimensions, const int n_runs) {
		const int n_dims = static_cast<int>(dimensions.size());		

		std::ofstream myfile;
		myfile.open(filename);
		myfile << "dims; no_amx; amx0; amx1; amx2" << std::endl;

		for (int i = 0; i < n_dims; ++i) {
			int dim = dimensions[i];
			int M = dim;
			int N = dim;
			int K = dim;
			const long long n_computations = static_cast<long long>(N) * static_cast<long long>(M) * static_cast<long long>(K);

			std::cout << std::endl << "benchmark " << i << "/" << n_dims << "; dim= " << dim << "; n_computations " << n_computations << std::endl;

			volatile double fastest_no_amx = 1000000; // volatile to prevent BUG in VS: it loads the value in xmm10 and hopes that xmm10 is preserved by asm code...
			volatile double fastest_amx0 = 1000001;
			volatile double fastest_amx1 = 1000002;
			volatile double fastest_amx2 = 1000003;

			//C[N][M] += A[K][M] * B[N][K]
			AmxMatrix<BF16> A = AmxMatrix<BF16>(K, M);
			AmxMatrix<BF16> Bt = AmxMatrix<BF16>(K, N); //B[N][K]
			AmxMatrix<FP32> C = AmxMatrix<FP32>(M, N);

			fill_random(A);
			fill_random(Bt);
			C.clear();

			for (int r = 0; r < n_runs; ++r) {
				std::cout << "x";
				if (true) {
					::tools::timing::reset_and_start_timer();
					tmul::spr::tdpbf16ps_asm_no_amx(C, A, Bt);
					const double cycle_per_update = static_cast<double>(::tools::timing::get_elapsed_cycles()) / n_computations;
					if (cycle_per_update < fastest_no_amx) {
						fastest_no_amx = cycle_per_update;
					}
				}
				if (true) {
					::tools::timing::reset_and_start_timer();
					tmul::spr::tdpbf16ps_intrin_amx3(C, A, Bt); // not optimized: all tiles are read (even C tiles)
					const double cycle_per_update = static_cast<double>(::tools::timing::get_elapsed_cycles()) / n_computations;
					if (cycle_per_update < fastest_amx0) {
						fastest_amx0 = cycle_per_update;
					}
				}
				if (true) {
					::tools::timing::reset_and_start_timer();
					tmul::spr::tdpbf16ps_intrin_amx2(C, A, Bt); // optimized to compute 1x1 tile in one go
					const double cycle_per_update = static_cast<double>(::tools::timing::get_elapsed_cycles()) / n_computations;
					if (cycle_per_update < fastest_amx2) {
						fastest_amx2 = cycle_per_update;
					}
				}
				if (true) {
					::tools::timing::reset_and_start_timer();
					tmul::spr::tdpbf16ps_intrin_amx(C, A, Bt); // optimized to compute 2x2 tiles in one go
					const double cycle_per_update = static_cast<double>(::tools::timing::get_elapsed_cycles()) / n_computations;
					if (cycle_per_update < fastest_amx1) {
						fastest_amx1 = cycle_per_update;
					}
				}
			}

			myfile << dim << ";" << std::fixed << std::setprecision(10) << 
				fastest_no_amx << "; " << fastest_amx0 << "; " << fastest_amx2 << "; " << fastest_amx1 << std::endl;
			myfile.flush();
		}
		myfile.close();
	}
}