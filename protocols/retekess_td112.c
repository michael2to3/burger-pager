#include "retekess_td112.h"
#include "_protocols.h"

static const char* get_name(const Payload* payload) {
    UNUSED(payload);
    return "retekess_td112";
}

uint16_t extract_station_id(const uint8_t* packet) {
    uint32_t key = (packet[0] << 16) | (packet[1] << 8) | packet[2];
    uint16_t station_id = (key >> 12) & 0x0FFF;
    return station_id;
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
    switch(cfg ? payload->mode : PayloadModeFindAndBruteforce) {
    case PayloadModeFindAndBruteforce:
    default:
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
        station_id, cfg->state == RetekessTd112StateTurnOff ? action_turnoff : 3, packet);

    *_size = size;
    *_packet = packet;
}
static void process_packet(uint8_t size, uint8_t* packet, Payload* payload) {
    if(size != 3) {
        return;
    }
    payload->cfg.retekess_td112.station_id = extract_station_id(packet);
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
        scene_manager_next_scene(ctx->scene_manager, SceneRetekessTd112Mode);
        break;
    default:
        ctx->fallback_config_enter(ctx, index);
        break;
    }
}

static void mode_changed(VariableItem* item) {
    Payload* payload = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    if(index == 1) {
        payload->mode = PayloadModeBruteforce;
        variable_item_set_current_value_text(item, "Bruteforce");
    } else {
        payload->mode = PayloadModeFindAndBruteforce;
        variable_item_set_current_value_text(item, "Find and Bruteforce");
    }
}
static void extra_config(Ctx* ctx) {
    Payload* payload = &ctx->attack->payload;
    VariableItemList* list = ctx->variable_item_list;
    VariableItem* item;
    uint8_t value_index;

    item = variable_item_list_add(list, "Mode", 2, mode_changed, payload);
    const char* mode_name = NULL;
    switch(payload->mode) {
    case PayloadModeFindAndBruteforce:
    default:
        mode_name = "Find and Bruteforce";
        value_index = 0;
        break;
    case PayloadModeBruteforce:
        mode_name = "Bruteforce";
        value_index = 1;
        break;
    }
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, mode_name);

    variable_item_list_set_enter_callback(list, config_callback, ctx);
}
static uint8_t config_count(const Payload* payload) {
    UNUSED(payload);
    return ConfigCOUNT - ConfigExtraStart - 1;
}

static void mode_callback(void* _ctx, uint32_t index) {
    Ctx* ctx = _ctx;
    Payload* payload = &ctx->attack->payload;
    if(index == 1) {
        payload->mode = PayloadModeBruteforce;
        scene_manager_next_scene(ctx->scene_manager, SceneRetekessTd112StationIdStart);
    } else {
        payload->mode = PayloadModeFindAndBruteforce;
        view_dispatcher_send_custom_event(ctx->view_dispatcher, 0);
    }
}
void scene_retekess_td112_mode_on_enter(void* _ctx) {
    Ctx* ctx = _ctx;
    Payload* payload = &ctx->attack->payload;
    Submenu* submenu = ctx->submenu;
    uint32_t selected = 0;
    uint16_t i = 0;

    submenu_add_item(submenu, "FindAndBruteforce", i++, mode_callback, ctx);
    if(payload->mode == PayloadModeFindAndBruteforce) {
        selected = 0;
    }

    submenu_add_item(submenu, "Bruteforce", i++, mode_callback, ctx);
    if(payload->mode == PayloadModeBruteforce) {
        selected = 1;
    }

    submenu_set_selected_item(submenu, selected);
    view_dispatcher_switch_to_view(ctx->view_dispatcher, ViewSubmenu);
}
bool scene_retekess_td112_mode_on_event(void* _ctx, SceneManagerEvent event) {
    Ctx* ctx = _ctx;
    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_previous_scene(ctx->scene_manager);
        return true;
    }
    return false;
}
void scene_retekess_td112_mode_on_exit(void* _ctx) {
    Ctx* ctx = _ctx;
    submenu_reset(ctx->submenu);
}

static void station_id_start_callback(void* _ctx) {
    Ctx* ctx = _ctx;
    Payload* payload = &ctx->attack->payload;
    payload->mode = PayloadModeBruteforce;
    view_dispatcher_send_custom_event(ctx->view_dispatcher, 0);
}
void scene_retekess_td112_station_id_start_on_enter(void* _ctx) {
    Ctx* ctx = _ctx;
    IntInput* int_input = ctx->int_input;

    int_input_set_header_text(int_input, "Enter brute from start Station Id");

    int_input_set_result_callback(
        int_input, station_id_start_callback, ctx, (void*)ctx->int_input, 10, false);

    view_dispatcher_switch_to_view(ctx->view_dispatcher, ViewIntInput);
}
bool scene_retekess_td112_station_id_start_on_event(void* _ctx, SceneManagerEvent event) {
    Ctx* ctx = _ctx;
    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_previous_scene(ctx->scene_manager);
        scene_manager_previous_scene(ctx->scene_manager);
        return true;
    }
    return false;
}
void scene_retekess_td112_station_id_start_on_exit(void* _ctx) {
    UNUSED(_ctx);
}

const Protocol protocol_retekess_td112 = {
    .icon = &I_heart,
    .get_name = get_name,
    .make_packet = make_packet,
    .process_packet = process_packet,
    .extra_config = extra_config,
    .config_count = config_count,
};
