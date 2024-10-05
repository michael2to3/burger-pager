#include "retekess_td112.h"
#include "_protocols.h"

static const char* get_name(const Payload* payload) {
    UNUSED(payload);
    return "retekess_td112";
}

static void make_packet(uint8_t* _size, uint8_t** _packet, Payload* payload) {
    RetekessTd112Cfg* cfg = &payload->cfg.retekess_td112;

    RetekessTd112State state;
    UNUSED(state);
    if(cfg && cfg->state != 0x00) {
        state = cfg->state;
    } else {
        const RetekessTd112State states[] = {
            RetekessTd112StateBeep,
            RetekessTd112StateTurnOff,
        };
        state = states[rand() % COUNT_OF(states)];
    }

    uint16_t station_id;
    switch(cfg ? payload->mode : PayloadModeRandom) {
    case PayloadModeRandom:
    default:
        station_id = rand() % 0x0FFF;
        break;
    case PayloadModeFind:
    case PayloadModeValue:
        station_id = cfg->station_id;
        break;
    case PayloadModeBruteforce:
        station_id = cfg->station_id = payload->bruteforce.value;
        break;
    }

    uint8_t size = 8;
    uint8_t* packet = (uint8_t*)calloc(size, sizeof(uint8_t));

    uint32_t key = (station_id << 12) | (cfg->pager_id & 0x0FFF);

    uint8_t i = 0;
    packet[i++] = (key >> 16) & 0xFF;
    packet[i++] = (key >> 8) & 0xFF;
    packet[i++] = key & 0xFF;
    *_size = size;
    *_packet = packet;
}

enum {
    _ConfigExtraStart = ConfigExtraStart,
    ConfigMode,
    ConfigCOUNT,
};
static void config_callback(void* _ctx, uint32_t index) {
    Ctx* ctx = _ctx;
    scene_manager_set_scene_state(ctx->scene_manager, SceneConfig, index);
    switch(index) {
    case ConfigMode:
        break;
    default:
        ctx->fallback_config_enter(ctx, index);
        break;
    }
}

static void extra_config(Ctx* ctx) {
    VariableItemList* list = ctx->variable_item_list;
    variable_item_list_set_enter_callback(list, config_callback, ctx);
}

static uint8_t config_count(const Payload* payload) {
    UNUSED(payload);
    return ConfigCOUNT - ConfigExtraStart - 1;
}

const Protocol protocol_retekess_td112 = {
    .icon = &I_heart,
    .get_name = get_name,
    .make_packet = make_packet,
    .extra_config = extra_config,
    .config_count = config_count,
};
