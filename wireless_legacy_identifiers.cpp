#include "adapter_config.h"
#ifdef EMULATE_WIRELESS_LEGACY_ADAPTER

#include "adapter_identifiers.h"
#include "adapter_structs.h"
#include "version_helper.h"
#include "packet_helpers.h"

const uint8_t PROGMEM wla_announce[] = {
    0x02, 0x20, 0x01, 0x1C, 0x7e, 0xed, 0x82, 0x8b, 0xec, 0x97, 0x00, 0x00, 0x38, 0x07, 0x64, 0x41,
    0x01, 0x00, 0x00, 0x00, 0x6F, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00};

const uint8_t PROGMEM wla_identify1[] = {
    0x04, 0xF0, 0x01, 0x3A, 0xA5, 0x02, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x25, 0x01, 0xA1, 0x00, 0x16, 0x00, 0x1B, 0x00, 0x1C, 0x00, 0x23, 0x00,
    0x29, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x06, 0x01, 0x02, 0x03, 0x04, 0x06, 0x07, 0x05, 0x01, 0x04, 0x05, 0x06, 0x0A, 0x02};

const uint8_t PROGMEM wla_identify2[] = {
    0x04, 0xA0, 0x01, 0xBA, 0x00, 0x3A, 0x1B, 0x00, 0x4D, 0x61, 0x64, 0x43, 0x61, 0x74, 0x7A, 0x2E,
    0x58, 0x62, 0x6F, 0x78, 0x2E, 0x4D, 0x6F, 0x64, 0x75, 0x6C, 0x65, 0x2E, 0x42, 0x72, 0x61, 0x6E,
    0x67, 0x75, 0x73, 0x27, 0x00, 0x57, 0x69, 0x6E, 0x64, 0x6F, 0x77, 0x73, 0x2E, 0x58, 0x62, 0x6F,
    0x78, 0x2E, 0x49, 0x6E, 0x70, 0x75, 0x74, 0x2E, 0x4E, 0x61, 0x76, 0x69, 0x67, 0x61, 0x74, 0x69};

const uint8_t PROGMEM wla_identify3[] = {
    0x04, 0xA0, 0x01, 0xBA, 0x00, 0x74, 0x6F, 0x6E, 0x43, 0x6F, 0x6E, 0x74, 0x72, 0x6F, 0x6C, 0x6C,
    0x65, 0x72, 0x03, 0x0F, 0x9D, 0x25, 0xAF, 0xB0, 0x76, 0xDB, 0x4C, 0xBF, 0xD1, 0xCE, 0xA8, 0xC0,
    0xA8, 0xF5, 0xEE, 0xE7, 0x1F, 0xF3, 0xB8, 0x86, 0x73, 0xE9, 0x40, 0xA9, 0xF8, 0x2F, 0x21, 0x26,
    0x3A, 0xCF, 0xB7, 0x56, 0xFF, 0x76, 0x97, 0xFD, 0x9B, 0x81, 0x45, 0xAD, 0x45, 0xB6, 0x45, 0xBB};

const uint8_t PROGMEM wla_identify4[] = {
    0x04, 0xA0, 0x01, 0x3A, 0xAE, 0x01, 0xA5, 0x26, 0xD6, 0x05, 0x17, 0x00, 0x20, 0x36, 0x00, 0x01,
    0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x17, 0x00, 0x21, 0x06, 0x00, 0x01, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x22, 0x02, 0x01, 0x01, 0x00, 0x14};

const uint8_t PROGMEM wla_identify5[] = {
    0x04, 0xA0, 0x01, 0x3A, 0xE8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x23, 0x05, 0x00, 0x01, 0x00, 0x14, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x24, 0x04,
    0x00, 0x01, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const uint8_t PROGMEM wla_identify6[] = {0x04, 0xB0, 0x01, 0x03, 0xA2, 0x02, 0x00, 0x00, 0x00};

const uint8_t PROGMEM wla_identify7[] = {0x04, 0xA0, 0x01, 0x00, 0xA5, 0x02};

void fill_from_pgm(xb_packet_t *packet, const uint8_t *pgm, const uint8_t &size) {
    for (uint8_t i = 0; i < size; i++) {
        packet->buf.buffer[i] = pgm_read_byte(pgm + i);
    }
    packet->header.triggered_time = 0;
    packet->header.length         = size;
    packet->buf.frame.sequence = getSequence();
}

namespace identifiers {

int get_n_identify() {
    return 7;
}
int get_announce(xb_packet_t *packet) {
    fill_from_pgm(packet, wla_announce, sizeof(wla_announce) / sizeof(wla_announce[0]));
    return 0;
}

int get_identify(const uint8_t &sequence, xb_packet_t *packet) {
    switch (sequence) {
        case 0:
            fill_from_pgm(packet, wla_identify1, sizeof(wla_identify1) / sizeof(wla_identify1[0]));
            return 0;
        case 1:
            fill_from_pgm(packet, wla_identify2, sizeof(wla_identify2) / sizeof(wla_identify2[0]));
            return 0;
        case 2:
            fill_from_pgm(packet, wla_identify3, sizeof(wla_identify3) / sizeof(wla_identify3[0]));
            return 0;
        case 3:
            fill_from_pgm(packet, wla_identify4, sizeof(wla_identify4) / sizeof(wla_identify4[0]));
            return 0;
        case 4:
            fill_from_pgm(packet, wla_identify5, sizeof(wla_identify5) / sizeof(wla_identify5[0]));
            return 0;
        case 5:
            fill_from_pgm(packet, wla_identify6, sizeof(wla_identify6) / sizeof(wla_identify6[0]));
            return 0;
        case 6:
            fill_from_pgm(packet, wla_identify7, sizeof(wla_identify7) / sizeof(wla_identify7[0]));
            return 0;
        default:
            return 1;
    }
}

}  // namespace identifiers

#endif  // EMULATE_WIRELESS_LEGACY_ADAPTER