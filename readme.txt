

1. Class Tile<T> is a matrix of 16 columns (of 4 bytes) and 16 rows. It represents an AMX hardware tile.


2. Class Matrix<T> is matrix of values of type T. It is not designed for computation, only for storage.
  It contains methods such as pretty_print, and standard transposition


3. Class AmxMatrix<T> is a matrix of either int8_t, uint8_t, BF16, FP16 or FP32. It is desinged for computations.
   AmxMatrix can be split into a Matrix<Tile<T>> 

   It contains methods such as multiply, pretty_print, load_from_matrix, store_to_matrix. It does not contain a Transpose.
   It does not contain other methods to change the content of the matrix.
   The load_from_matrix has an option to load the matrix in transposed from.
