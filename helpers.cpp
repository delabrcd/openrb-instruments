#include "helpers.h"
#include "adapter_structs.h"
#include "HardwareSerial.h"

uint8_t getSequence() {
    static uint8_t sequence = 0;

    if (sequence == 0x00) {
        sequence = 0x01;
    }

    return sequence++;
}

void FillInputPacketFromControllerData(const uint8_t *data, const uint8_t &ndata,
                                       XBPACKET *packet) {
    if (ndata > ADAPTER_OUT_SIZE)
        return;

    // Refuse to fill non-handled packet
    if (!packet->header.handled) {
        return;
    }
    memset(packet->buf.buffer, 0, sizeof(packet->buf.buffer));
    packet->header.length  = sizeof(DrumInputData);
    packet->header.handled = false;

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

bool FillPacket(const uint8_t *input, const uint8_t &length, XBPACKET *packet) {
    if (length > ADAPTER_OUT_SIZE)
        return false;

    // Refuse to fill non-handled packet
    if (!packet->header.handled) {
        return false;
    }

    // set size and copy data
    memset(packet->buf.buffer, 0, sizeof(packet->buf.buffer));
    packet->header.length  = length;
    packet->header.handled = false;
    memcpy(packet->buf.buffer, input, length);
    // if (packet->buf.frame.command != CMD_ACKNOWLEDGE)
    //     packet->buf.frame.sequence = getSequence();
    return true;
}

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

const char *getCommandName(int cmd) {
    switch (cmd) {
        case CMD_ACKNOWLEDGE:
            return "ACK";
        case CMD_ANNOUNCE:
            return "ANNOUNCE";
        case CMD_STATUS:
            return "CMD_STATUS";
        case CMD_IDENTIFY:
            return "CMD_IDENTIFY";
        case CMD_POWER_MODE:
            return "CMD_POWER_MODE";
        case CMD_AUTHENTICATE:
            return "CMD_AUTHENTICATE";
        case CMD_GUIDE_BTN:
            return "CMD_GUIDE_BTN";
        case CMD_AUDIO_CONFIG:
            return "CMD_AUDIO_CONFIG";
        case CMD_RUMBLE:
            return "CMD_RUMBLE";
        case CMD_LED_MODE:
            return "CMD_LED_MODE";
        case CMD_SERIAL_NUM:
            return "CMD_SERIAL_NUM";
        case CMD_INPUT:
            return "CMD_INPUT";
        case CMD_AUDIO_SAMPLES:
            return "CMD_AUDIO_SAMPLES";
        default:
            return "Unknown CMD";
    }
}
void SerialPrintHex(const uint8_t *val, const uint8_t &nval) {
    // SERIAL_DEBUG.print(" [ ");
    for (int i = 0; i < nval; i++) {
        char buf[8];
        snprintf(buf, 8, "%02x, ", val[i]);
        // SERIAL_DEBUG.print(buf);
    }
    // SERIAL_DEBUG.print(" ] ");
}

void printPacket(const XBPACKET &packet, const char *descriptor) {
    // SERIAL_DEBUG.print(descriptor);
    // SERIAL_DEBUG.print(getCommandName(packet.buf.frame.command));
    SerialPrintHex(packet.buf.buffer, packet.header.length);
    // SERIAL_DEBUG.print("\r\n");
    return;
}