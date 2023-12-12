#pragma once
#include <iostream> //std::cout
#include <cstdint>

#include "amx.tile.h"
#include "amx.amx_matrix.h"
#include "amx.types.h"
#include "amx.tools.h"


namespace amx::tmul::ref {

	// tdpbf16ps: m[col][row]: C[N][M] += A[K][M] * B[N][K]
	// NOTE B needs to be transposed
	inline void tdpbf16ps_intel_doc(
		Tile<FP32>& C,
		const Tile<BF16>& A,
		const Tile<BF16>& B
	) {
		constexpr bool print_spec = false;

		// Intel docs:
		// 
		//FOR m := 0 TO dst.rows - 1
		//	tmp := dst.row[m]
		//	FOR k := 0 TO(a.colsb / 4) - 1
		//	  FOR n := 0 TO(dst.colsb / 4) - 1
		//	    tmp.fp32[n] += FP32(a.row[m].bf16[2 * k + 0]) * FP32(b.row[k].bf16[2 * n + 0])
		//	    tmp.fp32[n] += FP32(a.row[m].bf16[2 * k + 1]) * FP32(b.row[k].bf16[2 * n + 1])
		//	  ENDFOR
		//	ENDFOR
		//	write_row_and_zero(dst, m, tmp, dst.colsb)
		//ENDFOR
		//zero_upper_rows(dst, dst.rows)
		//zero_tileconfig_start()

		float* c_ptr = C.data();
		const uint32_t* a_ptr = A.data<const uint32_t>();
		const uint32_t* b_ptr = B.data<const uint32_t>();

		for (int m = 0; m < 16; ++m) {
			for (int k = 0; k < 16; ++k) {
				const uint32_t data_a = a_ptr[(m * 16) + k];
				const float f0 = bf16_to_float(static_cast<BF16>(data_a & 0xFFFF));
				const float f2 = bf16_to_float(static_cast<BF16>(data_a >> 16));

				for (int n = 0; n < 16; ++n) {
					const uint32_t data_b = b_ptr[(k * 16) + n];
					const float f1 = bf16_to_float(static_cast<BF16>(data_b & 0xFFFF));
					const float f3 = bf16_to_float(static_cast<BF16>(data_b >> 16));

					if constexpr (print_spec) {
						std::cout << "C[" << m << "][" << n << "] += (A[" << m << "][" << ((k * 2) + 0) << "] * B[" << k << "][" << ((n * 2) + 0) << "]) + (A[" << m << "][" << ((k * 2) + 1) << "] * B[" << k << "][" << ((n * 2) + 1) << "])" << std::endl;
					}
					c_ptr[(m * 16) + n] += (f0 * f1) + (f2 * f3);
				}
			}
		}
		if constexpr (print_spec) {
			std::cout << std::endl;
		}
	}

	// TDPBSSD: tile dot product byte signed signed dword. Matrix multiply signed byte elements 
	// from src1 by signed byte elements from src2 and accumulate the dword elements in dst
	inline void tdpbssd_intel_doc(
		Tile<Int32>& C,
		const Tile<Int8>& A,
		const Tile<Int8>& B
	) {
		// Intel docs:
		// 
		//FOR m := 0 TO dst.rows - 1
		//	tmp := dst.row[m]
		//	FOR k := 0 TO (a.colsb / 4) - 1
		//		FOR n := 0 TO (dst.colsb / 4) - 1
		//			tmp.dword[n] := DPBD(tmp.dword[n], a.row[m].dword[k], b.row[k].dword[n])
		//		ENDFOR
		//	ENDFOR
		//	write_row_and_zero(dst, m, tmp, dst.colsb)
		//ENDFOR
		//zero_upper_rows(dst, dst.rows)
		//zero_tileconfig_start()		

		Int32* c_ptr = C.data();
		const uint32_t* a_ptr = A.data<const uint32_t>();
		const uint32_t* b_ptr = B.data<const uint32_t>();

		for (int m = 0; m < 16; ++m) {
			for (int k = 0; k < 16; ++k) {
				for (int n = 0; n < 16; ++n) {
					int pos_A = (m * 16) + k;
					int pos_B = (k * 16) + n;
					int pos_C = (m * 16) + n;

					const uint32_t a = a_ptr[pos_A];
					const uint32_t b = b_ptr[pos_B];

					const Int32 x0 = static_cast<Int32>(static_cast<Int8>(0xFF & (a >> 0 * 8)));
					const Int32 x1 = static_cast<Int32>(static_cast<Int8>(0xFF & (a >> 1 * 8)));
					const Int32 x2 = static_cast<Int32>(static_cast<Int8>(0xFF & (a >> 2 * 8)));
					const Int32 x3 = static_cast<Int32>(static_cast<Int8>(0xFF & (a >> 3 * 8)));

					const Int32 y0 = static_cast<Int32>(static_cast<Int8>(0xFF & (b >> 0 * 8)));
					const Int32 y1 = static_cast<Int32>(static_cast<Int8>(0xFF & (b >> 1 * 8)));
					const Int32 y2 = static_cast<Int32>(static_cast<Int8>(0xFF & (b >> 2 * 8)));
					const Int32 y3 = static_cast<Int32>(static_cast<Int8>(0xFF & (b >> 3 * 8)));

					c_ptr[pos_C] += ((x0 * y0) + (x1 * y1) + (x2 * y2) + (x3 * y3));
				}
			}
		}
	}

	//tdpbf16ps: m[col][row] : C[N][M] += A[K][M] * B[N][K]
	// method2 loop over all tiles, and call the matrix multiplication on tiles 
	// NOTE: B_IS_TRANSPOSED indicates that B is already transposed
	inline void tdpbf16ps_intel_doc(
		AmxMatrix<FP32>& C,
		const AmxMatrix<BF16>& A,
		const AmxMatrix<BF16>& B
	) {
		if constexpr (DEBUG) {
			tools::check_matmul_dims<true>(C, A, B);
		}
		
		const int M = C.n_rows_tile_;
		const int N = C.n_cols_tile_;
		const int K = A.n_cols_tile_; // equal to src2.nRows_

		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				Tile<FP32> tc = C.get_tile(j, i);
				for (int p = 0; p < K; ++p) {
					tdpbf16ps_intel_doc(tc, A.get_tile(p, i), B.get_tile(p, j));
				}
			}
		}
	}

	inline void tdpbssd_intel_doc(
		AmxMatrix<Int32>& C,
		const AmxMatrix<Int8>& A,
		const AmxMatrix<Int8>& B
	) {
		if constexpr (DEBUG) {
			tools::check_matmul_dims<true>(C, A, B);
		}

		const int M = C.n_rows_tile_;
		const int N = C.n_cols_tile_;
		const int K = A.n_cols_tile_; // equal to src2.nRows_

		for (int i = 0; i < M; ++i) {
			for (int j = 0; j < N; ++j) {
				Tile<Int32> tc = C.get_tile(j, i);
				for (int p = 0; p < K; ++p) {
					const Tile<Int8> ta = A.get_tile(p, i);
					const Tile<Int8> tb = B.get_tile(p, j);
					tdpbssd_intel_doc(tc, ta, tb);
				}
			}
		}
	}
}