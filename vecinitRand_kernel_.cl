void kernel simple_add(global float* vectorA, global float* vectorB) {
    vectorA[get_global_id(0)] = 0;
    vectorB[get_global_id(0)] = 0;
}
