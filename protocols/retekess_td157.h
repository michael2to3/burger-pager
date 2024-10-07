#pragma once
#include "_base.h"

typedef enum {
    RetekessTd157StateBeep = 0x01, // Skip 0 as it means unset
    RetekessTd157StateTurnOff,
} RetekessTd157State;

typedef struct {
    RetekessTd157State state;
    uint16_t station_id;
    uint16_t pager_id;
    uint16_t start_station_id;
    uint16_t end_station_id;
    uint16_t start_pager_id;
    uint16_t end_pager_id;
} RetekessTd157Cfg;

extern const Protocol protocol_retekess_td157;
