
#ifdef _IDENTIFIERS_HPP
#pragma error "Identifiers alread defined?"
#endif

#define _IDENTIFIERS_HPP
#include <stdint.h>
#include <avr/pgmspace.h>

constexpr uint8_t announce[] = {0x02, 0x20, 0x01, 0x1c, 0x7e, 0xed, 0x82, 0x8b, 0xec, 0x97, 0x00,
                                0x00, 0x38, 0x07, 0x62, 0x42, 0x01, 0x00, 0x00, 0x00, 0xe6, 0x00,
                                0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00};

constexpr uint8_t drumidentify1[] = {
    0x04, 0xf0, 0x01, 0x3a, 0xc5, 0x01, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xc5, 0x00, 0x9d, 0x00, 0x16, 0x00, 0x1b, 0x00, 0x1c, 0x00, 0x23, 0x00,
    0x29, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x06, 0x01, 0x02, 0x03, 0x04, 0x06, 0x07, 0x05, 0x01, 0x04, 0x05, 0x06, 0x0a, 0x02};

constexpr uint8_t drumidentify2[] = {
    0x04, 0xa0, 0x01, 0xba, 0x00, 0x3a, 0x17, 0x00, 0x4d, 0x61, 0x64, 0x43, 0x61, 0x74, 0x7a, 0x2e,
    0x58, 0x62, 0x6f, 0x78, 0x2e, 0x44, 0x72, 0x75, 0x6d, 0x73, 0x2e, 0x47, 0x6c, 0x61, 0x6d, 0x27,
    0x00, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x73, 0x2e, 0x58, 0x62, 0x6f, 0x78, 0x2e, 0x49, 0x6e,
    0x70, 0x75, 0x74, 0x2e, 0x4e, 0x61, 0x76, 0x69, 0x67, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x43, 0x6f};

constexpr uint8_t drumidentify3[] = {
    0x04, 0xa0, 0x01, 0xba, 0x00, 0x74, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x6c, 0x65, 0x72, 0x03, 0x93,
    0x28, 0x18, 0x06, 0xe0, 0xcc, 0x85, 0x4b, 0x92, 0x71, 0x0a, 0x10, 0xdb, 0xab, 0x7e, 0x07, 0xe7,
    0x1f, 0xf3, 0xb8, 0x86, 0x73, 0xe9, 0x40, 0xa9, 0xf8, 0x2f, 0x21, 0x26, 0x3a, 0xcf, 0xb7, 0x56,
    0xff, 0x76, 0x97, 0xfd, 0x9b, 0x81, 0x45, 0xad, 0x45, 0xb6, 0x45, 0xbb, 0xa5, 0x26, 0xd6, 0x01};

constexpr uint8_t drumidentify4[] = {
    0x04, 0xb0, 0x01, 0x17, 0xae, 0x01, 0x17, 0x00, 0x20, 0x0a, 0x00, 0x01, 0x00, 0x14, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

constexpr uint8_t drumidentify5[] = {0x04, 0xa0, 0x01, 0x00, 0xc5, 0x01, 0x00, 0x00};

constexpr struct identify_list {
    uint8_t        size;
    const uint8_t *data;
} identify_packets[] = {{sizeof(drumidentify1), drumidentify1},
                        {sizeof(drumidentify2), drumidentify2},
                        {sizeof(drumidentify3), drumidentify3},
                        {sizeof(drumidentify4), drumidentify4},
                        {sizeof(drumidentify5), drumidentify5}};
