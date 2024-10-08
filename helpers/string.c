#include "string.h"
#include <stdint.h>

char* alloc_extract_value_from_string(const char* key, const char* input_str) {
    size_t key_len = strlen(key);
    size_t pattern_size = key_len + 2;
    char* key_pattern = (char*)calloc(pattern_size, sizeof(char));

    snprintf(key_pattern, pattern_size, "%s:", key);

    const char* value_start = strstr(input_str, key_pattern);
    free(key_pattern);

    if(value_start) {
        value_start += key_len + 1;

        const char* value_end = value_start;
        while(*value_end && *value_end != ' ' && *value_end != '\n') {
            value_end++;
        }

        size_t value_length = value_end - value_start;
        char* value = (char*)calloc(value_length + 1, sizeof(char));

        strncpy(value, value_start, value_length);
        value[value_length] = '\0';
        return value;
    }
    return NULL;
}

void convert_key_to_data(const char* key_value_str, uint8_t* data, uint8_t size) {
    uint64_t key_value = 0;

    for(const char* ptr = key_value_str; *ptr; ptr++) {
        if(*ptr >= '0' && *ptr <= '9') {
            key_value = (key_value << 4) | (*ptr - '0');
        } else if(*ptr >= 'A' && *ptr <= 'F') {
            key_value = (key_value << 4) | (*ptr - 'A' + 10);
        } else if(*ptr >= 'a' && *ptr <= 'f') {
            key_value = (key_value << 4) | (*ptr - 'a' + 10);
        }
    }

    for(int i = 0; i < size; i++) {
        data[size - 1 - i] = (uint8_t)(key_value & 0xFF);
        key_value >>= 8;
    }
}
