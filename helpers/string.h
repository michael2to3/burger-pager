#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

char* alloc_extract_value_from_string(const char* key, const char* input_str);
void convert_key_to_data(const char* input_str, uint8_t* data, uint8_t size);
