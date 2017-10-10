void kernel vecinit(global float* vectorA, global float* vectorB) {
    vectorA[get_global_id(0)] = (float)get_global_id(0);
    vectorB[get_global_id(0)] = (float)get_global_id(0)/2;
}
