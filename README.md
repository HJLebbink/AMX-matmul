# Some Intel AMX Matrix Multiplication Experiments

Here is a writeup of my meandering experiences writing code for Intel's Advanced Matrix Extensions (AMX) instruction set. Initially, I relied on Intel's Software Development Emulator (SDE) for AMX development. However, with Visual Studio now offering a more effective debugging experience to inspect tile registers, I invested in a SapphireRapids development rig to streamline my workflow and make things less cumbersome...  

## Memory layout needed for `tdpbf16ps`

After some deciphering of the cryptic documentation to figure out what it truly meant, I managed to compile the specification of this operation into an extensive list of operations, spanning [4096 lines](https://github.com/HJLebbink/AMX-matmul/blob/main/spec_tdpbf16ps_1tile.txt). Here are the initial five lines:

    C[0][0] += (A[0][0] * B[0][0]) + (A[0][1] * B[0][1])
    C[0][1] += (A[0][0] * B[0][2]) + (A[0][1] * B[0][3])
    C[0][2] += (A[0][0] * B[0][4]) + (A[0][1] * B[0][5])
    C[0][3] += (A[0][0] * B[0][6]) + (A[0][1] * B[0][7])
    C[0][4] += (A[0][0] * B[0][8]) + (A[0][1] * B[0][9])
    ...

Take note that in the B matrix, two BF16 elements are packed into a single 32-bit storage element. If you envision B as an MxN matrix and transpose it element-wise into an NxM matrix, it results in... quite a mess. However, if you reinterpret this matrix with 32-bit storage elements, transpose it, and then cast it back into the original storage format, you achieve a proper transpose. It may not be overly complex, but it did take some time to figure this out.

## MatMul code for fixed size matrices

Each tile provides 1KB of space, accommodating 16x32 BF16 elements. The `tdpbf16ps` operation involves taking two of these tiles and storing the outcome, represented as 16x16 32-bit floats, in a third tile. The assembly is [this](https://github.com/HJLebbink/AMX-matmul/blob/main/generated/asm/tdpbf16ps_N16_M16_K32.asm): 

```asm
GLOBAL tdpbf16ps_N16_M16_K32_asm

SECTION .text
tdpbf16ps_N16_M16_K32_asm:
	mov         r10d, 64 ; set STRIDE to 64
	ldtilecfg   [config]

	tileloadd   tmm0, [rcx + r10] ; load C[0][0]
	tileloadd   tmm1, [rdx + r10] ; load A[0][0]
	tileloadd   tmm2, [r8 + r10]  ; load B[0][0]
	tdpbf16ps   tmm0, tmm1, tmm2  ; C[0][0] += A[0][0] * B[0][0]
	tilestored  [rcx + r10], tmm0 ; store C[0][0]
	tilerelease	
	ret
```

To perform multiplication with N=32, M=32, and K=64, we require eight tile multiplications:

    C[0][1] += A[0][1] * B[0][0]
    C[0][0] += A[0][0] * B[0][0]
    C[1][0] += A[0][0] * B[0][1]
    C[1][1] += A[0][1] * B[0][1]
    C[0][0] += A[1][0] * B[1][0]
    C[0][1] += A[1][1] * B[1][0]
    C[1][0] += A[1][0] * B[1][1]
    C[1][1] += A[1][1] * B[1][1]

If we had nine tile registers, we could load all four tiles for matrix A and B, consecutively load one of the four C tiles, update it with the products, and store the results back to memory. However, given that we only have eight tile registers, the key is to cleverly arrange the loads and computations to avoid spilling tiles to memory and prevent loading tiles more than once. In the [following implementation](https://github.com/HJLebbink/AMX-matmul/blob/main/generated/asm/tdpbf16ps_N32_M32_K64.asm), there are four stores, twelve loads, and no spills. As there's no need to keep matrix A and B in the cache, we can load them with a temporal hint (`tileloaddt1`).

```asm
SECTION .text
tdpbf16ps_N32_M32_K64_asm:
  mov         r10d, 64     ; stride is always 64
  ldtilecfg   [config]
  
  ; registers: 0= 1= 2= 3= 4= 5= 6= 7= 
  tileloaddt1 tmm0, [rdx + r10 + 1*1024]	; load A[0][1]
  tileloaddt1 tmm1, [r8  + r10 + 0*1024]	; load B[0][0]
  tileloaddt1 tmm2, [rcx + r10 + 1*1024]	; load C[0][1]
  tdpbf16ps   tmm2, tmm0, tmm1				; C[0][1] += A[0][1] * B[0][0]
  ;tilestored  [rcx + r10 + 1*1024], tmm2 	; store C[0][1]

  ; registers: 0=A[0][1] 1=B[0][0] 2=C[0][1] 3= 4= 5= 6= 7= 
  tileloaddt1 tmm3, [rdx + r10 + 0*1024]	; load A[0][0]
  ;tileloadd   tmm1, [r8  + r10 + 0*1024]	; load B[0][0]
  tileloaddt1 tmm4, [rcx + r10 + 0*1024]	; load C[0][0]
  tdpbf16ps   tmm4, tmm3, tmm1				; C[0][0] += A[0][0] * B[0][0]
  ;tilestored  [rcx + r10 + 0*1024], tmm4 	; store C[0][0]

  ; registers: 0=A[0][1] 1= 2=C[0][1] 3=A[0][0] 4=C[0][0] 5= 6= 7= 
  ;tileloadd   tmm3, [rdx + r10 + 0*1024]	; load A[0][0]
  tileloaddt1 tmm1, [r8  + r10 + 1*1024]	; load B[0][1]
  tileloaddt1 tmm5, [rcx + r10 + 2*1024]	; load C[1][0]
  tdpbf16ps   tmm5, tmm3, tmm1				; C[1][0] += A[0][0] * B[0][1]
  ;tilestored  [rcx + r10 + 2*1024], tmm5 	; store C[1][0]

  [removed 5 other computations]
 
  tilerelease
  ret
```
Nevertheless, it becomes increasingly computationally complex to determine the optimal sequence of loads and computations that minimizes memory access. In the case of [tdpbf16ps_N256_M256_K256_asm](https://github.com/HJLebbink/AMX-matmul/blob/main/generated/asm/tdpbf16ps_N256_M256_K256.asm), we require 256 stores and 2678 loads, while the minimum required is 512. This indicates a total of 2166 spills.

## MatMul code for dynamically sized matrices 

Another approach to optimizing matrix multiplications involves algorithms that can handle matrices of any dimension. These algorithms are probably less efficient compared to the highly optimized computing networks designed for fixed-size matrices. However, given the practical impossibility of finding such compute networks for larger dimensions (where N, M, K > 512), we still need these algorithms.

Below are the performance characteristics of four algorithms designed for a SapphireRapids to perform matrix multiplications with BF16:

![performance](https://github.com/HJLebbink/AMX-matmul/blob/main/experiments/Matrix%20Multiplication%20BF16.png?raw=true "Performance Overview")

1. Blue line: no_amx is the fastest [ASM implementation](https://github.com/HJLebbink/AMX-matmul/blob/main/generated/asm/tdpbf16ps_N16_M16_K32_no_AMX.asm) for BF16 matrix multiplication that I could come up with. It achieves approximately 0.06 operations (C += A*B) per CPU cycle across dimensions ranging from 128 to 2240.

2. Red line: amx0 is a rather simplistic AMX implementation with no tile reuse. The memory load for multiplying two tiles, A and B, involves loading both tiles A and B along with the result C from memory. The computation is performed, and the result C is then written back to memory. This process requires a 3KB load and a 1KB write.

```c++
for (int i = 0; i < M; ++i) {
  for (int j = 0; j < N; ++j) {
    FP32* c_ptr = C.get_tile(j, i).data();
    for (int p = 0; p < K; ++p) {
      _tile_loadd(0, c_ptr, 64);
      _tile_loadd(1, A.get_tile(p, i).data(), 64);
      _tile_loadd(2, B.get_tile(p, j).data(), 64);
      _tile_dpbf16ps(0, 1, 2);
      _tile_stored(0, c_ptr, 64);
    }
  }
}
```

3. Yellow line: amx1 takes a somewhat straightforward approach by reusing certain tiles, though it doesn't utilize all eight available tiles. In this implementation, tiles A and B are loaded from memory, and adjacent tiles of C are loaded. The result C is then updated (C += A*B), and it's only when the entire row is processed that C is written back to memory. When the row is wide enough, only a 1KB load is required.

```c++
for (int i = 0; i < M; ++i) {
  for (int j = 0; j < N; ++j) {
    FP32* c_ptr = C.get_tile(j, i).data();
    _tile_stream_loadd(0, c_ptr, 64);
    for (int p = 0; p < K; ++p) {
      _tile_loadd(1, A.get_tile(p, i).data(), 64);
      _tile_loadd(2, B.get_tile(p, j).data(), 64);
      _tile_dpbf16ps(0, 1, 2);
    }
    _tile_stored(0, c_ptr, 64);
  }
}
```

4. Green line: amx2 aims for maximal reuse of the eight available tiles. Four tiles from C are loaded, spanning two adjacent rows and columns, and two tiles each from A and B are loaded in the K dimension. This allows for four computations, effectively halving the number of bytes that need to be loaded compared to amx1.

```c++
for (int i = 0; i < M; i += 2) {
  for (int j = 0; j < N; j += 2) {
    FP32* c_ptr00 = C.get_tile(j + 0, i + 0).data();
    FP32* c_ptr10 = C.get_tile(j + 1, i + 0).data();
    FP32* c_ptr01 = C.get_tile(j + 0, i + 1).data();
    FP32* c_ptr11 = C.get_tile(j + 1, i + 1).data();
    _tile_stream_loadd(0, c_ptr00, 64); // 0 = C[0][0]
    _tile_stream_loadd(2, c_ptr01, 64); // 2 = C[0][1]
    _tile_stream_loadd(1, c_ptr10, 64); // 1 = C[1][0]
    _tile_stream_loadd(3, c_ptr11, 64); // 3 = C[1][1]

    for (int p = 0; p < K; ++p) {
      _tile_loadd(4, A.get_tile(p, i + 0).data(), 64); // 4 = A[0][0]
      _tile_loadd(6, B.get_tile(p, j + 0).data(), 64); // 6 = B[0][0]
      _tile_loadd(5, A.get_tile(p, i + 1).data(), 64); // 5 = A[0][1]
      _tile_loadd(7, B.get_tile(p, j + 1).data(), 64); // 7 = B[0][1]

      _tile_dpbf16ps(0, 4, 6); // C[0][0] += A[0][0] * B[0][0]
      _tile_dpbf16ps(2, 5, 6); // C[0][1] += A[0][1] * B[0][0]
      _tile_dpbf16ps(1, 4, 7); // C[1][0] += A[0][0] * B[0][1]
      _tile_dpbf16ps(3, 5, 7); // C[1][1] += A[0][1] * B[0][1]
    }
    _tile_stored(0, c_ptr00, 64);
    _tile_stored(2, c_ptr01, 64);
    _tile_stored(1, c_ptr10, 64);
    _tile_stored(3, c_ptr11, 64);
  }
}
```

