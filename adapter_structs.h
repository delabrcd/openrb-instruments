#pragma once
#include <stdint.h>
#include "Config/adapter_config.h"

enum frame_command_t {
    CMD_ACKNOWLEDGE   = 0x01,
    CMD_ANNOUNCE      = 0x02,
    CMD_STATUS        = 0x03,
    CMD_IDENTIFY      = 0x04,
    CMD_POWER_MODE    = 0x05,
    CMD_AUTHENTICATE  = 0x06,
    CMD_GUIDE_BTN     = 0x07,
    CMD_AUDIO_CONFIG  = 0x08,
    CMD_RUMBLE        = 0x09,
    CMD_LED_MODE      = 0x0a,
    CMD_SERIAL_NUM    = 0x1e,
    CMD_INPUT         = 0x20,
    CMD_AUDIO_SAMPLES = 0x60,
};

enum frame_type_t {
    TYPE_COMMAND = 0x00,
    TYPE_ACK     = 0x01,
    TYPE_REQUEST = 0x02,
};

enum power_mode_t {
    POWER_ON    = 0x00,
    POWER_SLEEP = 0x01,
    POWER_OFF   = 0x04,
};

enum led_mode_t {
    LED_OFF        = 0x00,
    LED_ON         = 0x01,
    LED_BLINK_FAST = 0x02,
    LED_BLINK_MED  = 0x03,
    LED_BLINK_SLOW = 0x04,
    LED_FADE_SLOW  = 0x08,
    LED_FADE_FAST  = 0x09,
};

struct frame_pkt_t {
    uint8_t command;
    uint8_t deviceId : 4;
    uint8_t type : 4;
    uint8_t sequence;
    uint8_t length;
} __attribute__((packed));

struct xb_one_controller_input_pkt_t : public frame_pkt_t {
    struct Buttons {
        uint32_t : 2;
        uint32_t start : 1;
        uint32_t select : 1;
        uint32_t a : 1;
        uint32_t b : 1;
        uint32_t x : 1;
        uint32_t y : 1;
        uint32_t dpadUp : 1;
        uint32_t dpadDown : 1;
        uint32_t dpadLeft : 1;
        uint32_t dpadRight : 1;
        uint32_t bumperLeft : 1;
        uint32_t bumperRight : 1;
        uint32_t stickLeft : 1;
        uint32_t stickRight : 1;

    } __attribute__((packed)) buttons;

    uint16_t triggerLeft;
    uint16_t triggerRight;
    int16_t  stickLeftX;
    int16_t  stickLeftY;
    int16_t  stickRightX;
    int16_t  stickRightY;

} __attribute__((packed));

struct xb_one_wireless_legacy_adapter_pkt_t : public frame_pkt_t {
    xb_one_wireless_legacy_adapter_pkt_t()
        : frame_pkt_t{CMD_INPUT, TYPE_COMMAND, 0,
                      sizeof(xb_one_wireless_legacy_adapter_pkt_t) - sizeof(frame_pkt_t)} {
        unknown = 0x01;
    }
    uint16_t : 16;
    uint8_t playerId;
    uint8_t unknown;
};

struct xb_one_guitar_input_pkt_t : public xb_one_wireless_legacy_adapter_pkt_t {
    xb_one_guitar_input_pkt_t(uint8_t playerId) : xb_one_wireless_legacy_adapter_pkt_t() {
        playerId = playerId;
        length   = sizeof(xb_one_guitar_input_pkt_t) - sizeof(frame_pkt_t);
    }

    uint8_t yellowButton : 1;
    uint8_t blueButton : 1;
    uint8_t redButton : 1;
    uint8_t greenButton : 1;

    uint8_t selectButton : 1;
    uint8_t startButton : 1;
    uint8_t : 2;

    uint8_t : 3;
    uint8_t orangeButton : 1;

    uint8_t right : 1;
    uint8_t left : 1;
    uint8_t down : 1;
    uint8_t up : 1;

    uint8_t : 8;
    uint8_t : 8;
    uint8_t : 8;
    uint8_t : 8;

    uint16_t whammy;
    uint16_t tilt;

    uint8_t : 8;
    uint8_t : 8;
};

struct xb_one_drum_input_pkt_t : public frame_pkt_t {
    xb_one_drum_input_pkt_t()
        : frame_pkt_t{CMD_INPUT, TYPE_COMMAND, 0,
                      sizeof(xb_one_drum_input_pkt_t) - sizeof(frame_pkt_t)} {}
    uint8_t : 2;
    uint8_t start : 1;
    uint8_t select : 1;
    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1;

    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t kick : 1;
    uint8_t doublekick : 1;
    uint8_t : 2;

    uint8_t : 3;
    uint8_t padYellow : 1;
    uint8_t : 3;
    uint8_t padRed : 1;

    uint8_t : 3;
    uint8_t padGreen : 1;
    uint8_t : 3;
    uint8_t padBlue : 1;

    uint8_t : 3;
    uint8_t cymbalBlue : 1;
    uint8_t : 3;
    uint8_t cymbalYellow : 1;

    uint8_t : 7;
    uint8_t cymbalGreen : 1;

    uint8_t : 8;
    uint8_t : 8;

} __attribute__((packed));

struct led_mode_pkt_t : public frame_pkt_t {
    uint8_t : 8;
    uint8_t mode;
    uint8_t brightness;
} __attribute__((packed));

struct xb_packet_t {
    struct {
        uint32_t triggered_time = 0;
        uint16_t length         = 0;
    } header;
    union {
        uint8_t        buffer[ADAPTER_OUT_SIZE];
        frame_pkt_t    frame;
        led_mode_pkt_t led_mode;

        xb_one_drum_input_pkt_t   drum_input;
        xb_one_guitar_input_pkt_t guitar_input;
    } buf;
};

static_assert(ADAPTER_OUT_SIZE == sizeof(xb_packet_t::buf), "Wrong Packet size in packet union");

enum adapter_state_t {
    none,
    init_state,
    identifying,
    authenticating,
    running,
    power_off,
};

enum output_t {
    FIRST_OUT,
    OUT_KICK = FIRST_OUT,
    OUT_PAD_RED,
    OUT_PAD_YELLOW,
    OUT_PAD_BLUE,
    OUT_PAD_GREEN,
    OUT_CYM_YELLOW,
    OUT_CYM_BLUE,
    OUT_CYM_GREEN,
    LAST_OUT = OUT_CYM_GREEN,
    NUM_OUT,
    NO_OUT,
};

struct output_state_t {
    typedef uint32_t milliseconds;

    output_state_t() {
        triggeredAt = 0;
        triggered   = false;
    }

    milliseconds triggeredAt;
    bool         triggered;
};

enum drum_state_flags_t {
    no_flag      = 0,
    changed_flag = (1 << 0),
};