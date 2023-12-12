#pragma once
#include <immintrin.h>

#include "amx.amx_matrix.h"
#include "amx.tile.h"
#include "amx.tile_config.h"
#include "amx.tools.h"
#include "amx.types.h"
#include "generated/asm/amx.asm.h"
#include <iostream>
#include <ostream>

// Sapphire-Rapids
namespace amx::tmul::spr {

	inline void print_statistics(int N, int M, int K) {
		{
			const int Nt = N / 16;
			const int Mt = M / 16;
			const int Kt = K / 32;
			std::cout << "N=" << N << "; M=" << M << "; K=" << K << "; Nt=" << Nt << "; Mt=" << Mt << "; Kt=" << Kt << std::endl;
			std::cout << "Matrix size: C (" << (Nt * Mt) << "KB) += (" << (Nt * Kt) << "KB) * B (" << (Mt * Kt) << "KB)" << std::endl;
			std::cout << "Matrix size: C (" << (Nt * Mt) / 1024 << "MB) += A (" << (Nt * Kt) / 1024 << "MB) * B (" << (Mt * Kt) / 1024 << "MB)" << std::endl;
			{
				int load_C = (Nt * Mt * Kt);
				std::cout << "tdpbf16pf AMX3:        load C tiles: " << load_C;
				int load_AB = (Nt * Mt * Kt) * 2;
				std::cout << ";\tload A&B tiles: " << load_AB << ";\tsave C tiles " << load_C;
				int mem1_KB = (load_C + load_AB + load_C);
				std::cout << ";\tmem: " << mem1_KB << "KB;\t" << (mem1_KB / 1024) << "MB" << std::endl;
			}
			{
				int load_C = (Nt * Mt);
				std::cout << "tdpbf16pf AMX2: stream load C tiles: " << load_C;
				int load_AB = (Nt * Mt * Kt) * 2;
				std::cout << ";\tload A&B tiles: " << load_AB << ";\tsave C tiles " << load_C;
				int mem1_KB = load_AB + load_C;
				std::cout << ";\tmem: " << mem1_KB << "KB;\t" << (mem1_KB / 1024) << "MB" << std::endl;
			}
			{
				int load_C = (Nt * Mt);
				std::cout << "tdpbf16pf  AMX: stream load C tiles: " << load_C;
				int load_AB = (Nt * Mt * Kt);
				std::cout << ";\tload A&B tiles: " << load_AB << ";\tsave C tiles " << load_C;
				int mem1_KB = load_AB + load_C;
				std::cout << ";\tmem: " << mem1_KB << "KB;\t" << (mem1_KB / 1024) << "MB" << std::endl;
			}
			std::cout << std::endl;
		}
	}

	// TDPBSSD: Compute dot-product of BF16 (16-bit) floating-point pairs in tiles a and b, accumulating the 
	// intermediate single-precision (32-bit) floating-point elements with elements in dst, and store the 32-bit
	// result back to tile dst. C[M][N] += A[M][K] * B[K][N]
	// NOTE: B is transposed
	inline void tdpbf16ps_intrin_amx(Tile<FP32>& C, const Tile<BF16>& A, const Tile<BF16>& B) {
		{
			amx::Tile_config config = { 0 };
			config.palette_id = 1;
			config.start_row = 0;

			config.rows[0] = 16;
			config.rows[1] = 16;
			config.rows[2] = 16;

			config.colsb[0] = 64;
			config.colsb[1] = 64;
			config.colsb[2] = 64;
			_tile_loadconfig(&config);
		}
		_tile_loadd(0, C.data(), 64);
		_tile_loadd(1, A.data(), 64);
		_tile_loadd(2, B.data(), 64);
		_tile_dpbf16ps(0, 1, 2);
		_tile_stored(0, C.data(), 64);
		_tile_release();
	}

	inline void tdpbssd_intrin_amx(Tile<Int32>& C, const Tile<Int8>& A, const Tile<Int8>& B) {
		{
			amx::Tile_config config = { 0 };
			config.palette_id = 1;
			config.start_row = 0;

			config.rows[0] = 16;
			config.rows[1] = 16;
			config.rows[2] = 16;

			config.colsb[0] = 64;
			config.colsb[1] = 64;
			config.colsb[2] = 64;
			_tile_loadconfig(&config);
		}
		_tile_loadd(0, C.data(), 64);
		_tile_loadd(1, A.data(), 64);
		_tile_loadd(2, B.data(), 64);
		_tile_dpbssd(0, 1, 2);
		_tile_stored(0, C.data(), 64);
		_tile_release();
	}

	inline void tdpbf16ps_intrin_amx(AmxMatrix<FP32>& C, const AmxMatrix<BF16>& A, const AmxMatrix<BF16>& B) {
		if constexpr (DEBUG) {
			tools::check_matmul_dims<true>(C, A, B);
		}

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

		const int M = C.n_rows_tile_;
		const int N = C.n_cols_tile_;
		const int K = A.n_cols_tile_;

		const bool use_2x2_tiles = ((M & 0b1) == 0) && ((N & 0b1) == 0);

		if (use_2x2_tiles) {

			//; C[0][0] += A[1][0] * B[1][0]
			//; C[0][0] += A[0][0] * B[0][0]
			//; C[1][0] += A[0][0] * B[0][1]
			//; C[1][0] += A[1][0] * B[1][1]

			//; C[0][1] += A[0][1] * B[0][0]
			//; C[0][1] += A[1][1] * B[1][0]
			//; C[1][1] += A[0][1] * B[0][1]
			//; C[1][1] += A[1][1] * B[1][1]

			//int n_loads_AB = 0;
			//int n_loads_C = 0;

			for (int i = 0; i < M; i += 2) {
				for (int j = 0; j < N; j += 2) {
					FP32* c_ptr00 = C.get_tile(j + 0, i + 0).data();
					FP32* c_ptr10 = C.get_tile(j + 1, i + 0).data();
					FP32* c_ptr01 = C.get_tile(j + 0, i + 1).data();
					FP32* c_ptr11 = C.get_tile(j + 1, i + 1).data();
					_tile_stream_loadd(0, c_ptr00, 64); // 0 = C[0][0]
					_tile_stream_loadd(2, c_ptr01, 64); // 2 = C[0][1]
					_tile_stream_loadd(1, c_ptr10, 64); // 1 = C[1][0]
					_tile_stream_loadd(3, c_ptr11, 64); // 3 = C[1][1]
					//n_loads_C += 4;

					for (int p = 0; p < K; ++p) {
						_tile_loadd(4, A.get_tile(p, i + 0).data(), 64); // 4 = A[0][0]
						_tile_loadd(6, B.get_tile(p, j + 0).data(), 64); // 6 = B[0][0]
						_tile_loadd(5, A.get_tile(p, i + 1).data(), 64); // 5 = A[0][1]
						_tile_loadd(7, B.get_tile(p, j + 1).data(), 64); // 7 = B[0][1]
						//n_loads_AB += 4;

						_tile_dpbf16ps(0, 4, 6); // C[0][0] += A[0][0] * B[0][0]
						_tile_dpbf16ps(2, 5, 6); // C[0][1] += A[0][1] * B[0][0]
						// 6 is available
						_tile_dpbf16ps(1, 4, 7); // C[1][0] += A[0][0] * B[0][1]
						// 4 if available
						_tile_dpbf16ps(3, 5, 7); // C[1][1] += A[0][1] * B[0][1]
						// 5 and 7 are available
					}
					_tile_stored(0, c_ptr00, 64);
					_tile_stored(2, c_ptr01, 64);
					_tile_stored(1, c_ptr10, 64);
					_tile_stored(3, c_ptr11, 64);
				}
			}
			//std::cout << "tdpbf16ps_intrin_amx: Nt=" << N << "; Mt=" << M << "; Kt=" << K << ": n_loads_AB=" << n_loads_AB << "; n_loads_C=" << n_loads_C << std::endl;
		}
		else {
			std::cout << "WARNING tdpbf16ps_intrin_amx using regular code " << std::endl;
			for (int i = 0; i < M; ++i) {
				for (int j = 0; j < N; ++j) {
					FP32* c_ptr = C.get_tile(j, i).data();
					_tile_stream_loadd(0, c_ptr, 64);
					for (int p = 0; p < K; ++p) {
						_tile_loadd(1, A.get_tile(p, i).data(), 64);
						_tile_loadd(2, B.get_tile(p, j).data(), 64);
						_tile_dpbf16ps(0, 1, 2);
					}
					_tile_stored(0, c_ptr, 64);
				}
			}
		}
		_tile_release();
	}

	inline void tdpbf16ps_intrin_amx2(AmxMatrix<FP32>& C, const AmxMatrix<BF16>& A, const AmxMatrix<BF16>& B) {
		if constexpr (DEBUG) {
			tools::check_matmul_dims<true>(C, A, B);
		}

		{
			amx::Tile_config config = { 0 };
			config.palette_id = 1;
			config.start_row = 0;

			config.rows[0] = 16;
			config.rows[1] = 16;
			config.rows[2] = 16;

			config.colsb[0] = 64;
			config.colsb[1] = 64;
			config.colsb[2] = 64;
			_tile_loadconfig(&config);
		}

		const int M = C.n_rows_tile_;
		const int N = C.n_cols_tile_;
		const int K = A.n_cols_tile_;

		//int n_loads_AB = 0;
		//int n_loads_C = 0;

		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				FP32* c_ptr = C.get_tile(j, i).data();
				//n_loads_C += 1;

				_tile_stream_loadd(0, c_ptr, 64);
				for (int p = 0; p < K; ++p) {
					_tile_loadd(1, A.get_tile(p, i).data(), 64);
					_tile_loadd(2, B.get_tile(p, j).data(), 64);
					//n_loads_AB += 2;
					_tile_dpbf16ps(0, 1, 2);
				}
				_tile_stored(0, c_ptr, 64);
			}
		}		
		//std::cout << "tdpbf16ps_intrin_amx2: Nt=" << N << "; Mt=" << M << "; Kt=" << K << ": n_loads_AB=" << n_loads_AB << "; n_loads_C=" << n_loads_C << std::endl;
		_tile_release();
	}

	inline void tdpbf16ps_intrin_amx3(AmxMatrix<FP32>& C, const AmxMatrix<BF16>& A, const AmxMatrix<BF16>& B) {
		if constexpr (DEBUG) {
			tools::check_matmul_dims<true>(C, A, B);
		}

		{
			amx::Tile_config config = { 0 };
			config.palette_id = 1;
			config.start_row = 0;

			config.rows[0] = 16;
			config.rows[1] = 16;
			config.rows[2] = 16;

			config.colsb[0] = 64;
			config.colsb[1] = 64;
			config.colsb[2] = 64;
			_tile_loadconfig(&config);
		}

		const int M = C.n_rows_tile_;
		const int N = C.n_cols_tile_;
		const int K = A.n_cols_tile_;

		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				FP32* c_ptr = C.get_tile(j, i).data();
				for (int p = 0; p < K; ++p) {
					_tile_loadd(0, c_ptr, 64);
					_tile_loadd(1, A.get_tile(p, i).data(), 64);
					_tile_loadd(2, B.get_tile(p, j).data(), 64);
					_tile_dpbf16ps(0, 1, 2);
					_tile_stored(0, c_ptr, 64);
				}
			}
		}
		_tile_release();
	}

	// loop over all tiles, and call the matrix multiplication on tiles 
	inline void tdpbf16ps_asm_no_amx(AmxMatrix<FP32>& C, const AmxMatrix<BF16>& A, const AmxMatrix<BF16>& B) {
		if constexpr (DEBUG) {
			tools::check_matmul_dims<true>(C, A, B);
		}

		const int M = C.n_rows_tile_;
		const int N = C.n_cols_tile_;
		const int K = A.n_cols_tile_;

		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				FP32* ptr_c = C.get_tile(j, i).data();
				for (int p = 0; p < K; ++p) {
					const BF16* ptr_a = A.get_tile(p, i).data();
					const BF16* ptr_b = B.get_tile(p, j).data();
					assembly::tdpbf16ps_N16_M16_K32_no_AMX_asm(ptr_c, ptr_a, ptr_b);
				}
			}
		}
	}

	inline void tdpbf16ps_asm_no_amx_old(AmxMatrix<FP32>& C, const AmxMatrix<BF16>& A, const AmxMatrix<BF16>& B) {
		if constexpr (DEBUG) {
			tools::check_matmul_dims<true>(C, A, B);
		}

		const int M = C.n_rows_tile_;
		const int N = C.n_cols_tile_;
		const int K = A.n_cols_tile_;

		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				FP32* ptr_c = C.get_tile(j, i).data();
				for (int p = 0; p < K; ++p) {
					const BF16* ptr_a = A.get_tile(p, i).data();
					const BF16* ptr_b = B.get_tile(p, j).data();
					assembly::tdpbf16ps_N16_M16_K32_no_AMX_old_asm(ptr_c, ptr_a, ptr_b);
				}
			}
		}
	}

	inline void tdpbssd_intrin_amx(AmxMatrix<Int32>& C, const AmxMatrix<Int8>& A, const AmxMatrix<Int8>& B) {
		if constexpr (DEBUG) {
			tools::check_matmul_dims<true>(C, A, B);
		}

		{
			amx::Tile_config config = { 0 };
			config.palette_id = 1;
			config.start_row = 0;

			config.rows[0] = 16;
			config.rows[1] = 16;
			config.rows[2] = 16;

			config.colsb[0] = 64;
			config.colsb[1] = 64;
			config.colsb[2] = 64;
			_tile_loadconfig(&config);
		}

		const int M = C.n_rows_tile_;
		const int N = C.n_cols_tile_;
		const int K = A.n_cols_tile_;

		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				Int32* c_ptr = C.get_tile(j, i).data();
				_tile_stream_loadd(0, c_ptr, 64);
				for (int p = 0; p < K; ++p) {
					_tile_loadd(1, A.get_tile(p, i).data(), 64);
					_tile_loadd(2, B.get_tile(p, j).data(), 64);
					_tile_dpbssd(0, 1, 2);
				}
				_tile_stored(0, c_ptr, 64);
			}
		}
		_tile_release();
	}
}
