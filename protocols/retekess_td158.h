#pragma once
#include "_base.h"

typedef enum {
    RetekessTd158StateBeep = 0x01, // Skip 0 as it means unset
    RetekessTd158StateTurnOff,
} RetekessTd158State;

typedef struct {
    RetekessTd158State state;
    uint8_t station_id;
    uint16_t pager_id;
    uint16_t start_station_id;
    uint16_t end_station_id;
    uint16_t start_pager_id;
    uint16_t end_pager_id;
} RetekessTd158Cfg;

extern const Protocol protocol_retekess_td158;
