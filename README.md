# AMX-matmul
Intel AMX matrix multiplication experiments

![performance](https://github.com/HJLebbink/AMX-matmul/blob/main/experiments/Matrix%20Multiplication%20BF16.png?raw=true "Performance Overview")

Performance overview comparing (square) matrices dimensions with number of operations per CPU cycle. 
1. Blue: no_amx is the fastest [implementation in ASM](https://github.com/HJLebbink/AMX-matmul/blob/main/generated/asm/tdpbf16ps_N16_M16_K32_no_AMX.asm) of BF16 matrix multiplication (I could come up with) gives (between dimension 128 to 2240) about 0.06 operations (c += a*b) per CPU cycle

2. Red: amx0 is a very naive AMX implementation in which there is no reuse of tiles. The memory load just to multiply two tiles a and b, is that tile a, b and the result c are loaded from memory, the result is computed, and the result c is written back to memory. We need 3KB load and 1KB write.
  
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

3. Yellow: amx1: an obvious approach to reuse some tiles, but not all available eight tiles are used. Tile a and c are loaded from memory, and adjacent tiles of b are loaded and c is updated (c+=a*b), and only when the full row is exhausted, c is written back to memory. When the row is sufficiently wide, we only need 1KB load.

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

4. Green: amx2: maximal reuse of the eigth available tiles. We load c1 and c2 from two adjacent rows, and we load two adjacent columns a1 and a2, and two TODO

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
      // 6 is available
      _tile_dpbf16ps(1, 4, 7); // C[1][0] += A[0][0] * B[0][1]
      // 4 if available
      _tile_dpbf16ps(3, 5, 7); // C[1][1] += A[0][1] * B[0][1]
      // 5 and 7 are available
    }
    _tile_stored(0, c_ptr00, 64);
    _tile_stored(2, c_ptr01, 64);
    _tile_stored(1, c_ptr10, 64);
    _tile_stored(3, c_ptr11, 64);
  }
}
```

