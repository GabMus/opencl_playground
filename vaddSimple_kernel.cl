void kernel vaddSimple(global const float* A, global const float* B, global float* C) {
    C[get_global_id(0)] = A[get_global_id(0)] + B[get_global_id(0)];
}
