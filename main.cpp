#include <array>
#include <chrono>
#include <cstdint>
#include <immintrin.h>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "amx.benchmark.h"
#include "amx.gen.h"
#include "amx.test.h"
#include "amx.tile.h"
#include "amx.tile_config.h"
#include "amx.tmul.spr.h"
#include "amx.tools.h"
#include "amx.types.h"

#include "tools.timing.h"
#include "vecmul_example.h"
#include "example.vecmul.h"
#include <intrin.h>

namespace amx {
 
    inline void print_influence(int column, int row) {
        constexpr int M = 16;
        constexpr int N = 16;
        constexpr int K = 64;

        std::unordered_set<std::string> results;

        Tile<Int8> a = Tile<Int8>();
        Tile<Int8> b = Tile<Int8>();
        Tile<Int32> c = Tile<Int32>();

        for (int column1 = 0; column1 < K; ++column1) {
            for (int row1 = 0; row1 < M; ++row1) {
                for (int column2 = 0; column2 < K; ++column2) {
                    for (int row2 = 0; row2 < N; ++row2) {

                        a.clear();
                        b.clear();
                        c.clear();

                        a.set(column1, row1, 2);
                        b.set(column2, row2, 3);

                        tmul::spr::tdpbssd_intrin_amx(c, a, b);

                        const auto v = c.get(column, row);
                        if (v == 0) {
                            // do nothing
                        }
                        else if (v == 2 * 3) {

                            std::string str = "C[" + std::to_string(column) + "][" + std::to_string(row) + "] += A[" + std::to_string(column1) + "][" + std::to_string(row1) + "] * B[" + std::to_string(column2) + "][" + std::to_string(row2) + "]";
                            if (!results.contains(str)) {
                                std::cout << str << std::endl;
                                results.insert(str);
                            }
                            else {
                                std::cout << "ERROR: already present ?!" << std::endl;
                                return;
                            }
                        }
                        else {
                            std::cout << "ERROR" << std::endl;
                            std::cout << "column1 " << column1 << "; row1 " << row1 << "; column2 " << column2 << "; row2 " << row2 << std::endl;

                            std::cout << "A: " << a.pretty_print();
                            std::cout << "B: " << b.pretty_print();
                            std::cout << "C: " << c.pretty_print();
                            std::cout << "v = " << v << std::endl;
                            return;
                        }
                    }
                }
            }
        }
    }

    inline void print_influence2() {

        if (true) {
            /*
            FOR m := 0 TO 15
                FOR k := 0 TO 15
                    FOR n := 0 TO 15
                        c.row[m].fp32[n] += FP32(a.row[m].bf16[2*k+0]) * FP32(b.row[k].bf16[2*n+0])
                        c.row[m].fp32[n] += FP32(a.row[m].bf16[2*k+1]) * FP32(b.row[k].bf16[2*n+1])
                    ENDFOR
                ENDFOR
                write_row_and_zero(c, m, c.row[m], dst.colsb)
            ENDFOR
            */

            int min_m = 1;
            int min_n = 1;

            int max_m = 2;
            int max_n = 2;

            for (int m = min_m; m < max_m; ++m) {
                for (int k = 0; k < 16; ++k) {
                    for (int n = min_n; n < max_n; ++n) {
                        std::cout << "c[" << m << "][" << n << "] += a[" << m << "][" << ((2 * k) + 0) << "] * b[" << k << "][" << ((2 * n) + 0) << "]" << std::endl;
                        std::cout << "c[" << m << "][" << n << "] += a[" << m << "][" << ((2 * k) + 1) << "] * b[" << k << "][" << ((2 * n) + 1) << "]" << std::endl;
                    }
                }
            }
        }

        amx::Tile_config config = { 0 };
        {
            config.palette_id = 1;
            config.start_row = 0;

            config.rows[0] = 16;
            config.colsb[0] = 64;
            config.rows[1] = 16;
            config.colsb[1] = 64;
            config.rows[2] = 16;
            config.colsb[2] = 64;
        }

        _tile_loadconfig(&config);
        _tile_zero(0);

        std::array<uint16_t, 512> buf_a;
        uint16_t* ptr_a = buf_a.data();
        std::array<uint16_t, 512> buf_b;
        uint16_t* ptr_b = buf_b.data();
        std::array<float, 256> buf_c;
        float* ptr_c = buf_c.data();

        constexpr BF16 one = float_to_bf16(1.f);

        for (int column1a = 1; column1a < 32; ++column1a) {
            
            buf_a.fill(0);
            buf_a[column1a] = one;

            for (int column1b = 0; column1b < 32; ++column1b) {
                buf_b.fill(0);
                buf_b[column1b] = one;
               
                //__m512 a = _mm512_undefined();
                //__m512bh b = _mm512_undefined_ph();
                //__m512bh c = _mm512_undefined_ph();
                //_mm512_dpbf16_ps(a, b, c);

                _tile_zero(0);
                _tile_loadd(1, ptr_a, 32);
                _tile_loadd(2, ptr_b, 32);
                _tile_dpbf16ps(0, 1, 2);
                _tile_stored(0, ptr_c, 64);

                {
                    constexpr int x = 64;

                    std::cout << "A: ";
                    for (int i = 0; i < x; ++i) {
                        std::cout << bf16_to_float(ptr_a[i]) << " ";
                    }
                    std::cout << std::endl;
                    std::cout << "B: ";
                    for (int i = 0; i < x; ++i) {
                        std::cout << bf16_to_float(ptr_b[i]) << " ";
                    }
                    std::cout << std::endl;
                    std::cout << "C: ";
                    for (int i = 0; i < x; ++i) {
                        std::cout << ptr_c[i] << " ";
                    }
                    std::cout << std::endl << std::endl;
                }

                for (int column2 = 0; column2 < 32; ++column2) {
                    if (buf_c[column2] != 0) {
                        std::cout << "column1a " << column1a << "; column1b " << column1b << std::endl;
                    }
                }
            }
        }

        _tile_release();
    }
}

int main()
{
    const auto start = std::chrono::system_clock::now();
    std::cout << "Started at " << tools::timing::current_time_str() << std::endl;

    // Request permission to linux kernel to run AMX 
    if (!amx::tools::set_tiledata_use()) {
        std::cout << "Could not setup AMX" << std::endl;
    }

    // generate optimized assembly code by minimizing the number of memory spills
    if (true) {
        amx::gen::generate_all("C:\\Source\\Github\\AMX-matmul\\generated\\asm\\");
    }

    // run tests to determine the correctness of implementations
    if (false) {
        const int n_experiments = 100;
        amx::test::test_correctness_1x1x1_tiles_tdpbf16ps(n_experiments);
        amx::test::test_correctness_2x2x2_tiles_tdpbf16ps(n_experiments);
        amx::test::test_correctness_1x1x1_tiles_tdpbssd(n_experiments);
        amx::test::test_correctness_2x2x2_tiles_tdpbssd(n_experiments);
    }

    // run benchmark for graph
    if (false) {
        std::vector<int>dims;
        { // fill the vector of dimensions (N,M,K)
            dims.push_back(32);

            int dim = 64;
            for (int i = 0; i < 10; ++i) {
                dims.push_back(dim);
                dim += 1 * 64;
            }
            for (int i = 0; i < 10; ++i) {
                dims.push_back(dim);
                dim += 2 * 64;
            }
            for (int i = 0; i < 10; ++i) {
                dims.push_back(dim);
                dim += 4 * 64;
            }
            /*
            for (int i = 0; i < 10; ++i) {
                dims.push_back(dim);
                dim += 6 * 64;
            }
            */
        }
        if (true) { // print read/load of tiles

            /* 
            N = 64; M = 64; K = 64; Nt = 4; Mt = 4; Kt = 2
            Matrix size : C(16KB) += (8KB) * B(8KB)
            Matrix size : C(0MB) += A(0MB) * B(0MB)
            tdpbf16pf AMX3 : load C tiles : 32;        load A& B tiles : 64;     save C tiles 32;        mem: 128KB;     0MB
            tdpbf16pf AMX2 : stream load C tiles : 16;        load A& B tiles : 64;     save C tiles 16;        mem: 80KB;      0MB
            tdpbf16pf  AMX : stream load C tiles : 16;        load A& B tiles : 32;     save C tiles 16;        mem: 48KB;      0MB

            N = 128; M = 128; K = 128; Nt = 8; Mt = 8; Kt = 4
            Matrix size : C(64KB) += (32KB) * B(32KB)
            Matrix size : C(0MB) += A(0MB) * B(0MB)
            tdpbf16pf AMX3 : load C tiles : 256;       load A& B tiles : 512;    save C tiles 256;       mem: 1024KB;    1MB
            tdpbf16pf AMX2 : stream load C tiles : 64;        load A& B tiles : 512;    save C tiles 64;        mem: 576KB;     0MB
            tdpbf16pf  AMX : stream load C tiles : 64;        load A& B tiles : 256;    save C tiles 64;        mem: 320KB;     0MB
            */


            std::cout << "SapphireRapids: L1: 80KB/core" << std::endl;
            std::cout << "SapphireRapids: L2: 2MB/core" << std::endl;
            std::cout << "SapphireRapids: L3: 30MB" << std::endl << std::endl;
            for (int i : dims) {
                amx::tmul::spr::print_statistics(i, i, i);
            }
        }
        if (true) { // run the benchmarks and save to file
            constexpr int n_runs = 100;
            amx::benchmark::benchmark_to_file("C:\\Source\\Github\\AMX-matmul\\experiments\\benchmark.csv", dims, n_runs);
        }
    }

    // run benchmark
    if (false) {
        amx::benchmark::tdpbf16ps_1tile_N16_M16_K32(100000);
        //amx::benchmark::tdpbf16ps(16, 16, 32, 100000);
        amx::benchmark::tdpbf16ps(32, 32, 32, 10000);
        amx::benchmark::tdpbf16ps(64, 64, 64, 10000);
        amx::benchmark::tdpbf16ps(128, 128, 128, 10000);
        amx::benchmark::tdpbf16ps(256, 256, 256, 1000);
        amx::benchmark::tdpbf16ps(512, 512, 512, 1000);
        amx::benchmark::tdpbf16ps(1024, 1024, 1024, 100);
        amx::benchmark::tdpbf16ps(2048, 2048, 2048, 10);

        //amx::benchmark::tdpbssd(64, 64, 64, 100000);
        //amx::benchmark::tdpbssd(128, 128, 128, 10000);
        //amx::benchmark::tdpbssd(256, 256, 256, 1000);
    }

    // code to see the disassembled instructions for _mm512_cvtpbh_ps
    if (false) { 
        __debugbreak();
        __m256bh y = _mm256_setr_epi16(
            amx::float_to_bf16(0), amx::float_to_bf16(1), amx::float_to_bf16(2), amx::float_to_bf16(3),
            amx::float_to_bf16(4), amx::float_to_bf16(5), amx::float_to_bf16(6), amx::float_to_bf16(7),
            amx::float_to_bf16(8), amx::float_to_bf16(9), amx::float_to_bf16(10), amx::float_to_bf16(11),
            amx::float_to_bf16(12), amx::float_to_bf16(13), amx::float_to_bf16(14), amx::float_to_bf16(15));
        __m512 x = _mm512_cvtpbh_ps(y);      
        
        // vpmovzxwd   zmm1,zmm0       // word->dword zero extension
        // vpslld      zmm1,zmm1,10h   // shift left 16 position and shift in zeros
        __debugbreak();
        std::cout << x.m512_f32[0];
    }

    // run fp16 vector multiplication example
    if (false) {

        //example::run();
        amx::example::vector_mul();

        if (false) {
            amx::vecmul_example();
            amx::vecmul_example_speed();
        }
    }

    //TODO describe
    if (false) {
        if (false) amx::print_influence(1, 1);
        amx::print_influence2();
    }

    const auto end = std::chrono::system_clock::now();
    std::cout << "DONE: passed time: " << tools::timing::elapsed_time_str(start, end);
    std::cout << "-------------------" << std::endl;
    //std::cout << "Press RETURN to finish:" << std::endl;
    std::cout.flush();
    //static_cast<void>(getchar());
    return 0;
}
