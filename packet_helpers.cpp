#include "packet_helpers.h"
#include <string.h>
#include "Arduino.h"
#include "ARDWIINO.h"

void fill_from_pgm(xb_packet_t *packet, const uint8_t *pgm, const uint8_t &size) {
    for (uint8_t i = 0; i < size; i++) {
        packet->buf.buffer[i] = pgm_read_byte(pgm + i);
    }
    packet->header.triggered_time = 0;
    packet->header.length         = size;
    packet->buf.frame.sequence    = getSequence();
}

output_e outputForNote(const uint8_t &note) {
    switch (note) {
#define MIDI_MAP(midi_note, rb_out) \
    case midi_note:                 \
        return rb_out;

#include "Config/midi_map.h"
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
                                       xb_packet_t *packet) {
    if (ndata > ADAPTER_OUT_SIZE)
        return;

    memset(packet->buf.buffer, 0, sizeof(packet->buf.buffer));
    packet->header.length = sizeof(xb_one_drum_input_pkt_t);

    auto cast_data    = (const xb_one_controller_input_pkt_t *)data;
    auto drum_input_d = &packet->buf.drum_input;

    drum_input_d->command  = cast_data->command;
    drum_input_d->deviceId = cast_data->deviceId;
    drum_input_d->type     = cast_data->deviceId;
    drum_input_d->sequence = getSequence();
    drum_input_d->length   = sizeof(xb_one_drum_input_pkt_t) - sizeof(frame_pkt_t);
    drum_input_d->playerId = DRUMS;

    drum_input_d->dpadState1 = cast_data->buttons.dpadState;
    drum_input_d->dpadState2 = cast_data->buttons.dpadState;

    drum_input_d->coloredButtonState1 = cast_data->buttons.coloredButtonState;
    drum_input_d->coloredButtonState2 = cast_data->buttons.coloredButtonState;

    drum_input_d->select = cast_data->buttons.select;
    drum_input_d->start  = cast_data->buttons.start;
    return;
}

bool fillPacket(const uint8_t *input, const uint8_t &length, xb_packet_t *packet,
                unsigned long triggered_time) {
    if (length > ADAPTER_OUT_SIZE)
        return false;
    packet->header.triggered_time = triggered_time;
    packet->header.length         = length;
    memcpy(packet->buf.buffer, input, length);
    return true;
}

void updateDrumStateWithDrumInput(const output_e &out, const uint8_t &state,
                                  xb_one_drum_input_pkt_t *drum_input) {
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

void fillInputPacketFromGuitarData(const ARDWIINO::xb_three_gh_input_pkt_t *guitar_in,
                                   xb_packet_t *packet_out, uint8_t playerId) {
    memset(packet_out->buf.buffer, 0, sizeof(packet_out->buf.buffer));

    packet_out->header.length         = sizeof(xb_one_guitar_input_pkt_t);
    // packet_out->header.triggered_time = millis();

    auto *guitar_pkt = &packet_out->buf.guitar_input;

    guitar_pkt->command  = CMD_INPUT;
    guitar_pkt->sequence = getSequence();
    guitar_pkt->length   = sizeof(xb_one_guitar_input_pkt_t) - sizeof(frame_pkt_t);
    guitar_pkt->playerId = playerId;
    guitar_pkt->unknown  = 0x01;

    guitar_pkt->coloredButtonState1 = guitar_in->coloredButtonState;
    guitar_pkt->coloredButtonState2 = guitar_in->coloredButtonState;
    guitar_pkt->orangeButton        = guitar_in->orangeButton;

    guitar_pkt->selectButton = guitar_in->selectButton || (guitar_in->tilt >= (UINT16_MAX / 2));
    guitar_pkt->startButton  = guitar_in->startButton;

    guitar_pkt->select = guitar_in->selectButton || (guitar_in->tilt >= (UINT16_MAX / 2));
    guitar_pkt->start  = guitar_in->startButton;

    guitar_pkt->dpadState1 = guitar_in->dpadState;
    guitar_pkt->dpadState2 = guitar_in->dpadState;

    guitar_pkt->whammy = ((guitar_in->whammy >> 8) & 0xFF);

    return;
}