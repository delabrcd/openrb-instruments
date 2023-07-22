#pragma once

// USB Protocol Settings
#define ADAPTER_IN_NUM (ENDPOINT_DIR_IN | 1)
#define ADAPTER_IN_SIZE 64
#define ADAPTER_IN_INTERVAL 4
#define ADAPTER_OUT_NUM (ENDPOINT_DIR_OUT | 2)
#define ADAPTER_OUT_SIZE 64
#define ADAPTER_OUT_INTERVAL 4
#define MAX_CONTROL_TRANSFER_SIZE 64

// 0-255 velocity threshold
#define VELOCITY_THRESH 10
// milliseconds
#define TRIGGER_HOLD_MS 40
#define ON_DELAY_MS 40

#define ANNOUNCE_INTERVAL_MS 2000

// #define SERIAL_DEBUG Serial1
// #define CUSTOM_MIDI_MAP "Config/MyCustomMidiMap.tbl"
