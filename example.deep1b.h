#pragma once
#include <string>
#include <iosfwd>
#include <fstream>
#include <ios>
#include <iostream>
#include <array>
#include <istream>
#include <cstdint>

namespace example {

	void load_fbin(const std::string& filename) {
		std::ifstream input(filename, std::ios::in | std::ios::binary);
		if (!input.is_open()) {
			std::cout << "Could not open " << filename << std::endl;
			return;
		}

		constexpr int buf_size = 96;
		std::array<float, buf_size> buf{};

		input.seekg(5 * sizeof(int));

		for (int j = 0; j < 10; ++j) {
			input.read(reinterpret_cast<char*>(buf.data()), buf_size * sizeof(float));

			int64_t index;
			input.read(reinterpret_cast<char*>(&index), sizeof(uint64_t));



			std::cout << std::endl << "index: " << index << ": ";
			for (int i = 0; i < buf_size; i++) {
				std::cout << i << ":" << buf[i] << " ";
			}
		}
	}

	void run() {
		//load_fbin("C:\\Data\\sneller\\deep1b\\query.public.10K.fbin");
		load_fbin("C:\\Data\\sneller\\deep1b\\base.1B.fbin");
	}
}