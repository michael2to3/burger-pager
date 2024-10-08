#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

void generic_swap(void* a, void* b, size_t size);

int generic_partition(
    void* array,
    int low,
    int high,
    size_t size,
    int (*comparator)(const void*, const void*));

void sort(void* array, int low, int high, size_t size, int (*comparator)(const void*, const void*));

size_t extract_unique_values(
    void* array,
    size_t element_size,
    size_t array_size,
    void* unique_array,
    int (*comparator)(const void*, const void*));

int compare_uint32_t(const void* a, const void* b);
