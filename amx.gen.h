#pragma once
#include <array>
#include <cstdint>
#include <intrin.h>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "amx.amx_matrix.h"
#include "amx.tools.h"
#include "amx.types.h"


namespace amx::gen {

	enum Instruct {
		TDPBF16PS,
		TDPBSSD
	};

	inline std::string to_string(Instruct i) {
		switch (i) {
		case TDPBF16PS: return "tdpbf16ps";
		case TDPBSSD: return "tdpbssd";
		default: return "TODO";
		}
	}

	using UpdateClause = uint64_t;

	class RegFile {
	private:
		std::array<MatrixKey, 8> data_;
	public:

		RegFile() {
			data_.fill(0);
		}

		[[nodiscard]] bool contains(const MatrixKey name) const {
			for (int i = 0; i < 8; ++i) {
				if (this->data_[i] == name) {
					return true;
				}
			}
			return false;
		}

		void remove(const MatrixKey& name) {
			for (int i = 0; i < 8; ++i) {
				if (this->data_[i] == name) {
					this->data_[i] = 0;
				}
			}
		}

		[[nodiscard]] bool has_free_slot() {
			for (int i = 0; i < 8; ++i) {
				if (this->data_[i] == 0) {
					return true;
				}
			}
			return false;
		}

		[[nodiscard]] bool free_one_slot(MatrixKey keep_name_1, MatrixKey keep_name_2) {
			for (int i = 0; i < 8; ++i) {
				const MatrixKey key = this->data_[i];
				if (key == 0) {
					return true; // unexpected, but found an empty slot, no register needs to be evicted
				}
				if ((key != keep_name_1) && (key != keep_name_2) && !has_type_C(key)) {
					this->data_[i] = 0; // evict register at position i
					return true;
				}
			}
			return false;
		}

		[[nodiscard]] int add(MatrixKey name) {
			const int reg = get(name);
			if (reg == -1) {
				for (int i = 0; i < 8; i++) {
					if (this->data_[i] == 0) {
						this->data_[i] = name;
						return i;
					}
				}
				__debugbreak();
				return -1;
			}
			else {
				return reg;
			}
		}

		[[nodiscard]] int get(MatrixKey name) const {
			for (int i = 0; i < 8; ++i) {
				if (this->data_[i] == name) {
					return i;
				}
			}
			return -1;
		}

		[[nodiscard]] std::string to_string() const {
			std::stringstream ss;
			for (int i = 0; i < 8; ++i) {
				ss << i << "=" << to_string_matrix_key(this->data_[i]) << " ";
			}
			return ss.str();
		}
 
	};

	inline [[nodiscard]] std::string to_string(UpdateClause uc) {
		return "C[" + 
			std::to_string((uc >> 0 * 8) & 0xFF) + "][" + 
			std::to_string((uc >> 1 * 8) & 0xFF) + "] += A[" + 
			std::to_string((uc >> 2 * 8) & 0xFF) + "][" +
			std::to_string((uc >> 3 * 8) & 0xFF) + "] * B[" +
			std::to_string((uc >> 4 * 8) & 0xFF) + "][" +
			std::to_string((uc >> 5 * 8) & 0xFF) + "]";
	}

	inline [[nodiscard]] constexpr UpdateClause create_UpdateClause(int col_C, int row_C, int col_A, int row_A, int col_B, int row_B) noexcept {
		return
			((static_cast<UpdateClause>(col_C) & 0xFF) << 0 * 8) |
			((static_cast<UpdateClause>(row_C) & 0xFF) << 1 * 8) |
			((static_cast<UpdateClause>(col_A) & 0xFF) << 2 * 8) |
			((static_cast<UpdateClause>(row_A) & 0xFF) << 3 * 8) |
			((static_cast<UpdateClause>(col_B) & 0xFF) << 4 * 8) |
			((static_cast<UpdateClause>(row_B) & 0xFF) << 5 * 8);
	}

	inline [[nodiscard]] constexpr MatrixKey get_MatrixKey_C(UpdateClause uc) noexcept {
		return make_matrix_key_C((uc >> 0 * 8) & 0xFF, (uc >> 1 * 8) & 0xFF);
	}
	inline [[nodiscard]] constexpr MatrixKey get_MatrixKey_A(UpdateClause uc) noexcept {
		return make_matrix_key_A((uc >> 2 * 8) & 0xFF, (uc >> 3 * 8) & 0xFF);
	}
	inline [[nodiscard]] constexpr MatrixKey get_MatrixKey_B(UpdateClause uc) noexcept {
		return make_matrix_key_B((uc >> 4 * 8) & 0xFF, (uc >> 5 * 8) & 0xFF);
	}

	inline [[nodiscard]] std::vector<UpdateClause> generate_matmul_spec(int n, int m, int k) {
		std::vector<UpdateClause> result;
		for (int j = 0; j < n; ++j) {
			for (int i = 0; i < m; ++i) {
				for (int p = 0; p < k; ++p) {
					result.push_back(create_UpdateClause(j, i, p, i, p, j));
				}
			}
		}
		std::cout << "size: " << result.size() << std::endl;

		return result;
	}

	inline [[nodiscard]] std::vector<UpdateClause> generate_tdpbssd_tile_spec(int n, int m, int k)
	{
		return generate_matmul_spec(n / 16, m / 16, k / 64);
	}

	inline [[nodiscard]] std::vector<UpdateClause> generate_tdpbf16ps_tile_spec(int n, int m, int k)
	{
		return generate_matmul_spec(n / 16, m / 16, k / 32);
	}

	struct LastUsage {
		std::unordered_map<MatrixKey, int> map_A_;
		std::unordered_map<MatrixKey, int> map_B_;
		std::unordered_map<MatrixKey, int> map_C_;

		LastUsage(const std::vector<UpdateClause>& data) {
			const int n_elements = static_cast<int>(data.size());

			std::vector<MatrixKey> data_A = std::vector<MatrixKey>(n_elements);
			std::vector<MatrixKey> data_B = std::vector<MatrixKey>(n_elements);
			std::vector<MatrixKey> data_C = std::vector<MatrixKey>(n_elements);

			for (int i = 0; i < n_elements; ++i) {
				const UpdateClause uc = data[i];
				data_A[i] = get_MatrixKey_A(uc);
				data_B[i] = get_MatrixKey_B(uc);
				data_C[i] = get_MatrixKey_C(uc);
			}

			for (int j = 0; j < n_elements; ++j) {
				const MatrixKey kA = data_A[j];
				const MatrixKey kB = data_B[j];
				const MatrixKey kC = data_C[j];

				if (!this->map_A_.contains(kA)) {
					for (int i = n_elements - 1; i >= 0; --i) {
						if (data_A[i] == kA) {
							this->map_A_.insert({ kA, i });
							break;
						}
					}
				}
				if (!this->map_B_.contains(kB)) {
					for (int i = n_elements - 1; i >= 0; --i) {
						if (data_B[i] == kB) {
							this->map_B_.insert({ kB, i });
							break;
						}
					}
				}
				if (!this->map_C_.contains(kC)) {
					for (int i = n_elements - 1; i >= 0; --i) {
						if (data_C[i] == kC) {
							this->map_C_.insert({ kC, i });
							break;
						}
					}
				}
			}
		}

		[[nodiscard]] int get_A(MatrixKey key) const {
			return this->map_A_.at(key);
		}
		[[nodiscard]] int get_B(MatrixKey key) const {
			return this->map_B_.at(key);
		}
		[[nodiscard]] int get_C(MatrixKey key) const {
			return this->map_C_.at(key);
		}
	};

	[[nodiscard]] std::string pretty_print_offset(int offset) {
		const int multiple = offset / 1024;
		if ((multiple * 1024) == offset) {
			return std::to_string(multiple) + "*1024";
		}
		return std::to_string(offset);
	}

	// ugly method to extract the load key from the line of code
	std::string get_load_key(const std::string& line) {		
		std::string result = "";
		if (line[1] == ' ') {
			for (int i = 1; i < line.size(); ++i) {
				if ((line[i + 0] == 'l') &&
					(line[i + 1] == 'o') &&
					(line[i + 2] == 'a') &&
					(line[i + 3] == 'd') &&
					(line[i + 4] == ' ')) 
				{
					bool processed_first_bracket = false;
					for (int j = i + 5; j < line.size(); ++j) {
						result += line[j];
						if (line[j] == ']') {
							if (processed_first_bracket) {
								return result;
							}
							processed_first_bracket = true;
						}
					}
				}
			}
		}
		return result;
	}


	template <bool GEN_CODE>
	[[nodiscard]] std::tuple<bool, std::string, int, int, int, int> compile(
		int N, int M, int K,
		Instruct i,
		const std::vector<UpdateClause>& data
	) {
		std::vector<std::string> lines;

		std::unique_ptr<AmxMatrix<BF16>> A;
		std::unique_ptr<AmxMatrix<BF16>> B;
		std::unique_ptr<AmxMatrix<FP32>> C;

		if constexpr (GEN_CODE) {
			A = std::make_unique<AmxMatrix<BF16>>(K, N);
			B = std::make_unique<AmxMatrix<BF16>>(K, M);
			C = std::make_unique<AmxMatrix<FP32>>(N, M);

			if (true) {
				lines.push_back("  ; specification:");
				for (UpdateClause up : data) {
					lines.push_back("  ; " + to_string(up));
				}
				lines.push_back("\n");
			}
		}

		if constexpr (GEN_CODE) {
			lines.push_back("  mov         r10d, 64     ; stride is always 64");
		}

		int n_loads = 0;
		int n_stores = 0;
		int n_spills = 0;
		int n_reuse = 0;

		const LastUsage lu(data);
		RegFile reg_file;

		for (int pos = 0; pos < data.size(); ++pos) {
			UpdateClause uc = data[pos];
			const MatrixKey kA = get_MatrixKey_A(uc);
			const MatrixKey kB = get_MatrixKey_B(uc);
			const MatrixKey kC = get_MatrixKey_C(uc);

			if constexpr (GEN_CODE) {
				lines.push_back("  ; registers: " + reg_file.to_string());
			}
			int rA, rB, rC;
			{
				std::string line = "";
				if (reg_file.contains(kA)) {
					rA = reg_file.get(kA);
					if constexpr (GEN_CODE) line += " ;";
					n_reuse++;
				}
				else {
					if (!reg_file.has_free_slot()) {
						//if (!reg_file.free_one_slot_2(kA, kB, pos, data)) {
						if (!reg_file.free_one_slot(kA, kB)) {
							return { false, "", 0,  0,  0,  0 };
						}
						n_spills++;
					}
					rA = reg_file.add(kA);
					if constexpr (GEN_CODE) line += "  ";
					n_loads++;
				}
				if constexpr (GEN_CODE) {
					line += "tileloadd   tmm" + std::to_string(rA) + ", [rdx + r10 + " + pretty_print_offset(A->get_offset(kA)) + "]	; load " + to_string_matrix_key(kA);
					lines.push_back(line);
				}
			}
			{
				std::string line = "";
				if (reg_file.contains(kB)) {
					rB = reg_file.get(kB);
					if constexpr (GEN_CODE) line += " ;";
					n_reuse++;
				}
				else {
					if (!reg_file.has_free_slot()) {
						if (!reg_file.free_one_slot(kA, kB)) {
							return { false, "", 0,  0,  0,  0 };
						}
						n_spills++;
					}
					rB = reg_file.add(kB);
					if constexpr (GEN_CODE) line += "  ";
					n_loads++;
				}
				if constexpr (GEN_CODE) {
					line += "tileloadd   tmm" + std::to_string(rB) + ", [r8  + r10 + " + pretty_print_offset(B->get_offset(kB)) + "]	; load " + to_string_matrix_key(kB);
					lines.push_back(line);
				}
			}
			{
				std::string line = "";
				if (reg_file.contains(kC)) {
					rC = reg_file.get(kC);
					if constexpr (GEN_CODE) line += " ;";
					n_reuse++;
				}
				else {
					if (!reg_file.has_free_slot()) {
						if (!reg_file.free_one_slot(kA, kB)) {
							return { false, "", 0,  0,  0,  0 };
						}
						n_spills++;
					}
					rC = reg_file.add(kC);
					if constexpr (GEN_CODE) line += "  ";
					n_loads++;
				}
				if constexpr (GEN_CODE) {
					line += "tileloaddt1 tmm" + std::to_string(rC) + ", [rcx + r10 + " + pretty_print_offset(C->get_offset(kC)) + "]	; load " + to_string_matrix_key(kC);
					lines.push_back(line);
				}
			}

			if constexpr (GEN_CODE) {
				lines.push_back("  " + to_string(i) + "   tmm" + std::to_string(rC) + ", tmm" + std::to_string(rA) + ", tmm" + std::to_string(rB) + "				; " + to_string(uc));
			}

			// check if key_A is used in the future, if not it can be removed from the register file
			if (lu.get_A(kA) <= pos) {
				reg_file.remove(kA);
			}
			if (lu.get_B(kB) <= pos) {
				reg_file.remove(kB);
			}
			{
				std::string line = "";
				if (lu.get_C(kC) <= pos) {
					reg_file.remove(kC);
					if constexpr (GEN_CODE) {
						line += "  ";
					}
					n_stores++;
				}
				else {
					if constexpr (GEN_CODE) {
						line += " ;";
					}
				}
				if constexpr (GEN_CODE) {
					line += "tilestored  [rcx + r10 + " + pretty_print_offset(C->get_offset(kC)) + "], tmm" + std::to_string(rC) + " 	; store " + to_string_matrix_key(kC);
					lines.push_back(line);
					lines.push_back("");
				}
			}
		}
		if constexpr (GEN_CODE) {
			std::stringstream ss;

			for (int k = 0; k < lines.size(); ++k) {

				if (const std::string load_key = get_load_key(lines[k]); load_key != "") {
					bool final_load = true;

					for (int j = k + 1; j < lines.size(); ++j) {
						if (load_key == get_load_key(lines[j])) {
							final_load = false;
							break;
						}
					}
					if (final_load) {
						lines[k] = std::regex_replace(lines[k], std::regex("tileloadd   "), "tileloaddt1 ");
					}
				}
			}
			for (const std::string& line : lines) {
				ss << line << std::endl;
			}
			return { true, ss.str(), n_loads, n_stores, n_spills, n_reuse };
		}
		else {
			return { true, "", n_loads, n_stores, n_spills, n_reuse };
		}
	}

	inline [[nodiscard]] std::string compile_asm(
		Instruct i,
		int N, int M, int K,
		const std::vector<UpdateClause>& data)
	{
		std::stringstream ss;

		{
			ss << "BITS     64" << std::endl;
			ss << "ALIGN	 8, nop" << std::endl;
			ss << std::endl;

			ss << "SECTION .data" << std::endl;
			ss << std::endl;

			ss << ";   struct Tile_config {" << std::endl;
			ss << ";        uint8_t palette_id;" << std::endl;
			ss << ";        uint8_t start_row;" << std::endl;
			ss << ";        uint8_t reserved_0[14];" << std::endl;
			ss << ";        uint16_t colsb[8];" << std::endl;
			ss << ";        uint16_t reserved_1[8];" << std::endl;
			ss << ";        uint8_t rows[8];" << std::endl;
			ss << ";        uint8_t reserved_2[8];" << std::endl;
			ss << std::endl;

			ss << "config:" << std::endl;
			ss << "	db 1,0," << std::endl;
			ss << "	db 0,0,0,0,0,0,0,0,0,0,0,0,0,0 ; reserved_0" << std::endl;
			ss << "	dw 64,64,64,64,64,64,64,64     ; colsb[8];" << std::endl;
			ss << "	dw 0,0,0,0,0,0,0,0             ; reserved_1" << std::endl;
			ss << "	db 16,16,16,16,16,16,16,16     ; rows[8];" << std::endl;
			ss << "	db 0,0,0,0,0,0,0,0             ; reserved_2" << std::endl;
			ss << std::endl;
		}
		const std::string name = to_string(i) + "_N" + std::to_string(N) + "_M" + std::to_string(M) + "_K" + std::to_string(K) + "_asm";

		ss << ";----------------------------------------------------------------------------" << std::endl;
		ss << "; extern \"C\" void " << name << "(void* c, void* a, void* b); //RCX, RDX, R8, R9" << std::endl;
		ss << "GLOBAL " << name << std::endl;
		ss << std::endl;
		ss << "SECTION .text" << std::endl;
		ss << name << ":" << std::endl;
		ss << std::endl;

		ss << "  ldtilecfg   [config]" << std::endl;
		ss << std::endl;

		auto [success, code, n_loads, n_stores, n_spills, n_reuse] = compile<true>(N, M, K, i, data);

		if (success) {
			const int Nt = N / 16;
			const int Mt = M / 16;
			const int Kt = K / 32;


			const int min_n_stores = (Nt * Mt);
			const int min_n_loads = (Nt * Mt) + (Nt * Kt) + (Mt * Kt);

			ss << "  ; n_stores = " << n_stores << " (min " << min_n_stores << "); ";
			ss << "n_loads = " << n_loads << " (min " << min_n_loads << "); ";
			ss << "n_spills " << n_spills << "; ";
			ss << "n_reuse = " << n_reuse << "" << std::endl;
			ss << "  ; ========================================" << std::endl;
			ss << code;
		}
		else {
			ss << "  ; ERROR" << std::endl;
		}
		ss << "  tilerelease" << std::endl;
		ss << "  ret" << std::endl;
		ss << std::endl;

		return ss.str();
	}

	inline [[nodiscard]] std::vector<UpdateClause> minimize_greedy_swap(
		int N, int M, int K,
		const std::vector<UpdateClause>& data_in, 
		int window_size = -1
	) {
		if (window_size == -1) window_size = 1000000000;

		std::vector<UpdateClause> data = data_in;
		const int n_elements = static_cast<int>(data.size());

		std::cout << "n_elements = " << n_elements << std::endl;

		int min_n_spills = 1000000;
		bool changed = true;
		while (changed) {
			changed = false;
			for (int i = 0; i < n_elements; ++i) {
				for (int j = i + 1; j < std::min(i+window_size, n_elements); ++j) {
					UpdateClause tmp_i = data[i];
					UpdateClause tmp_j = data[j];
					data[i] = tmp_j;
					data[j] = tmp_i;
					auto [success, _, n_loads, n_stores, n_spills, n_reuse] = compile<false>(N, M, K, Instruct::TDPBF16PS, data);
					if (success && (n_spills < min_n_spills)) {
						min_n_spills = n_spills;
						std::cout << "new n_spills = " << n_spills << " (swapped " << i << " with " << j << ")" << std::endl;
						changed = true;
					}
					else {
						data[i] = tmp_i;
						data[j] = tmp_j;
					}
				}
			}
		}
		return data;
	}

	inline [[nodiscard]] std::vector<UpdateClause> minimize_non_greedy_swap(
		int N, int M, int K,
		const std::vector<UpdateClause>& data_in,
		int window_size = -1
	) {
		if (window_size == -1) window_size = 1000000000;

		std::vector<UpdateClause> data = data_in;
		const int n_elements = static_cast<int>(data.size());

		std::cout << "n_elements = " << n_elements << std::endl;

		int min_n_spills = 1000000;
		int best_i = -1;
		int best_j = -1;

		bool changed = true;
		while (changed) {
			changed = false;

			for (int i = 0; i < n_elements; ++i) {
				for (int j = i + 1; j < std::min(i + window_size, n_elements); ++j) {
					const UpdateClause tmp_i = data[i];
					const UpdateClause tmp_j = data[j];
					data[i] = tmp_j;
					data[j] = tmp_i;
					auto [success, _, n_loads, n_stores, n_spills, n_reuse] = compile<false>(N, M, K, Instruct::TDPBF16PS, data);
					if (success && (n_spills < min_n_spills)) {
						min_n_spills = n_spills;
						best_i = i;
						best_j = j;
						changed = true;
						//std::cout << "new n_spills = " << n_spills << " (swapped " << i << " with " << j << ")" << std::endl;
					}
					data[i] = tmp_i;
					data[j] = tmp_j;
				}
			}
			if (changed) {
				const UpdateClause tmp_i = data[best_i];
				const UpdateClause tmp_j = data[best_j];
				data[best_i] = tmp_j;
				data[best_j] = tmp_i;
				std::cout << "new n_spills = " << min_n_spills << " (swapped " << best_i << " with " << best_j << ")" << std::endl;
			}
		}
		return data;
	}

	inline [[nodiscard]] std::vector<UpdateClause> generate_tdpbf16ps_dynamic_profile(int N, int M, int K) {

		int Nt = N / 16;
		int Mt = M / 16;
		int Kt = K / 32;

		std::vector<UpdateClause> result;
		const bool use_2x2_tiles = ((Mt & 0b1) == 0) && ((Nt & 0b1) == 0);

		if (use_2x2_tiles) {
			for (int i = 0; i < Mt; i += 2) {
				for (int j = 0; j < Nt; j += 2) {
					//FP32* c_ptr00 = C.get_tile(j + 0, i + 0).data();
					//FP32* c_ptr10 = C.get_tile(j + 1, i + 0).data();
					//FP32* c_ptr01 = C.get_tile(j + 0, i + 1).data();
					//FP32* c_ptr11 = C.get_tile(j + 1, i + 1).data();
					//_tile_stream_loadd(0, c_ptr00, 64); // 0 = C[0][0]
					//_tile_stream_loadd(2, c_ptr01, 64); // 2 = C[0][1]
					//_tile_stream_loadd(1, c_ptr10, 64); // 1 = C[1][0]
					//_tile_stream_loadd(3, c_ptr11, 64); // 3 = C[1][1]

					for (int p = 0; p < Kt; ++p) {
						//_tile_loadd(4, A.get_tile(p, i + 0).data(), 64); // 4 = A[0][0]
						//_tile_loadd(6, B.get_tile(p, j + 0).data(), 64); // 6 = B[0][0]
						//_tile_loadd(5, A.get_tile(p, i + 1).data(), 64); // 5 = A[0][1]
						//_tile_loadd(7, B.get_tile(p, j + 1).data(), 64); // 7 = B[0][1]

						//_tile_dpbf16ps(0, 4, 6); // C[0][0] += A[0][0] * B[0][0]
						result.push_back(gen::create_UpdateClause(j + 0, i + 0, p, i + 0, p, j + 0));
						//_tile_dpbf16ps(2, 5, 6); // C[0][1] += A[0][1] * B[0][0]
						result.push_back(gen::create_UpdateClause(j + 0, i + 1, p, i + 1, p, j + 0));
						//_tile_dpbf16ps(1, 4, 7); // C[1][0] += A[0][0] * B[0][1]
						result.push_back(gen::create_UpdateClause(j + 1, i + 0, p, i + 0, p, j + 1));
						//_tile_dpbf16ps(3, 5, 7); // C[1][1] += A[0][1] * B[0][1]
						result.push_back(gen::create_UpdateClause(j + 1, i + 1, p, i + 1, p, j + 1));
					}
					//_tile_stored(0, c_ptr00, 64);
					//_tile_stored(2, c_ptr01, 64);
					//_tile_stored(1, c_ptr10, 64);
					//_tile_stored(3, c_ptr11, 64);
				}
			}
			//std::cout << "tdpbf16ps_intrin_amx: Nt=" << N << "; Mt=" << M << "; Kt=" << K << ": n_loads_AB=" << n_loads_AB << "; n_loads_C=" << n_loads_C << std::endl;
		}
		else {
			__debugbreak();
		}
		return result;
	}

	inline void generate_tdpbf16ps(int N, int M, int K, bool optimize, const std::string& path) {
		//std::vector<UpdateClause> data = generate_tdpbf16ps_tile_spec(N, M, K);
		std::vector<UpdateClause> data = generate_tdpbf16ps_dynamic_profile(N, M, K);

		if (optimize) {
			data = minimize_greedy_swap(N, M, K, data, 9);
			//data = minimize_non_greedy_swap(N, M, K, data, 5);
		}
		const std::string content_asm = compile_asm(Instruct::TDPBF16PS, N, M, K, data);
		const std::string filename = path + "tdpbf16ps_N" + std::to_string(N) + "_M" + std::to_string(M) + "_K" + std::to_string(K) + ".asm";
		amx::tools::save_code_ansi(filename, content_asm);
	}

	inline void generate_all(const std::string& path_asm) 
	{
		if (true) {
			generate_tdpbf16ps(32, 32, 64, true, path_asm);
		}
		if (true) {
			generate_tdpbf16ps(1 * 64, 1 * 64, 1 * 64, true, path_asm);
		}
		if (true) {
			generate_tdpbf16ps(2 * 64, 2 * 64, 2 * 64, true, path_asm);
		}
		if (true) {
			generate_tdpbf16ps(4 * 64, 4 * 64, 4 * 64, true, path_asm);
		}
		if (true) {
			generate_tdpbf16ps(8 * 64, 8 * 64, 8 * 64, false, path_asm);
		}
	}
}