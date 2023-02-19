#include "debug_helpers.h"

#ifdef SERIAL_DEBUG
#include "adapter_structs.h"
#include "HardwareSerial.h"

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

void serialPrintHex(const uint8_t *val, const uint8_t &nval) {
    SERIAL_DEBUG.print(" [ ");
    for (int i = 0; i < nval; i++) {
        char buf[8];
        snprintf(buf, 8, "%02x, ", val[i]);
        SERIAL_DEBUG.print(buf);
    }
    SERIAL_DEBUG.print(" ] ");
}

void printPacket(const XBPACKET &packet, const char *descriptor) {
    SERIAL_DEBUG.print(descriptor);
    SERIAL_DEBUG.print(getCommandName(packet.buf.frame.command));
    serialPrintHex(packet.buf.buffer, packet.header.length);
    SERIAL_DEBUG.print("\r\n");
    return;
}
#endif