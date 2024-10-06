#include "retekess_td112.h"
#include "_protocols.h"

static const char* get_name(const Payload* payload) {
    UNUSED(payload);
    return "retekess_td112";
}

uint8_t reverse_byte(uint8_t b) {
    b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
    b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
    b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
    return b;
}

void encode_retekess_td112(uint16_t station_id, uint16_t pager_id, uint8_t* packet) {
    for(int i = 0; i < 8; i++) {
        packet[i] = 0;
    }

    uint16_t base_key = station_id & 0x0FFF;

    uint8_t group_num_input = (pager_id / 256) << 2;
    uint8_t group_num_reversed = reverse_byte(group_num_input);
    uint8_t group_number = (group_num_reversed >> 4) & 0x0F;

    uint8_t pager_key_input = pager_id % 256;
    uint8_t pager_key_reversed = reverse_byte(pager_key_input);

    uint32_t key = (base_key << 12) | (group_number << 8) | pager_key_reversed;

    packet[0] = (uint8_t)((key >> 16) & 0xFF);
    packet[1] = (uint8_t)((key >> 8) & 0xFF);
    packet[2] = (uint8_t)(key & 0xFF);
}

static void make_packet(uint8_t* _size, uint8_t** _packet, Payload* payload) {
    RetekessTd112Cfg* cfg = &payload->cfg.retekess_td112;

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

    station_id = 107;

    uint8_t size = 3;
    uint8_t* packet = (uint8_t*)calloc(size, sizeof(uint8_t));

    const uint16_t action_turnoff = 1005;
    encode_retekess_td112(
        station_id,
        cfg->state == RetekessTd112StateTurnOff ? action_turnoff : 3,
        packet);

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
