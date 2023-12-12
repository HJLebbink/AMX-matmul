#pragma once
#include <iostream>
#include <fstream>

//#define LINUX

#ifdef LINUX
#include <unistd.h> // POSIX operating system API
#include <sys/syscall.h>
#endif

#ifdef _DEBUG
constexpr inline bool DEBUG = true;
#else
constexpr inline bool DEBUG = false;
#endif


namespace amx {
    constexpr inline int MAX = 1024;
    constexpr inline int MAX_ROWS = 16;
    constexpr inline int MAX_COLS = 64;
    constexpr inline int STRIDE = 64;

    constexpr inline int ARCH_GET_XCOMP_PERM = 0x1022;
    constexpr inline int ARCH_REQ_XCOMP_PERM = 0x1023;
    constexpr inline int XFEATURE_XTILECFG = 17;
    constexpr inline int XFEATURE_XTILEDATA = 18;
}

namespace amx::tools {
    
    inline void save_code_ansi(const std::string& filename, const std::string& content)
    {
        if (content != "") {
            std::ofstream out(filename);
            out << content;
            out.close();
        }
    }

    // Check whether the provided params can be used in multiplication
    
    template <bool B_IS_TRANSPOSED, typename T1, typename T2>
    inline void check_matmul_dims(
        const T1& C,
        const T2& A,
        const T2& B
    ) {
        bool invalid = false;
        if constexpr (B_IS_TRANSPOSED) {
            if (A.n_cols_ != B.n_cols_) {
                std::cout << "incompatible dimensions tile: A.n_cols (" << A.n_cols_ << ") != B.n_cols (" << B.n_cols_ << ")" << std::endl;
                invalid = true;
            }
            if (B.n_rows_ != C.n_cols_) {
                std::cout << "incompatible dimensions tile: B.n_rows (" << B.n_rows_ << ") != C.n_cols (" << C.n_cols_ << ")" << std::endl;
                invalid = true;
            }
        }
        else {
            if (A.n_cols_ != B.n_rows_) {
                std::cout << "incompatible dimensions tile: A.n_cols (" << A.n_cols_ << ") != B.n_rows (" << B.n_rows_ << ")" << std::endl;
                invalid = true;
            }
            if (B.n_cols_ != C.n_cols_) {
                std::cout << "incompatible dimensions tile: B.n_cols_ (" << B.n_cols_ << ") != C.n_cols (" << C.n_cols_ << ")" << std::endl;
                invalid = true;
            }
        }
        if (A.n_rows_ != C.n_rows_) {
            std::cout << "incompatible dimensions tile: A.n_rows (" << A.n_rows_ << ") != C.n_rows (" << C.n_rows_ << ")" << std::endl;
            invalid = true;
        }
        if (invalid) {
            std::cout << "dimensions (row x col): C[" << C.n_rows_ << "][" << C.n_cols_ << "] = A[" << A.n_rows_ << "][" << A.n_cols_ << "] * B[" << B.n_rows_ << "][" << B.n_cols_ << "]" << std::endl;
            __debugbreak();
        }
    }


    // Set_tiledata_use() - Invoke syscall to set ARCH_SET_STATE_USE
    inline bool set_tiledata_use()
    {
#ifdef LINUX
        if (syscall(SYS_arch_prctl, ARCH_REQ_XCOMP_PERM, XFEATURE_XTILEDATA)) {
            printf("\n Fail to do XFEATURE_XTILEDATA \n\n");
            return false;
        }
        else {
            printf("\n TILE DATA USE SET - OK \n\n");
            return true;
        }
#endif
        return true;
    }
}
