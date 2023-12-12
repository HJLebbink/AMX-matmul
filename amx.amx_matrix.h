#pragma once
#include <string>
#include <ios> //hex
#include <iomanip> //setw
#include <iostream> //std::cout
#include <cstdint>
#include <cstdlib>
#include <iosfwd>
#include <random>
#include <sstream>
#include <utility>
#include <intrin.h>
#include <bit>

#include "amx.tile.h"
#include "amx.print.h"
#include "amx.types.h"
#include "amx.tile_array.h"
#include "amx.tools.h"

namespace amx {

	template <typename STORAGE>
	class AmxMatrix {
	private:
		TileArray data_;

	public:
		int n_cols_;
		int n_rows_;
		int n_cols_tile_; // number of tile columns in the data structure
		int n_rows_tile_; // number of tile rows in the data structure

		AmxMatrix() = delete;
		AmxMatrix(int n_cols, int n_rows) :
			data_(TileArray(calc_n_cols_tile<STORAGE>(n_cols), calc_n_rows_tile(n_rows))),
			n_cols_tile_(calc_n_cols_tile<STORAGE>(n_cols)),
			n_rows_tile_(calc_n_rows_tile(n_rows)),
			n_cols_(n_cols),
			n_rows_(n_rows)
		{}

		bool operator==(const AmxMatrix<STORAGE>& other) const
		{
			if ((this->n_cols_ != other.n_cols_) || (this->n_rows_ != other.n_rows_)) {
				return false;
			}

			const STORAGE* ptr1 = this->data();
			const STORAGE* ptr2 = other.data();

			for (int i = 0; i < this->n_cols_ * this->n_rows_; ++i) {
				if (ptr1[i] != ptr2[i]) {
					return false;
				}
			}
			return true;
		}

		[[nodiscard]] Tile<STORAGE> get_tile(int col, int row) const {
			// const_cast will haunt you!
			STORAGE* ptr = const_cast<STORAGE*>(this->data_.get_ptr<STORAGE>(col, row));
			return Tile<STORAGE>(ptr);
		}

		[[nodiscard]] int get_offset(MatrixKey key) const {
			return this->data_.get_offset(get_col(key), get_row(key));
		}

		template <typename T = STORAGE>
		[[nodiscard]] T* data() {
			return reinterpret_cast<T*>(this->data_.data());
		}

		template <typename T = STORAGE>
		[[nodiscard]] const T* data() const {
			return reinterpret_cast<const T*>(this->data_.data());
		}

		template <bool CHECK = true>
		[[nodiscard]] STORAGE get(int col, int row) const
		{
			int tile_col, col_in_tile;
			if constexpr (sizeof(STORAGE) == 1) {
				tile_col = col >> 6;
				col_in_tile = col & 0b11'1111;
			}
			else if constexpr (sizeof(STORAGE) == 2) {
				tile_col = col >> 5;
				col_in_tile = col & 0b1'1111;
			}
			else if constexpr (sizeof(STORAGE) == 4) {
				tile_col = col >> 4;
				col_in_tile = col & 0b1111;
			}
			else {
				__debugbreak();
			}
			const int tile_row = row >> 4;
			const int row_in_tile = row & 0b1111;
			return this->get_tile(tile_col, tile_row).get<CHECK>(col_in_tile, row_in_tile);
		}

		template <bool CHECK = true>
		void set(int col, int row, STORAGE value)
		{
			int tile_col, col_in_tile;
			if constexpr (sizeof(STORAGE) == 1) {
				tile_col = col >> 6;
				col_in_tile = col & 0b11'1111;
			}
			else if constexpr (sizeof(STORAGE) == 2) {
				tile_col = col >> 5;
				col_in_tile = col & 0b1'1111;
			}
			else if constexpr (sizeof(STORAGE) == 4) {
				tile_col = col >> 4;
				col_in_tile = col & 0b1111;
			}
			else {
				__debugbreak();
			}
			const int tile_row = row >> 4;
			const int row_in_tile = row & 0b1111;
			this->get_tile(tile_col, tile_row).set<CHECK>(col_in_tile, row_in_tile, value);
		}

		void clear() {
			std::memset(this->data_.data(), 0, static_cast<size_t>(this->n_cols_tile_) * static_cast<size_t>(this->n_rows_tile_) * 1024);
		}

		[[nodiscard]] std::string pretty_print(bool colour = false, tools::PrintType pt = tools::PrintType::dec) const
		{
			std::stringstream ss;
			{
				ss << "(#columns=" << (this->n_cols_tile_ * 16) << ", #rows=" << (this->n_rows_tile_ * 16) << "); tiles: " << std::endl;
				int counter = 0;
				for (int row = 0; row < this->n_rows_tile_; ++row) {
					for (int col = 0; col < this->n_cols_tile_; ++col) {
						ss << std::dec << std::setw(2) << std::setfill(' ') << counter << " ";
						counter++;
					}
					ss << std::endl;
				}
				ss << std::endl;
			}
			{
				int counter = 0;
				for (int row = 0; row < this->n_rows_tile_; ++row) {
					for (int col = 0; col < this->n_cols_tile_; ++col) {
						ss << "tile" << counter << ": (" << col << "," << row << ") ";
						ss << this->get_tile(col, row).pretty_print(colour, pt);
						counter++;
					}
				}
			}
			return ss.str();
		}
	};

	inline [[nodiscard]] bool approx_equal(const AmxMatrix<FP32>& a, const AmxMatrix<FP32>& b, float precision_percentage)
	{		
		if ((a.n_cols_ != b.n_cols_) || (a.n_rows_ != b.n_rows_)) {
			return false;
		}
		
		const uint32_t* ptr_a = a.data<uint32_t>();
		const uint32_t* ptr_b = b.data<uint32_t>();

		for (int i = 0; i < a.n_cols_ * a.n_rows_; ++i) {
			if (ptr_a[i] != ptr_b[i]) {
				const FP32 av = std::bit_cast<FP32>(ptr_a[i]);
				const FP32 bv = std::bit_cast<FP32>(ptr_b[i]);
				const float sub = av - bv;
				const float diff = std::abs(sub);
				if (diff > precision_percentage) {
					const float threshold = precision_percentage * std::max(std::abs(av), std::abs(bv));
					if (diff > threshold) {
						std::cout << "diff = " << diff << "; threshold = " << threshold << std::endl;
						return false;
					}
				}
			}
		}
		return true;
	}

	inline void fill_random(INOUT AmxMatrix<BF16>& m, float min_value = 0.f, float max_value = 1.f, int seed = 0) {
		std::random_device rd;
		std::mt19937 gen((seed == 0) ? rd() : seed);
		std::uniform_real_distribution<> dist(min_value, max_value);

		BF16* ptr = m.data();
		for (int pos = 0; pos < m.n_cols_ * m.n_rows_; ++pos) {
			ptr[pos] = float_to_bf16(static_cast<float>(dist(rd)));
		}
	}

	inline void fill_random(INOUT AmxMatrix<Int8>& m, int min_value = 0, int max_value = 127, int seed = 0) {
		std::random_device rd;
		std::mt19937 gen((seed == 0) ? rd() : seed);
		std::uniform_int_distribution<> dist(min_value, max_value);

		Int8* ptr = m.data();
		for (int pos = 0; pos < m.n_cols_ * m.n_rows_; ++pos) {
			ptr[pos] = static_cast<Int8>(dist(rd));
		}
	}

	template <typename T>
	void subtract(INOUT AmxMatrix<T>& a, const AmxMatrix<T>& b) {
		if constexpr (DEBUG) {
			if ((a.n_cols_ != b.n_cols_) || (a.n_rows_ != b.n_rows_)) {
				__debugbreak();
			}
		}

		for (int col = 0; col < a.n_rows_; ++col) {
			for (int row = 0; row < b.n_cols_; ++row) {
				const T av = a.get<DEBUG>(col, row);
				const T bv = b. get<DEBUG>(col, row);
				const T sub = av - bv;
				a.set<DEBUG>(col, row, sub);
			}
		}
	}
}