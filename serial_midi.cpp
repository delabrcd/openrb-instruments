#include "serial_midi.h"
#include <stdint.h>

static MidiType getTypeFromStatusByte(uint8_t inStatus) {
    if ((inStatus < 0x80) || (inStatus == Undefined_F4) || (inStatus == Undefined_F5) ||
        (inStatus == Undefined_FD))
        return InvalidType;  // Data bytes and undefined.

    if (inStatus < 0xf0)
        // Channel message, remove channel nibble.
        return MidiType(inStatus & 0xf0);

    return MidiType(inStatus);
}

static constexpr uint8_t note_on_length = 3;

static uint8_t pending_idx = 0;
static uint8_t pending_msg[note_on_length];

uint8_t readMIDI(HardwareSerial *serial_transport, uint8_t *buf) {
    if (serial_transport->available() == 0)
        return 0;

    while (serial_transport->available()) {
        uint8_t extracted = serial_transport->read();

        if (extracted == Undefined_FD)
            return 0;

        if (pending_idx) {
            pending_msg[pending_idx] = extracted;
            pending_idx++;
        }

        if ((pending_idx == 0) && getTypeFromStatusByte(extracted) == MidiType::NoteOn) {
            pending_msg[pending_idx] = extracted;
            pending_idx++;
        }

        if (pending_idx >= (note_on_length)) {
            pending_idx = 0;
            memcpy(buf, pending_msg, note_on_length);
            return note_on_length;
        }
    }
    return 0;
}
