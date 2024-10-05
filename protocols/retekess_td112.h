#pragma once
#include "_base.h"

typedef enum {
    RetekessTd112StateBeep = 0x01, // Skip 0 as it means unset
    RetekessTd112StateTurnOff,
} RetekessTd112State;

typedef struct {
    RetekessTd112State state;
    uint16_t station_id;
    uint16_t pager_id;
} RetekessTd112Cfg;

extern const Protocol protocol_retekess_td112;
