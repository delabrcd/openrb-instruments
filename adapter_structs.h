#pragma once
#include <stdint.h>
#include "Config/AdapterConfig.h"

enum FrameCommand {
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

struct Frame {
    uint8_t command;
    uint8_t deviceId : 4;
    uint8_t type : 4;
    uint8_t sequence;
    uint8_t length;
} __attribute__((packed));

struct ControllerInputData : public Frame {
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

struct DrumInputData : public Frame {
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

// constexpr size_t size = sizeof(DrumInputData);
// static_assert(sizeof(DrumInputData) <= ADAPTER_OUT_SIZE);

struct XBPACKET {
    struct {
        uint8_t  handled;
        uint16_t length;
    } header;
    union buf {
        uint8_t       buffer[ADAPTER_OUT_SIZE];
        Frame         frame;
        DrumInputData drum_input;
    } buf;
};

enum ADAPTER_STATE {
    none,
    init_state,
    identifying,
    authenticating,
    running,
};

// Different frame types
// Command: controller doesn't respond
// Request: controller responds with data
// Request (ACK): controller responds with ack + data
enum FrameType {
    TYPE_COMMAND = 0x00,
    TYPE_ACK     = 0x01,
    TYPE_REQUEST = 0x02,
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

enum DrumStateFlags {
    no_flag      = 0,
    changed_flag = (1 << 0),
};