#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <iosfwd>
#include <typeinfo>

#include "amx.types.h"
#include "amx.print.h"
#include "amx.tools.h"
#include <utility>
#include <cstdint>
#include <type_traits>
#include <bit>

namespace amx {

	template <typename STORAGE>
	class Tile {

	private:

		STORAGE* const ptr_;

		template <bool CHECK>
		constexpr [[nodiscard]] int offset(int col, int row) const noexcept
		{
			if constexpr (sizeof(STORAGE) == 4) {
				if constexpr (CHECK) {
					if ((col < 0) || (row < 0) || (col > 15) || (row > 15)) {
						__debugbreak();
					}
				}
				return (row << 4) | col;
			}
			else if constexpr (sizeof(STORAGE) == 2) {
				if constexpr (CHECK) {
					if ((col < 0) || (row < 0) || (col > 32) || (row > 15)) {
						__debugbreak();
					}
				}
				return (row << 5) | col;
			}
			else if constexpr (sizeof(STORAGE) == 1) {
				if constexpr (CHECK) {
					if ((col < 0) || (row < 0) || (col > 64) || (row > 15)) {
						__debugbreak();
					}
				}
				return (row << 6) | col;
			}
			else {
				__debugbreak();
			}
		}

	public:

		Tile() : ptr_(nullptr) {}
		Tile(STORAGE* ptr) : ptr_(ptr) {}

		bool operator==(const Tile<STORAGE>& other) const {
			return std::memcmp(this->ptr_, other.ptr_, 1024) == 0;
		}

		template <typename T = STORAGE>
		[[nodiscard]] const T* const data() const noexcept {
			return reinterpret_cast<T*>(this->ptr_);
		}

		template <typename T = STORAGE>
		[[nodiscard]] T* const data() noexcept {
			return reinterpret_cast<T*>(this->ptr_);
		}

		template <bool CHECK = true>
		void set(int col, int row, STORAGE value) {
			this->ptr_[this->offset<CHECK>(col, row)] = value;
		}

		template <bool CHECK = true>
		[[nodiscard]] STORAGE get(int col, int row) const {
			return this->ptr_[this->offset<CHECK>(col, row)];
		}

		void clear() {
			std::memset(this->ptr_, 0, 1024);
		}

		[[nodiscard]] std::string pretty_print(bool colour = true, tools::PrintType pt = tools::PrintType::dec) const {
			std::stringstream ss;
			if constexpr (std::is_same_v<STORAGE, BF16>) {
				ss << "tile BF16:";
			}
			else if constexpr (std::is_same_v<STORAGE, Int8>) {
				ss << "tile INT8:";
			}
			else if constexpr (std::is_same_v<STORAGE, FP32>) {
				ss << "tile FP32:";
			}
			else if constexpr (std::is_same_v<STORAGE, Int32>) {
				ss << "tile Int32:";
			}
			else {
				ss << "tile " << typeid(STORAGE).name() << ":";
			}
			ss << std::endl;
			ss << ((colour) ? "\u001b[0m" : ""); // reset colour
			constexpr int n_cols = calc_n_cols<STORAGE>();
			for (int row = 0; row < 16; ++row) {
				for (int col = 0; col < n_cols; ++col) {
					const STORAGE value = this->get<DEBUG>(col, row);
					ss << tools::pretty_print_value(value, col, colour, pt);
				}
				ss << std::endl;
			}
			return ss.str();
		}
	};

	// approximate equality, if the absolute difference of all elements the tiles is smaller than a 
	// certain percentage of the highest element, then the matrices are considered approximate equal.
	inline [[nodiscard]] bool approx_equal(const Tile<FP32>& a, const Tile<FP32>& b, float precision_percentage) {
		const uint32_t* ptr_a = a.data<uint32_t>();
		const uint32_t* ptr_b = b.data<uint32_t>();

		for (int i = 0; i < 256; ++i) {
			if (ptr_a[i] != ptr_b[i]) {
				const FP32 av = std::bit_cast<FP32>(ptr_a[i]);
				const FP32 bv = std::bit_cast<FP32>(ptr_b[i]);
				const FP32 sub = av - bv;
				const FP32 diff = std::abs(sub);
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

	template <typename T>
	void subtract(INOUT Tile<T>& a, const Tile<T>& b) {
		constexpr int n_cols = calc_n_cols<T>();
		for (int col = 0; col < 16; ++col) {
			for (int row = 0; row < n_cols; ++row) {
				const T av = a.get<DEBUG>(col, row);
				const T bv = b.get<DEBUG>(col, row);
				const T sub = av - bv;
				a.set<DEBUG>(col, row, sub);
			}
		}
	}
}
