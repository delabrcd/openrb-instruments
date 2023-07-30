#include "debug_helpers.h"

#ifdef SERIAL_DEBUG
#include "adapter_structs.h"
#include "HardwareSerial.h"
#include "Usb.h"

#ifdef SERIAL_DEBUG
#define debug(msg) Notify(PSTR(msg), 0x80);
#else
#define debug(msg) (void)
#endif

void printCommandName(int cmd) {
    switch (cmd) {
        case CMD_ACKNOWLEDGE:
            debug("ACK");
            break;
        case CMD_ANNOUNCE:
            debug("ANNOUNCE");
            break;
        case CMD_STATUS:
            debug("CMD_STATUS");
            break;
        case CMD_IDENTIFY:
            debug("CMD_IDENTIFY");
            break;
        case CMD_POWER_MODE:
            debug("CMD_POWER_MODE");
            break;
        case CMD_AUTHENTICATE:
            debug("CMD_AUTHENTICATE");
            break;
        case CMD_GUIDE_BTN:
            debug("CMD_GUIDE_BTN");
            break;
        case CMD_AUDIO_CONFIG:
            debug("CMD_AUDIO_CONFIG");
            break;
        case CMD_RUMBLE:
            debug("CMD_RUMBLE");
            break;
        case CMD_LED_MODE:
            debug("CMD_LED_MODE");
            break;
        case CMD_SERIAL_NUM:
            debug("CMD_SERIAL_NUM");
            break;
        case CMD_INPUT:
            debug("CMD_INPUT");
            break;
        case CMD_AUDIO_SAMPLES:
            debug("CMD_AUDIO_SAMPLES");
            break;
        default:
            debug("Unknown CMD");
            break;
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

void printPacket(const xb_packet_t &packet, const char *descriptor) {
    SERIAL_DEBUG.print(descriptor);
    printCommandName(packet.buf.frame.command);
    serialPrintHex(packet.buf.buffer, packet.header.length);
    SERIAL_DEBUG.print("\r\n");
    return;
}
#endif