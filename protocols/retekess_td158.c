#include "retekess_td158.h"
#include "_protocols.h"
#include "../helpers/log.h"

static const char* get_name(const Payload* payload) {
    UNUSED(payload);
    return "Retekess TD158";
}

static uint8_t extract_station_id(const uint8_t* packet) {
    return packet[0];
}

static void encode_retekess_td158(
    uint16_t station_id,
    uint16_t pager_id,
    uint16_t action_id,
    uint8_t* packet) {
    FURI_LOG_D(
        LOGGING_TAG,
        "Encoding station_id: %u, pager_id: %u, action_id: %u",
        station_id,
        pager_id,
        action_id);

    uint32_t packet_value = (station_id << 12) | (pager_id << 5) | action_id;

    packet[0] = (packet_value >> 16) & 0xFF;
    packet[1] = (packet_value >> 8) & 0xFF;
    packet[2] = packet_value & 0xFF;
}

static void make_packet(uint8_t* _size, uint8_t** _packet, Payload* payload) {
    RetekessTd158Cfg* cfg = &payload->cfg.retekess_td158;

    uint16_t station_id;
    uint16_t pager_id;
    uint32_t total_combinations;

    payload->bruteforce.counter_limit = 2;

    switch(payload->mode) {
    case PayloadModeBruteforce:
        uint32_t num_station_ids = cfg->end_station_id - cfg->start_station_id + 1;
        uint32_t num_pager_ids = cfg->end_pager_id - cfg->start_pager_id + 1;
        total_combinations = num_station_ids * num_pager_ids;

        payload->bruteforce.size = sizeof(uint32_t);

        uint32_t value = payload->bruteforce.value % total_combinations;
        uint32_t station_offset = value / num_pager_ids;
        uint32_t pager_offset = value % num_pager_ids;

        station_id = cfg->start_station_id + station_offset;
        pager_id = cfg->start_pager_id + pager_offset;
        break;
    case PayloadModeFindAndBruteforce:
        station_id = cfg->station_id;
        pager_id = payload->bruteforce.value % 101;

        payload->bruteforce.size = sizeof(uint8_t);
        break;
    default:
        station_id = cfg->station_id;
        pager_id = cfg->pager_id;
        break;
    }

    FURI_LOG_D(
        LOGGING_TAG,
        "Make packet! Payload Mode: %u, Station ID: %u, Pager ID: %u, Start Station Id: %u, End Station Id: %u, Start Pager Id: %u, End Pager Id: %u",
        payload->mode,
        station_id,
        pager_id,
        cfg->start_station_id,
        cfg->end_station_id,
        cfg->start_pager_id,
        cfg->end_pager_id);

    const uint8_t size = 3;
    uint8_t* packet = (uint8_t*)calloc(size, sizeof(uint8_t));

    const uint16_t turnoff = 16;
    const uint16_t call = 4;
    uint16_t action = cfg->state == RetekessTd158StateTurnOff ? turnoff : call;

    encode_retekess_td158(station_id, pager_id, action, packet);

    *_size = size;
    *_packet = packet;
}

static void process_packet(uint8_t size, uint8_t* packet, Payload* payload) {
    if(size != 3) {
        return;
    }
    payload->cfg.retekess_td158.station_id = extract_station_id(packet);
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
        scene_manager_next_scene(ctx->scene_manager, SceneRetekessTd158Mode);
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
    uint8_t value_index;

    uint8_t count_items = 2;
    VariableItem* item = variable_item_list_add(list, "Mode", count_items, mode_changed, payload);
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

static void check_and_set_bruteforce(Ctx* ctx) {
    Payload* payload = &ctx->attack->payload;
    RetekessTd158Cfg* cfg = &payload->cfg.retekess_td158;
    uint32_t start_station_id = cfg->start_station_id;
    uint32_t end_station_id = cfg->end_station_id;
    uint32_t start_pager_id = cfg->start_pager_id;
    uint32_t end_pager_id = cfg->end_pager_id;

    if(end_station_id < start_station_id || end_pager_id < start_pager_id) {
        payload->mode = PayloadModeFindAndBruteforce;
        return;
    }
    payload->mode = PayloadModeBruteforce;
}
static void mode_callback(void* _ctx, uint32_t index) {
    Ctx* ctx = _ctx;
    Payload* payload = &ctx->attack->payload;
    if(index == 1) {
        if(ctx->attack->payload.cfg.retekess_td158.state != RetekessTd158StateTurnOff) {
            scene_manager_next_scene(ctx->scene_manager, SceneRetekessTd158PagerIdEnd);
            scene_manager_next_scene(ctx->scene_manager, SceneRetekessTd158PagerIdStart);
        }
        scene_manager_next_scene(ctx->scene_manager, SceneRetekessTd158StationIdEnd);
        scene_manager_next_scene(ctx->scene_manager, SceneRetekessTd158StationIdStart);
        check_and_set_bruteforce(ctx);
    } else {
        payload->mode = PayloadModeFindAndBruteforce;
        view_dispatcher_send_custom_event(ctx->view_dispatcher, 0);
    }
}
void scene_retekess_td158_mode_on_enter(void* _ctx) {
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
bool scene_retekess_td158_mode_on_event(void* _ctx, SceneManagerEvent event) {
    Ctx* ctx = _ctx;
    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_previous_scene(ctx->scene_manager);
        return true;
    }
    return false;
}
void scene_retekess_td158_mode_on_exit(void* _ctx) {
    Ctx* ctx = _ctx;
    submenu_reset(ctx->submenu);
}
static void station_id_start_input_callback(void* _ctx) {
    Ctx* ctx = _ctx;
    RetekessTd158Cfg* cfg = &ctx->attack->payload.cfg.retekess_td158;
    cfg->start_station_id = (uint16_t)strtoul(ctx->text_store, NULL, 10);
    FURI_LOG_D(LOGGING_TAG, "Start Station ID: %u", cfg->start_station_id);
    view_dispatcher_send_custom_event(ctx->view_dispatcher, 0);
}

static void station_id_end_input_callback(void* _ctx) {
    Ctx* ctx = _ctx;
    RetekessTd158Cfg* cfg = &ctx->attack->payload.cfg.retekess_td158;
    cfg->end_station_id = (uint16_t)strtoul(ctx->text_store, NULL, 10);
    FURI_LOG_D(LOGGING_TAG, "End Station ID: %u", cfg->end_station_id);
    view_dispatcher_send_custom_event(ctx->view_dispatcher, 0);
}

static void pager_id_start_input_callback(void* _ctx) {
    Ctx* ctx = _ctx;
    RetekessTd158Cfg* cfg = &ctx->attack->payload.cfg.retekess_td158;
    cfg->start_pager_id = (uint16_t)strtoul(ctx->text_store, NULL, 10);
    FURI_LOG_D(LOGGING_TAG, "Start Pager ID: %u", cfg->start_pager_id);
    view_dispatcher_send_custom_event(ctx->view_dispatcher, 0);
}

static void pager_id_end_input_callback(void* _ctx) {
    Ctx* ctx = _ctx;
    RetekessTd158Cfg* cfg = &ctx->attack->payload.cfg.retekess_td158;
    cfg->end_pager_id = (uint16_t)strtoul(ctx->text_store, NULL, 10);
    FURI_LOG_D(LOGGING_TAG, "End Pager ID: %u", cfg->end_pager_id);
    view_dispatcher_send_custom_event(ctx->view_dispatcher, 0);
}

static void create_int_input_view(const char* message, IntInputCallback callback, void* _ctx) {
    Ctx* ctx = _ctx;
    IntInput* int_input = ctx->int_input;

    int_input_set_header_text(int_input, message);

    const size_t length = sizeof(ctx->text_store);
    int_input_set_result_callback(int_input, callback, ctx, ctx->text_store, length, true);

    view_dispatcher_switch_to_view(ctx->view_dispatcher, ViewIntInput);
}

void scene_retekess_td158_station_id_start_on_enter(void* _ctx) {
    Ctx* ctx = _ctx;
    create_int_input_view("START Station Id (0-9999)", station_id_start_input_callback, ctx);
}

void scene_retekess_td158_station_id_end_on_enter(void* _ctx) {
    Ctx* ctx = _ctx;
    create_int_input_view("END Station Id (0-9999)", station_id_end_input_callback, ctx);
}

void scene_retekess_td158_pager_id_start_on_enter(void* _ctx) {
    Ctx* ctx = _ctx;
    create_int_input_view("START Pager Id (0-999)", pager_id_start_input_callback, ctx);
}

void scene_retekess_td158_pager_id_end_on_enter(void* _ctx) {
    Ctx* ctx = _ctx;
    create_int_input_view("END Pager Id (0-999)", pager_id_end_input_callback, ctx);
}

static bool input_on_event(void* _ctx, SceneManagerEvent event) {
    Ctx* ctx = _ctx;
    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_previous_scene(ctx->scene_manager);
        return true;
    }
    return false;
}

bool scene_retekess_td158_station_id_start_on_event(void* _ctx, SceneManagerEvent event) {
    return input_on_event(_ctx, event);
}
bool scene_retekess_td158_station_id_end_on_event(void* _ctx, SceneManagerEvent event) {
    return input_on_event(_ctx, event);
}
bool scene_retekess_td158_pager_id_start_on_event(void* _ctx, SceneManagerEvent event) {
    return input_on_event(_ctx, event);
}
bool scene_retekess_td158_pager_id_end_on_event(void* _ctx, SceneManagerEvent event) {
    return input_on_event(_ctx, event);
}
void scene_retekess_td158_station_id_start_on_exit(void* _ctx) {
    UNUSED(_ctx);
}
void scene_retekess_td158_station_id_end_on_exit(void* _ctx) {
    UNUSED(_ctx);
}
void scene_retekess_td158_pager_id_start_on_exit(void* _ctx) {
    UNUSED(_ctx);
}
void scene_retekess_td158_pager_id_end_on_exit(void* _ctx) {
    UNUSED(_ctx);
}

const Protocol protocol_retekess_td158 = {
    .icon = &I_td158,
    .get_name = get_name,
    .make_packet = make_packet,
    .process_packet = process_packet,
    .extra_config = extra_config,
    .config_count = config_count,
};
