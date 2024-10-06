#pragma once

#include <notification/notification_messages.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/byte_input.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/variable_item_list.h>
#include <gui/gui.h>
#include <gui/elements.h>
#include <lib/subghz/devices/devices.h>
#include <lib/subghz/devices/cc1101_int/cc1101_int_interconnect.h>
#include <applications/drivers/subghz/cc1101_ext/cc1101_ext_interconnect.h>
#include <lib/subghz/transmitter.h>
#include <furi.h>
#include <furi_hal.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/devices/devices.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/protocols/protocol_items.h>

#include "scenes/_setup.h"

extern const char* LOGGING_TAG;

enum {
    ViewMain,
    ViewByteInput,
    ViewSubmenu,
    ViewTextInput,
    ViewVariableItemList,
};

enum {
    ConfigLedIndicator,
    ConfigExtraStart = ConfigLedIndicator,
    ConfigLockKeyboard,
};

typedef struct Attack Attack;

typedef struct {
    Attack* attack;
    uint8_t byte_store[3];
    VariableItemListEnterCallback fallback_config_enter;
    bool led_indicator;
    bool lock_keyboard;

    NotificationApp* notification;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    ByteInput* byte_input;
    Submenu* submenu;
    TextInput* text_input;
    VariableItemList* variable_item_list;
} Ctx;

typedef struct {
    Ctx ctx;
    View* main_view;
    bool lock_warning;
    uint8_t lock_count;
    FuriTimer* lock_timer;

    bool advertising;
    uint8_t delay;
    FuriThread* thread;

    int8_t index;
    bool ignore_bruteforce;

    // SubGhz
    const SubGhzDevice* device;
    FlipperFormat* flipper_format;
    SubGhzEnvironment* environment;

    // SubGhz Rx
    FuriStreamBuffer* stream;
    SubGhzReceiver* receiver;
} State;
