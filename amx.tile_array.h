#pragma once
#include <corecrt_malloc.h>
#include <cstddef>
#include <intrin.h>

#include "amx.tools.h"

namespace amx {

	class TileArray {

	private:
		int const n_cols_;
		int const n_rows_;
		std::byte* const ptr_; // the pointer shall not change!

	public:
		TileArray() : n_cols_(0), n_rows_(0), ptr_(nullptr) {};
		TileArray(int n_tile_cols, int n_tile_rows) :
			n_cols_(n_tile_cols),
			n_rows_(n_tile_rows),
			ptr_(static_cast<std::byte*>(_aligned_malloc( n_tile_cols * n_tile_rows * 1024, 1024)))
		{}

		[[nodiscard]] int get_offset(int tile_col, int tile_row) const {
			if constexpr (DEBUG) {
				if ((tile_col >= this->n_cols_) || (tile_col < 0) || (tile_row >= this->n_rows_) || (tile_row < 0)) {
					__debugbreak();
				}
			}
			return ((tile_col * this->n_rows_) + tile_row) * 1024;
		}

		template <typename T = std::byte>
		[[nodiscard]] const T* get_ptr(int tile_col, int tile_row) const {
			return reinterpret_cast<T*>(this->ptr_ + this->get_offset(tile_col, tile_row));
		}

		template <typename T = std::byte>
		[[nodiscard]] T* get_ptr(int tile_col, int tile_row) {
			return reinterpret_cast<T*>(this->ptr_ + this->get_offset(tile_col, tile_row));
		}

		template <typename T = std::byte>
		[[nodiscard]] const T* data() const {
			return reinterpret_cast<const T*>(this->ptr_);
		}

		template <typename T = std::byte>
		[[nodiscard]] T* data() {
			return reinterpret_cast<T*>(this->ptr_);
		}

		~TileArray() {
			if (this->ptr_) _aligned_free(this->ptr_);
		}
	};
}