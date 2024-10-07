#pragma once

#include "retekess_td112.h"

typedef enum {
    PayloadModeFindAndBruteforce,
    PayloadModeRandom,
    PayloadModeValue,
    PayloadModeBruteforce,
} PayloadMode;

struct Payload {
    const char* subghz_protocol;
    uint32_t frequency;
    uint32_t bits;
    uint32_t te;
    uint32_t repeat;
    uint32_t guard_time;
    PayloadMode mode;
    struct {
        uint8_t counter;
        uint8_t counter_limit;
        uint32_t value;
        uint8_t size;
    } bruteforce;
    union {
        RetekessTd112Cfg retekess_td112;
    } cfg;
};

extern const Protocol* protocols[];

extern const size_t protocols_count;

struct Attack {
    const char* title;
    const char* text;
    const Protocol* protocol;
    Payload payload;
};
