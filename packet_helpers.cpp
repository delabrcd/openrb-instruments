#include "packet_helpers.h"
#include <string.h>
#include "Arduino.h"

output_t outputForNote(const uint8_t &note) {
    switch (note) {
#define MIDI_MAP(midi_note, rb_out) \
    case midi_note:                 \
        return rb_out;

#include "MidiMap.h"
#undef MIDI_MAP
        default:
            return NO_OUT;
    }
}

uint8_t getSequence() {
    static uint8_t sequence = 0;

    if (sequence == 0x00) {
        sequence = 0x01;
    }

    return sequence++;
}

void fillInputPacketFromControllerData(const uint8_t *data, const uint8_t &ndata,
                                       XBPACKET *packet) {
    if (ndata > ADAPTER_OUT_SIZE)
        return;

    memset(packet->buf.buffer, 0, sizeof(packet->buf.buffer));
    packet->header.length = sizeof(DrumInputData);

    auto cast_data    = (const ControllerInputData *)data;
    auto drum_input_d = &packet->buf.drum_input;

    drum_input_d->command  = cast_data->command;
    drum_input_d->deviceId = cast_data->deviceId;
    drum_input_d->type     = cast_data->deviceId;
    drum_input_d->sequence = getSequence();
    drum_input_d->length   = sizeof(DrumInputData) - sizeof(Frame);

    drum_input_d->a = cast_data->buttons.a;
    drum_input_d->b = cast_data->buttons.b;
    drum_input_d->x = cast_data->buttons.x;
    drum_input_d->y = cast_data->buttons.y;

    drum_input_d->dpadDown  = cast_data->buttons.dpadDown;
    drum_input_d->dpadUp    = cast_data->buttons.dpadUp;
    drum_input_d->dpadLeft  = cast_data->buttons.dpadLeft;
    drum_input_d->dpadRight = cast_data->buttons.dpadRight;

    drum_input_d->select = cast_data->buttons.select;
    drum_input_d->start  = cast_data->buttons.start;
    return;
}

bool fillPacket(const uint8_t *input, const uint8_t &length, XBPACKET *packet,
                unsigned long triggered_time) {
    if (length > ADAPTER_OUT_SIZE)
        return false;
    packet->header.triggered_time = triggered_time;
    packet->header.length         = length;
    memcpy(packet->buf.buffer, input, length);
    return true;
}

void updateDrumStateWithDrumInput(const output_t &out, const uint8_t &state,
                                  DrumInputData *drum_input) {
    switch (out) {
        case OUT_KICK:
            drum_input->kick = state;
            break;
        case OUT_PAD_RED:
            drum_input->padRed = state;
            break;
        case OUT_PAD_BLUE:
            drum_input->padBlue = state;
            break;
        case OUT_PAD_GREEN:
            drum_input->padGreen = state;
            break;
        case OUT_PAD_YELLOW:
            drum_input->padYellow = state;
            break;
        case OUT_CYM_YELLOW:
            drum_input->cymbalYellow = state;
            break;
        case OUT_CYM_BLUE:
            drum_input->cymbalBlue = state;
            break;
        case OUT_CYM_GREEN:
            drum_input->cymbalGreen = state;
            break;
        default:
            break;
    }
    return;
}