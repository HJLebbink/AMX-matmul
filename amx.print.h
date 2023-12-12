#pragma once
#include <sstream>
#include <ios> //hex
#include <iomanip> //setw
#include <type_traits>
#include <cstdint>
#include <iosfwd>
#include <ostream>

#include "amx.types.h"

namespace amx::tools {

    enum PrintType {
        dec,
        hex,
        bf16   // assume content is BF16 and cast to FP32 and print
    };

    template <typename T>
    [[nodiscard]] std::string pretty_print_value(T value, int column, bool colour, PrintType pt) {
        std::stringstream ss;

        if constexpr (std::is_arithmetic_v<T>) {
            ss << ((colour && (value != 0)) ? "\u001b[31m" : "");
            if ((pt == PrintType::hex) && (sizeof(T) == 1)) {
                ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<uint32_t>(static_cast<uint8_t>(value));
            }
            else if ((pt == PrintType::bf16) && (sizeof(T) == 2)) {
                ss << std::fixed << std::setprecision(4) << bf16_to_float(static_cast<BF16>(value));
            }
            else {
                if constexpr (sizeof(T) == 1) {
                    ss << static_cast<int>(value);
                }
                else {
                    ss << value;
                }
            }
            ss << ((colour) ? "\u001b[0m" : ""); // reset colour
        }
        else {
            ss << value;
        }
        
        ss << (((column & 0b11) == 0b11) ? "'" : " ");
        return ss.str();
    }
}