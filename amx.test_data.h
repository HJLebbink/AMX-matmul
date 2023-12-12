#pragma once
#include <sstream>
#include <iostream> //std::cout
#include <iosfwd>
#include <string>

#include "amx.amx_matrix.h"
#include "amx.matrix.h"

namespace amx {

	template <typename T1, typename T2>
	struct test_data {
		Matrix<T1> src1_;
		Matrix<T1> src2_;
		Matrix<T2> exp_;

		[[nodiscard]] std::string pretty_print(bool colour = true) const {
			std::stringstream ss;
			ss << "src1: " << this->src1_.pretty_print(colour) << std::endl;
			ss << "src2: " << this->src2_.pretty_print(colour) << std::endl;
			ss << "exp: " << this->exp_.pretty_print(colour) << std::endl;
			return ss.str();
		}
	};

	// create test data 1. used wolfram-alpha:
	// {{1, 2, 3}, {3, 2, 1}, {1, 2, 3}, {1, 1, 1}, {2, 2, 2}} . {{4, 5, 6, 2}, {6, 5, 4, 1}, {3, 6, 5, 2}}
	// gives {{25, 33, 29, 10}, {27, 31, 31, 10}, {25, 33, 29, 10}, {13, 16, 15, 5}, {26, 32, 30, 10}}
	// but then when src2 is transposed.

	template <typename T1, typename T2> [[nodiscard]]
	test_data<T1, T2> createTD1() 
	{
		test_data<T1, T2> result{};

		constexpr int M = 5;
		constexpr int N = 4;
		constexpr int K = 3;

		result.src1_ = Matrix<T1>(K, M, 0);
		{
			result.src1_.set(0, 0, 1);
			result.src1_.set(1, 0, 2);
			result.src1_.set(2, 0, 3);

			result.src1_.set(0, 1, 3);
			result.src1_.set(1, 1, 2);
			result.src1_.set(2, 1, 1);

			result.src1_.set(0, 2, 1);
			result.src1_.set(1, 2, 2);
			result.src1_.set(2, 2, 3);

			result.src1_.set(0, 3, 1);
			result.src1_.set(1, 3, 1);
			result.src1_.set(2, 3, 1);

			result.src1_.set(0, 4, 2);
			result.src1_.set(1, 4, 2);
			result.src1_.set(2, 4, 2);
		}
		result.src2_ = Matrix<T1>(N, K, 0);
		{
			result.src2_.set(0, 0, 4);
			result.src2_.set(1, 0, 5);
			result.src2_.set(2, 0, 6);
			result.src2_.set(3, 0, 2);

			result.src2_.set(0, 1, 6);
			result.src2_.set(1, 1, 5);
			result.src2_.set(2, 1, 4);
			result.src2_.set(3, 1, 1);

			result.src2_.set(0, 2, 3);
			result.src2_.set(1, 2, 6);
			result.src2_.set(2, 2, 5);
			result.src2_.set(3, 2, 2);
		}
		result.exp_ = Matrix<T2>(N, M, 0);
		{
			result.exp_.set(0, 0, 25);
			result.exp_.set(1, 0, 33);
			result.exp_.set(2, 0, 29);
			result.exp_.set(3, 0, 10);

			result.exp_.set(0, 1, 27);
			result.exp_.set(1, 1, 31);
			result.exp_.set(2, 1, 31);
			result.exp_.set(3, 1, 10);

			result.exp_.set(0, 2, 25);
			result.exp_.set(1, 2, 33);
			result.exp_.set(2, 2, 29);
			result.exp_.set(3, 2, 10);

			result.exp_.set(0, 3, 13);
			result.exp_.set(1, 3, 16);
			result.exp_.set(2, 3, 15);
			result.exp_.set(3, 3, 5);

			result.exp_.set(0, 4, 26);
			result.exp_.set(1, 4, 32);
			result.exp_.set(2, 4, 30);
			result.exp_.set(3, 4, 10);
		}
		return result;
	};

	template <typename T1, typename T2> [[nodiscard]]
	test_data<T1, T2> createTD2() 
	{
		test_data<T1, T2> result{};

		//constexpr int M = 5;
		//constexpr int N = 4;
		//constexpr int K = 3;

		constexpr int M = 16;
		constexpr int N = 16;
		constexpr int K = 64;

		const int row = 0;

		result.src1_ = AmxMatrix<T1>(K, M, 0);
		{
			result.src1_.set(8, 2, 2);//Q
		}
		result.src2_ = AmxMatrix<T1>(K, N, 0);
		{
			//result.src2_.set((0 * 16) + 8, 8, 3);

			for (int j = 16; j < 64; ++j) {
				result.src2_.set(j, 2, 3);
			}
			result.src2_.set(16, 2, 5);


			//result.src2_.set((1 * 16) + 0, row+1, 5);
			//result.src2_.set((2 * 16) + 0, row+1, 7);
		}
		result.exp_ = AmxMatrix<T2>(N, M, 0);
		{
			result.exp_.set(0, 0, 3);
			result.exp_.set(0, 4, 3);
		}
		return std::move(result);
	};
}