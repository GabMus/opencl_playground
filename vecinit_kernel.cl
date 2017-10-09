void kernel simple_add(global int* vector) {
    vector[get_global_id(0)] = 0;
}
