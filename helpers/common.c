#include "common.h"

void generic_swap(void* a, void* b, size_t size) {
    uint8_t temp[size];
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
}

int generic_partition(
    void* array,
    int low,
    int high,
    size_t size,
    int (*comparator)(const void*, const void*)) {
    uint8_t* base = (uint8_t*)array;
    void* pivot = base + high * size;
    int i = low - 1;

    for(int j = low; j < high; j++) {
        void* element = base + j * size;
        if(comparator(element, pivot) <= 0) {
            i++;
            generic_swap(base + i * size, element, size);
        }
    }
    generic_swap(base + (i + 1) * size, pivot, size);
    return i + 1;
}

void sort(
    void* array,
    int low,
    int high,
    size_t size,
    int (*comparator)(const void*, const void*)) {
    if(low < high) {
        int pi = generic_partition(array, low, high, size, comparator);
        sort(array, low, pi - 1, size, comparator);
        sort(array, pi + 1, high, size, comparator);
    }
}

size_t extract_unique_values(
    void* array,
    size_t element_size,
    size_t array_size,
    void* unique_array,
    int (*comparator)(const void*, const void*)) {
    if(array_size == 0) return 0;

    uint8_t* arr = (uint8_t*)array;
    uint8_t* unique_arr = (uint8_t*)unique_array;

    memcpy(unique_arr, arr, element_size);
    size_t unique_count = 1;

    for(size_t i = 1; i < array_size; i++) {
        if(comparator(arr + i * element_size, arr + (i - 1) * element_size) != 0) {
            memcpy(unique_arr + unique_count * element_size, arr + i * element_size, element_size);
            unique_count++;
        }
    }

    return unique_count;
}
int compare_uint32_t(const void* a, const void* b) {
    uint32_t val_a = *(uint32_t*)a;
    uint32_t val_b = *(uint32_t*)b;

    if(val_a < val_b) return -1;
    if(val_a > val_b) return 1;
    return 0;
}
