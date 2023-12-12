#pragma once
#include <string>
#include <cstdint>
#include <sstream>

namespace amx {

    // tile config data structure 
    struct Tile_config {
        uint8_t palette_id;
        uint8_t start_row;
        uint8_t reserved_0[14];
        uint16_t colsb[8];
        uint16_t reserved_1[8];
        uint8_t rows[8];
        uint8_t reserved_2[8];
    };
}