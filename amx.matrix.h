#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include "amx.print.h"
#include "amx.types.h"
#include "amx.tools.h"

namespace amx {

	// There are three types of Matrixes/Tiles:
	// - vanilla tile/matrix: memory has row-major layout. If the source data is also row-major, 
	//     you can start reading from an offset and read with stride 64.
	// - transposed tile: the tile is annotated as transposed. All operations on this tile work as 
	//     if the tile is a vanilla tile with the content (1024 bytes) transposed in memory. Making
	//     a tile tile transposed by setting this property prevent expensive memory shuffling.
	// - amx_layout: the tile has the memory layout needed by AMX TMUL operations second src tile.
	//
	// Eg. to multiply two source matrices A[N][K] and B[K][M] (N<=16, M<=16, K<=64) load matrix A 
	// into vanilla tile T1. If matrix B is vanilla matrix, it needs to be transposed and put into
	// amx_layout, this tile T2 can be used with instruction 'tdpbssd' to yield C[N][M]
	// 

	template <typename T>
	class Matrix {

	private:

		std::vector<T> data_;

		void init(int n_cols, int n_rows) {
			this->n_cols_ = n_cols;
			this->n_rows_ = n_rows;
			this->data_ = std::vector<T>(this->n_cols_ * this->n_rows_);
		}

	public:

		int n_cols_;
		int n_rows_;

		Matrix() : Matrix(0, 0) {
			init(0, 0);
		}

		explicit Matrix(int n_cols, int n_rows, T value) 
		{
			init(n_cols, n_rows);
			fill(value);
		}

		explicit Matrix(int n_cols, int n_rows) 
		{
			init(n_cols, n_rows);
		}

		inline bool operator==(const Matrix& other) const noexcept
		{
			return
				(this->n_cols_ == other.n_cols_) &&
				(this->n_rows_ == other.n_rows_) &&
				(this->data_ == other.data_);
		}

		Matrix& operator=(Matrix other)
		{
			this->n_rows_ = other.n_rows_;
			this->n_cols_ = other.n_cols_;
			this->data_ = std::vector<T>(other.data_);
			return *this;
		}

		[[nodiscard]] int get_n_cols() const {
			return this->n_cols_;
		}

		[[nodiscard]] int get_n_rows() const {
			return this->n_rows_;
		}

		[[nodiscard]] int pos(int col, int row) const noexcept {
			if constexpr (DEBUG) {
				if (col > this->n_cols_) {
					std::cerr << "Matrix.pos: invalid dimensions: column > n_cols_ (column = " << col << ", n_cols_ = " << this->n_cols_ << ")" << std::endl;
					__debugbreak();
				}
				if (row > this->n_rows_) {
					std::cerr << "Matrix.pos: invalid dimensions: row > n_rows_ (row = " << row << ", n_rows_ = " << this->n_rows_ << ")" << std::endl;
					__debugbreak();
				}
			}
			return (row * this->n_cols_) + col;
		}


		[[nodiscard]] const T& get(int col, int row) const {
			const int idx = this->pos(col, row);
			return this->data_.at(idx);
		}

		[[nodiscard]] T& get(int col, int row) {
			const int idx = this->pos(col, row);
			return this->data_.at(idx);
		}

		[[nodiscard]] const T& get(MatrixKey key) const {
			return this->get(get_col(key), get_row(key));
		}

		[[nodiscard]] T& get(MatrixKey key) {
			return this->get(get_col(key), get_row(key));
		}

		void set(int col, int row, const T& value) {
			const int idx = this->pos(col, row);
			this->data_.at(idx) = value;
		}

		void set(int col, int row, T&& value) {
			const int idx = this->pos(col, row);
			this->data_.at(idx) = std::move(value);
		}

		void fill(T value) {
			for (int col = 0; col < this->n_cols_; ++col) {
				for (int row = 0; row < this->n_rows_; ++row) {
					this->set(col, row, value);
				}
			}
		}

		[[nodiscard]] std::string pretty_print(bool colour, tools::PrintType pt) const {
			std::stringstream ss;
			ss << "(#columns = " << this->n_cols_ << "; #rows = " << this->n_rows_ << ")" << std::endl;
			ss << ((colour) ? "\u001b[0m" : ""); // reset colour

			for (int row = 0; row < this->n_rows_; ++row) {
				for (int column = 0; column < this->n_cols_; ++column) {
					ss << amx::tools::pretty_print_value(this->get(column, row), column, colour, pt);
				}
				ss << std::endl;
			}
			return ss.str();
		}
	};
}